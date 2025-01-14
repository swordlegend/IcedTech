/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
/*
** WIN_GLIMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_SwapBuffers
** GLimp_Init
** GLimp_Shutdown
** GLimp_SetGamma
**
** Note that the GLW_xxx functions are Windows specific GL-subsystem
** related functions that are relevant ONLY to win_glimp.c
*/
#include "engine_precompiled.h"
#pragma hdrstop

#include "win_local.h"
#include "rc/AFEditor_resource.h"
#include "rc/doom_resource.h"
#include "../../renderer/tr_local.h"

int   (WINAPI* qwglChoosePixelFormat)(HDC, CONST PIXELFORMATDESCRIPTOR*);
int   (WINAPI* qwglDescribePixelFormat) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
int   (WINAPI* qwglGetPixelFormat)(HDC);
BOOL(WINAPI* qwglSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR*);
BOOL(WINAPI* qwglSwapBuffers)(HDC);

BOOL(WINAPI* qwglCopyContext)(HGLRC, HGLRC, UINT);
HGLRC(WINAPI* qwglCreateContext)(HDC);
HGLRC(WINAPI* qwglCreateLayerContext)(HDC, int);
BOOL(WINAPI* qwglDeleteContext)(HGLRC);
HGLRC(WINAPI* qwglGetCurrentContext)(VOID);
HDC(WINAPI* qwglGetCurrentDC)(VOID);
PROC(WINAPI* qwglGetProcAddress)(LPCSTR);
BOOL(WINAPI* qwglMakeCurrent)(HDC, HGLRC);
BOOL(WINAPI* qwglShareLists)(HGLRC, HGLRC);
BOOL(WINAPI* qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD);

BOOL(WINAPI* qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT,
	FLOAT, int, LPGLYPHMETRICSFLOAT);

BOOL(WINAPI* qwglDescribeLayerPlane)(HDC, int, int, UINT,
	LPLAYERPLANEDESCRIPTOR);
int  (WINAPI* qwglSetLayerPaletteEntries)(HDC, int, int, int,
	CONST COLORREF*);
int  (WINAPI* qwglGetLayerPaletteEntries)(HDC, int, int, int,
	COLORREF*);
BOOL(WINAPI* qwglRealizeLayerPalette)(HDC, int, BOOL);
BOOL(WINAPI* qwglSwapLayerBuffers)(HDC, UINT);

static void		GLW_InitExtensions( void );
void *GLimp_ExtensionPointer(const char *name);

/*
=============================================================================

WglExtension Grabbing

This is gross -- creating a window just to get a context to get the wgl extensions

=============================================================================
*/

/*
====================
FakeWndProc

Only used to get wglExtensions
====================
*/
LONG WINAPI FakeWndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam) {

	if ( uMsg == WM_DESTROY ) {
        PostQuitMessage(0);
	}

	if ( uMsg != WM_CREATE ) {
	    return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	const static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,
		0, 0, 0, 0, 0, 0,
		8, 0,
		0, 0, 0, 0,
		24, 8,
		0,
		PFD_MAIN_PLANE,
		0,
		0,
		0,
		0,
	};
	int		pixelFormat;
	HDC hDC;
	HGLRC hGLRC;

    hDC = GetDC(hWnd);

    // Set up OpenGL
    pixelFormat = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, pixelFormat, &pfd);
    hGLRC = qwglCreateContext(hDC);
    qwglMakeCurrent(hDC, hGLRC);

	// free things
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hGLRC);
    ReleaseDC(hWnd, hDC);

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


/*
==================
GLW_GetWGLExtensionsWithFakeWindow
==================
*/
void GLW_CheckWGLExtensions( HDC hDC ) {
	common->Printf("Initilizing Glew...\n");
	if (glewInit() != GLEW_OK)
	{
		common->FatalError("Failed to initilize glew!\n");
	}

	if ( wglGetExtensionsStringARB ) {
		glConfig.wgl_extensions_string = (const char *) wglGetExtensionsStringARB(hDC);
	} else {
		glConfig.wgl_extensions_string = "";
	}

	r_swapInterval.SetModified();	// force a set next frame
}

