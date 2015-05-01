#ifndef _CANVASGLX_H
#define _CANVASGLX_H

#include "canvas_base.h"

#include <GL/glx.h>

// A Canvas for drawing to a GL window via GLX
// (currently only works on root window)
class CanvasGLX : public CanvasBase {
protected:
    Window window;
    Display *display;
    int screen;

    // create the window (either SDL or GLX)
    virtual int create_window();
public:
    Window window_id;

    CanvasGLX(Scene *s, bool full_screen, int mspf, Window wid);
    virtual ~CanvasGLX() {}

    // resize the viewport and apply frustum transformation
    virtual void resize();
    // repaint what's on the canvas
    virtual void draw();
    // handle all events, and call proper handlers.
    // returns 0 normally, else >0 on QUIT
    virtual int handle_events();
    // get current millisecond (arbitrary reference: used for change in millis)
    virtual int get_ms();
    // delay for specified number of milliseconds
    virtual void delay(int ms);
};

#endif // canvas_glx.h
