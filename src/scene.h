#ifndef _SCENE_H
#define _SCENE_H

#include "main.h"

#include "control.h"
#include "bait.h"
#include "firefly.h"
#include "tail.h"

#include <gfx/quat.h>
#include <vector>

class Scene {
 public:
  vector<Bait*> baits;
  vector<Firefly*> flies;
  vector<Tail*> dead_tails;

  Control camera;     // camera orientation
  double curtime;     // total time the program's been running
  Vec3f wind;         // current wind direction
  Vec3f accel;        // wind is changing
  double wind_when;   // time before next wind change
  int mode_next;      // the next mode to activate
  double mode_when;   // next time to activate a mode
  double matrix;      // -1 if not active, else a timer for how long
                      // the "matrix" mode has been active
  Vec3f matrix_axis;  // the axis to rotate around matrix-style
  Mat4 projectionMatrix;
  Mat4 viewMatrix;
  Mat4 viewProjectionInverse;

  // options
  RandVar smodes;  // enabled modes for scene
  RandVar bmodes;  // enabled modes for baits

  unsigned fast_forward;
  unsigned minbaits;
  unsigned maxbaits;
  unsigned minflies;
  unsigned maxflies;
  double fsize;
  double bspeed;
  double baccel;
  double fspeed;
  double faccel;
  double hue_rate;
  double tail_length;
  double tail_width;
  double tail_opaq;
  double glow_factor;
  double wind_speed;
  bool draw_bait;

  Scene();
  ~Scene();

  // set default options (called from constructor)
  void set_defaults();
  // create the scene with the following parameters
  // NOTE: the scene does not exist until this is called!
  void create();
  // add 'n' flies to random baits
  void add_flies(unsigned n);
  // remove 'n' flies from random baits
  void rem_flies(unsigned n);
  // resize the scene.
  void resize(int width, int height);
  // apply the camera transformations (translate+rotate)
  void apply_camera(const Vec3& offset);
  // draw the scene (CREATE it first!)
  void draw();
  // animation: let t seconds elapse (fast_forward times)
  void elapse(double t);
  // animation: let t seconds elapse once
  void elapse_once(double t);
  Vec3 getWorldPos(int winX, int winY, int winWidth, int winHeight);
};

extern Vec3f world;
extern Scene scene;

#endif  // scene.h
