#ifndef _CANVASSDL_H
#define _CANVASSDL_H

#include "canvas_base.h"

#include "SDL.h"

// A Canvas for drawing to a GL window via SDL
class CanvasSDL : public CanvasBase {
protected:
    SDL_Surface *surface;
    char *wm_title;
    char *wm_class;

    // create the window (either SDL or GLX)
    virtual int create_window();
public:
    CanvasSDL(Scene *s, bool full_screen, int mspf, char *wm_title, char *wm_class);
    virtual ~CanvasSDL() {}

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

    // handle a mouse drag event
    int on_mouse_drag(SDL_MouseMotionEvent& event);
    int on_keydown(SDL_KeyboardEvent& event);
};

#endif // canvas_sdl.h
