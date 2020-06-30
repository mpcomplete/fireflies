const webgl2 = require("./regl-webgl2-compat.js");
const regl = webgl2.overrideContextType(() => require("regl")({extensions: ['WEBGL_draw_buffers', 'OES_texture_float', 'ANGLE_instanced_arrays']}));
const mat4 = require("gl-mat4");
const pointers = require("./pointers.js");

const NUM_LEADERS = 3; // Leaders wander aimlessly.
const NUM_FLIES = 253; // Flies chase leaders.
const NUM_CRITTERS = NUM_FLIES + NUM_LEADERS;
const TAIL_LENGTH = 256;

// Textures hold a single property for every critter.
// Each critter has a single row in a given property's texture, with the column representing
// history over time (the "tail").
// ex. positionTexture[X, Y] = critter X's position at history Y.
// History is done as a circular queue, with the current history index incrementing each frame from 0 to TAIL_LENGTH.
var TEX_PROPS = {
  type: 'float32',
  format: 'rgba',
  wrap: 'clamp',
  width: NUM_CRITTERS,
  height: TAIL_LENGTH,
};

function createFBO() {
  return regl.framebuffer({
    color: [
      regl.texture(TEX_PROPS), // position
      regl.texture(TEX_PROPS), // velocity
      regl.texture(TEX_PROPS), // scalars: leaderIndex, hue(leaderOnly), age, 0
    ],
    depthStencil: false,
    depth: false,
    stencil: false,
  });
}

function createDoubleFBO() {
  return {
    src: createFBO(),
    dst: createFBO(),
    swap: function () {
      [this.src, this.dst] = [this.dst, this.src];
    }
  }
}

// Utils.
// const extend = (a, b) => Object.assign(b, a);
const rand = (min, max) => Math.random() * (max - min) + min;
const randInt = (min, max) => Math.floor(rand(min, max+1));
const jitterVec = (v, d) => v.map((x) => x + rand(-d, d))

// Initialize flies with random position and velocity.
let leaderPos = Array.from({length: NUM_LEADERS}, () => jitterVec([0,0,0,0], 10));
let leaderHues = Array.from({length: NUM_LEADERS}, () => randInt(0, 12)/12);

// Spawn 3 groups of flies per leader.
let groupPos = Array.from({length: NUM_LEADERS*3}, () => jitterVec([0,0,0,0], 20));
let fliesToGroup = Array.from({length: NUM_CRITTERS}, () => randInt(0, groupPos.length-1));
let fliesToLeader = Array.from({length: NUM_CRITTERS}, (_, i) => i < NUM_LEADERS ? -1 : Math.floor(fliesToGroup[i]/3));

var fliesFBO = createDoubleFBO();
fliesFBO.src.color[0].subimage({ // position
  width: NUM_CRITTERS,
  height: 1,
  data: Array.from({length: NUM_CRITTERS}, (_, i) => i < NUM_LEADERS ?
    leaderPos[i] :
    jitterVec(groupPos[fliesToGroup[i]], 2))
});
fliesFBO.src.color[1].subimage({ // velocity
  width: NUM_CRITTERS,
  height: 1,
  data: Array.from({length: NUM_CRITTERS*4}, (_, i) => i < NUM_LEADERS*4 ? rand(-1, 1) : 0)
});
fliesFBO.src.color[2].subimage({ // leaderIndex
  width: NUM_CRITTERS,
  height: 1,
  data: Array.from({length: NUM_CRITTERS}, (_, i) => i < NUM_LEADERS ?
    [-1,leaderHues[i],0,0] :
    [fliesToLeader[i],leaderHues[fliesToLeader[i]],rand(0, 5),0])
});

