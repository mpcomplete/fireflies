#include "canvas_glx.h"

#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <X11/XKBlib.h>

#include "vroot.h"

static Window create_glx_window(Display* display);
static Window init_root_window(Display* display, Window window_id);

CanvasGLX::CanvasGLX(Scene* s, bool fs, int m, Window wid)
    : CanvasBase(s, fs, m), window_id(wid) {
  window = 0;
  display = 0;
}

int CanvasGLX::create_window() {
  if ((display = XOpenDisplay(0)) == 0)
    return -1;
  if (window_id != 0)
    window = init_root_window(display, window_id);
  else
    window = create_glx_window(display);

  GLenum err = glewInit();
  if (err != GLEW_OK) {
    cout << "Error initializing: " << glewGetErrorString(err);
    return -1;
  }

  return 0;
}

void CanvasGLX::resize() {
  XWindowAttributes attrib;
  XGetWindowAttributes(display, window, &attrib);
  width = attrib.width;
  height = attrib.height;

  CanvasBase::resize();
}

void CanvasGLX::draw() {
  CanvasBase::draw();

  glXSwapBuffers(display, window);
}

int CanvasGLX::handle_events() {
  XEvent event;
  while (XPending(display)) {
    XNextEvent(display, &event);
    switch (event.type) {
      case KeyPress: {
        KeySym sym = XkbKeycodeToKeysym(display, event.xkey.keycode, 0,
                                        event.xkey.state & ShiftMask ? 1 : 0);
        if (event.xkey.keycode == 9)  // ESC
          return 1;
        if (sym == 's')
          take_screenshot();
        if (sym == 'p')
          animate = !animate;
        break;
      }
      case ConfigureNotify:
        resize();
        break;
    }
  }
  return 0;
}

int CanvasGLX::get_ms() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return (1000 * tv.tv_sec + tv.tv_usec / 1000);
}

void CanvasGLX::delay(int ms) {
  struct timeval tv;
  tv.tv_sec = ms / 1000;
  tv.tv_usec = (ms % 1000) * 1000;
  select(0, 0, 0, 0, &tv);
}

static Window create_glx_window(Display* display) {
  if (!display) {
    printf("Failed to open X display\n");
    exit(1);
  }

  // Get a matching FB config
  static int visual_attribs[] = {
      GLX_X_RENDERABLE, True, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
      GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
      GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8,
      GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, True,
      // GLX_SAMPLE_BUFFERS  , 1,
      // GLX_SAMPLES         , 4,
      None};

  int glx_major, glx_minor;

  // FBConfigs were added in GLX version 1.3.
  if (!glXQueryVersion(display, &glx_major, &glx_minor) ||
      ((glx_major == 1) && (glx_minor < 3)) || (glx_major < 1)) {
    printf("Invalid GLX version");
    exit(1);
  }

  printf("Getting matching framebuffer configs\n");
  int fbcount;
  GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display),
                                       visual_attribs, &fbcount);
  if (!fbc) {
    printf("Failed to retrieve a framebuffer config\n");
    exit(1);
  }
  printf("Found %d matching FB configs.\n", fbcount);

  // Pick the FB config/visual with the most samples per pixel
  printf("Getting XVisualInfos\n");
  int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;

  int i;
  for (i = 0; i < fbcount; ++i) {
    XVisualInfo* vi = glXGetVisualFromFBConfig(display, fbc[i]);
    if (vi) {
      int samp_buf, samples;
      glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
      glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLES, &samples);

      //      printf( "  Matching fbconfig %d, visual ID 0x%2x: SAMPLE_BUFFERS =
      //      %d,"
      //              " SAMPLES = %d\n",
      //              i, vi -> visualid, samp_buf, samples );

      if (best_fbc < 0 || (samp_buf && samples > best_num_samp))
        best_fbc = i, best_num_samp = samples;
      if (worst_fbc < 0 || !samp_buf || samples < worst_num_samp)
        worst_fbc = i, worst_num_samp = samples;
    }
    XFree(vi);
  }

  GLXFBConfig bestFbc = fbc[best_fbc];

  // Be sure to free the FBConfig list allocated by glXChooseFBConfig()
  XFree(fbc);

  // Get a visual
  XVisualInfo* vi = glXGetVisualFromFBConfig(display, bestFbc);
  // printf( "Chosen visual ID = 0x%x\n", vi->visualid );

  printf("Creating colormap\n");
  XSetWindowAttributes swa;
  Colormap cmap;
  swa.colormap = cmap = XCreateColormap(
      display, RootWindow(display, vi->screen), vi->visual, AllocNone);
  swa.background_pixmap = None;
  swa.border_pixel = 0;
  swa.event_mask = StructureNotifyMask;

  printf("Creating window\n");
  Window win = XCreateWindow(display, RootWindow(display, vi->screen), 0, 0,
                             640, 360, 0, vi->depth, InputOutput, vi->visual,
                             CWBorderPixel | CWColormap | CWEventMask, &swa);
  if (!win) {
    printf("Failed to create window.\n");
    exit(1);
  }

  // Done with the visual info data
  XFree(vi);

  XStoreName(display, win, "GL 3.0 Window");

  XSelectInput(display, win, KeyPressMask | KeyReleaseMask);

  printf("Mapping window\n");
  XMapWindow(display, win);

  GLXContext ctx =
      glXCreateNewContext(display, bestFbc, GLX_RGBA_TYPE, 0, True);

  // Sync to ensure any errors generated are processed.
  XSync(display, False);

  if (!ctx) {
    printf("Failed to create an OpenGL context\n");
    exit(1);
  }

  // Verifying that context is a direct context
  if (!glXIsDirect(display, ctx)) {
    printf("Indirect GLX rendering context obtained\n");
  } else {
    printf("Direct GLX rendering context obtained\n");
  }

  printf("Making context current\n");
  glXMakeCurrent(display, win, ctx);

  return win;
}

static Window init_root_window(Display* display, Window window_id) {
  int screen = DefaultScreen(display);
  Window window = window_id ? window_id : RootWindow(display, screen);
  XWindowAttributes wa;
  XGetWindowAttributes(display, window, &wa);
  Visual* visual = wa.visual;

  XVisualInfo templ;
  templ.screen = screen;
  templ.visualid = XVisualIDFromVisual(visual);
  int out_count;
  XVisualInfo* vinfo = XGetVisualInfo(display, VisualScreenMask | VisualIDMask,
                                      &templ, &out_count);
  if (!vinfo)
    return -1;

  GLXContext context = glXCreateContext(display, vinfo, 0, GL_TRUE);
  XFree(vinfo);
  if (!context)
    return -1;

  if (!glXMakeCurrent(display, window, context))
    return -1;

  XMapRaised(display, window);

  return window;
}
