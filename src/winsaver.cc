#include "main.h"
#include "scene.h"
#include "modes.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <windows.h>
#include <scrnsave.h>
#include <iostream>
#include <sys/timeb.h>

#include "resource.h"

#define MY_HKEY "Software\\Fireflies\\2.0"

//Define a Windows timer
#define TIMER 1

// the default fps
double fps = 20;

Scene scene;

static struct timeb then;

void init_gl(HWND hWnd, HDC & hDC, HGLRC & hRC)
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory( &pfd, sizeof pfd );
    pfd.nSize = sizeof pfd;
    pfd.nVersion = 1;
    //pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL; //blaine's
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;

    hDC = GetDC( hWnd );

    int i = ChoosePixelFormat( hDC, &pfd );  
    SetPixelFormat( hDC, i, &pfd );

    hRC = wglCreateContext( hDC );
    wglMakeCurrent( hDC, hRC );
}

// Shut down OpenGL
void close_gl(HWND hWnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent( NULL, NULL );
    wglDeleteContext( hRC );

    ReleaseDC( hWnd, hDC );
}

void start_animate(int width, int height)
{
    glViewport(0, 0, width, height);

    scene.resize(width, height);
    scene.create();

    ftime(&then);
}

void on_timer(HDC hDC) //increment and display
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    struct timeb now;
    ftime(&now);
    double t = double(now.time - then.time)
       	+ double((now.millitm - then.millitm)/1000.0);
    then = now;
    scene.elapse(t);
    scene.apply_camera();
    scene.draw();

    glFinish();
    SwapBuffers(hDC);
}

// Registry bullshit
void reg_get_val(HKEY key, char *str, bool *val)
{
    DWORD dsize = sizeof(int);
    DWORD dwtype = 0;
    int tmp;

    if (RegQueryValueEx(key, str, 0, &dwtype, (BYTE*)&tmp, &dsize)==0)
	*val = (tmp==1);
}

void reg_get_val(HKEY key, char *str, int *val)
{
    DWORD dsize = sizeof(int);
    DWORD dwtype = 0;
    int tmp;

    if (RegQueryValueEx(key, str, 0, &dwtype, (BYTE*)&tmp, &dsize)==0)
	*val = tmp;
}

void reg_get_val(HKEY key, char *str, unsigned *val)
{
    DWORD dsize = sizeof(int);
    DWORD dwtype = 0;
    int tmp;

    if (RegQueryValueEx(key, str, 0, &dwtype, (BYTE*)&tmp, &dsize)==0)
	*val = (unsigned)tmp;
}

void reg_get_val(HKEY key, char *str, double *val)
{
    DWORD dsize = sizeof(int);
    DWORD dwtype = 0;
    int tmp;

    if (RegQueryValueEx(key, str, 0, &dwtype, (BYTE*)&tmp, &dsize)==0)
	*val = (double)tmp;
}

void reg_get_val_div10(HKEY key, char *str, double *val)
{
    DWORD dsize = sizeof(int);
    DWORD dwtype = 0;
    int tmp;

    if (RegQueryValueEx(key, str, 0, &dwtype, (BYTE*)&tmp, &dsize)==0)
	*val = (double)tmp/10.0;
}

void reg_get_val_div100(HKEY key, char *str, double *val)
{
    DWORD dsize = sizeof(int);
    DWORD dwtype = 0;
    int tmp;

    if (RegQueryValueEx(key, str, 0, &dwtype, (BYTE*)&tmp, &dsize)==0)
	*val = (double)tmp/100.0;
}

void reg_set_val(HKEY key, char *str, bool val)
{
    int tmp = val ? 1 : 0;
    RegSetValueEx(key, str, 0, REG_DWORD, (BYTE*)&tmp, sizeof(tmp));
}

void reg_set_val(HKEY key, char *str, int val)
{
    RegSetValueEx(key, str, 0, REG_DWORD, (BYTE*)&val, sizeof(val));
}

void reg_set_val(HKEY key, char *str, unsigned val)
{
    RegSetValueEx(key, str, 0, REG_DWORD, (BYTE*)&val, sizeof(val));
}

void reg_set_val(HKEY key, char *str, double val)
{
    int tmp = (int)val;
    RegSetValueEx(key, str, 0, REG_DWORD, (BYTE*)&tmp, sizeof(tmp));
}

void reg_set_val_tim10(HKEY key, char *str, double val)
{
    int tmp = (int)(val*10);
    RegSetValueEx(key, str, 0, REG_DWORD, (BYTE*)&tmp, sizeof(tmp));
}

void reg_set_val_tim100(HKEY key, char *str, double val)
{
    int tmp = (int)(val*100);
    RegSetValueEx(key, str, 0, REG_DWORD, (BYTE*)&tmp, sizeof(tmp));
}