// Common functions some shaders share.
const shaderCommon = `#version 300 es
mat4 rotation(float angle, vec3 axis) {
  vec3 a = normalize(axis);
  float s = sin(angle);
  float c = cos(angle);
  float oc = 1.0 - c;

  return mat4(oc * a.x * a.x + c,        oc * a.x * a.y - a.z * s,  oc * a.z * a.x + a.y * s, 0.0,
              oc * a.x * a.y + a.z * s,  oc * a.y * a.y + c,        oc * a.y * a.z - a.x * s, 0.0,
              oc * a.z * a.x - a.y * s,  oc * a.y * a.z + a.x * s,  oc * a.z * a.z + c,       0.0,
              0.0,                       0.0,                       0.0,                      1.0);
}

mat4 pointAt(vec3 dir) {
  vec3 front = vec3(0, 0, -1); // why is this reversed?
  vec3 ndir = normalize(dir);
  vec3 axis = cross(front, ndir);
  float angle = acos(dot(front, ndir));
  return rotation(angle, axis);
}

// http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
vec4 hsv2rgb(vec4 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return vec4(c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y), c.a);
}`;

const updatePositions = regl({
  frag: 
  '#version 300 es\n' +
  // '#extension GL_EXT_draw_buffers : require\n' +
  '#define NUM_CRITTERS ' + NUM_CRITTERS + '\n' +
  '#define NUM_LEADERS ' + NUM_LEADERS + '\n' + `

  precision mediump float;
  precision highp int;
  in vec2 ijf;
  in vec4 mousePos;

  uniform sampler2D positionTex;
  uniform sampler2D velocityTex;
  uniform sampler2D scalarTex;
  uniform int historyIdx, prevHistoryIdx;
  uniform float dt;

  layout(location = 0) out vec4 fragData0;
  layout(location = 1) out vec4 fragData1;
  layout(location = 2) out vec4 fragData2;

  vec3 chaseLeader(vec3 pos, vec3 vel, ivec2 leaderIJ) {
    vec3 leaderPos = texelFetch(positionTex, leaderIJ, 0).xyz;
    vec3 dir = leaderPos - pos;
    float idealDist = 0.2;
    float dist = max(0.01, length(dir));
    float factor = (mousePos.w > 0. && leaderIJ.x == 0) ? 3. : 0.2;
    float acc = max(0.1, pow(dist - idealDist, factor));
    vel += acc*dt*dir;
    return vel;
  }

  vec3 avoidOthers(vec3 pos, vec3 vel, ivec2 leaderIJ) {
    float minDistance = 0.3;
    float factor = 0.02;
    vec3 moveV = vec3(0,0,0);
    for (int i = 0; i < NUM_CRITTERS; i++) {
      if (i != int(ijf.x)) {
        vec3 otherPos = texelFetch(positionTex, ivec2(i, prevHistoryIdx), 0).xyz;
        if (distance(pos, otherPos) < minDistance)
          moveV += pos - otherPos;
      }
    }

    vel += factor*moveV;
    return vel;
  }

  vec3 matchVelocity(vec3 pos, vec3 vel, ivec2 leaderIJ) {
    float factor = 0.01; // Adjust by this % of average velocity

    vec3 avgVel = vec3(0,0,0);
    float numNeighbors = 0.;
    for (int i = 0; i < NUM_CRITTERS; i++) {
      int otherLeaderIdx = int(texelFetch(scalarTex, ivec2(i, prevHistoryIdx), 0).x);
      if (otherLeaderIdx == leaderIJ.x) {
        avgVel += texelFetch(velocityTex, ivec2(i, prevHistoryIdx), 0).xyz;
        numNeighbors += 1.;
      }
    }

    if (numNeighbors > 0.) {
      avgVel *= 1.0/numNeighbors;
      vel += factor*(avgVel - vel);
    }
    return vel;
  }

  vec3 flyAimlessly(vec3 pos, vec3 vel) {
    vec3 bounds = vec3(10., 6., 6.);
    // const accel = fly.pos.scaled(-factor);
    // fly.vel.add(accel.scaled(dt));
    // Reverse direction when out of bounds
    if (pos.x < -bounds.x) vel.x = abs(vel.x);
    if (pos.y < -bounds.y) vel.y = abs(vel.y);
    if (pos.z < -bounds.z) vel.z = abs(vel.z);
    if (pos.x >  bounds.x) vel.x = -abs(vel.x);
    if (pos.y >  bounds.y) vel.y = -abs(vel.y);
    if (pos.z >  bounds.z) vel.z = -abs(vel.z);
    return vel;
  }

  // https://thebookofshaders.com/10/
  float rand(float offset) {
    vec2 pos = texelFetch(positionTex, ivec2(ijf), 0).xy;
    return fract(sin(dot(pos.xy + offset,vec2(12.9898,78.233)))*43758.5453123);
  }

  void main () {
    int numHistory = textureSize(positionTex, 0).y;
    ivec2 ij = ivec2(ijf);
    ivec2 ijPrev = ivec2(ij.x, prevHistoryIdx);

    if (ij.y != historyIdx) {
      // Not the current history index: this is a tail. Keep most data intact.
      fragData0 = texelFetch(positionTex, ij, 0);
      fragData1 = texelFetch(velocityTex, ij, 0);
      fragData2 = texelFetch(scalarTex, ij, 0);

      // Apply wind.
      float age = mod(1.0 + float(ij.y - historyIdx)/float(numHistory), 1.0);
      vec3 wind = .3*normalize(vec3(2,1,1));
      fragData0.xyz += wind * age * age * dt;
      return;
    }

    vec3 pos = texelFetch(positionTex, ijPrev, 0).xyz;
    vec3 vel = texelFetch(velocityTex, ijPrev, 0).xyz;
    vec4 scalars = texelFetch(scalarTex, ijPrev, 0);
    ivec2 leaderIJ = ivec2(int(scalars.x), ijPrev.y);

    if (mousePos.w > 0. && ij.x == 0) {
      // Mouse controls the first leader.
      pos = mousePos.xyz;
    } else if (scalars.x >= 0.) {  // a firefly
      bool isAffectedByMouse = mousePos.w > 0. && leaderIJ.x == 0;
      float maxSpeed = isAffectedByMouse ? 10.0 : 2.7;
      const float followTime = 20.;

      vel = chaseLeader(pos, vel, leaderIJ);
      // Turns out it looks cooler without these factors.
      // Hypothesis: chasing the same leader already causes flies to match each other's velocity;
      // and avoiding others prevents the cool-looking effect of crossing paths.
      // vel = avoidOthers(pos, vel, leaderIJ);
      // vel = matchVelocity(pos, vel, leaderIJ);
      float speed = length(vel);
      if (speed > maxSpeed)
        vel = vel*(maxSpeed/speed);

      if (scalars.z > followTime) {
        // Switch leaders.
        int leaderIdx = int(rand(0.)*float(NUM_LEADERS));
        float hue = texelFetch(scalarTex, ivec2(leaderIdx, ijPrev.y), 0).y; // new leader's hue
        float age = rand(0.)*followTime/2.;
        scalars.xyz = vec3(float(leaderIdx), hue, age);
      } else {
        float hue = texelFetch(scalarTex, leaderIJ, 0).y;
        float factor = isAffectedByMouse ? .1 : .1*(2.*speed/maxSpeed - 1.);
        scalars.y = hue + factor;
      }
    } else {  // a leader
      const float wanderTime = 10.0;
      const float maxSpeed = 2.1;
      vel = flyAimlessly(pos, vel);
      vel = maxSpeed*normalize(vel);

      if (scalars.z > wanderTime) {
        // Switch directions.
        vec3 randVec = vec3(rand(0.), rand(.1), rand(.2)) * 2. - 1.;
        if (length(randVec) > 0.001)
          vel = maxSpeed*normalize(randVec);
        scalars.z = rand(0.)*wanderTime/2.;
      }

      float hueRate = .005 + .02*rand(0.);
      scalars.y = mod(scalars.y + hueRate * dt, 1.0);
    }
    scalars.z += dt; // age

    pos += vel*dt;
    fragData0.xyz = pos;
    fragData1.xyz = vel;
    fragData2 = scalars;
  }
  `,
  vert: `#version 300 es
  precision mediump float;
  in vec2 position;
  out vec2 ijf;
  out vec4 mousePos;
  uniform sampler2D positionTex;
  uniform mat4 projection, view;
  uniform int historyIdx, prevHistoryIdx;
  uniform vec4 mouseDown;
  void main () {
    ijf = vec2(textureSize(positionTex, 0)) * (position * .5 + .5);
    gl_Position = vec4(position, 0., 1.);
    if (mouseDown.w >= 0.)
      mousePos = vec4((projection * view * vec4(mouseDown.xy, 20., 1.)).xyz, 1.0);
    else
      mousePos = vec4(0.);
  }`,

  attributes: {
    position: [[-1, -1], [-1, 1], [1, 1], [-1, -1], [1, 1], [1, -1]]
  },
  uniforms: {
    positionTex: () => fliesFBO.src.color[0],
    velocityTex: () => fliesFBO.src.color[1],
    scalarTex: () => fliesFBO.src.color[2],
    prevHistoryIdx: regl.prop("prevHistoryIdx"), // read from
    historyIdx: regl.prop("historyIdx"), // write to
    mouseDown: regl.prop("mouseDown"),
    dt: 1./24,
  },
  count: 6,
  framebuffer: () => fliesFBO.dst,
});

