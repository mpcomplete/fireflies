#ifndef _Arrow_H
#define _Arrow_H

#define DEG_TO_RAD(angle) (angle*M_PI/180.0)
#define RAD_TO_DEG(angle) (angle*180.0/M_PI)

#include "control.h"
#include "utils.h"

class Arrow : public Control
{
public:
    hsvColor hsv;
    rgbColor color;
    Vec3f velocity;	// current velocity
    Vec3f accel;		// current acceleration

    Arrow() : hsv(0.0f, 0.8f, 0.8f, 1.0f) {}
    virtual ~Arrow() {}

    // draw the Arrow
    virtual void draw();
    // let t seconds elapse
    virtual void elapse(double t) = 0;
    // point me in direction of 'dir'.
    void point(Vec3f dir);
};

void draw_box(const Vec3f& min, const Vec3f& max);

#endif // Arrow.h