void read_config()
{
    HKEY key;
    char buf[256];
    double tmp;

    scene.set_defaults();
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
		MY_HKEY,
		0,			//reserved
		KEY_QUERY_VALUE,
		&key) == ERROR_SUCCESS)	
    {
	reg_get_val(key, "minbaits", &scene.minbaits);
	reg_get_val(key, "maxbaits", &scene.maxbaits);
	reg_get_val(key, "minflies", &scene.minflies);
	reg_get_val(key, "maxflies", &scene.maxflies);
	reg_get_val_div10(key, "fsize", &scene.fsize);
	reg_get_val(key, "bspeed", &scene.bspeed);
	reg_get_val(key, "baccel", &scene.baccel);
	reg_get_val(key, "fspeed", &scene.fspeed);
	reg_get_val(key, "faccel", &scene.faccel);
	reg_get_val(key, "hue_rate", &scene.hue_rate);
	reg_get_val_div10(key, "tail_length", &scene.tail_length);
	reg_get_val_div10(key, "tail_width", &scene.tail_width);
	reg_get_val_div100(key, "tail_opaq", &scene.tail_opaq);
	reg_get_val_div10(key, "glow_factor", &scene.glow_factor);
	reg_get_val_div10(key, "wind_speed", &scene.wind_speed);
	reg_get_val(key, "draw_bait", &scene.draw_bait);
	reg_get_val(key, "fast_forward", &scene.fast_forward);
	reg_get_val(key, "fps", &fps);

	for (GLuint i = 0; i < NUM_BMODES; i++) {
	    snprintf(buf, sizeof(buf), "bmode%d", i);
	    reg_get_val(key, buf, &tmp);
	    scene.bmodes.change(i, tmp);
	}
	for (GLuint i = 0; i < NUM_SMODES; i++) {
	    snprintf(buf, sizeof(buf), "smode%d", i);
	    reg_get_val(key, buf, &tmp);
	    scene.smodes.change(i, tmp);
	}

	RegCloseKey(key);
    }
}

void write_config(HWND hDlg)
{
    HKEY key;
    DWORD lpdw;

    scene.minbaits = (int)GetDlgItemInt(hDlg, IDC_CONF_MINBAITS, 0, TRUE);
    scene.maxbaits = (int)GetDlgItemInt(hDlg, IDC_CONF_MAXBAITS, 0, TRUE);
    scene.minflies = (int)GetDlgItemInt(hDlg, IDC_CONF_MINFLIES, 0, TRUE);
    scene.maxflies = (int)GetDlgItemInt(hDlg, IDC_CONF_MAXFLIES, 0, TRUE);
    scene.fsize = ((int)GetDlgItemInt(hDlg, IDC_CONF_FSIZE, 0, TRUE))/10.0;
    scene.bspeed = (int)GetDlgItemInt(hDlg, IDC_CONF_BSPEED, 0, TRUE);
    scene.baccel = (int)GetDlgItemInt(hDlg, IDC_CONF_BACCEL, 0, TRUE);
    scene.fspeed = (int)GetDlgItemInt(hDlg, IDC_CONF_FSPEED, 0, TRUE);
    scene.faccel = (int)GetDlgItemInt(hDlg, IDC_CONF_FACCEL, 0, TRUE);
    scene.hue_rate = (int)GetDlgItemInt(hDlg, IDC_CONF_HUERATE, 0, TRUE);
    scene.tail_length =
	((int)GetDlgItemInt(hDlg, IDC_CONF_TAILLENGTH, 0, TRUE))/10.0;
    scene.tail_width =
	((int)GetDlgItemInt(hDlg, IDC_CONF_TAILWIDTH, 0, TRUE))/10.0;
    scene.tail_opaq =
	((int)GetDlgItemInt(hDlg, IDC_CONF_TAILOPAQ, 0, TRUE))/100.0;
    scene.glow_factor =
       	((int)GetDlgItemInt(hDlg, IDC_CONF_GLOWFACTOR, 0, TRUE))/10.0;
    scene.wind_speed =
       	((int)GetDlgItemInt(hDlg, IDC_CONF_WIND, 0, TRUE))/10.0;
    scene.draw_bait = (IsDlgButtonChecked(hDlg, IDC_CONF_DRAWBAIT)==BST_CHECKED);
    scene.fast_forward = (int)GetDlgItemInt(hDlg, IDC_CONF_FASTFORWARD, 0, TRUE);
    fps = (int)GetDlgItemInt(hDlg, IDC_CONF_FPS, 0, TRUE);
    for (GLuint i = 0; i < NUM_BMODES; i++) {
	scene.bmodes.change(i, (double)
		(UINT)GetDlgItemInt(hDlg, IDC_CONF_BMODE(i), 0, FALSE));
    }
    for (GLuint i = 0; i < NUM_SMODES; i++) {
	scene.smodes.change(i, (double)
		(UINT)GetDlgItemInt(hDlg, IDC_CONF_SMODE(i), 0, FALSE));
    }

    if (RegCreateKeyEx( HKEY_CURRENT_USER,
		MY_HKEY,
		0,			//reserved
		"",			//ptr to null-term string specifying the object type of this key
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&key,
		&lpdw) == ERROR_SUCCESS)
    {
	reg_set_val(key, "minbaits", scene.minbaits);
	reg_set_val(key, "maxbaits", scene.maxbaits);
	reg_set_val(key, "minflies", scene.minflies);
	reg_set_val(key, "maxflies", scene.maxflies);
	reg_set_val_tim10(key, "fsize", scene.fsize);
	reg_set_val(key, "bspeed", scene.bspeed);
	reg_set_val(key, "baccel", scene.baccel);
	reg_set_val(key, "fspeed", scene.fspeed);
	reg_set_val(key, "faccel", scene.faccel);
	reg_set_val(key, "hue_rate", scene.hue_rate);
	reg_set_val_tim10(key, "tail_length", scene.tail_length);
	reg_set_val_tim10(key, "tail_width", scene.tail_width);
	reg_set_val_tim100(key, "tail_opaq", scene.tail_opaq);
	reg_set_val_tim10(key, "glow_factor", scene.glow_factor);
	reg_set_val_tim10(key, "wind_speed", scene.wind_speed);
	reg_set_val(key, "draw_bait", scene.draw_bait);
	reg_set_val(key, "fast_forward", scene.fast_forward);
	reg_set_val(key, "fps", fps);

	char buf[256];
	for (GLuint i = 0; i < NUM_BMODES; i++) {
	    snprintf(buf, sizeof(buf), "bmode%d", i);
	    reg_set_val(key, buf, scene.bmodes.events[i].second);
	}
	for (GLuint i = 0; i < NUM_SMODES; i++) {
	    snprintf(buf, sizeof(buf), "smode%d", i);
	    reg_set_val(key, buf, scene.smodes.events[i].second);
	}

	RegCloseKey(key);
    }
}