const S = 0.1;
const drawFly = regl({
  frag: `#version 300 es
  precision mediump float;
  in vec4 vColor;
  out vec4 fragColor;
  void main() {
    fragColor = vColor;
  }`,

  vert: shaderCommon + `
  precision mediump float;
  in vec3 position;
  out vec4 vColor;

  uniform vec3 offset;
  uniform mat4 projection, view;
  uniform sampler2D positionTex;
  uniform sampler2D velocityTex;
  uniform sampler2D scalarTex;
  uniform int flyIdx, historyIdx;

  void main() {
    ivec2 ij = ivec2(flyIdx, historyIdx);
    vec4 offset = texelFetch(positionTex, ij, 0);
    vec4 velocity = texelFetch(velocityTex, ij, 0);
    vec4 scalars = texelFetch(scalarTex, ij, 0);
    vec4 color = hsv2rgb(vec4(scalars.y, .8, .8, 1.));
    gl_Position = projection * view * (pointAt(velocity.xyz) * vec4(position.xyz, 1) + vec4(offset.xyz, 0));
    vColor = color;
  }`,

  attributes: {
    position: [[0,0,S*3], [0,0,-S*1], [S,0,0], [0,-S,0], [-S,0,0], [0,S,0]],
    // colorz: [1, 0, 0, 0, 0, 0, 0, 0]
  },
  elements: [
    [0,2,3], [0,3,4], [0,4,5], [0,5,2], // front
    [1,2,3], [1,3,4], [1,4,5], [1,5,2], // back
  ],

  uniforms: {
    positionTex: () => fliesFBO.src.color[0],
    velocityTex: () => fliesFBO.src.color[1],
    scalarTex: () => fliesFBO.src.color[2],
    flyIdx: regl.prop('flyIdx'),
    historyIdx: regl.prop('historyIdx'),
  },

  depth: {
    enable: false
  },
});

