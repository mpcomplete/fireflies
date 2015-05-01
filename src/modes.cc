#include "modes.h"
#include "bait.h"
#include "scene.h"

#define BMODE_WAIT 	rand_real(10., 15.)
#define SMODE_WAIT	rand_real(10., 20.)

void bait_start_mode(Bait *b, int mode)
{
    b->mode_next = scene.bmodes.rand();
    b->mode_when = b->age + BMODE_WAIT;

    switch (mode) {
    case BMODE_NORMAL: // normal mode
	if (b->attractor) {
	    delete b->attractor;
	    b->attractor = 0;
	}
	b->bspeed = b->fuzz*scene.bspeed;
	b->baccel = b->fuzz*scene.baccel;
	b->fspeed = b->fuzz*scene.fspeed;
	b->faccel = b->fuzz*scene.faccel;

	b->hue_rate = b->fuzz*scene.hue_rate;
	break;
    case BMODE_STOP: // stop mode
	if (b->attractor || b->bspeed==0) // don't do it if we're already doing it
	    break;
	b->bspeed = 0;
	b->stop_timer.add(mode, b->age + rand_real(2., 3.));
	break;
    case BMODE_ATTRACTOR: // attractor
	if (b->attractor || b->bspeed==0) // don't do it if we're already doing it
	    break;
	b->attractor = new Vec3f(b->pos + rand_vec3(-10, 10));
	b->baccel = (b->bspeed*b->bspeed/20.0);
	b->stop_timer.add(mode, b->age + rand_real(5., 10.));
	break;
    case BMODE_RAINBOW: // rainbow mode
	b->hue_rate = rand_int(10, 15)*scene.hue_rate;
	b->stop_timer.add(mode, b->age + rand_real(10., 15.));
	break;
    case BMODE_GLOW: // glow mode
	b->glow = true;
	b->stop_timer.add(mode, b->age + rand_real(10., 20.));
	break;
    case BMODE_HYPERSPEED: // hyperspeed mode
	b->bspeed = 1.5*scene.bspeed;
	b->baccel = 1.5*scene.baccel;
	b->fspeed = 2*scene.fspeed;
	b->faccel = 3*scene.faccel;
	b->stop_timer.add(mode, b->age + rand_real(10., 20.));
	break;
    case BMODE_FADED: // faded color mode
	b->hsv[1] = rand_real(0.4, 0.6);
	b->stop_timer.add(mode, b->age + rand_real(10., 20.));
	break;
    }

#ifdef DEBUG
    cerr << "baitmode=" << mode << " for " 
	<< (b->stop_timer.events.empty() ? 0 : b->stop_timer.events[0].second-b->age)
	<< "s\tnext=" << b->mode_next << " in " << b->mode_when-b->age << "s" << endl;
#endif
}

void bait_stop_mode(Bait* b, int mode)
{
    switch (mode) {
    case BMODE_NOTHING:
    case BMODE_NORMAL:
	return;
	break;
    case BMODE_STOP: // anti stop mode
	b->bspeed = b->fuzz*scene.bspeed;
	break;
    case BMODE_ATTRACTOR: // anti attractor
	delete b->attractor;
	b->attractor = 0;
	b->baccel = b->fuzz*scene.baccel;
	break;
    case BMODE_RAINBOW: // anti rainbow mode
	b->hue_rate = b->fuzz*scene.hue_rate;
	break;
    case BMODE_GLOW: // anti glow mode
	b->glow = false;
	break;
    case BMODE_HYPERSPEED: // anti hyperspeed mode
	b->bspeed = b->fuzz*scene.bspeed;
	b->baccel = b->fuzz*scene.baccel;
	b->fspeed = b->fuzz*scene.fspeed;
	b->faccel = b->fuzz*scene.faccel;
	break;
    case BMODE_FADED:
	b->hsv[1] = 0.8;
	break;
    }

#ifdef DEBUG
    cerr << "\tstopped=" << mode << endl;
#endif
}

