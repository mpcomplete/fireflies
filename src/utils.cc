#include "utils.h"

#ifndef DEBUG
#define NDEBUG
#endif
#include <assert.h>

void clamp_vec(Vec3f& vec, double max)
{
    for (int i = 0; i < 3; i++) {
	if (vec[i] > max)	    vec[i] = max;
	else if (vec[i] < -max)	    vec[i] = -max;
    }
}

int rand_other(int lo, int hi, int other)
{
    int r;
    assert(lo != hi);
    while ((r = rand_int(lo, hi)) == other)
	;
    return r;
}

// rgb range [0,1]
// hsv range hue:[0,360], sat and val:[0,1]
hsvColor rgb_to_hsv(const rgbColor& rgb)
{
    hsvColor hsv;
    float max, min, diff;

    hsv[3] = rgb[3]; // alpha value

    max = MAX3(rgb[0], rgb[1], rgb[2]);
    min = MIN3(rgb[0], rgb[1], rgb[2]);
    diff = max - min;

    hsv[2] = max; 	// value = max

    if (FEQ(max, 0)) {
	hsv[0] = 0;	// hue should be undefined.. oh well
	hsv[1] = 0;	// saturation = 0
	return hsv;
    }
    else
	hsv[1] = diff/max; // saturation

    if (rgb[0] == max)		// red is max
	hsv[0] = (rgb[1] - rgb[2]) / diff;	// hue between yellow and mag
    else if (rgb[1] == max)	// green is max
	hsv[0] = 2 + (rgb[2] - rgb[1]) / diff;	// hue between cyan and yellow
    else			// blue is max
	hsv[0] = 4 + (rgb[0] - rgb[1]) / diff;	// hue between magenta and cyan

    hsv[0] *= 60.;	// degrees
    if (hsv[0] < 0)
	hsv[0] += 360.;

    return hsv;
}

rgbColor hsv_to_rgb(const hsvColor& hsv)
{
    rgbColor rgb;
    float h = hsv[0]/60, s = hsv[1], v = hsv[2];
    float f, p, q, t;
    int i;

    rgb[3] = hsv[3]; // alpha value

    if (FEQ(s, 0)) {
	rgb[0] = rgb[1] = rgb[2] = v;
	return rgb;
    }

    // don't ask me what this means.. I yoinked it from a website.
    i = (int)floor(h); // sectors 0 - 5
    f = h - i;
    p = v*(1 - s);
    q = v*(1 - s*f);
    t = v*(1 - s*(1-f));

    switch (i) { // what sector?
    case 0:  rgb[0] = v; rgb[1] = t; rgb[2] = p; break;
    case 1:  rgb[0] = q; rgb[1] = v; rgb[2] = p; break;
    case 2:  rgb[0] = p; rgb[1] = v; rgb[2] = t; break;
    case 3:  rgb[0] = p; rgb[1] = q; rgb[2] = v; break;
    case 4:  rgb[0] = t; rgb[1] = p; rgb[2] = v; break;
    default: rgb[0] = v; rgb[1] = p; rgb[2] = q; break;
    }
    return rgb;
}

void Timer::add(int what, double when)
{
    deque<Event>::iterator it = events.begin();
    while (it != events.end()) {
	if (when < (*it).second)
	    break;
	it++;
    }
    events.insert(it, Event(what, when));
}

bool Timer::is_ready(double now)
{
    return (!events.empty() && (now >= events[0].second));
}

void Timer::clear()
{
    events.clear();
}

int Timer::pop()
{
    int what = events[0].first;
    events.pop_front();
    return what;
}

void RandVar::add(int val, double prob)
{
    max_prob += prob;
    events.push_back(Event(val, prob));
}

void RandVar::change(int val, double newprob)
{
    for (size_t i = 0; i < events.size(); i++) {
	if (events[i].first == val) {
	    max_prob += (newprob - events[i].second);
	    events[i].second = newprob;
#ifdef DEBUG
	    cerr << val << " changed to " << newprob << " out of " <<
		max_prob << endl;
#endif
	    return;
	}
    }
    // didn't find a match, so add it
    add(val, newprob);
}

void RandVar::clear()
{
    events.clear();
    max_prob = 0.0;
}

int RandVar::rand()
{
    double r = rand_real(0., max_prob);
    double prob = 0.;
    for (size_t i = 0; i < events.size(); i++) {
	prob += events[i].second;
	if (r < prob)
	    return events[i].first;
    }
    return -1; // should never happen
}
