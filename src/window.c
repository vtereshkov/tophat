#include "tophat.h"
#include <stdlib.h>
#include <stdio.h>

extern th_global thg;

#ifdef linux
#include <X11/Xlib.h>
#include <GL/glx.h>
Display *th_dpy;
Window th_win;
Window th_root_win;
XWindowAttributes th_win_attribs;
GLXContext th_ctx;


void th_window_deinit() {
	XDestroyWindow(th_dpy, th_win);
	XCloseDisplay(th_dpy);
}

void th_window_setup(char *name, int w, int h) {
	th_dpy = XOpenDisplay(NULL);
	if (!th_dpy) {
		th_error("Could not open X display.");
		return;
	}

	int screen = DefaultScreen(th_dpy);
	th_root_win = DefaultRootWindow(th_dpy);

	int gl_attribs[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		None
	};

	XVisualInfo *vi = glXChooseVisual(th_dpy, screen, gl_attribs);
	if (!vi) {
		th_error("No visual found.");
		return;
	}

	XSetWindowAttributes attribs;
	attribs.background_pixel = 0;
	attribs.colormap = XCreateColormap(th_dpy, th_root_win, vi->visual, AllocNone);
	th_win = XCreateWindow(th_dpy, th_root_win, 0, 0, w, h, 0, vi->depth, InputOutput,
		vi->visual, CWColormap | CWBackPixel, &attribs);

	XMapWindow(th_dpy, th_win);
	XStoreName(th_dpy, th_win, name);
	th_ctx = glXCreateContext(th_dpy, vi, NULL, GL_TRUE);
	if (!th_ctx) {
		th_error("Could not create a context.");
		return;
	}
	glXMakeCurrent(th_dpy, th_win, th_ctx);

	XGetWindowAttributes(th_dpy, th_win, &th_win_attribs);
	XSelectInput(th_dpy, th_win, KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | ExposureMask | PointerMotionMask);
	atexit(th_window_deinit);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void th_window_get_dimensions(int *w, int *h) {
	*w = th_win_attribs.width;
	*h = th_win_attribs.height;
}

int th_window_handle() {
	if (!th_win)
		return 0;

	XEvent ev;
	while (XPending(th_dpy)) {
		XNextEvent(th_dpy, &ev);

		int keyDir = 1;
		switch (ev.type) {
		case Expose:
			XGetWindowAttributes(th_dpy, th_win, &th_win_attribs);
			glViewport(0, 0, th_win_attribs.width, th_win_attribs.height);
		case KeyRelease:
		case KeyPress:
			th_input_key(XLookupKeysym(&ev.xkey, 0), ev.type == KeyPress);
			break;
		case ButtonRelease:
			keyDir = 0;
		case ButtonPress:
			th_input_key(ev.xbutton.button, keyDir);
			break;
		case MotionNotify:
			thg.mouse = (th_vf2){ .x = ev.xmotion.x, .y = ev.xmotion.y };
			break;
		}
	}

	return 1;
}

void th_window_clear_frame() {
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void th_window_swap_buffers() {
	th_canvas_flush();
	th_image_flush();
	th_input_cycle();

	glXSwapBuffers(th_dpy, th_win);
}
#elif _WIN32
#include <windows.h>
#include <wingdi.h>
#include <windowsx.h>

static HWND th_win;
static HDC th_hdc;
static RECT th_win_rect;
static int should_close = 0;
static th_vf2 win_size;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_DESTROY)
		should_close = 1;
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void th_window_setup(char *name, int w, int h) {
	const char CLASS_NAME[] = "tophat";

	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {
		.lpfnWndProc = WndProc,
		.hInstance = hInstance,
		.hCursor = LoadCursor(NULL, IDC_ARROW),
		.lpszClassName = CLASS_NAME
	};

	RegisterClass(&wc);

	th_win = CreateWindow(
		CLASS_NAME,
		name,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		w, h,
		NULL, NULL,
		hInstance, NULL);
	GetWindowRect(th_win, &th_win_rect);
	win_size.x = th_win_rect.right;
	win_size.y = th_win_rect.bottom;

	if (th_win == NULL) {
		th_error("could not create a window");
		exit(1);
	}

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,
		8, 0, 8, 8, 8, 16, 
		8,
		24,
		32,
		8, 8, 8, 8,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	th_hdc = GetDC(th_win);
	GLuint pf = ChoosePixelFormat(th_hdc, &pfd);
	if (!SetPixelFormat(th_hdc, pf, &pfd)) {
		th_error("couldn't choose pixel format");
		exit(1);
	}
	HGLRC ctx = wglCreateContext(th_hdc);
	if (!ctx) {
		th_error("couldn't create an opengl context");
		exit(1);
	}
	wglMakeCurrent(th_hdc, ctx);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	ShowWindow(th_win, 1);
}

void th_window_get_dimensions(int *w, int *h) {
	*w = th_win_rect.right - th_win_rect.left;
	*h = th_win_rect.bottom - th_win_rect.top;
}

int th_window_handle() {
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, 1)) {
		TranslateMessage(&msg);

		// i found a list of these on the wine website, but not on MSDN -_-
		switch (msg.message) {
		case WM_KEYDOWN:
		case WM_KEYUP:
			th_input_key(tolower(msg.wParam), msg.message == WM_KEYDOWN);
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			th_input_key(1, msg.message == WM_LBUTTONDOWN);
			break;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			th_input_key(2, msg.message == WM_MBUTTONDOWN);
			break;
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			th_input_key(3, msg.message == WM_RBUTTONDOWN);
			break;
		case WM_MOUSEMOVE:
			thg.mouse = (th_vf2){ .x = GET_X_LPARAM(msg.lParam), .y = GET_Y_LPARAM(msg.lParam) };
			break;
		default:
			DispatchMessage(&msg);
			break;
		}
	}
	GetClientRect(th_win, &th_win_rect);
	glViewport(0, 0, th_win_rect.right, th_win_rect.bottom);

	return !should_close;
}

void th_window_swap_buffers() {
	th_canvas_flush();
	th_image_flush();
	th_input_cycle();
	SwapBuffers(th_hdc);
}

void th_window_clear_frame() {
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

#else
#error tophat cant create a window on this platform yet
#endif
