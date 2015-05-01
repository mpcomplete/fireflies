#ifndef _UTILS_H
#define _UTILS_H

#include "main.h"
#include <gfx/mat4.h>
#include <deque>
#include <vector>
#include <utility>

#define SIGN(x) ((x >= 0) ? 1 : -1)

#define MAX3(x, y, z) ( (x>y) ? ((x>z) ? x : z) : y )
#define MIN3(x, y, z) ( (x<y) ? ((x<z) ? x : z) : y )

typedef Vec4f rgbColor;
typedef Vec4f hsvColor;

// clamp vector's components each to +/- magnitude
void clamp_vec(Vec3f& vec, double max);

// return a random number between lo and hi inclusive
inline int rand_int(int lo, int hi)
{ return lo + (int)((hi-lo+1)*((double)rand()/(double)(RAND_MAX+1.0))); }

inline double rand_real(double lo, double hi)
{ return lo + (hi-lo)*((double)rand()/(double)(RAND_MAX+1.0)); }

inline Vec3f rand_vec3(double lo, double hi)
{ return Vec3f(rand_real(lo, hi), rand_real(lo, hi), rand_real(lo, hi)); }

// return a random int other than 'other'
int rand_other(int lo, int hi, int other);

// return the unit vector in the direction of v
inline Vec3f unit_vec(const Vec3f& v)
{ double n = norm(v); return (n == 0) ? v : v/n; }

// color space conversion
hsvColor rgb_to_hsv(const rgbColor& rgb);
rgbColor hsv_to_rgb(const hsvColor& hsv);

// a set of events and the time for them to occur
class Timer {
public:
    typedef pair<int, double> Event;
    deque<Event> events;

    Timer() {}
    // add an event that needs to be done at time 'when'
    void add(int what, double when);
    // checks if an event is ready, ie the current time (now) is past one
    // of the times for an event.
    bool is_ready(double now);
    // clears all events
    void clear();
    // return the earliest-to-occur event. note: this (obviously) does not
    // check that the current time is past the event's time.
    int pop();
};

// a random variable which takes on a given value with a given probability
class RandVar {
public:
    typedef pair<int, double> Event;
    vector<Event> events;
    double max_prob; // the sum of probabilities of all events

    RandVar() : max_prob(0.0) {}

    // add a value and it's probability weight to the set
    void add(int val, double prob);
    // change a value's probability weight. if val is not in the set,
    // it is added.
    void change(int val, double newprob);
    // clear all events
    void clear();
    // return one of the values based on the weighted probability of each
    // value. for example, if value '0' has probability 0.9, it will be
    // returned 90% of the time.
    int rand();
};

#endif // _UTILS_H