void scene_start_mode(int mode)
{
    scene.mode_next = scene.smodes.rand();
    scene.mode_when = scene.curtime + SMODE_WAIT;

    // negatives keep them off the modelist
    switch (mode) {
    case SMODE_SWARMS: { // all-swarm mode
	int bmode = scene.bmodes.rand();
	if (bmode < 0)
	    break;
	for (GLuint i = 0; i < scene.baits.size(); i++) {
	    scene.baits[i]->stop_timer.clear(); // clear out any stops
	    bait_start_mode(scene.baits[i], BMODE_NORMAL); // set default.
	    bait_start_mode(scene.baits[i], bmode);
	}
	break;
	    }
    case SMODE_FLYKILL: { // firefly go byebye
	if (scene.flies.size() <= scene.minflies) // not too few
	    break;
	int max = (scene.flies.size() - scene.minflies);
	int n = rand_int(max/3, max);
	scene.rem_flies(n);
#ifdef DEBUG
	cerr << "deleted " << n << " flies" << endl;
#endif
	break;
	    }
    case SMODE_FLYBIRTH: { // hello firefly
	if (scene.flies.size() >= scene.maxflies) // not too many
	    break;
	int max = (scene.maxflies - scene.flies.size());
	int n = rand_int(max/3, max);
	scene.add_flies(n);
#ifdef DEBUG
	cerr << "created " << n << " flies" << endl;
#endif
	break;
	    }
    case SMODE_WINDY: // it's getting windy in here!
	scene.wind_speed *= 3;
	scene.mode_next = -SMODE_WINDY;
	break;
    case -SMODE_WINDY: // calm the wind down
	scene.wind_speed /= 3;
	break;
    case SMODE_MATRIX: {// matrix mode
	scene.matrix = scene.curtime;
	scene.matrix_axis = Vec3f(rand_int(-1, 1), rand_int(-1, 1), rand_int(-1, 1));
	if (norm2(scene.matrix_axis) == 0)
	    scene.matrix_axis = Vec3f(0, 1, 0);
	unitize(scene.matrix_axis);
	scene.mode_next = -SMODE_MATRIX;
	scene.mode_when = scene.curtime + rand_real(4.0, 5.0);
	break;
	    }
    case -SMODE_MATRIX: // anti matrix
	scene.matrix = -1.0;
	break;
    case SMODE_SWARMSPLIT: { // split mode
	if (scene.baits.size() >= scene.maxbaits) // not too many
	    break;
	Bait *b1 = scene.baits[rand_int(0, scene.baits.size()-1)];
	Bait *b2 = new Bait();
	b2->pos = b1->pos;
	scene.baits.push_back(b2);
	double fpb = (double)scene.flies.size()/scene.baits.size();
	int n = rand_int((int)(fpb/4), (int)(fpb/2));
	for (GLuint i = 0; i < scene.flies.size() && n > 0; i++) {
	    if (scene.flies[i]->bait == b1) {
		scene.flies[i]->bait = b2;
		scene.flies[i]->age = 0.0;
		n--;
	    }
	}
	break;
	    }
    case SMODE_SWARMMERGE: { // merge mode
	if (scene.baits.size() <= scene.minbaits)
	    break;
	int i1 = rand_int(0, scene.baits.size()-1);
	int i2 = rand_other(0, scene.baits.size()-1, i1);
	Bait *b1 = scene.baits[i1];
	Bait *b2 = scene.baits[i2];
	
	for (GLuint i = 0; i < scene.flies.size(); i++) {
	    if (scene.flies[i]->bait == b2) {
		scene.flies[i]->bait = b1;
		scene.flies[i]->age = 0.0;
	    }
	}
	vector<Bait*>::iterator it = scene.baits.begin();
	for (; it != scene.baits.end(); it++) {
	    if ((*it) == b2) {
		scene.baits.erase(it);
		break;
	    }
	}
	delete b2;
	break;
	    }
    }
#ifdef DEBUG
    cerr << "scenemode=" << mode << ", next=" << scene.mode_next << " in "
	 << scene.mode_when-scene.curtime << endl;
#endif
}
