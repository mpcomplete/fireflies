#include "firefly.h"
#include "scene.h"
#include "modes.h"

Firefly::Firefly(Bait* _bait, Vec3f ctr, double spread)
    : Arrow(), bait(_bait), age(0.) {
  pos = ctr;
  pos += rand_vec3(-spread, spread);

  velocity = bait->fspeed * unit_vec(bait->pos - pos);
  tail = new Tail(this);
}

Firefly::~Firefly() {
  tail->owner = 0;
  scene.dead_tails.push_back(tail);
}

void Firefly::draw() {
  // glPushMatrix();
  // Arrow::draw();
  // glPopMatrix();

  tail->draw();
}

void Firefly::elapse(double t) {
  age += t;

  calc_accel();
  velocity += accel * t;
  clamp_vec(velocity, bait->fspeed);
  pos += velocity * t;

  point(velocity);
  set_color();

  // elapse, my children
  tail->elapse(t);
}

void Firefly::calc_accel() {
  double bait_accel = bait->repel > 0.0 ? -bait->faccel : bait->faccel;
  accel = bait_accel * unit_vec(bait->pos - pos);
}

void Firefly::set_color() {
  hsv = bait->hsv;
  hsv[0] += 40 * norm2(velocity) / (bait->fspeed * bait->fspeed) - 20;

  // clamp to my range
  while (hsv[0] > 360.f)
    hsv[0] -= 360.f;
  while (hsv[0] < 0.f)
    hsv[0] += 360.f;

  color = hsv_to_rgb(hsv);
}
