#include "scene.h"
#include "modes.h"

#include <GL/glu.h>

Vec3f world;

#define WIND_WAIT rand_real(4 * tail_length, 8 * tail_length)
#define OFFSCREEN_VEC3() rand_vec3(-2 * world[0], 2 * world[0])

// max elapse time is 1/10th of a second so we don't get jaggies even if
// the CPU load is high and everything slows down
#define MAX_ELAPSE 0.1

Scene::Scene() : matrix(-1.0) {
  set_defaults();
}

Scene::~Scene() {
  GLuint i;
  for (i = 0; i < baits.size(); i++)
    delete baits[i];
  for (i = 0; i < flies.size(); i++)
    delete flies[i];
}

void Scene::set_defaults() {
  bmodes.clear();
  smodes.clear();

  // all bmodes equally likely
  for (int i = 0; i < NUM_BMODES; i++)
    bmodes.add(i, 10);

  // except these
  bmodes.change(BMODE_NORMAL, 20);
  bmodes.change(BMODE_GLOW, 15);

  // all smodes equally likely
  for (int i = 0; i < NUM_SMODES; i++)
    smodes.add(i, 10);

  // except...
  smodes.change(SMODE_SWARMS, 5);

  fast_forward = 1;
  minbaits = 2;
  maxbaits = 5;
  minflies = 100;
  maxflies = 175;
  fsize = 1.5;
  bspeed = 50.;
  baccel = 600.;
  fspeed = 100.;
  faccel = 300.;
  hue_rate = 15.;
  tail_length = 2.25;
  tail_width = 2.5;
  tail_opaq = 0.6;
  glow_factor = 2.;
  wind_speed = 3.;
  draw_bait = false;
}

void Scene::create() {
  GLuint i, nbaits, nflies;

  curtime = 0.0;
  wind_when = curtime + WIND_WAIT;
  scene_start_mode(-1);  // non-existent, just to initialize

  nbaits = (minbaits + maxbaits) / 2;
  nflies = (minflies + maxflies) / 2;

  baits.reserve(nbaits);
  for (i = 0; i < nbaits; i++) {
    baits.push_back(new Bait());
  }

  add_flies(nflies);

  for (i = 0; i < 3; i++) {
    switch (rand_int(0, 1)) {
      case 0:
        accel[i] = -1;
        wind[i] = -wind_speed;
        break;
      default:
        accel[i] = 1;
        wind[i] = wind_speed;
        break;
    }
  }
}

void Scene::add_flies(unsigned n) {
  if (flies.size() >= maxflies)
    return;
  if (flies.size() + n >= maxflies)
    n = maxflies - flies.size();

  // about 3 groups per bait
  int groupsize = (flies.size() + n) / (3 * baits.size());
  if (groupsize < 10)  // but at least size 10
    groupsize = 10;
  Vec3f where;
  Bait* b = baits[0];
  flies.reserve(flies.size() + n);
  for (unsigned i = 0; i < n; i++) {
    if ((i % groupsize) == 0) {
      where = OFFSCREEN_VEC3();
      b = baits[rand_int(0, baits.size() - 1)];
    }
    flies.push_back(new Firefly(b, where, world[2] / 3));
  }
}

void Scene::rem_flies(unsigned n) {
  if (flies.size() <= minflies)
    return;
  if (flies.size() - n <= minflies)
    n = flies.size() - minflies;

  Bait* b = baits[rand_int(0, baits.size() - 1)];
  vector<Firefly*>::iterator it = flies.begin();
  while (it != flies.end() && n > 0) {
    if ((*it)->bait == b) {
      delete (*it);
      it = flies.erase(it);
      n--;
    } else
      it++;
  }
}

void Scene::resize(int width, int height) {
  GLfloat aspect = (GLfloat)width / (GLfloat)height;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(80., aspect, 5, 2000);

  world[2] = 50.;
  if (width > height) {
    world[1] = 80.;
    world[0] = world[1] * (width) / height;
  } else {
    world[0] = 80.;
    world[1] = world[0] * (height) / width;
  }
  camera.pos = Vec3f(0., 0., 3 * world[2]);

  // For some reason this needs to be done everytime we resize, otherwise
  // blending is disabled (and I assume other functions)
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
}

void Scene::apply_camera(const Vec3& offset) {
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslated(-camera.pos[0] + offset[0], -camera.pos[1] + offset[1],
               -camera.pos[2] + offset[2]);
  glRotated(camera.rot_angle, camera.rot_axis[0], camera.rot_axis[1],
            camera.rot_axis[2]);
}

void Scene::draw() {
#if 0
    glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    draw_box(-world, world);
#endif

  for (GLuint i = 0; i < baits.size(); i++)
    baits[i]->draw();

  for (GLuint i = 0; i < flies.size(); i++)
    flies[i]->draw();

  vector<Tail*>::iterator it = dead_tails.begin();
  for (; it != dead_tails.end(); it++)
    (*it)->draw();
}

void Scene::elapse(double t) {
  for (unsigned i = 0; i < fast_forward; i++)
    elapse_once(t);
}

void Scene::elapse_once(double t) {
  if (t > MAX_ELAPSE)
    t = MAX_ELAPSE;
  // matrix mode?
  if (matrix > 0) {
    matrix += t;
    if (matrix >= mode_when)
      scene_start_mode(-SMODE_MATRIX);
    else if (matrix - curtime >= 0.5) {
      Quat q = axis_to_quat(camera.rot_axis, DEG_TO_RAD(camera.rot_angle));
      Quat dq = axis_to_quat(matrix_axis, 0.7 * t);

      q = q * dq;
      unitize(q);

      camera.rot_axis = q.vector();
      camera.rot_angle = RAD_TO_DEG(2.0 * acos(q.scalar()));
    }
    return;
  }

  curtime += t;
  if (curtime >= mode_when)
    scene_start_mode(mode_next);

  // the wind, she's a changin!
  if (curtime >= wind_when) {
    for (int i = 0; i < 3; i++) {
      if (rand_int(0, 1) == 0)
        accel[i] = -accel[i];
    }
    // next change based on tail length (so we can see a whole cycle of
    // prettiness blow one way before it gets tossed another)
    wind_when = curtime + WIND_WAIT;
  }

  wind += accel * t;
  clamp_vec(wind, wind_speed);

  // elapse, my children
  for (GLuint i = 0; i < baits.size(); i++)
    baits[i]->elapse(t);

  for (GLuint i = 0; i < flies.size(); i++)
    flies[i]->elapse(t);

  vector<Tail*>::iterator it = dead_tails.begin();
  while (it != dead_tails.end()) {
    if ((*it)->elapse(t)) {  // he's dead!
      delete (*it);
      it = dead_tails.erase(it);
    } else
      it++;
  }
}
