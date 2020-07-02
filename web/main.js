// TODO: rename fragData0-3

const webgl2 = require("./regl-webgl2-compat.js");
const regl = webgl2.overrideContextType(() => require("regl")({extensions: ['WEBGL_draw_buffers', 'OES_texture_float', 'ANGLE_instanced_arrays']}));
const mat4 = require("gl-mat4");
const pointers = require("./pointers.js");
const dat = require("dat.gui");

var config = {};
window.onload = function() {
  var gui = new dat.GUI();
  function addConfig(name, initial, min, max) {
    config[name] = initial;
    return gui.add(config, name, min, max);
  }
  addConfig("paused", false);
  addConfig("NUM_LEADERS", 3, 1, 10).name("flocks").step(1).onFinishChange(initFramebuffers);
  addConfig("NUM_FLIES", 250, 10, 500).name("flies").step(10).onFinishChange(initFramebuffers);
  addConfig("TAIL_LENGTH", 180, 0, 500).name("tail length").step(10).onFinishChange(initFramebuffers);
  addConfig("SOFT_TAILS", true).name("soft tails");

  initFramebuffers();
};

// Textures hold a single property for every critter.
// Each critter has a single row in a given property's texture, with the column representing
// history over time (the "tail").
// ex. positionTexture[X, Y] = critter X's position at history Y.
// History is done as a circular queue, with the current history index incrementing each frame from 0 to TAIL_LENGTH.
function createFBO(count, props) {
  return regl.framebuffer({
    color: Array.from({length: count}, () => regl.texture(props)),
    depthStencil: false,
    depth: false,
    stencil: false,
  });
}

function createDoubleFBO(count, props) {
  return {
    src: createFBO(count, props),
    dst: createFBO(count, props),
    swap: function () {
      [this.src, this.dst] = [this.dst, this.src];
    }
  }
}

// Utils.
const rand = (min, max) => Math.random() * (max - min) + min;
const randInt = (min, max) => Math.floor(rand(min, max+1));
const jitterVec = (v, d) => v.map((x) => x + rand(-d, d))