const T = .15;
const TH = .15;
const drawTails = regl({
  frag: `#version 300 es
  precision mediump float;
  in vec4 vColor;
  out vec4 fragColor;
  void main() {
    fragColor = vColor;
  }`,

  vert: shaderCommon + `
  precision mediump float;
  in vec3 position;
  in float alpha;
  in float instanceHistoryIdx;
  out vec4 vColor;

  uniform mat4 projection, view;
  uniform sampler2D positionTex;
  uniform sampler2D velocityTex;
  uniform sampler2D scalarTex;
  uniform int flyIdx, historyIdx;

  void main() {
    int numHistory = textureSize(positionTex, 0).y;
    float age = mod(1.0 + float(historyIdx - int(instanceHistoryIdx))/float(numHistory), 1.0);
    ivec2 ij = ivec2(flyIdx, instanceHistoryIdx);
    vec4 offset = texelFetch(positionTex, ij, 0);
    vec4 velocity = texelFetch(velocityTex, ij, 0);
    vec4 scalars = texelFetch(scalarTex, ij, 0);
    vec4 color = hsv2rgb(vec4(scalars.y, .8, .8, 1.));
    vec4 pos = pointAt(velocity.xyz) * vec4(position.xyz, 1) + vec4(offset.xyz, 0);
    gl_Position = projection * view * pos;
    vColor = vec4(color.rgb, alpha * pow(1.-age, 0.7));
  }`,

  attributes: {
    position: [[-T,0,0], [-T,0,TH], [0,0,0], [0,0,TH], [T,0,0], [T,0,TH]],
    alpha: [0, 0, 1, 1, 0, 0,],
    instanceHistoryIdx: {
      buffer: Array.from({length: TAIL_LENGTH}, (_, i) => i),
      divisor: 1,
    }
  },
  elements: [
    [0,1,2], [1,2,3], [2,3,4], [3,4,5],
  ],

  uniforms: {
    positionTex: () => fliesFBO.src.color[0],
    velocityTex: () => fliesFBO.src.color[1],
    scalarTex: () => fliesFBO.src.color[2],
    flyIdx: regl.prop('flyIdx'),
    historyIdx: regl.prop('historyIdx'),
  },

  depth: {
    enable: false
  },

  blend: {
    enable: true,
    func: {
      srcRGB: 'src alpha',
      srcAlpha: 1,
      dstRGB: 1,
      dstAlpha: 'dst alpha',
    },
    equation: {
      rgb: 'add',
      alpha: 'add'
    },
    color: [0, 0, 0, 0]
  },

  instances: TAIL_LENGTH,
});

