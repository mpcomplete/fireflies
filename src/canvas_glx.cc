#include "canvas_glx.h"

#include <sys/time.h>
#include <unistd.h>

#include "vroot.h"

CanvasGLX::CanvasGLX(Scene *s, bool fs, int m, Window wid)
    : CanvasBase(s, fs, m), window_id(wid)
{
    window = 0;
    display = 0;
}

int CanvasGLX::create_window()
{
    if ((display = XOpenDisplay(0)) == 0)
	return -1;
    screen = DefaultScreen(display);

    window = window_id ? window_id : RootWindow(display, screen);
    XWindowAttributes wa;
    XGetWindowAttributes(display, window, &wa);
    Visual *visual = wa.visual;

    XVisualInfo templ;
    templ.screen = screen;
    templ.visualid = XVisualIDFromVisual(visual);
    int out_count;
    XVisualInfo *vinfo = XGetVisualInfo(display,
	    VisualScreenMask | VisualIDMask, &templ, &out_count);
    if (!vinfo)
	return -1;

    GLXContext context = glXCreateContext(display, vinfo, 0, GL_TRUE);
    XFree(vinfo);
    if (!context)
	return -1;

    if (!glXMakeCurrent(display, window, context))
	return -1;

    XMapRaised(display, window);

    return 0;
}

void CanvasGLX::resize()
{
    XWindowAttributes attrib;
    XGetWindowAttributes(display, window, &attrib);
    width = attrib.width;
    height = attrib.height;

    CanvasBase::resize();
}

void CanvasGLX::draw()
{
    CanvasBase::draw();

    glXSwapBuffers(display, window);
}

int CanvasGLX::handle_events()
{
    XEvent event;
    while (XPending(display)) {
	XNextEvent(display, &event);
	switch (event.type) {
	case ConfigureNotify:
	    resize();
	    break;
	}
    }
    return 0;
}

int CanvasGLX::get_ms()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (1000*tv.tv_sec + tv.tv_usec/1000);
}

void CanvasGLX::delay(int ms)
{
    struct timeval tv;
    tv.tv_sec = ms/1000;
    tv.tv_usec = (ms%1000)*1000;
    select(0, 0, 0, 0, &tv);
}
