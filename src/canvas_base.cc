#include "canvas_base.h"

#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

CanvasBase::CanvasBase(Scene *s, bool fs, int m)
		: scene(s), full_screen(fs), mspf(m)
{
	animate = true;
	need_refresh = true;
	width = height = 0;
}

int CanvasBase::create_window()
{
	// it would be nice if this were pure virtual, but that makes my
	// version of g++ fucks up for some reason.
	return -1;
}

int CanvasBase::init()
{
#ifdef WIN32
	srand(time(0));
#else

	struct timeval tv;
	gettimeofday(&tv, 0);
	srand(1000*tv.tv_sec + tv.tv_usec / 1000);
#endif

	int ret;
	if ((ret = create_window()) < 0)
		return ret;

	resize();

	return 0;
}

void CanvasBase::resize()
{
	glViewport(0, 0, width, height);
	scene->resize(width, height);
}

void CanvasBase::draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	scene->apply_camera(Vec3(0,0,0));
	scene->draw();
}

int CanvasBase::loop()
{
	int remain, ret;
	last_tick = get_ms();

	while (true) {
		if ((remain = tick()) > 0) // time left till tick: sleep
			delay(remain);
		if ((ret = handle_events()) > 0) // wants us to quit
			return ret;
	}
}

int CanvasBase::tick()
{
	if (need_refresh) {
		draw();
		need_refresh = false;
	}

	int now = get_ms();
	int ms = now - last_tick;

	// Animate the scene
	if (ms >= mspf) {
		last_tick = now;
		if (animate) {
			scene->elapse(ms / 1000.0);
			need_refresh = true;
		}
		return 0;
	}
	return (mspf - ms);
}

// these are dumb.  See create_window()
int CanvasBase::handle_events()
{
	return 1;
}

int CanvasBase::get_ms()
{
	return 0;
}

void CanvasBase::delay(int ms)
{}