// main() function
LRESULT WINAPI
ScreenSaverProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HDC hDC;
    static HGLRC hRC;
    static RECT rect;
    int width, height;

    srand(time(0));
    switch ( message ) {
    case WM_CREATE:
	GetClientRect(hWnd, &rect);
	width = rect.right;		
	height = rect.bottom;

	read_config();

	init_gl( hWnd, hDC, hRC );
	start_animate(width, height);

	// tick every 1000/fps ms
	SetTimer( hWnd, TIMER, (unsigned)(1000/fps), NULL );
	return 0;

    case WM_DESTROY:
	KillTimer( hWnd, TIMER );
	close_gl(hWnd, hDC, hRC);
	return 0;

    case WM_TIMER:
	on_timer(hDC);
	return 0;
    }

    return DefScreenSaverProc(hWnd, message, wParam, lParam);
}

void set_dialog(HWND hDlg)
{
    SetDlgItemInt(hDlg, IDC_CONF_MINBAITS, (UINT)scene.minbaits, TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_MAXBAITS, (UINT)scene.maxbaits, TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_MINFLIES, (UINT)scene.minflies, TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_MAXFLIES, (UINT)scene.maxflies, TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_FSIZE, (UINT)(scene.fsize*10), TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_BSPEED, (UINT)scene.bspeed, TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_BACCEL, (UINT)scene.baccel, TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_FSPEED, (UINT)scene.fspeed, TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_FACCEL, (UINT)scene.faccel, TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_HUERATE, (UINT)scene.hue_rate, TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_TAILLENGTH, (UINT)(scene.tail_length*10), TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_TAILWIDTH, (UINT)(scene.tail_width*10), TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_TAILOPAQ, (UINT)(scene.tail_opaq*100), TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_GLOWFACTOR, (UINT)(scene.glow_factor*10), TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_WIND, (UINT)(scene.wind_speed*10), TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_FASTFORWARD, (UINT)(scene.fast_forward), TRUE);
    SetDlgItemInt(hDlg, IDC_CONF_FPS, (UINT)(fps), TRUE);

    CheckDlgButton(hDlg, IDC_CONF_DRAWBAIT,
	    scene.draw_bait ? BST_CHECKED : BST_UNCHECKED);
    for (GLuint i = 0; i < NUM_BMODES; i++) {
	SetDlgItemInt(hDlg, IDC_CONF_BMODE(scene.bmodes.events[i].first),
		(UINT)scene.bmodes.events[i].second, FALSE);
    }
    for (GLuint i = 0; i < NUM_SMODES; i++) {
	SetDlgItemInt(hDlg, IDC_CONF_SMODE(scene.smodes.events[i].first),
		(UINT)scene.smodes.events[i].second, FALSE);
    }
}

// configure dialog
BOOL WINAPI
ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hIDOK;

    switch (message) {
    case WM_INITDIALOG:
	LoadString(hMainInstance, IDS_DESCRIPTION, szAppName, 40);
	read_config();
	set_dialog(hDlg);

	hIDOK = GetDlgItem(hDlg, IDOK);
	return TRUE;

    case WM_COMMAND:
	switch (wParam) {
	    case IDOK:
		write_config(hDlg);
		EndDialog(hDlg, TRUE);
		return TRUE;

	    case IDCANCEL:
		EndDialog(hDlg, FALSE);
		return TRUE;

	    case IDC_DEFAULTS:
		scene.set_defaults();
		fps = 20;
		set_dialog(hDlg);
		return TRUE;
	}
	break;
    }
    return FALSE;
}

// needed for SCRNSAVE.LIB
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    return TRUE;
}
