/************************************************************************

  MxGUI-like emulation in MFC.
  
  $Id: gui-mfc.cxx 427 2004-09-27 04:45:31Z garland $

 ************************************************************************/

#include <gfx/win/gui-mfc.h>

namespace gfx
{

static inline MfcGUI *GuiGetApp() { return (MfcGUI *)AfxGetApp(); }

MfcGUI::MfcGUI()
{
    canvas = NULL;
    timer_id = 0;
    default_fps = 24.0f;
    target_fps = 0.0f;
}

void MfcGUI::status(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    CString str;
    str.FormatV(fmt, args);
    va_end(args);

    if( canvas && canvas->status_line )
	canvas->status_line->SetPaneText(0, str);
}

static VOID CALLBACK cb_timeout(HWND w, UINT msg, UINT id, DWORD system_time)
{
    GuiGetApp()->update_animation();
    GuiGetApp()->canvas->post_redraw();
}

void MfcGUI::animate(bool will)
{
    if( will )
    {
	target_fps = default_fps;

	float millisecs = 1000 / target_fps;
	timer_id = ::SetTimer(NULL, 0, (UINT)millisecs, cb_timeout);
    }
    else
    {
	target_fps = 0;
	if( timer_id )  ::KillTimer(NULL, timer_id);
	timer_id = 0;
    }
}

BOOL MfcGUI::InitInstance()
{
    m_pMainWnd = canvas = new Canvas;
    m_pMainWnd->ShowWindow(m_nCmdShow);
    m_pMainWnd->UpdateWindow();
    return TRUE;
}



BEGIN_MESSAGE_MAP(Canvas, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SIZE()

    //ON_WM_ACTIVATE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()

    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_MBUTTONDOWN()
    ON_WM_MBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_CHAR()
END_MESSAGE_MAP()

Canvas::Canvas()
{
    last_click[0] = last_click[1] = -1;
    glcontext = NULL;
    pixfmt = 0;
    
    
    Create(NULL, "Sample Application");

    CMenu menu;
    menu.CreateMenu();

    CMenu popup;
    popup.CreatePopupMenu();
    popup.AppendMenu(MF_STRING, 0, "&New\tCtrl+N");
    menu.AppendMenu(MF_POPUP, (UINT)popup.Detach(), "&File");

    SetMenu(&menu);
    menu.Detach();

    status_line = new CStatusBar();
    status_line->Create(this);
    UINT indicator = ID_SEPARATOR;
    status_line->SetIndicators(&indicator, 1);
}

BOOL Canvas::PreCreateWindow(CREATESTRUCT &cs)
{
    // Need to set special style requirements for OpenGL windows
    cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    return CFrameWnd::PreCreateWindow(cs);
}


int Canvas::OnCreate(LPCREATESTRUCT lpcs)
{
    if( CFrameWnd::OnCreate(lpcs) == -1 )  return -1;

    HWND wnd = GetSafeHwnd();
    HDC dc = ::GetDC(wnd);

    pixfmt = set_pixel_format(dc);
    if( !pixfmt )
	MessageBox("Failed to set up proper pixel format.");

    glcontext = create_glcontext(dc);
    if( !glcontext )
	MessageBox("Failed to create GL context.");
    
    return 0;
}

void Canvas::OnPaint()
{
    CPaintDC dc(this);

    HDC hdc = dc.GetSafeHdc();
    make_current(hdc);

    GuiGetApp()->draw_contents();

    finish(hdc);
}

void Canvas::OnDestroy()
{
    if( wglGetCurrentContext() )
	wglMakeCurrent(NULL, NULL);

    if( glcontext )
    {
	wglDeleteContext(glcontext);
	glcontext = NULL;
    }

    CFrameWnd::OnDestroy();
}

void Canvas::OnSize(UINT type, int width, int height)
{
    CFrameWnd::OnSize(type, width, height);
    glViewport(0, 0, width, height);
    GuiGetApp()->setup_for_drawing();
}

BOOL Canvas::OnEraseBkgnd(CDC *dc)
{
    return FALSE;   // Don't want background to be erased thank you
}

void Canvas::immediate_redraw()
{
    SendMessage(WM_PAINT);
}

void Canvas::post_redraw()
{
    InvalidateRect(NULL, FALSE);
}

int Canvas::decode_mouse_button(UINT flags, int which)
{
    if( which==0 )
    {
	if( flags&MK_MBUTTON )  which = 2;
	else if( flags&MK_LBUTTON )  which = 1;
	else if( flags&MK_RBUTTON )  which = 3;
    }

    // Emulate middle button by double click
    if( flags&MK_LBUTTON && flags&MK_RBUTTON )  which = 2;

    return which;
}

void Canvas::do_mouse_down(int which, UINT flags, CPoint point)
{
    which = decode_mouse_button(flags, which);

    int where[2];  where[0]=point.x; where[1]=point.y;
    last_click[0]=point.x;  last_click[1]=point.y;

    SetCapture();
    if( GuiGetApp()->mouse_down(where, which) )
	post_redraw();
}

void Canvas::do_mouse_up(int which, UINT flags, CPoint point)
{
    which = decode_mouse_button(flags, which);

    int where[2];  where[0]=point.x; where[1]=point.y;
	
    ReleaseCapture();
    if( GuiGetApp()->mouse_up(where, which) )
	post_redraw();
}

void Canvas::do_mouse_move(UINT flags, CPoint point)
{
    int which = decode_mouse_button(flags);

    int where[2];  where[0]=point.x; where[1]=point.y;

    if( GuiGetApp()->mouse_drag(where, last_click, which) )
	post_redraw();

    last_click[0]=point.x;  last_click[1]=point.y;
}

// Each mouse handler is given a set of flags 'f' and a position 'p'
void Canvas::OnLButtonDown(UINT f, CPoint p) { do_mouse_down(1, f, p); }
void Canvas::OnRButtonDown(UINT f, CPoint p) { do_mouse_down(3, f, p); }
void Canvas::OnMButtonDown(UINT f, CPoint p) { do_mouse_down(2, f, p); }
void Canvas::OnLButtonUp(UINT f, CPoint p)   { do_mouse_up(1, f, p); }
void Canvas::OnRButtonUp(UINT f, CPoint p)   { do_mouse_up(3, f, p); }
void Canvas::OnMButtonUp(UINT f, CPoint p)   { do_mouse_up(2, f, p); }
void Canvas::OnMouseMove(UINT f, CPoint p)   { do_mouse_move(f, p); }

void Canvas::OnChar(UINT ch, UINT repcount, UINT flags)
{
    GuiGetApp()->key_press(ch);
}

#if 0
void Canvas::OnActivate(UINT state, CWnd *other, BOOL is_minimized)
{
    if( state==WA_ACTIVE || state==WA_CLICKACTIVE )
    {
	wglMakeCurrent(dc, glcontext);
    }
}
#endif



/*****************************************

NOTES:

    AfxGetApp() returns a CWinApp* to the current application
    AfxGetAppName() returns a const char* to application name


  ****************************************/

void MfcGUI::setup_for_drawing()
{
    status("Hello There");
}

void MfcGUI::draw_contents()
{
    glClearColor(0.8f, 0.2f, 0.2f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool MfcGUI::mouse_down(int *where, int which) { return false; }

bool MfcGUI::mouse_up(int *where, int which) { return false; }

bool MfcGUI::mouse_drag(int *where, int *last, int which) { return false; }

bool MfcGUI::key_press(int key) { return false; }

void MfcGUI::update_animation() { }


} // namespace gfx
