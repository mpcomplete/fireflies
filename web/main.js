const regl = require("regl")({extensions: ['WEBGL_draw_buffers', 'OES_texture_float', 'ANGLE_instanced_arrays']});
const Vec3 = require("vec3");
const mat4 = require("gl-mat4");

const NUM_LEADERS = 3; // Leaders wander aimlessly.
const NUM_FLIES = 100; // Flies chase leaders.
const NUM_CRITTERS = NUM_FLIES + NUM_LEADERS;
const TAIL_LENGTH = 200;

// Textures hold a single property for every critter.
// Each critter has a single row in a given property's texture, with the column representing
// history over time (the "tail").
// ex. positionTexture[X, Y] = critter X's position at history Y.
// History is done as a circular queue, with the current history index incrementing each frame from 0 to TAIL_LENGTH.
var TEX_PROPS = {
  type: 'float',
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
    depthStencil: false
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

// Initialize flies with random position and velocity.
var fliesFBO = createDoubleFBO();
fliesFBO.src.color[0].subimage({ // position
  width: NUM_CRITTERS,
  height: 1,
  data: Array.from({length: NUM_CRITTERS*4}, () => rand(-3, 3))
});
fliesFBO.src.color[1].subimage({ // velocity
  width: NUM_CRITTERS,
  height: 1,
  data: Array.from({length: NUM_CRITTERS*4}, () => rand(-1, 1))
});
let leaderHues = [0, .25, .5, .75];
// let leaderHues = Array.from({length: NUM_LEADERS}, () => rand(0, 1));
let fliesToLeaders = Array.from({length: NUM_CRITTERS}, (_, i) => i < NUM_LEADERS ? -1 : randInt(0, NUM_LEADERS-1));
fliesFBO.src.color[2].subimage({ // leaderIndex
  width: NUM_CRITTERS,
  height: 1,
  data: Array.from({length: NUM_CRITTERS}, (_, i) => i < NUM_LEADERS ?
    [-1,leaderHues[i],0,0] :
    [fliesToLeaders[i]/NUM_CRITTERS,leaderHues[fliesToLeaders[i]],rand(0, 5),0])
});

const updatePositions = regl({
  frag: `
#extension GL_EXT_draw_buffers : require
  precision mediump float;
  uniform sampler2D positionTex;
  uniform sampler2D velocityTex;
  uniform sampler2D scalarTex;
  uniform float historyIdx;
  uniform float dt;
  uniform float numLeaders;
  uniform float numCritters;
  varying vec2 uv;
  varying vec2 uvPrev;

  vec3 chaseLeader(vec3 pos, vec3 vel, vec2 leaderUV) {
    vec3 leaderPos = texture2D(positionTex, leaderUV).xyz;
    float factor = 0.9;
    vec3 accel = factor*normalize(leaderPos - pos);
    vel += accel*dt;
    return vel;
  }

  vec3 matchVelocity(vec3 pos, vec3 vel, vec2 leaderUV) {
    float factor = 0.005; // Adjust by this % of average velocity

    vec3 avgVel = vec3(0,0,0);
    float numNeighbors = 0.;
    // for (vec2 fuv = vec2(0., 0.); fuv.x < 1.0; fuv.x += 1./100.) {
    //   vec3 otherVel = texture2D(velocityTex, fuv).xyz;
    for (float u = 0.; u < 1.0; u += 1./100.) {
      vec3 otherVel = texture2D(velocityTex, vec2(uvPrev)).xyz;
      float otherLeaderIdx = texture2D(scalarTex, vec2(u, uvPrev.y)).x;
      if (otherLeaderIdx == leaderUV.x) {
        avgVel += otherVel;
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
    float bounds = 8.;
    // const accel = fly.pos.scaled(-factor);
    // fly.vel.add(accel.scaled(dt));
    // Reverse direction when out of bounds
    if (pos.x < -bounds) vel.x = abs(vel.x);
    if (pos.y < -bounds) vel.y = abs(vel.y);
    if (pos.z < -bounds) vel.z = abs(vel.z);
    if (pos.x >  bounds) vel.x = -abs(vel.x);
    if (pos.y >  bounds) vel.y = -abs(vel.y);
    if (pos.z >  bounds) vel.z = -abs(vel.z);
    return vel;
  }

  // https://thebookofshaders.com/10/
  float rand() {
    return fract(sin(dot(uv,vec2(12.9898,78.233)))*43758.5453123);
  }

  void main () {
    if (abs(uv.y - historyIdx) >= 0.0001) {
      // Only write to the current row in history.
      gl_FragData[0] = texture2D(positionTex, uv);
      gl_FragData[1] = texture2D(velocityTex, uv);
      gl_FragData[2] = texture2D(scalarTex, uv);
      return;
    }

    vec4 scalars = texture2D(scalarTex, uvPrev);
    vec2 leaderUV = vec2(scalars.x, uvPrev.y);
    vec3 pos = texture2D(positionTex, uvPrev).xyz;
    vec3 vel = texture2D(velocityTex, uvPrev).xyz;

    if (scalars.x >= 0.) {
      const float maxVelocity = 1.5;
      const float followTime = 20.;

      vel = chaseLeader(pos, vel, leaderUV);
      vel = matchVelocity(pos, vel, leaderUV);
      // pos = texture2D(positionTex, leaderUV).xyz;
      vel = clamp(vel, -maxVelocity, maxVelocity);

      if (scalars.z > followTime) {
        float leaderIdx = rand()*numLeaders/numCritters;
        float hue = texture2D(scalarTex, vec2(leaderIdx, uvPrev.y)).y; // new leader's hue
        float age = rand()*3.;
        scalars.xyz = vec3(leaderIdx, hue, age);
      }
    } else {
      const float maxVelocity = 1.2;
      vel = flyAimlessly(pos, vel);
      vel = maxVelocity*normalize(vel);
    }
    const float hueRate = .005;
    scalars.y += hueRate * dt;
    scalars.z += dt;

    pos += vel*dt;
    gl_FragData[0].xyz = pos;
    gl_FragData[1].xyz = vel;
    gl_FragData[2] = scalars;
  }
  `,
  vert: `
  precision mediump float;
  attribute vec2 position;
  varying vec2 uv;
  varying vec2 uvPrev;
  uniform float prevHistoryIdx;
  void main () {
    uv = position * 0.5 + 0.5;
    uvPrev = vec2(uv.x, prevHistoryIdx);
    gl_Position = vec4(position, 0., 1.);
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
    numLeaders: NUM_LEADERS,
    numCritters: NUM_CRITTERS,
    dt: 1./24,
  },
  count: 6,
  framebuffer: () => fliesFBO.dst,
});

const S = 0.1;
const drawFly = regl({
  frag: `
  precision mediump float;
  varying vec4 vColor;
  void main() {
    gl_FragColor = vColor;
  }`,

  vert: `
  precision mediump float;
  attribute vec3 position;
  uniform vec3 offset;
  uniform mat4 projection, view;
  varying vec4 vColor;
  uniform sampler2D positionTex;
  uniform sampler2D velocityTex;
  uniform sampler2D scalarTex;
  uniform vec2 uv;

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
  }

  void main() {
    vec4 offset = texture2D(positionTex, uv);
    vec4 velocity = texture2D(velocityTex, uv);
    vec4 scalars = texture2D(scalarTex, uv);
    vec4 color = hsv2rgb(vec4(scalars.y, .8, .8, 1.));
    gl_Position = projection * view * (pointAt(velocity.xyz) * vec4(position.xyz, 1) + vec4(offset.xyz, 0));
    vColor = color;
  }`,

  attributes: {
    position: [[0,0,S*3], [0,0,-S*1], [S,0,0], [0,-S,0], [-S,0,0], [0,S,0]],
  },
  elements: [
    [0,2,3], [0,3,4], [0,4,5], [0,5,2], // front
    [1,2,3], [1,3,4], [1,4,5], [1,5,2], // back
  ],

  uniforms: {
    positionTex: () => fliesFBO.src.color[0],
    velocityTex: () => fliesFBO.src.color[1],
    scalarTex: () => fliesFBO.src.color[2],
    uv: regl.prop('uv'),
  },

  depth: {
    enable: false
  },
});

const T = .1;
const TH = .15;
const drawTails = regl({
  frag: `
  precision mediump float;
  varying vec4 vColor;
  void main() {
    gl_FragColor = vColor;
  }`,

  vert: `
  precision mediump float;
  attribute vec3 position;
  attribute float alpha;
  attribute float historyIdx;
  uniform mat4 projection, view;

  uniform sampler2D positionTex;
  uniform sampler2D velocityTex;
  uniform sampler2D scalarTex;
  uniform float flyIdx;
  uniform float currentHistoryIdx;
  varying vec4 vColor;

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
  }

  void main() {
    float age = mod(1.0 + currentHistoryIdx - historyIdx, 1.0);
    vec2 uv = vec2(flyIdx, historyIdx);
    vec4 offset = texture2D(positionTex, uv);
    vec4 velocity = texture2D(velocityTex, uv);
    vec4 scalars = texture2D(scalarTex, uv);
    vec4 color = hsv2rgb(vec4(scalars.y, .8, .8, 1.));
    vec4 pos = pointAt(velocity.xyz) * vec4(position.xyz, 1) + vec4(offset.xyz, 0);
    gl_Position = projection * view * pos;
    vColor = vec4(color.rgb, alpha * pow(1.-age, 0.7));
  }`,

  attributes: {
    position: [[-T,0,0], [-T,0,TH], [0,0,0], [0,0,TH], [T,0,0], [T,0,TH]],
    // alpha: [0,0,0, 0,0,0, 1,1,1, 1,1,1, 0,0,0, 0,0,0],
    alpha: [0, 0, 1, 1, 0, 0,],
    historyIdx: {
      buffer: Array.from({length: TAIL_LENGTH}, (_, i) => i/TAIL_LENGTH),
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
    currentHistoryIdx: regl.prop('currentHistoryIdx'),
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
  frag: `
  precision mediump float;
  uniform sampler2D quantity;
  varying vec2 uv;

  void main() {
    gl_FragColor = vec4(texture2D(quantity, uv).rgb, 1.);
  }`,
  vert: `
  precision mediump float;
  attribute vec2 position;
  varying vec2 uv;
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
                      [0, 0, 20], // why is this reversed?
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

// let canvas = document.getElementsByTagName("canvas")[0];
// canvas.addEventListener('mousedown', e => {
//   frame++;
// });

let frame = 1;
// let sec = 0;
// setInterval(function() {
//   sec += 1;
//   console.log(frame / sec);
// }, 1000);
regl.frame(function () {
  camera(() => {
    regl.clear({
      color: [0, 0, 0, 1]
    })

    let dy = 0.5 / TAIL_LENGTH;
    let prevHistoryIdx = dy+((frame-1) % TAIL_LENGTH) / TAIL_LENGTH;
    let historyIdx = dy+(frame % TAIL_LENGTH) / TAIL_LENGTH;
    // console.log(prevHistoryIdx, historyIdx);
    updatePositions({historyIdx: historyIdx, prevHistoryIdx: prevHistoryIdx});
    fliesFBO.swap();
    // testDraw({quantity: fliesFBO.src.color[0]});

    for (let i = 0; i < NUM_FLIES; i++) {
      let u = (i+.5+NUM_LEADERS)/NUM_CRITTERS;
      drawFly({uv: [u, historyIdx]});
      drawTails({flyIdx: u, currentHistoryIdx: historyIdx});
    }
    for (let i = 0; i < NUM_LEADERS; i++) {
      drawFly({uv: [i/NUM_CRITTERS, historyIdx]});
    }
  });

  frame++;
});