var screenFBO;
var fliesFBO;
var currentTick;
var tailsBuffer;
function initFramebuffers() {
  currentTick = 0;
  config.NUM_CRITTERS = config.NUM_LEADERS + config.NUM_FLIES;

  // Initialize flies with random position and velocity.
  let leaderPos = Array.from({length: config.NUM_LEADERS}, () => jitterVec([0,0,0,0], 10));
  let leaderHues = Array.from({length: config.NUM_LEADERS}, () => randInt(0, 12)/12);

  // Spawn 3 groups of flies per leader.
  let groupPos = Array.from({length: config.NUM_LEADERS*3}, () => jitterVec([0,0,0,0], 20));
  let fliesToGroup = Array.from({length: config.NUM_CRITTERS}, () => randInt(0, groupPos.length-1));
  let fliesToLeader = Array.from({length: config.NUM_CRITTERS}, (_, i) => i < config.NUM_LEADERS ? -1 : Math.floor(fliesToGroup[i]/3));

  screenFBO = createDoubleFBO(1, {
    type: 'float32',
    format: 'rgba',
    wrap: 'clamp',
    width: 1920,
    height: 1090,
  });
  fliesFBO = createDoubleFBO(3, {
    type: 'float32',
    format: 'rgba',
    wrap: 'clamp',
    width: config.NUM_CRITTERS,
    height: config.TAIL_LENGTH,
  });

  fliesFBO.src.color[0].subimage({ // position
    width: config.NUM_CRITTERS,
    height: 1,
    data: Array.from({length: config.NUM_CRITTERS}, (_, i) => i < config.NUM_LEADERS ?
      leaderPos[i] :
      jitterVec(groupPos[fliesToGroup[i]], 2))
  });
  fliesFBO.src.color[1].subimage({ // velocity
    width: config.NUM_CRITTERS,
    height: 1,
    data: Array.from({length: config.NUM_CRITTERS*4}, (_, i) => i < config.NUM_LEADERS*4 ? rand(-1, 1) : 0)
  });
  fliesFBO.src.color[2].subimage({ // leaderIndex
    width: config.NUM_CRITTERS,
    height: 1,
    data: Array.from({length: config.NUM_CRITTERS}, (_, i) => i < config.NUM_LEADERS ?
      [-1,leaderHues[i],0,0] :
      [fliesToLeader[i],leaderHues[fliesToLeader[i]],rand(0, 5),0])
  });

  tailsBuffer = Array.from({length: config.TAIL_LENGTH}, (_, i) => i);
}

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
  frag: `#version 300 es
  precision mediump float;
  precision highp int;
  in vec2 ijf;
  in vec4 mousePos;

  uniform sampler2D positionTex;
  uniform sampler2D velocityTex;
  uniform sampler2D scalarTex;
  uniform int writeHistoryIdx, readHistoryIdx;
  uniform float dt;
  uniform float time;
  uniform int NUM_LEADERS, NUM_CRITTERS;

  layout(location = 0) out vec4 fragData0; // position
  layout(location = 1) out vec4 fragData1; // velocity
  layout(location = 2) out vec4 fragData2; // scalars: leaderIndex, hue(leaderOnly), age, 0

  // https://thebookofshaders.com/10/
  float noise(vec2 st) {
    return fract(sin(dot(st,vec2(12.9898,78.233)))*43758.5453123);
  }
  // float flynoise() {
  //   return noise(vec2(ijf.x, 0.));
  // }
  float rand(float offset) {
    return noise(texelFetch(positionTex, ivec2(ijf), 0).xy + vec2(offset, 0));
  }

  // Evil globals for the lazy programmer.
  ivec2 ij;
  ivec2 ijPrev;
  float flynoise;  // constant noise for a given fly.

  vec3 chaseLeader(vec3 pos, vec3 vel, ivec2 leaderIJ) {
    vec3 leaderPos = texelFetch(positionTex, leaderIJ, 0).xyz;
    vec3 dir = leaderPos - pos;
    const float idealDist = 0.2;
    float dist = max(0.01, length(dir));
    float factor = (mousePos.w > 0. && leaderIJ.x == 0) ? 3. : 0.2;
    float acc = max(0.1, pow(dist - idealDist, factor));
    vel += acc*dt*dir;
    return vel;
  }

  vec3 flyTowardsCenter(vec3 pos, vec3 vel, ivec2 leaderIJ) {
    float factor = min(0., .05*sin(time*3.));
    vec3 center = vec3(0,0,0);
    float numNeighbors = 0.;
    for (int i = 0; i < NUM_CRITTERS; i++) {
      int otherLeaderIdx = int(texelFetch(scalarTex, ivec2(i, readHistoryIdx), 0).x);
      if (otherLeaderIdx == leaderIJ.x) {
        center += texelFetch(positionTex, ivec2(i, readHistoryIdx), 0).xyz;
        numNeighbors += 1.;
      }
    }

    if (numNeighbors > 0.) {
      center = center / numNeighbors;
      vel += factor*(center - pos);
    }

    return vel;
  }

  vec3 avoidOthers(vec3 pos, vec3 vel, ivec2 leaderIJ) {
    const float minDistance = .5;
    // Make it periodic to allow occasional clumping because it looks cool.
    float factor = max(0., .05*sin(time));
    vec3 moveV = vec3(0,0,0);
    for (int i = 0; i < NUM_CRITTERS; i++) {
      if (i != int(ijf.x)) {
        vec3 otherPos = texelFetch(positionTex, ivec2(i, readHistoryIdx), 0).xyz;
        if (distance(pos, otherPos) < minDistance)
          moveV += pos - otherPos;
      }
    }

    vel += factor*moveV;
    return vel;
  }

  vec3 matchVelocity(vec3 pos, vec3 vel, ivec2 leaderIJ) {
    const float factor = 0.01;
    vec3 avgVel = vec3(0,0,0);
    float numNeighbors = 0.;
    for (int i = 0; i < NUM_CRITTERS; i++) {
      int otherLeaderIdx = int(texelFetch(scalarTex, ivec2(i, readHistoryIdx), 0).x);
      if (otherLeaderIdx == leaderIJ.x) {
        avgVel += texelFetch(velocityTex, ivec2(i, readHistoryIdx), 0).xyz;
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

  void main () {
    ij = ivec2(ijf);
    ijPrev = ivec2(ij.x, readHistoryIdx);
    flynoise = noise(vec2(ijf.x, 0));

    if (ij.y != writeHistoryIdx) {
      // Not the current history index: this is a tail. Keep data intact.
      fragData0 = texelFetch(positionTex, ij, 0);
      fragData1 = texelFetch(velocityTex, ij, 0);
      fragData2 = texelFetch(scalarTex, ij, 0);
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
      float maxSpeed = (isAffectedByMouse ? 10.0 : 2.7) * (.75 + .5*flynoise);
      const float followTime = 20.;

      vel = chaseLeader(pos, vel, leaderIJ);
      // Turns out it often looks cooler without these other factors.
      // Hypothesis: chasing the same leader already causes flies to match velocities and fly towards the center,
      // and avoiding others prevents the cool-looking effect of crossing paths.
      // vel = flyTowardsCenter(pos, vel, leaderIJ);
      vel = avoidOthers(pos, vel, leaderIJ);
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
        scalars.y = hue + factor + .3*flynoise;
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

      float hueRate = .005*(1.+flynoise);
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
  uniform int writeHistoryIdx, readHistoryIdx;
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
    readHistoryIdx: regl.prop("readHistoryIdx"),
    writeHistoryIdx: regl.prop("writeHistoryIdx"),
    mouseDown: regl.prop("mouseDown"),
    NUM_LEADERS: () => config.NUM_LEADERS,
    NUM_CRITTERS: () => config.NUM_CRITTERS,
  },
  count: 6,
  framebuffer: () => fliesFBO.dst,
});

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
    float scale = .1;
    gl_Position = projection * view * (pointAt(velocity.xyz) * vec4(position.xyz * scale, 1) + vec4(offset.xyz, 0));
    vColor = color;
  }`,

  attributes: {
    position: [[0,0,3], [0,0,-1], [1,0,0], [0,-1,0], [-1,0,0], [0,1,0]],
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
  framebuffer: regl.prop('framebuffer'),
});

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
  uniform float time, dt;
  uniform int TAIL_LENGTH;

  void main() {
    ivec2 ij = ivec2(flyIdx, instanceHistoryIdx);
    vec4 offset = texelFetch(positionTex, ij, 0);
    vec4 velocity = texelFetch(velocityTex, ij, 0);
    vec4 scalars = texelFetch(scalarTex, ij, 0);

    // From updatePositions, we know that pos[i] = pos[i-1] + vel[i]*dt;
    // So pos[i-1] = pos[i] - vel[i]*dt;
    float len = length(velocity)*dt;
    vec4 worldPos = pointAt(velocity.xyz) * vec4(position.xyz * vec3(.15,.15,len), 1) + vec4(offset.xyz, 0);

    float age = mod(1.0 + float(historyIdx - int(instanceHistoryIdx))/float(TAIL_LENGTH), 1.0);
    const vec3 wind = 1.5*normalize(vec3(2,1,1));
    worldPos.xyz += wind * pow(age, 4.);

    gl_Position = projection * view * worldPos;

    vec4 color = hsv2rgb(vec4(scalars.y, .8, .8, 1.));
    vColor = vec4(color.rgb, .7 * alpha * pow(1.-age, 0.7));
  }`,

  attributes: {
    position: [[-1,0,0], [-1,0,-1], [0,0,0], [0,0,-1], [1,0,0], [1,0,-1]],
    alpha: [0, 0, 1, 1, 0, 0,],
    instanceHistoryIdx: {
      buffer: () => tailsBuffer,
      divisor: 1,
    }
  },
  elements: [[0,1,2], [1,2,3], [2,3,4], [3,4,5]],
  primitive: () => config.SOFT_TAILS ? "triangles" : "lines",
  instances: () => config.TAIL_LENGTH,

  uniforms: {
    positionTex: () => fliesFBO.src.color[0],
    velocityTex: () => fliesFBO.src.color[1],
    scalarTex: () => fliesFBO.src.color[2],
    flyIdx: regl.prop('flyIdx'),
    historyIdx: regl.prop('historyIdx'),
    TAIL_LENGTH: () => config.TAIL_LENGTH,
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
      dstAlpha: 1,
    },
    equation: {
      rgb: 'add',
      alpha: 'add'
    },
    color: [0, 0, 0, 0]
  },

  framebuffer: regl.prop('framebuffer'),
});

const bloomBase = (opts) => regl(Object.assign(opts, {
  vert: `#version 300 es
  precision highp float;
  in vec2 position;
  out vec2 vUv;
  out vec2 vL;
  out vec2 vR;
  out vec2 vT;
  out vec2 vB;
  uniform vec2 texelSize;
  void main () {
    vUv = position * 0.5 + 0.5;
    vL = vUv - vec2(texelSize.x, 0.0);
    vR = vUv + vec2(texelSize.x, 0.0);
    vT = vUv + vec2(0.0, texelSize.y);
    vB = vUv - vec2(0.0, texelSize.y);
    gl_Position = vec4(position, 0.0, 1.0);
  }`,

  attributes: {
    position: [[-1, -1], [-1, 1], [1, 1], [-1, -1], [1, 1], [1, -1]]
  },
  count: 6,

  uniforms: Object.assign(opts.uniforms||{}, {
    texelSize: regl.prop("texelSize"),
  }),
  framebuffer: regl.prop("framebuffer"),
}));

const bloomPrefilterShader = bloomBase({
  frag: `#version 300 es
  precision mediump float;
  precision mediump sampler2D;

  in vec2 vUv;
  uniform sampler2D inputTex;
  uniform vec3 curve;
  uniform float threshold;

  out vec4 fragColor;

  void main () {
    vec3 c = texture(inputTex, vUv).rgb;
    float br = max(c.r, max(c.g, c.b));
    float rq = clamp(br - curve.x, 0.0, curve.y);
    rq = curve.z * rq * rq;
    c *= max(rq, br - threshold) / max(br, 0.0001);
    fragColor = vec4(c, 0.0);
  }`,

  uniforms: {
    inputTex: regl.prop("inputTex"),
    curve: () => {
      let config = {BLOOM_THRESHOLD: .8, BLOOM_SOFT_KNEE: .7};
      let knee = config.BLOOM_THRESHOLD * config.BLOOM_SOFT_KNEE + 0.0001;
      return [config.BLOOM_THRESHOLD - knee, knee * 2, 0.25 / knee]
    },
    threshold: .8,
  }
});

const bloomBlurShader = bloomBase({
  frag: `#version 300 es
  precision mediump float;
  precision mediump sampler2D;

  in vec2 vL, vR, vT, vB;
  uniform sampler2D inputTex;

  out vec4 fragColor;

  void main () {
    vec4 sum = vec4(0.0);
    sum += texture(inputTex, vL);
    sum += texture(inputTex, vR);
    sum += texture(inputTex, vT);
    sum += texture(inputTex, vB);
    sum *= 0.25;
    fragColor = sum;
  }`,

  uniforms: {
    inputTex: regl.prop("inputTex"),
  }
});

const bloomFinalShader = bloomBase({
  frag: `#version 300 es
  precision mediump float;
  precision mediump sampler2D;

  in vec2 vL, vR, vT, vB;
  uniform sampler2D inputTex;
  uniform float intensity;

  out vec4 fragColor;

  void main () {
    vec4 sum = vec4(0.0);
    sum += texture(inputTex, vL);
    sum += texture(inputTex, vR);
    sum += texture(inputTex, vT);
    sum += texture(inputTex, vB);
    sum *= 0.25;
    fragColor = sum * intensity;
  }`,

  uniforms: {
    inputTex: regl.prop("inputTex"),
    intensity: .7,
  }
});

const drawScreen = bloomBase({
  frag: `#version 300 es
  precision highp float;
  precision highp sampler2D;
