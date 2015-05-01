#include "main.h"
#include "canvas_sdl.h"
#include "modes.h"

#include <iostream>

#include <gfx/quat.h>
#include <gfx/mat4.h>

using namespace std;

CanvasSDL::CanvasSDL(Scene *s, bool fs, int m, char *t, char *c)
    : CanvasBase(s, fs, m), wm_title(t), wm_class(c)
{
    surface = 0;
}

int CanvasSDL::create_window()
{
    int ret, flags = SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER
        | SDL_HWPALETTE | SDL_HWACCEL | SDL_RESIZABLE | SDL_OPENGL;

    if ((ret=SDL_Init(SDL_INIT_VIDEO)) < 0)
	return ret;

    if (full_screen)
	flags |= SDL_FULLSCREEN;

    SDL_Rect **modes;
    modes = SDL_ListModes(0, flags);
    if (modes == (SDL_Rect**)0) {
	cerr << "CanvasSDL ERROR: no modes available" << endl;
	return -1;
    }
    if (modes == (SDL_Rect**)-1) {
	width = 1024;
	height = 768;
    }
    else {
	width = modes[0]->w;
	height = modes[0]->h;
    }

    surface = SDL_SetVideoMode(width, height, 16, flags|SDL_RESIZABLE);
    if (!surface)
	return -1;

    SDL_WM_SetCaption(wm_title, wm_class);

    return 0;
}

void CanvasSDL::resize()
{
    width = surface->w;
    height = surface->h;

    CanvasBase::resize();
}

void CanvasSDL::draw()
{
    CanvasBase::draw();

#if 0
#define BORDER_COLOR 150
    SDL_Rect rect;
    rect.x = 500;
    rect.y = 500;
    rect.w = rect.h = 100;
    SDL_FillRect(surface, &rect,
            SDL_MapRGB(surface->format, BORDER_COLOR,
                BORDER_COLOR, BORDER_COLOR));
#endif

    SDL_GL_SwapBuffers();
}

int CanvasSDL::handle_events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
	switch (event.type) {
	case SDL_VIDEOEXPOSE:
	    need_refresh = true;
	    break;
	case SDL_VIDEORESIZE:
	    need_refresh = true;
	    surface = SDL_SetVideoMode(event.resize.w, event.resize.h, 16, SDL_OPENGL|SDL_RESIZABLE);
	    if (!surface) {
		cerr << "CanvasSDL ERROR: cannot set video mode after resize" << endl;
		SDL_Quit();
		return 2;
	    }
	    resize();
	    break;
	case SDL_MOUSEMOTION:
	    if (on_mouse_drag(event.motion) < 0)
		return 1;
	    break;
	case SDL_KEYDOWN:
	    if (on_keydown(event.key) < 0)
		return 1;
	    break;
	case SDL_QUIT:
	    SDL_Quit();
	    return 1;
	    break;
	}
    }
    return 0;
}

int CanvasSDL::get_ms()
{
    return SDL_GetTicks();
}

void CanvasSDL::delay(int ms)
{
    SDL_Delay(ms);
}

int CanvasSDL::on_mouse_drag(SDL_MouseMotionEvent& event)
{
    Mat4 inv_camera = rotation_matrix_deg(-scene->camera.rot_angle, Vec3(scene->camera.rot_axis));
    Vec3 horiz = inv_camera*Vec3(1, 0, 0);
    Vec3 vert = inv_camera*Vec3(0, 1, 0);

    if (SDL_GetModState() & KMOD_CTRL) {
	Quat q = axis_to_quat(scene->camera.rot_axis, DEG_TO_RAD(scene->camera.rot_angle));
	if (event.state & SDL_BUTTON(1)) { // rotate
	    Quat qx = axis_to_quat(vert, event.xrel*0.05);
	    Quat qy = axis_to_quat(horiz, event.yrel*0.05);

	    q = q * qy;
	    q = q * qx;
	    unitize(q);

	    scene->camera.rot_axis = q.vector();
	    scene->camera.rot_angle = RAD_TO_DEG(2.0*acos(q.scalar()));
	    need_refresh = true;
	}
    }
    else {
	if (event.state & SDL_BUTTON(1)) { // move camera
	    scene->camera.pos += 0.5*Vec3f(event.xrel, -event.yrel, 0.);
	    need_refresh = true;
	}
	else if (event.state & SDL_BUTTON(3)) { // zoom in/out
	    scene->camera.pos += 0.5*Vec3f(0., 0., event.yrel);
	    need_refresh = true;
	}
    }

    return 0;
}

int CanvasSDL::on_keydown(SDL_KeyboardEvent& event)
{
    switch (event.keysym.sym) {
	case SDLK_q:
	    SDL_Quit();
	    return -1;
	    break;
	case SDLK_p: // pause or unpause
	    animate = !animate;
	    break;
	case SDLK_t: // show the time
	    cout << "Elapsed time: " << scene->curtime << "s" << endl;
	    break;
	case SDLK_UP:
	    scene->fast_forward *= 2;
	    cout << "fast forward: " << scene->fast_forward << "x" << endl;
	    break;
	case SDLK_DOWN:
	    if (scene->fast_forward > 1)
		scene->fast_forward /= 2;
	    cout << "fast forward: " << scene->fast_forward << "x" << endl;
	    break;
	case SDLK_RIGHT:
	    scene->fast_forward += 1;
	    cout << "fast forward: " << scene->fast_forward << "x" << endl;
	    break;
	case SDLK_LEFT:
	    if (scene->fast_forward > 1)
		scene->fast_forward -= 1;
	    cout << "fast forward: " << scene->fast_forward << "x" << endl;
	    break;
	default: {
	     int c = event.keysym.sym - '0';
	     if (event.keysym.mod & KMOD_SHIFT) {
		 if (c >= 0 && c < NUM_BMODES) {
		     vector<Bait*>::iterator it = scene->baits.begin();
		     for (; it != scene->baits.end(); it++)
			 bait_start_mode(*it, c);
		     cout << "started bait mode " << c << endl;
		 }
	     }
	     else if (event.keysym.mod & KMOD_CTRL) {
		 if (c >= 0 && c < NUM_BMODES) {
		     vector<Bait*>::iterator it = scene->baits.begin();
		     for (; it != scene->baits.end(); it++)
			 bait_stop_mode(*it, c);
		     cout << "stopped bait mode " << c << endl;
		 }
	     }
	     else {
		 if (c >= 0 && c < NUM_SMODES) {
		     scene_start_mode(c);
		     cout << "started scene mode " << c << endl;
		 }
	     }
	     break;
		 }
    }
    return 0;
}
