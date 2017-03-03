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

CanvasGLUT* glutCanvas;

CanvasGLUT::CanvasGLUT(Scene* s, bool fs, int m, int argc, char *argv[])
    : CanvasBase(s, fs, m) {
  assert(glutCanvas == nullptr);
  glutCanvas = this;
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

  // Animate the scene
  if (ms >= mspf) {
    last_tick = now;
    if (animate) {
      scene->elapse(ms / 1000.0);
      glutPostRedisplay();
    }
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
      int c = key - '0';
      if (c >= 0 && c < NUM_SMODES) {
        scene_start_mode(c);
        cout << "started scene mode " << c << endl;
      }
      break;
    }
  }
}

void CanvasGLUT::handle_mouse_button(int button, int state) {
  if (state == GLUT_DOWN)
    mouse_button = button;
  else if (state == GLUT_UP)
    mouse_button = -1;
}

void CanvasGLUT::handle_mouse_drag(int dx, int dy) {
  Mat4 inv_camera = rotation_matrix_deg(-scene->camera.rot_angle,
                                        Vec3(scene->camera.rot_axis));
  Vec3 horiz = inv_camera * Vec3(1, 0, 0);
  Vec3 vert = inv_camera * Vec3(0, 1, 0);

  if (mouse_button == GLUT_LEFT_BUTTON) {  // move camera
    scene->camera.pos += 0.5 * Vec3f(dx, -dy, 0.);
    glutPostRedisplay();
  } else if (mouse_button == GLUT_MIDDLE_BUTTON) {  // rotate
    Quat q = axis_to_quat(scene->camera.rot_axis,
                          DEG_TO_RAD(scene->camera.rot_angle));
    Quat qx = axis_to_quat(vert, dx * 0.05);
    Quat qy = axis_to_quat(horiz, dy * 0.05);

    q = q * qy;
    q = q * qx;
    unitize(q);

    scene->camera.rot_axis = q.vector();
    scene->camera.rot_angle = RAD_TO_DEG(2.0 * acos(q.scalar()));
    glutPostRedisplay();
  } else if (mouse_button == GLUT_RIGHT_BUTTON) {  // zoom in/out
    scene->camera.pos += 0.5 * Vec3f(0., 0., dy);
    glutPostRedisplay();
  }
}