//#define BLOOM 1

  in vec2 vUv;
  in vec2 vL, vR, vT, vB;
  uniform sampler2D uTexture;
  // uniform sampler2D uBloom;
  // uniform sampler2D uDithering;
  // uniform vec2 ditherScale;

  out vec4 fragColor;

  vec3 linearToGamma (vec3 color) {
    color = max(color, vec3(0));
    return max(1.055 * pow(color, vec3(0.416666667)) - 0.055, vec3(0));
  }

  void main () {
    vec3 c = texture(uTexture, vUv).rgb;

#ifdef BLOOM
    vec3 bloom = texture(uBloom, vUv).rgb;

    float noise = 0.5;
    // float noise = texture(uDithering, vUv * ditherScale).r;
    noise = noise * 2.0 - 1.0;
    bloom += noise / 255.0;
    bloom = linearToGamma(bloom);
    c += bloom;
#endif

    float a = max(c.r, max(c.g, c.b));
    fragColor = vec4(c, 1.);
  }`,

  uniforms: {
    uTexture: regl.prop("screen"),
    // uBloom: regl.prop("bloom"),
  }
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

const globalScope = regl({
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
    time: regl.context("time"),
    dt: 1./24., // using a constant time step seems to look better
  }
})

regl.frame(function (context) {
  if (!config.NUM_LEADERS)
    return;

  window.fbo = screenFBO.dst;
  let mouseDown = [-1,-1, -1, -1];
  for (let pointer of pointers.pointers) {
    if (pointer.isDown) {
      mouseDown = [pointer.pos[0] - .5, pointer.pos[1] - .5, 0, 1];
      mouseDown[0] *= 20;
      mouseDown[1] *= 7;
      break;
    }
  }

  globalScope(() => {
    regl.clear({color: [0, 0, 0, 1]});
    regl.clear({color: [0, 0, 0, 1], framebuffer: screenFBO.dst});

    let readHistoryIdx = currentTick % config.TAIL_LENGTH;
    let writeHistoryIdx = (currentTick+1) % config.TAIL_LENGTH;
    // testDraw({quantity: fliesFBO.src.color[0]});

    const drawLeaders = false;
    for (let i = 0; i < config.NUM_CRITTERS; i++) {
      if (i < config.NUM_LEADERS && !drawLeaders)
        continue;
      drawFly({flyIdx: i, historyIdx: readHistoryIdx, framebuffer: screenFBO.dst});
      drawTails({flyIdx: i, historyIdx: readHistoryIdx, framebuffer: screenFBO.dst});
    }
    screenFBO.swap();

    drawScreen({screen: screenFBO.src, texelSize: [1/screenFBO.src.width, 1/screenFBO.src.height]});

    if (!config.paused) {
      updatePositions({writeHistoryIdx: writeHistoryIdx, readHistoryIdx: readHistoryIdx, mouseDown: mouseDown});
      fliesFBO.swap();
      currentTick++;
    }
  });
});