const testDraw = regl({
  frag: `#version 300 es
  precision mediump float;
  uniform sampler2D quantity;
  in vec2 uv;
  out vec4 fragColor;

  void main() {
    fragColor = vec4(texture(quantity, uv).rgb, 1.);
  }`,
  vert: `#version 300 es
  precision mediump float;
  in vec2 position;
  out vec2 uv;
  void main () {
    uv = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0., 1.);
  }`,

  attributes: {
    position: [[-1, -1], [-1, 1], [1, 1], [-1, -1], [1, 1], [1, -1]]
  },
  uniforms: {
    quantity: regl.prop('quantity'),
  },
  count: 6,
});

const camera = regl({
  uniforms: {
    view: mat4.lookAt([],
                      [0, 0, 20],
                      [0, 0, 0],
                      [0, 1, 0]),
    projection: ({viewportWidth, viewportHeight}) =>
      mat4.perspective([],
                       Math.PI / 4,
                       viewportWidth / viewportHeight,
                       0.01,
                       100),
  }
})

let frame = 1;
// let sec = 0;
// setInterval(function() { console.log(frame / ++sec); }, 1000);
regl.frame(function () {
  camera(() => {
    regl.clear({
      color: [0, 0, 0, 1]
    })

    let mouseDown = [-1,-1, -1, -1];
    for (let pointer of pointers.pointers) {
      if (pointer.isDown) {
        mouseDown = [pointer.pos[0] - .5, pointer.pos[1] - .5, 0, 1];
        mouseDown[0] *= 20;
        mouseDown[1] *= 7;
        break;
      }
    }

    let prevHistoryIdx = (frame-1) % TAIL_LENGTH;
    let historyIdx = frame % TAIL_LENGTH;
    // console.log(prevHistoryIdx, historyIdx);
    updatePositions({historyIdx: historyIdx, prevHistoryIdx: prevHistoryIdx, mouseDown: mouseDown});
    fliesFBO.swap();
    // testDraw({quantity: fliesFBO.src.color[0]});

    const drawLeaders = true;
    for (let i = 0; i < NUM_CRITTERS; i++) {
      if (i < NUM_LEADERS && !drawLeaders)
        continue;
      drawFly({flyIdx: i, historyIdx: historyIdx});
      drawTails({flyIdx: i, historyIdx: historyIdx});
    }
  });

  frame++;
});