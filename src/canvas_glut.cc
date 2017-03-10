#include "main.h"
#include "canvas_glut.h"
#include "modes.h"

#include <iostream>
#include <assert.h>
#include <time.h>

#include <gfx/quat.h>
#include <gfx/mat4.h>

#include <GL/glut.h>

using namespace std;

static CanvasGLUT* glutCanvas;
static size_t curBait = 0;

CanvasGLUT::CanvasGLUT(Scene* s, bool fs, int m, int argc, char *argv[])
    : CanvasBase(s, fs, m) {
  assert(glutCanvas == nullptr);
  glutCanvas = this;
  mouse_button = -1;
  glutInit(&argc, argv);
}

int CanvasGLUT::create_window() {
  glutInitWindowSize(1280, 720);
  width = 1280; height = 720;
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("Fireflies");

  glutDisplayFunc([]() { glutCanvas->draw(); });
  glutIdleFunc([]() { glutCanvas->idle(); });
  glutReshapeFunc([](int x, int y) {
    glutCanvas->width = x;
    glutCanvas->height = y;
    glutCanvas->resize();
  });
  glutKeyboardFunc([](unsigned char key, int x, int y) {
    glutCanvas->handle_keypress(key);
  });
  static int last_x, last_y;
  glutMouseFunc([](int button, int state, int x, int y) {
    last_x = x;
    last_y = y;
    glutCanvas->handle_mouse_button(button, state);
  });
  glutMotionFunc([](int x, int y) {
    glutCanvas->handle_mouse_drag(x - last_x, y - last_y);
    last_x = x;
    last_y = y;
  });

  GLenum err = glewInit();
  if (err != GLEW_OK) {
    cout << "Error initializing: " << glewGetErrorString(err);
    return -1;
  }

  return 0;
}

void CanvasGLUT::resize() {
  CanvasBase::resize();
}

void CanvasGLUT::draw() {
  CanvasBase::draw();
  glutSwapBuffers();
}

int CanvasGLUT::loop() {
  glutMainLoop();
  return 0;
}

void CanvasGLUT::idle() {
  int now = get_ms();
  int ms = now - last_tick;
  float t = ms / 1000.0;

  // Animate the scene
  if (ms >= mspf) {
    last_tick = now;
    if (animate) {
      scene->elapse(ms / 1000.0);
      glutPostRedisplay();
    }
  }

  if (mouse_button != -1 && curBait < scene->baits.size()) {
    Bait* b = scene->baits[curBait];

    b->elapse(t);
    for (auto f: scene->flies) {
      if (f->bait == b)
        f->elapse(t);
    }

    glutPostRedisplay();
  }

}

int CanvasGLUT::handle_events() {
  return 0;
}

int CanvasGLUT::get_ms() {
  struct timespec clk;
  clock_gettime(CLOCK_REALTIME, &clk);
  return clk.tv_nsec / 1000 / 1000 + clk.tv_sec * 1000;
}

void CanvasGLUT::handle_keypress(unsigned char key) {
  switch (key) {
    case 'q':
    case 27:  // ESC
      exit(0);
      break;
    case 's':
      glutCanvas->take_screenshot();
      break;
    case 'p':  // pause or unpause
      animate = !animate;
      break;
    case 't':  // show the time
      cout << "Elapsed time: " << scene->curtime << "s" << endl;
      break;
    case '+':
      scene->fast_forward *= 2;
      cout << "fast forward: " << scene->fast_forward << "x" << endl;
      break;
    case '_':
      if (scene->fast_forward > 1)
        scene->fast_forward /= 2;
      cout << "fast forward: " << scene->fast_forward << "x" << endl;
      break;
    case '=':
      scene->fast_forward += 1;
      cout << "fast forward: " << scene->fast_forward << "x" << endl;
      break;
    case '-':
      if (scene->fast_forward > 1)
        scene->fast_forward -= 1;
      cout << "fast forward: " << scene->fast_forward << "x" << endl;
      break;
    default: {
      size_t c = ((key - '0') + 9) % 10;
      if (c == scene->baits.size())
        scene->baits.push_back(new Bait());
      if (c >= 0 && c < scene->baits.size()) {
        if (curBait == c) {
          Bait* b = scene->baits[curBait];
          for (int i = 0; i < 50; i++) {
            scene->flies.push_back(new Firefly(b, b->pos, world[2]));
          }
          cout << "added flies to bait " << c << endl;
        } else {
          curBait = c;
          cout << "selected bait " << c << endl;
        }
      }
      break;
    }
  }
}

void CanvasGLUT::handle_mouse_button(int button, int state) {
  last_tick = get_ms();
  if (state == GLUT_DOWN)
    mouse_button = button;
  else if (state == GLUT_UP)
    mouse_button = -1;
}

void CanvasGLUT::handle_mouse_drag(int dx, int dy) {
  // Animate the scene

  if (mouse_button == GLUT_LEFT_BUTTON) {  // move camera
    if (curBait < scene->baits.size()) {
      Bait* b = scene->baits[curBait];
      b->pos += 0.5 * Vec3f(dx, -dy, 0.);
      glutPostRedisplay();
    }
  }
}
