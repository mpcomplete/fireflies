#include "tail.h"
#include "firefly.h"
#include "scene.h"

Tail::Tail(Firefly* _owner) : owner(_owner) {}

#define SET_COLOR(c, a) glColor4f(c[0], c[1], c[2], a)
#define SET_VERTEX(v, dx) glVertex3d(v[0] + dx, v[1], v[2])
#define DO_POINT(t, dx, a) \
  SET_COLOR((t).color, a); \
  SET_VERTEX((t).pos, dx)

void Tail::draw() {
  if (links.size() < 2)  // need at least 2 links
    return;

  deque<Link>::iterator it = links.begin();
  double glow_width = scene.glow_factor * scene.tail_width;
  double stretch_factor = 2 * scene.fsize * scene.wind[0];
  double dx1, dx2;

  dx2 = ((*it).glow ? glow_width : scene.tail_width);
  for (; (it + 1) != links.end(); it++) {
    // half-width of the tail
    dx1 = dx2;
    dx2 = ((*(it + 1)).glow ? glow_width : scene.tail_width);

    // have the wind stretch the tail (greater effect on ends)
    double age = (*it).age / scene.tail_length;
    double stretch = stretch_factor * age * age;
    double alpha = 0.9 - age;
    if (alpha > scene.tail_opaq)
      alpha = scene.tail_opaq;

    // two rectangles: outer vertices have alpha=0, inner two have
    // alpha based on age. note: alpha goes negative, but opengl
    // should clamp it to 0.
    if (stretch > 0) {  // stretch to the right
      glBegin(GL_QUAD_STRIP);
      DO_POINT(*it, -dx1, 0);
      DO_POINT(*(it + 1), -dx2, 0);

      DO_POINT(*it, 0, alpha);
      DO_POINT(*(it + 1), 0, alpha);

      DO_POINT(*it, dx1 + stretch, 0);
      DO_POINT(*(it + 1), dx2 + stretch, 0);
    } else {  // stretch to the left
      glBegin(GL_QUAD_STRIP);
      DO_POINT(*it, -dx1 + stretch, 0);
      DO_POINT(*(it + 1), -dx2 + stretch, 0);

      DO_POINT(*it, 0, alpha);
      DO_POINT(*(it + 1), 0, alpha);

      DO_POINT(*it, dx1, 0);
      DO_POINT(*(it + 1), dx2, 0);
    }
    glEnd();
  }
}

bool Tail::elapse(double t) {
  // pop off the dead ones.
  // note we only have to check the end, since that's where they're gonna
  // be dying from.  deque is very nice for this, because it has constant
  // time  insertion/removal from both ends.
  while (!links.empty() && links.back().age >= scene.tail_length)
    links.pop_back();

  deque<Link>::iterator it = links.begin();
  for (; it != links.end(); it++) {
    (*it).age += t;
    double age = (*it).age / scene.tail_length;
    (*it).pos += scene.wind * age * age;
  }

  if (owner == 0)          // my owner died! grow no longer
    return links.empty();  // if we're empty, tell caller we're dead

  links.push_front(Link(owner->pos, owner->color, owner->bait->glow));

  return false;
}
