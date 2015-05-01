#ifndef _CONTROL_H
#define _CONTROL_H

#include "main.h"
#include <gfx/vec3.h>

#define DEG_TO_RAD(angle) (angle*M_PI/180.0)
#define RAD_TO_DEG(angle) (angle*180.0/M_PI)

// a set of controls for objects and the camera. directly corresponds to
// OpenGL calls.
class Control {
public:
    Vec3f pos;
    double rot_angle; // in degrees
    Vec3f rot_axis;

    Control() : rot_angle(0)
    {}
    void apply_transform()
    {
	glTranslated(pos[0], pos[1], pos[2]);
	glRotated(rot_angle, rot_axis[0], rot_axis[1], rot_axis[2]);
#if 0
	// we don't use this
	glScaled(scale[0], scale[1], scale[2]);
#endif
    }
};

#endif // _CONTROL_H
