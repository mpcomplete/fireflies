#include "canvas_base.h"

#include "lodepng.h"

#include <fstream>
#include <stdio.h>

#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

static GLuint screenshot_framebuffer = 0;
static GLubyte* screenshot_pixels = NULL;
static const GLuint DPI = 200;
static const GLuint SCREENSHOT_WIDTH = 36*DPI;
static const GLuint SCREENSHOT_HEIGHT = 24*DPI;
static const GLenum SCREENSHOT_FORMAT = GL_RGBA;
static const GLuint SCREENSHOT_FORMAT_NBYTES = 4;

static void create_screenshot_texture();
static void save_screenshot();

CanvasBase::CanvasBase(Scene* s, bool fs, int m)
    : scene(s), full_screen(fs), mspf(m) {
  animate = true;
  need_refresh = true;
  width = height = 0;
}

int CanvasBase::create_window() {
  // it would be nice if this were pure virtual, but that makes my
  // version of g++ fucks up for some reason.
  return -1;
}

int CanvasBase::init() {
#ifdef WIN32
  srand(time(0));
#else

  struct timeval tv;
  gettimeofday(&tv, 0);
  srand(1000 * tv.tv_sec + tv.tv_usec / 1000);
#endif

  int ret;
  if ((ret = create_window()) < 0)
    return ret;

  create_screenshot_texture();
  resize();
  last_tick = get_ms();

  return 0;
}

void CanvasBase::resize() {
  glViewport(0, 0, width, height);
  scene->resize(width, height);
}

void CanvasBase::draw() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  scene->apply_camera(Vec3(0, 0, 0));
  scene->draw();
}

int CanvasBase::loop() {
  int remain, ret;
  last_tick = get_ms();

  while (true) {
    if ((remain = tick()) > 0)  // time left till tick: sleep
      delay(remain);
    if ((ret = handle_events()) > 0)  // wants us to quit
      return ret;
  }
}

int CanvasBase::tick() {
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
int CanvasBase::handle_events() {
  return 1;
}

int CanvasBase::get_ms() {
  return 0;
}

void CanvasBase::delay(int ms) {}

void CanvasBase::take_screenshot() {
  glBindFramebuffer(GL_FRAMEBUFFER, screenshot_framebuffer);
  glViewport(0, 0, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT);
  scene->resize(SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT);

  CanvasBase::draw();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glReadBuffer(GL_FRONT);
  glReadPixels(0, 0, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT, SCREENSHOT_FORMAT,
               GL_UNSIGNED_BYTE, screenshot_pixels);

  save_screenshot();

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  resize();
}

static void create_screenshot_texture() {
  // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth
  // buffer.
  glGenFramebuffers(1, &screenshot_framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, screenshot_framebuffer);

  // The texture we're going to render to
  GLuint renderedTexture;
  glGenTextures(1, &renderedTexture);

  // "Bind" the newly created texture : all future texture functions will modify
  // this texture
  glBindTexture(GL_TEXTURE_2D, renderedTexture);

  // Give an empty image to OpenGL ( the last "0" )
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT,
               0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

  // Poor filtering. Needed !
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  // The depth buffer
  GLuint depthrenderbuffer;
  glGenRenderbuffers(1, &depthrenderbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCREENSHOT_WIDTH,
                        SCREENSHOT_HEIGHT);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depthrenderbuffer);

  // Set "renderedTexture" as our colour attachement #0
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         renderedTexture, 0);

  // Set the list of draw buffers.
  GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, DrawBuffers);  // "1" is the size of DrawBuffers

  // Always check that our framebuffer is ok
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    return;

  screenshot_pixels = (GLubyte*)malloc(SCREENSHOT_FORMAT_NBYTES *
                                       SCREENSHOT_WIDTH * SCREENSHOT_HEIGHT);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static bool file_exists(const char* filename) {
  std::ifstream fin(filename);
  return fin.good();
}

static void save_screenshot() {
  static unsigned int nscreenshots = 0;
  char filename[256];

  do {
    snprintf(filename, sizeof(filename), "screenshot%d.png", nscreenshots++);
  } while (file_exists(filename));

  cout << "Capturing " << filename << "..." << flush;
  std::vector<unsigned char> image_buf;
  lodepng::encode(image_buf, screenshot_pixels, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT);
  lodepng::save_file(image_buf, filename);
  cout << "done" << endl;
}