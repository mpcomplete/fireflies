#ifndef _CANVASBASE_H
#define _CANVASBASE_H

#include "scene.h"

// base class for a GL/DirectX Canvas
class CanvasBase {
//protected:
public:
    Scene *scene;	// the thing that handles drawing and such
    bool need_refresh; 	// do we need to redraw the canvas?
    int last_tick;

    // create the window
    virtual int create_window();
public:
    bool full_screen;	// use full screen?
    int mspf;		// ms per frame (ie, 1000/30 mspf instead of 30 fps)
    bool animate;
    int width;
    int height;

    CanvasBase(Scene *s, bool full_screen, int mspf);
    virtual ~CanvasBase() {}

    // initialize the window and GL stuff
    virtual int init();
    // resize the viewport and apply frustum transformation
    virtual void resize();
    // repaint what's on the canvas
    virtual void draw();
    // the event loop. handles events and animates the game
    virtual int loop();
    // tick if it spf seconds have elapsed since last tick
    // returns 0 if ticked, else time remaining till tick.
    virtual int tick();
    // handle all events, and call proper handlers.
    // returns 0 normally, else >0 on QUIT
    virtual int handle_events();
    // get current millisecond (arbitrary reference: used for change in millis)
    virtual int get_ms();
    // delay for specified number of milliseconds
    virtual void delay(int ms);
};

#endif // canvas_base.h
