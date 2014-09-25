#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
#include <cassert>
#include <crtdbg.h>
#include "TimeInterface.hpp"
#include "../../Client/Code/Game/Game.hpp"
#pragma comment( lib, "opengl32" ) // Link in the OpenGL32.lib static library
#pragma warning( disable : 4996 ) // Ignore warning about freopen (I prefer standard C++ libraries)

//-----------------------------------------------------------------------------------------------
#define UNUSED(x) (void)(x);

//-----------------------------------------------------------------------------------------------
bool g_isQuitting = false;
HWND g_hWnd = nullptr;
HDC g_displayDeviceContext = nullptr;
HGLRC g_openGLRenderingContext = nullptr;
const char* APP_NAME = "Square Tag";
bool g_openConsole = true;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int WINDOW_OFFSET_VERT = 50;
const int WINDOW_OFFSET_HORZ = 50;

static const double LOCKED_FRAME_RATE_SECONDS = 1.0 / 60.0;
static Game g_gameInstance( SCREEN_WIDTH, SCREEN_HEIGHT );

//-----------------------------------------------------------------------------------------------
LRESULT CALLBACK WindowsMessageHandlingProcedure( HWND windowHandle, UINT wmMessageCode, WPARAM wParam, LPARAM lParam )
{
	unsigned char asKey = (unsigned char) wParam;
	switch( wmMessageCode )
	{
		case WM_CLOSE:
		case WM_DESTROY:
		case WM_QUIT:
			g_isQuitting = true;
			return 0;

		case WM_KEYDOWN:
			if( asKey == VK_ESCAPE )
			{
				g_isQuitting = true;
				return 0;
			}
			else
			{
				bool wasProcessed = g_gameInstance.HandleKeyDownEvent( asKey );
				if( wasProcessed ) 
					return 0;
			}
			break;
		case WM_KEYUP:
			bool wasProcessed = g_gameInstance.HandleKeyUpEvent( asKey );
			if( wasProcessed ) 
				return 0;
			break;
	}

	return DefWindowProc( windowHandle, wmMessageCode, wParam, lParam );
}

