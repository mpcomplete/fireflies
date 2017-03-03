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
    switch(key) {
    case 27:
      exit(0);
    case 's':
      glutCanvas->take_screenshot();
      break;
    }
  });
//   glutMouseFunc(mouse);
  // glutMotionFunc([](int x, int y) {
  //   glutCanvas->scene->camera.pos += Vec3f(double(x)/100.0, double(y)/100.0, 0.0);
  //   glutPostRedisplay();
  // });

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