/*
==================
GLW_GetWGLExtensionsWithFakeWindow
==================
*/
static void GLW_GetWGLExtensionsWithFakeWindow( void ) {
	HWND	hWnd;
    MSG		msg;

    // Create a window for the sole purpose of getting
	// a valid context to get the wglextensions
    hWnd = CreateWindow(WIN32_FAKE_WINDOW_CLASS_NAME, GAME_NAME,
        WS_OVERLAPPEDWINDOW,
        40, 40,
        640,
        480,
        NULL, NULL, win32.hInstance, NULL );
    if ( !hWnd ) {
        common->FatalError( "GLW_GetWGLExtensionsWithFakeWindow: Couldn't create fake window" );
    }

    HDC hDC = GetDC( hWnd );
	HGLRC gRC = wglCreateContext( hDC );
	wglMakeCurrent( hDC, gRC );
	GLW_CheckWGLExtensions( hDC );
	wglDeleteContext( gRC );
	ReleaseDC( hWnd, hDC );

    DestroyWindow( hWnd );
    while ( GetMessage( &msg, NULL, 0, 0 ) ) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
}

//=============================================================================

/*
====================
GLW_WM_CREATE
====================
*/
void GLW_WM_CREATE( HWND hWnd ) {
}



/*
====================
GLW_InitDriver

Set the pixelformat for the window before it is
shown, and create the rendering context
====================
*/
static bool GLW_InitDriver( glimpParms_t parms ) {
    PIXELFORMATDESCRIPTOR src = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		32,								// 32-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		8,								// 8 bit destination alpha
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		24,								// 24-bit z-buffer	
		8,								// 8-bit stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
    };

	common->Printf( "Initializing OpenGL driver\n" );

	//
	// get a DC for our window if we don't already have one allocated
	//
	if ( win32.hDC == NULL ) {
		common->Printf( "...getting DC: " );

		if ( ( win32.hDC = GetDC( win32.hWnd ) ) == NULL ) {
			common->Printf( "^3failed^0\n" );
			return false;
		}
		common->Printf( "succeeded\n" );
	}

	// the multisample path uses the wgl 
	if ( wglChoosePixelFormatARB && parms.multiSamples > 1 ) {
		int		iAttributes[20];
		FLOAT	fAttributes[] = {0, 0};
		UINT	numFormats;

		// FIXME: specify all the other stuff
		iAttributes[0] = WGL_SAMPLE_BUFFERS_ARB;
		iAttributes[1] = 1;
		iAttributes[2] = WGL_SAMPLES_ARB;
		iAttributes[3] = parms.multiSamples;
		iAttributes[4] = WGL_DOUBLE_BUFFER_ARB;
		iAttributes[5] = TRUE;
		iAttributes[6] = WGL_STENCIL_BITS_ARB;
		iAttributes[7] = 8;
		iAttributes[8] = WGL_DEPTH_BITS_ARB;
		iAttributes[9] = 24;
		iAttributes[10] = WGL_RED_BITS_ARB;
		iAttributes[11] = 8;
		iAttributes[12] = WGL_BLUE_BITS_ARB;
		iAttributes[13] = 8;
		iAttributes[14] = WGL_GREEN_BITS_ARB;
		iAttributes[15] = 8;
		iAttributes[16] = WGL_ALPHA_BITS_ARB;
		iAttributes[17] = 8;
		iAttributes[18] = 0;
		iAttributes[19] = 0;

		wglChoosePixelFormatARB( win32.hDC, iAttributes, fAttributes, 1, &win32.pixelformat, &numFormats );
	} else {
		// this is the "classic" choose pixel format path

		// eventually we may need to have more fallbacks, but for
		// now, ask for everything
		if ( parms.stereo ) {
			common->Printf( "...attempting to use stereo\n" );
			src.dwFlags |= PFD_STEREO;
		}

		//
		// choose, set, and describe our desired pixel format.  If we're
		// using a minidriver then we need to bypass the GDI functions,
		// otherwise use the GDI functions.
		//
		if ( ( win32.pixelformat = ChoosePixelFormat( win32.hDC, &src ) ) == 0 ) {
			common->Printf( "...^3GLW_ChoosePFD failed^0\n");
			return false;
		}
		common->Printf( "...PIXELFORMAT %d selected\n", win32.pixelformat );
	}

	// get the full info
	DescribePixelFormat( win32.hDC, win32.pixelformat, sizeof( win32.pfd ), &win32.pfd );
	glConfig.colorBits = win32.pfd.cColorBits;
	glConfig.depthBits = win32.pfd.cDepthBits;
	glConfig.stencilBits = win32.pfd.cStencilBits;

	// XP seems to set this incorrectly
	if ( !glConfig.stencilBits ) {
		glConfig.stencilBits = 8;
	}

	// the same SetPixelFormat is used either way
	if ( SetPixelFormat( win32.hDC, win32.pixelformat, &win32.pfd ) == FALSE ) {
		common->Printf( "...^3SetPixelFormat failed^0\n", win32.hDC );
		return false;
	}

	GLint attribs[] =
	{
		// Here we ask for OpenGL 2.1
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
		// Uncomment this for forward compatibility mode
		//WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		// Uncomment this for Compatibility profile
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		// We are using Core profile here
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	//
	// startup the OpenGL subsystem by creating a context and making it current
	//
	common->Printf( "...creating GL context: " );
	win32.hGLRC = wglCreateContextAttribsARB(win32.hDC, 0, attribs);
	if (win32.hGLRC == 0) {
		common->Printf( "^3failed^0\n" );
		return false;
	}
	common->Printf( "succeeded\n" );

	common->Printf( "...making context current: " );
	if ( !qwglMakeCurrent( win32.hDC, win32.hGLRC ) ) {
		qwglDeleteContext( win32.hGLRC );
		win32.hGLRC = NULL;
		common->Printf( "^3failed^0\n" );
		return false;
	}
	common->Printf( "succeeded\n" );

	return true;
}