//-----------------------------------------------------------------------------------------------
void CreateOpenGLWindow( HINSTANCE applicationInstanceHandle )
{
	// Define a window class
	WNDCLASSEX windowClassDescription;
	memset( &windowClassDescription, 0, sizeof( windowClassDescription ) );
	windowClassDescription.cbSize = sizeof( windowClassDescription );
	windowClassDescription.style = CS_OWNDC; // Redraw on move, request own Display Context
	windowClassDescription.lpfnWndProc = static_cast< WNDPROC >( WindowsMessageHandlingProcedure ); // Assign a win32 message-handling function
	windowClassDescription.hInstance = GetModuleHandle( NULL );
	windowClassDescription.hIcon = NULL;
	windowClassDescription.hCursor = NULL;
	windowClassDescription.lpszClassName = TEXT( "Simple Window Class" );
	RegisterClassEx( &windowClassDescription );

	const DWORD windowStyleFlags = WS_CAPTION | WS_BORDER | WS_THICKFRAME | WS_SYSMENU | WS_OVERLAPPED;
	const DWORD windowStyleExFlags = WS_EX_APPWINDOW;

	RECT desktopRect;
	HWND desktopWindowHandle = GetDesktopWindow();
	GetClientRect( desktopWindowHandle, &desktopRect );

	RECT windowRect = { WINDOW_OFFSET_HORZ, WINDOW_OFFSET_VERT, WINDOW_OFFSET_HORZ + SCREEN_WIDTH, WINDOW_OFFSET_VERT + SCREEN_HEIGHT };
	AdjustWindowRectEx( &windowRect, windowStyleFlags, FALSE, windowStyleExFlags );

	g_hWnd = CreateWindowEx(
		windowStyleExFlags,
		windowClassDescription.lpszClassName,
		TEXT( APP_NAME ),
		windowStyleFlags,
		windowRect.left,
		windowRect.top,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		applicationInstanceHandle,
		NULL );

	ShowWindow( g_hWnd, SW_SHOW );
	SetForegroundWindow( g_hWnd );
	SetFocus( g_hWnd );

	g_displayDeviceContext = GetDC( g_hWnd );

	HCURSOR cursor = LoadCursor( NULL, IDC_ARROW );
	SetCursor( cursor );

	PIXELFORMATDESCRIPTOR pixelFormatDescriptor;
	memset( &pixelFormatDescriptor, 0, sizeof( pixelFormatDescriptor ) );
	pixelFormatDescriptor.nSize			= sizeof( pixelFormatDescriptor );
	pixelFormatDescriptor.nVersion		= 1;
	pixelFormatDescriptor.dwFlags		= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pixelFormatDescriptor.iPixelType	= PFD_TYPE_RGBA;
	pixelFormatDescriptor.cColorBits	= 24;
	pixelFormatDescriptor.cDepthBits	= 24;
	pixelFormatDescriptor.cAccumBits	= 0;
	pixelFormatDescriptor.cStencilBits	= 8;

	int pixelFormatCode = ChoosePixelFormat( g_displayDeviceContext, &pixelFormatDescriptor );
	SetPixelFormat( g_displayDeviceContext, pixelFormatCode, &pixelFormatDescriptor );
	g_openGLRenderingContext = wglCreateContext( g_displayDeviceContext );
	wglMakeCurrent( g_displayDeviceContext, g_openGLRenderingContext );

	//Enable blending/transparency
	glEnable( GL_BLEND );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//-----------------------------------------------------------------------------------------------
void RunMessagePump()
{
	MSG queuedMessage;
	for( ;; )
	{
		const BOOL wasMessagePresent = PeekMessage( &queuedMessage, NULL, 0, 0, PM_REMOVE );
		if( !wasMessagePresent )
		{
			break;
		}

		TranslateMessage( &queuedMessage );
		DispatchMessage( &queuedMessage );
	}
}

//-----------------------------------------------------------------------------------------------
void Update( double timeSpentLastFrameSeconds )
{
	g_gameInstance.Update( timeSpentLastFrameSeconds );
}

//-----------------------------------------------------------------------------------------------
void Render()
{
	glClearColor( 0.898f, 0.792f, 0.713f, 1.f );
	glClear( GL_COLOR_BUFFER_BIT );

	g_gameInstance.Render();
	SwapBuffers( g_displayDeviceContext );
}

//-----------------------------------------------------------------------------------------------
double WaitUntilNextFrameThenGiveFrameTime()
{
	static double targetTime = 0;
	targetTime = GetCurrentTimeSeconds() + LOCKED_FRAME_RATE_SECONDS;
	double timeNow = GetCurrentTimeSeconds();

	while( timeNow < targetTime )
	{
		timeNow = GetCurrentTimeSeconds();
	}

	return LOCKED_FRAME_RATE_SECONDS;
}

//-----------------------------------------------------------------------------------------------
void RunFrame()
{
	static double timeSpentLastFrameSeconds = 0.0;
	RunMessagePump();
	Update( LOCKED_FRAME_RATE_SECONDS );
	Render();
	timeSpentLastFrameSeconds = WaitUntilNextFrameThenGiveFrameTime();
}

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int )
{
	UNUSED( commandLineString );
	if( __argc != 4 )
	{
		printf( "Incorrect Number of Arguments.\n \tUsage: %s [client port] [server address] [server port] \n", __argv[ 0 ] );
		return -1;
	}
	std::string clientPort( __argv[ 1 ] );
	std::string serverAddress( __argv[ 2 ] );
	std::string serverPort( __argv[ 3 ] );

	InitializeTimer();

	if( g_openConsole )
	{
		AllocConsole();
		AttachConsole( GetCurrentProcessId() );
		freopen( "CON", "w", stdout );

		char consoleWindowTitle[ 16 ] = "Vingine Console";
		SetConsoleTitle( &consoleWindowTitle[ 0 ] );
		HWND consoleWindow = FindWindow( nullptr, &consoleWindowTitle[ 0 ] );

		SetWindowPos( consoleWindow, nullptr, 50, 850, 800, 200, SWP_NOZORDER );
	}

	CreateOpenGLWindow( applicationInstanceHandle );
	g_gameInstance.Start( clientPort, serverAddress, serverPort );
	while( !g_isQuitting )	
	{
		RunFrame();
	}
	Texture::CleanUpTextureRepository();

	if( g_openConsole )
		FreeConsole();

#if defined( _WIN32 ) && defined( _DEBUG )
	assert( _CrtCheckMemory() );
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}
