#ifndef _CANVASGLUT_H
#define _CANVASGLUT_H

#include "canvas_base.h"

// A Canvas for drawing to a GL window via GLUT
class CanvasGLUT : public CanvasBase {
 protected:
  int mouse_button;

  virtual int create_window();

 public:
  CanvasGLUT(Scene* s,
             bool full_screen,
             int mspf,
             int argc,
             char* argv[]);
  virtual ~CanvasGLUT() {}

  virtual int loop();
  // resize the viewport and apply frustum transformation
  virtual void resize();
  // repaint what's on the canvas
  virtual void draw();
  void idle();
  // handle all events, and call proper handlers.
  // returns 0 normally, else >0 on QUIT
  virtual int handle_events();
  // get current millisecond (arbitrary reference: used for change in millis)
  virtual int get_ms();
  void handle_keypress(unsigned char key);
  void handle_mouse_button(int button, int state);
  void handle_mouse_drag(int x, int y, int dx, int dy);
};

#endif  // canvas_glut.h