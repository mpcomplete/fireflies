#include "firefly.h"
#include "scene.h"
#include "modes.h"

Firefly::Firefly(Bait *_bait, Vec3f ctr, double spread)
    : Arrow(), bait(_bait), age(0.)
{
    pos = ctr;
    pos += rand_vec3(-spread, spread);

    velocity = bait->fspeed*unit_vec(bait->pos - pos);
    tail = new Tail(this);
}

Firefly::~Firefly()
{
    tail->owner = 0;
    scene.dead_tails.push_back(tail);
}

void Firefly::draw()
{
    glPushMatrix();
    Arrow::draw();
    glPopMatrix();

    tail->draw();
}

void Firefly::elapse(double t)
{
    age += t;

    calc_accel();
    velocity += accel*t;
    clamp_vec(velocity, bait->fspeed);
    pos += velocity*t;

    point(velocity);
    set_color();

    // elapse, my children
    tail->elapse(t);
}

void Firefly::calc_accel()
{
    if (age > 2.0 && rand_int(0, 60)==0) {
	GLuint i, closest_i = 0;
	double dist, closest_dist = 1e10;
	for (i = 0; i < scene.baits.size(); i++) {
	    if ((dist=norm(scene.baits[i]->pos - pos)) < closest_dist) {
		closest_dist = dist;
		closest_i = i;
	    }
	}
	dist = norm(bait->pos - pos);
	if (closest_dist < dist-1.0) {
	    bait = scene.baits[closest_i];
	    age = 0.;
	}
    }
    else if (age > 5.0) {
	if (norm(bait->pos - pos) >= 200 && !(bait->bspeed == 0 || bait->attractor)) {
	    bait->mode_next = BMODE_STOP;
	}
    }

    accel = bait->faccel*unit_vec(bait->pos - pos);
}

void Firefly::set_color()
{
    hsv = bait->hsv;
    hsv[0] += 40*norm2(velocity)/(bait->fspeed*bait->fspeed) - 20;

    // clamp to my range
    while (hsv[0] > 360.f)
	hsv[0] -= 360.f;
    while (hsv[0] < 0.f)
	hsv[0] += 360.f;

    color = hsv_to_rgb(hsv);
}