/*
====================
GLW_CreateWindowClasses
====================
*/
static void GLW_CreateWindowClasses( void ) {
	WNDCLASS wc;

	//
	// register the window class if necessary
	//
	if ( win32.windowClassRegistered ) {
		return;
	}

	memset( &wc, 0, sizeof( wc ) );

	wc.style         = 0;
	wc.lpfnWndProc   = (WNDPROC) MainWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = win32.hInstance;
	wc.hIcon         = LoadIcon( win32.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = (struct HBRUSH__ *)COLOR_GRAYTEXT;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = WIN32_WINDOW_CLASS_NAME;

	if ( !RegisterClass( &wc ) ) {
		common->FatalError( "GLW_CreateWindow: could not register window class" );
	}
	common->Printf( "...registered window class\n" );

	// now register the fake window class that is only used
	// to get wgl extensions
	wc.style         = 0;
	wc.lpfnWndProc   = (WNDPROC) FakeWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = win32.hInstance;
	wc.hIcon         = LoadIcon( win32.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = (struct HBRUSH__ *)COLOR_GRAYTEXT;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = WIN32_FAKE_WINDOW_CLASS_NAME;

	if ( !RegisterClass( &wc ) ) {
		common->FatalError( "GLW_CreateWindow: could not register window class" );
	}
	common->Printf( "...registered fake window class\n" );

	win32.windowClassRegistered = true;
}

/*
=======================
GLW_CreateWindow

Responsible for creating the Win32 window.
If cdsFullscreen is true, it won't have a border
=======================
*/
static bool GLW_CreateWindow( glimpParms_t parms ) {
	int				stylebits;
	int				x, y, w, h;
	int				exstyle;

	//
	// compute width and height
	//
	if ( parms.fullScreen ) {
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP|WS_VISIBLE|WS_SYSMENU;

		x = 0;
		y = 0;
		w = parms.width;
		h = parms.height;
	} else {
		RECT	r;

		// adjust width and height for window border
		r.bottom = parms.height;
		r.left = 0;
		r.top = 0;
		r.right = parms.width;

		exstyle = 0;
		stylebits = WINDOW_STYLE|WS_SYSMENU;
		AdjustWindowRect (&r, stylebits, FALSE);

		w = r.right - r.left;
		h = r.bottom - r.top;

		x = win32.win_xpos.GetInteger();
		y = win32.win_ypos.GetInteger();

		// adjust window coordinates if necessary 
		// so that the window is completely on screen
		if ( x + w > win32.desktopWidth ) {
			x = ( win32.desktopWidth - w );
		}
		if ( y + h > win32.desktopHeight ) {
			y = ( win32.desktopHeight - h );
		}
		if ( x < 0 ) {
			x = 0;
		}
		if ( y < 0 ) {
			y = 0;
		}
	}

	win32.hWnd = CreateWindowEx (
		 exstyle, 
		 WIN32_WINDOW_CLASS_NAME,
		 GAME_NAME,
		 stylebits,
		 x, y, w, h,
		 NULL,
		 NULL,
		 win32.hInstance,
		 NULL);

	if ( !win32.hWnd ) {
		common->Printf( "^3GLW_CreateWindow() - Couldn't create window^0\n" );
		return false;
	}

	::SetTimer( win32.hWnd, 0, 100, NULL );

	ShowWindow( win32.hWnd, SW_SHOW );
	UpdateWindow( win32.hWnd );
	common->Printf( "...created window @ %d,%d (%dx%d)\n", x, y, w, h );

	if ( !GLW_InitDriver( parms ) ) {
		ShowWindow( win32.hWnd, SW_HIDE );
		DestroyWindow( win32.hWnd );
		win32.hWnd = NULL;
		return false;
	}

	SetForegroundWindow( win32.hWnd );
	SetFocus( win32.hWnd );

	glConfig.isFullscreen = parms.fullScreen;

	return true;
}



static void PrintCDSError( int value ) {
	switch ( value ) {
	case DISP_CHANGE_RESTART:
		common->Printf( "restart required\n" );
		break;
	case DISP_CHANGE_BADPARAM:
		common->Printf( "bad param\n" );
		break;
	case DISP_CHANGE_BADFLAGS:
		common->Printf( "bad flags\n" );
		break;
	case DISP_CHANGE_FAILED:
		common->Printf( "DISP_CHANGE_FAILED\n" );
		break;
	case DISP_CHANGE_BADMODE:
		common->Printf( "bad mode\n" );
		break;
	case DISP_CHANGE_NOTUPDATED:
		common->Printf( "not updated\n" );
		break;
	default:
		common->Printf( "unknown error %d\n", value );
		break;
	}
}


/*
===================
GLW_SetFullScreen
===================
*/
static bool GLW_SetFullScreen( glimpParms_t parms ) {
#if 0
	// for some reason, bounds checker claims that windows is
	// writing past the bounds of dm in the get display frequency call
	union {
		DEVMODE dm;
		byte	filler[1024];
	} hack;
#endif
	DEVMODE dm;
	int		cdsRet;

	DEVMODE		devmode;
	int			modeNum;
	bool		matched;

	// first make sure the user is not trying to select a mode that his card/monitor can't handle
	matched = false;
	for ( modeNum = 0 ; ; modeNum++ ) {
		if ( !EnumDisplaySettings( NULL, modeNum, &devmode ) ) {
			if ( matched ) {
				// we got a resolution match, but not a frequency match
				// so disable the frequency requirement
				common->Printf( "...^3%dhz is unsupported at %dx%d^0\n", parms.displayHz, parms.width, parms.height );
				parms.displayHz = 0;
				break;
			}
			common->Printf( "...^3%dx%d is unsupported in 32 bit^0\n", parms.width, parms.height );
			return false;
		}
		if ( (int)devmode.dmPelsWidth >= parms.width
			&& (int)devmode.dmPelsHeight >= parms.height
			&& devmode.dmBitsPerPel == 32 ) {

			matched = true;

			if ( parms.displayHz == 0 || devmode.dmDisplayFrequency == parms.displayHz ) {
				break;
			}
		}
	}

	memset( &dm, 0, sizeof( dm ) );
	dm.dmSize = sizeof( dm );

	dm.dmPelsWidth  = parms.width;
	dm.dmPelsHeight = parms.height;
	dm.dmBitsPerPel = 32;
	dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

	if ( parms.displayHz != 0 ) {
		dm.dmDisplayFrequency = parms.displayHz;
		dm.dmFields |= DM_DISPLAYFREQUENCY;
	}
	
	common->Printf( "...calling CDS: " );
	
	// try setting the exact mode requested, because some drivers don't report
	// the low res modes in EnumDisplaySettings, but still work
	if ( ( cdsRet = ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL ) {
		common->Printf( "ok\n" );
		win32.cdsFullscreen = true;
		return true;
	}

	//
	// the exact mode failed, so scan EnumDisplaySettings for the next largest mode
	//
	common->Printf( "^3failed^0, " );
	
	PrintCDSError( cdsRet );

	common->Printf( "...trying next higher resolution:" );
	
	// we could do a better matching job here...
	for ( modeNum = 0 ; ; modeNum++ ) {
		if ( !EnumDisplaySettings( NULL, modeNum, &devmode ) ) {
			break;
		}
		if ( (int)devmode.dmPelsWidth >= parms.width
			&& (int)devmode.dmPelsHeight >= parms.height
			&& devmode.dmBitsPerPel == 32 ) {

			if ( ( cdsRet = ChangeDisplaySettings( &devmode, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL ) {
				common->Printf( "ok\n" );
				win32.cdsFullscreen = true;

				return true;
			}
			break;
		}
	}

	common->Printf( "\n...^3no high res mode found^0\n" );
	return false;
}



/*
===================
GLimp_Init

This is the platform specific OpenGL initialization function.  It
is responsible for loading OpenGL, initializing it,
creating a window of the appropriate size, doing
fullscreen manipulations, etc.  Its overall responsibility is
to make sure that a functional OpenGL subsystem is operating
when it returns to the ref.

If there is any failure, the renderer will revert back to safe
parameters and try again.
===================
*/
bool GLimp_Init( glimpParms_t parms ) {
	const char	*driverName;
	HDC		hDC;

	common->Printf( "Initializing OpenGL subsystem\n" );

	// check our desktop attributes
	hDC = GetDC( GetDesktopWindow() );
	win32.desktopBitsPixel = GetDeviceCaps( hDC, BITSPIXEL );
	win32.desktopWidth = GetDeviceCaps( hDC, HORZRES );
	win32.desktopHeight = GetDeviceCaps( hDC, VERTRES );
	ReleaseDC( GetDesktopWindow(), hDC );

	// we can't run in a window unless it is 32 bpp
	if ( win32.desktopBitsPixel < 32 && !parms.fullScreen ) {
		common->Printf("^3Windowed mode requires 32 bit desktop depth^0\n");
		return false;
	}

	// create our window classes if we haven't already
	GLW_CreateWindowClasses();

	// this will load the dll and set all our gl* function pointers,
	// but doesn't create a window

	{
		qwglCopyContext = wglCopyContext;
		qwglCreateContext = wglCreateContext;
		qwglCreateLayerContext = wglCreateLayerContext;
		qwglDeleteContext = wglDeleteContext;
		qwglDescribeLayerPlane = wglDescribeLayerPlane;
		qwglGetCurrentContext = wglGetCurrentContext;
		qwglGetCurrentDC = wglGetCurrentDC;
		qwglGetLayerPaletteEntries = wglGetLayerPaletteEntries;
		qwglGetProcAddress = wglGetProcAddress;
		qwglMakeCurrent = wglMakeCurrent;
		qwglRealizeLayerPalette = wglRealizeLayerPalette;
		qwglSetLayerPaletteEntries = wglSetLayerPaletteEntries;
		qwglShareLists = wglShareLists;
		qwglSwapLayerBuffers = wglSwapLayerBuffers;
		qwglUseFontBitmaps = wglUseFontBitmapsA;
		qwglUseFontOutlines = wglUseFontOutlinesA;

		qwglChoosePixelFormat = ChoosePixelFormat;
		qwglDescribePixelFormat = DescribePixelFormat;
		qwglGetPixelFormat = GetPixelFormat;
		qwglSetPixelFormat = SetPixelFormat;
		qwglSwapBuffers = SwapBuffers;
	}

	// getting the wgl extensions involves creating a fake window to get a context,
	// which is pretty disgusting, and seems to mess with the AGP VAR allocation
	GLW_GetWGLExtensionsWithFakeWindow();

	// try to change to fullscreen
	if ( parms.fullScreen ) {
		if ( !GLW_SetFullScreen( parms ) ) {
			GLimp_Shutdown();
			return false;
		}
	}

	// try to create a window with the correct pixel format
	// and init the renderer context
	if ( !GLW_CreateWindow( parms ) ) {
		GLimp_Shutdown();
		return false;
	}

	// wglSwapinterval, etc
	GLW_CheckWGLExtensions( win32.hDC );

	return true;
}


/*
===================
GLimp_SetScreenParms

Sets up the screen based on passed parms.. 
===================
*/
bool GLimp_SetScreenParms( glimpParms_t parms ) {
	int exstyle;
	int stylebits;
	int x, y, w, h;
	DEVMODE dm;

	memset( &dm, 0, sizeof( dm ) );
	dm.dmSize = sizeof( dm );
	dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
	if ( parms.displayHz != 0 ) {
		dm.dmDisplayFrequency = parms.displayHz;
		dm.dmFields |= DM_DISPLAYFREQUENCY;
	}

	win32.cdsFullscreen = parms.fullScreen;
	glConfig.isFullscreen = parms.fullScreen;

	if ( parms.fullScreen ) {
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP|WS_VISIBLE|WS_SYSMENU;
		SetWindowLong( win32.hWnd, GWL_STYLE, stylebits );
		SetWindowLong( win32.hWnd, GWL_EXSTYLE, exstyle );
		dm.dmPelsWidth  = parms.width;
		dm.dmPelsHeight = parms.height;
		dm.dmBitsPerPel = 32;
		x = y = w = h = 0;
	} else {
		RECT	r;

		// adjust width and height for window border
		r.bottom = parms.height;
		r.left = 0;
		r.top = 0;
		r.right = parms.width;

		w = r.right - r.left;
		h = r.bottom - r.top;

		x = win32.win_xpos.GetInteger();
		y = win32.win_ypos.GetInteger();

		// adjust window coordinates if necessary 
		// so that the window is completely on screen
		if ( x + w > win32.desktopWidth ) {
			x = ( win32.desktopWidth - w );
		}
		if ( y + h > win32.desktopHeight ) {
			y = ( win32.desktopHeight - h );
		}
		if ( x < 0 ) {
			x = 0;
		}
		if ( y < 0 ) {
			y = 0;
		}
		dm.dmPelsWidth  = win32.desktopWidth;
		dm.dmPelsHeight = win32.desktopHeight;
		dm.dmBitsPerPel = win32.desktopBitsPixel;
		exstyle = 0;
		stylebits = WINDOW_STYLE|WS_SYSMENU;
		AdjustWindowRect (&r, stylebits, FALSE);
		SetWindowLong( win32.hWnd, GWL_STYLE, stylebits );
		SetWindowLong( win32.hWnd, GWL_EXSTYLE, exstyle );
		common->Printf( "%i %i %i %i\n", x, y, w, h );
	}
	bool ret = ( ChangeDisplaySettings( &dm, parms.fullScreen ? CDS_FULLSCREEN : 0 ) == DISP_CHANGE_SUCCESSFUL );
	SetWindowPos( win32.hWnd, parms.fullScreen ? HWND_TOPMOST : HWND_NOTOPMOST, x, y, w, h, parms.fullScreen ? SWP_NOSIZE | SWP_NOMOVE : SWP_SHOWWINDOW );
	return ret;
}

/*
===================
GLimp_Shutdown

This routine does all OS specific shutdown procedures for the OpenGL
subsystem.
===================
*/
void GLimp_Shutdown( void ) {
	const char *success[] = { "failed", "success" };
	int retVal;

	common->Printf( "Shutting down OpenGL subsystem\n" );

	// set current context to NULL
	if ( qwglMakeCurrent ) {
		retVal = qwglMakeCurrent( NULL, NULL ) != 0;
		common->Printf( "...wglMakeCurrent( NULL, NULL ): %s\n", success[retVal] );
	}

	// delete HGLRC
	if ( win32.hGLRC ) {
		retVal = qwglDeleteContext( win32.hGLRC ) != 0;
		common->Printf( "...deleting GL context: %s\n", success[retVal] );
		win32.hGLRC = NULL;
	}

	// release DC
	if ( win32.hDC ) {
		retVal = ReleaseDC( win32.hWnd, win32.hDC ) != 0;
		common->Printf( "...releasing DC: %s\n", success[retVal] );
		win32.hDC   = NULL;
	}

	// destroy window
	if ( win32.hWnd ) {
		common->Printf( "...destroying window\n" );
		ShowWindow( win32.hWnd, SW_HIDE );
		DestroyWindow( win32.hWnd );
		win32.hWnd = NULL;
	}

	// reset display settings
	if ( win32.cdsFullscreen ) {
		common->Printf( "...resetting display\n" );
		ChangeDisplaySettings( 0, 0 );
		win32.cdsFullscreen = false;
	}

	// close the thread so the handle doesn't dangle
	if ( win32.renderThreadHandle ) {
		common->Printf( "...closing smp thread\n" );
		CloseHandle( win32.renderThreadHandle );
		win32.renderThreadHandle = NULL;
	}
}


/*
=====================
GLimp_SwapBuffers
=====================
*/
void GLimp_SwapBuffers( void ) {
	//
	// wglSwapinterval is a windows-private extension,
	// so we must check for it here instead of portably
	//
	if ( r_swapInterval.IsModified() ) {
		r_swapInterval.ClearModified();

		if ( wglSwapIntervalEXT ) {
			wglSwapIntervalEXT( r_swapInterval.GetInteger() );
		}
	}

#if defined(ID_ALLOW_TOOLS)
	if (com_editors) {
		RadiantSwapBuffer();
	}
	else {
		qwglSwapBuffers(win32.hDC);
	}
#else
	qwglSwapBuffers(win32.hDC);
#endif

//Sys_DebugPrintf( "*** SwapBuffers() ***\n" );
}

/*
===================
GLimp_ExtensionPointer

Returns a function pointer for an OpenGL extension entry point
===================
*/
void *GLimp_ExtensionPointer( const char *name ) {
	void	*proc;

	proc = (void *)wglGetProcAddress( name );

	if ( !proc ) {
		common->Printf( "Couldn't find proc address for: %s\n", name );
	}

	return proc;
}

