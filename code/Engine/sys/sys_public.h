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

#ifndef __SYS_PUBLIC__
#define __SYS_PUBLIC__

/*
===============================================================================

	Non-portable system services.

===============================================================================
*/


// Win32
#if defined(WIN32) || defined(_WIN32)

#define	BUILD_STRING					"win-x64"
#define BUILD_OS_ID						0
#define	CPUSTRING						"x64"
#define CPU_EASYARGS					1

#define ALIGN( x, a ) ( ( ( x ) + ((a)-1) ) & ~((a)-1) )
#define ALIGN16( x )					__declspec(align(16)) x
#define PACKED

#define _alloca16( x )					((void *)((((INT_PTR)_alloca( (x)+15 )) + 15) & ~15))

#define PATHSEPERATOR_STR				"\\"
#define PATHSEPERATOR_CHAR				'\\'

#define ID_INLINE						__forceinline
#define ID_INLINE_EXTERN				__forceinline
#define ID_FORCE_INLINE_EXTERN			__forceinline
#define ID_STATIC_TEMPLATE				static

#define assertmem( x, y )				assert( _CrtIsValidPointer( x, y, true ) )

#define verify( x )		( ( x ) ? true : false )

#endif

// Mac OSX
#if defined(MACOS_X) || defined(__APPLE__)

#define BUILD_STRING				"MacOSX-universal"
#define BUILD_OS_ID					1
#ifdef __ppc__
	#define	CPUSTRING					"ppc"
	#define CPU_EASYARGS				0
#elif defined(__i386__)
	#define	CPUSTRING					"x86"
	#define CPU_EASYARGS				1
#endif

#define ALIGN16( x )					x __attribute__ ((aligned (16)))

#ifdef __MWERKS__
#define PACKED
#include <alloca.h>
#else
#define PACKED							__attribute__((packed))
#endif

#define _alloca							alloca
#define _alloca16( x )					((void *)((((int)alloca( (x)+15 )) + 15) & ~15))

#define PATHSEPERATOR_STR				"/"
#define PATHSEPERATOR_CHAR				'/'

#define __cdecl
#define ASSERT							assert

#define ID_INLINE						inline
#define ID_STATIC_TEMPLATE

#define assertmem( x, y )

#endif


// Linux
#ifdef __linux__

#ifdef __i386__
	#define	BUILD_STRING				"linux-x86"
	#define BUILD_OS_ID					2
	#define CPUSTRING					"x86"
	#define CPU_EASYARGS				1
#elif defined(__ppc__)
	#define	BUILD_STRING				"linux-ppc"
	#define CPUSTRING					"ppc"
	#define CPU_EASYARGS				0
#endif

#define _alloca							alloca
#define _alloca16( x )					((void *)((((int)alloca( (x)+15 )) + 15) & ~15))

#define ALIGN16( x )					x
#define PACKED							__attribute__((packed))

#define PATHSEPERATOR_STR				"/"
#define PATHSEPERATOR_CHAR				'/'

#define __cdecl
#define ASSERT							assert

#define ID_INLINE						inline
#define ID_STATIC_TEMPLATE

#define assertmem( x, y )

#endif

#ifdef __GNUC__
#define id_attribute(x) __attribute__(x)
#else
#define id_attribute(x)  
#endif

#define ALIGN16( x )					__declspec(align(16)) x
#define ALIGNTYPE16						__declspec(align(16))
#define ALIGNTYPE128					__declspec(align(128))

#define MAX_TYPE( x )			( ( ( ( 1 << ( ( sizeof( x ) - 1 ) * 8 - 1 ) ) - 1 ) << 8 ) | 255 )
#define MIN_TYPE( x )			( - MAX_TYPE( x ) - 1 )
#define MAX_UNSIGNED_TYPE( x )	( ( ( ( 1U << ( ( sizeof( x ) - 1 ) * 8 ) ) - 1 ) << 8 ) | 255U )
#define MIN_UNSIGNED_TYPE( x )	0

#define assert_2_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) &  1 ) == 0 )
#define assert_4_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) &  3 ) == 0 )
#define assert_8_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) &  7 ) == 0 )
#define assert_16_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) & 15 ) == 0 )
#define assert_32_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) & 31 ) == 0 )
#define assert_64_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) & 63 ) == 0 )
#define assert_128_byte_aligned( ptr )		assert( ( ((UINT_PTR)(ptr)) & 127 ) == 0 )
#define assert_aligned_to_type_size( ptr )	assert( ( ((UINT_PTR)(ptr)) & ( sizeof( (ptr)[0] ) - 1 ) ) == 0 )

typedef enum {
	CPUID_NONE							= 0x00000,
	CPUID_UNSUPPORTED					= 0x00001,	// unsupported (386/486)
	CPUID_GENERIC						= 0x00002,	// unrecognized processor
	CPUID_INTEL							= 0x00004,	// Intel
	CPUID_AMD							= 0x00008,	// AMD
	CPUID_MMX							= 0x00010,	// Multi Media Extensions
	CPUID_3DNOW							= 0x00020,	// 3DNow!
	CPUID_SSE							= 0x00040,	// Streaming SIMD Extensions
	CPUID_SSE2							= 0x00080,	// Streaming SIMD Extensions 2
	CPUID_SSE3							= 0x00100,	// Streaming SIMD Extentions 3 aka Prescott's New Instructions
	CPUID_ALTIVEC						= 0x00200,	// AltiVec
	CPUID_HTT							= 0x01000,	// Hyper-Threading Technology
	CPUID_CMOV							= 0x02000,	// Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
	CPUID_FTZ							= 0x04000,	// Flush-To-Zero mode (denormal results are flushed to zero)
	CPUID_DAZ							= 0x08000	// Denormals-Are-Zero mode (denormal source operands are set to zero)
} cpuid_t;

typedef enum {
	FPU_EXCEPTION_INVALID_OPERATION		= 1,
	FPU_EXCEPTION_DENORMALIZED_OPERAND	= 2,
	FPU_EXCEPTION_DIVIDE_BY_ZERO		= 4,
	FPU_EXCEPTION_NUMERIC_OVERFLOW		= 8,
	FPU_EXCEPTION_NUMERIC_UNDERFLOW		= 16,
	FPU_EXCEPTION_INEXACT_RESULT		= 32
} fpuExceptions_t;

typedef enum {
	FPU_PRECISION_SINGLE				= 0,
	FPU_PRECISION_DOUBLE				= 1,
	FPU_PRECISION_DOUBLE_EXTENDED		= 2
} fpuPrecision_t;

typedef enum {
	FPU_ROUNDING_TO_NEAREST				= 0,
	FPU_ROUNDING_DOWN					= 1,
	FPU_ROUNDING_UP						= 2,
	FPU_ROUNDING_TO_ZERO				= 3
} fpuRounding_t;

typedef enum {
	AXIS_SIDE,
	AXIS_FORWARD,
	AXIS_UP,
	AXIS_ROLL,
	AXIS_YAW,
	AXIS_PITCH,
	MAX_JOYSTICK_AXIS
} joystickAxis_t;

typedef enum {
	SE_NONE,				// evTime is still valid
	SE_KEY,					// evValue is a key code, evValue2 is the down flag
	SE_CHAR,				// evValue is an ascii char
	SE_MOUSE,				// evValue and evValue2 are reletive signed x / y moves
	SE_JOYSTICK_AXIS,		// evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE				// evPtr is a char*, from typing something at a non-game console
} sysEventType_t;

typedef enum {
	M_ACTION1,
	M_ACTION2,
	M_ACTION3,
	M_ACTION4,
	M_ACTION5,
	M_ACTION6,
	M_ACTION7,
	M_ACTION8,
	M_DELTAX,
	M_DELTAY,
	M_DELTAZ
} sys_mEvents;

typedef struct sysEvent_s {
	sysEventType_t	evType;
	int				evValue;
	int				evValue2;
	int				evPtrLength;		// bytes of data pointed to by evPtr, for journaling
	void *			evPtr;				// this must be manually freed if not NULL
} sysEvent_t;

typedef struct sysMemoryStats_s {
	int memoryLoad;
	int totalPhysical;
	int availPhysical;
	int totalPageFile;
	int availPageFile;
	int totalVirtual;
	int availVirtual;
	int availExtendedVirtual;
} sysMemoryStats_t;

typedef unsigned long address_t;

template<class type> class idList;		// for Sys_ListFiles


void			Sys_Init( void );
void			Sys_Shutdown( void );
void			Sys_Error( const char *error, ...);
void			Sys_Quit( void );

#define ID_LANG_ENGLISH		"english"
#define ID_LANG_FRENCH		"french"
#define ID_LANG_ITALIAN		"italian"
#define ID_LANG_GERMAN		"german"
#define ID_LANG_SPANISH		"spanish"
#define ID_LANG_JAPANESE	"japanese"
int Sys_NumLangs();
const char * Sys_Lang(int idx);

bool			Sys_AlreadyRunning( void );

// note that this isn't journaled...
char *			Sys_GetClipboardData( void );
void			Sys_SetClipboardData( const char *string );

// will go to the various text consoles
// NOT thread safe - never use in the async paths
void			Sys_Printf( const char *msg, ... )id_attribute((format(printf,1,2)));

// returns true if the game window is focused.
bool			Sys_IsGameWindowFocused(void);
unsigned char   Sys_ScanKeyConvert(char ch);

// guaranteed to be thread-safe
void			Sys_DebugPrintf( const char *fmt, ... )id_attribute((format(printf,1,2)));
void			Sys_DebugVPrintf( const char *fmt, va_list arg );

// a decent minimum sleep time to avoid going below the process scheduler speeds
#define			SYS_MINSLEEP	20

// allow game to yield CPU time
// NOTE: due to SYS_MINSLEEP this is very bad portability karma, and should be completely removed
void			Sys_Sleep( int msec );

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int				Sys_Milliseconds( void );

uint64_t		Sys_Microseconds(void);

// for accurate performance testing
double			Sys_GetClockTicks( void );
double			Sys_ClockTicksPerSecond( void );

// returns a selection of the CPUID_* flags
cpuid_t			Sys_GetProcessorId( void );
const char *	Sys_GetProcessorString( void );

// returns true if the FPU stack is empty
bool			Sys_FPU_StackIsEmpty( void );

// empties the FPU stack
void			Sys_FPU_ClearStack( void );

// returns the FPU state as a string
const char *	Sys_FPU_GetState( void );

// enables the given FPU exceptions
void			Sys_FPU_EnableExceptions( int exceptions );

// sets the FPU precision
void			Sys_FPU_SetPrecision( int precision );

// sets the FPU rounding mode
void			Sys_FPU_SetRounding( int rounding );

// sets Flush-To-Zero mode (only available when CPUID_FTZ is set)
void			Sys_FPU_SetFTZ( bool enable );

// sets Denormals-Are-Zero mode (only available when CPUID_DAZ is set)
void			Sys_FPU_SetDAZ( bool enable );

// returns amount of system ram
int				Sys_GetSystemRam( void );

// returns amount of video ram
int				Sys_GetVideoRam( void );

// returns amount of drive space in path
int				Sys_GetDriveFreeSpace( const char *path );

// returns memory stats
void			Sys_GetCurrentMemoryStatus( sysMemoryStats_t &stats );
void			Sys_GetExeLaunchMemoryStatus( sysMemoryStats_t &stats );

// lock and unlock memory
bool			Sys_LockMemory( void *ptr, int bytes );
bool			Sys_UnlockMemory( void *ptr, int bytes );

// set amount of physical work memory
void			Sys_SetPhysicalWorkMemory( int minBytes, int maxBytes );

// allows retrieving the call stack at execution points
void			Sys_GetCallStack( address_t *callStack, const int callStackSize );
const char *	Sys_GetCallStackStr( const address_t *callStack, const int callStackSize );
const char *	Sys_GetCallStackCurStr( int depth );
const char *	Sys_GetCallStackCurAddressStr( int depth );
void			Sys_ShutdownSymbols( void );

// DLL loading, the path should be a fully qualified OS path to the DLL file to be loaded
INT_PTR			Sys_DLL_Load( const char *dllName );
void *			Sys_DLL_GetProcAddress(INT_PTR dllHandle, const char *procName );
void			Sys_DLL_Unload(INT_PTR dllHandle );

// event generation
void			Sys_GenerateEvents( void );
sysEvent_t		Sys_GetEvent( void );
void			Sys_ClearEvents( void );

// input is tied to windows, so it needs to be started up and shut down whenever 
// the main window is recreated
void			Sys_InitInput( void );
void			Sys_ShutdownInput( void );
void			Sys_InitScanTable( void );
const unsigned char *Sys_GetScanTable( void );
unsigned char	Sys_GetConsoleKey( bool shifted );
// map a scancode key to a char
// does nothing on win32, as SE_KEY == SE_CHAR there
// on other OSes, consider the keyboard mapping
unsigned char	Sys_MapCharForKey( int key );

// keyboard input polling
int				Sys_PollKeyboardInputEvents( void );
int				Sys_ReturnKeyboardInputEvent( const int n, int &ch, bool &state );
void			Sys_EndKeyboardInputEvents( void );

// mouse input polling
int				Sys_PollMouseInputEvents( void );
int				Sys_ReturnMouseInputEvent( const int n, int &action, int &value );
void			Sys_EndMouseInputEvents( void );

// when the console is down, or the game is about to perform a lengthy
// operation like map loading, the system can release the mouse cursor
// when in windowed mode
void			Sys_GrabMouseCursor( bool grabIt );

void			Sys_ShowWindow( bool show );
bool			Sys_IsWindowVisible( void );
void			Sys_ShowConsole( int visLevel, bool quitOnClose );


void			Sys_Mkdir( const char *path );
ID_TIME_T			Sys_FileTimeStamp( FILE *fp );
// NOTE: do we need to guarantee the same output on all platforms?
const char *	Sys_TimeStampToStr( ID_TIME_T timeStamp );
const char *	Sys_DefaultCDPath( void );
const char *	Sys_DefaultBasePath( void );
const char *	Sys_DefaultSavePath( void );
const char *	Sys_EXEPath( void );

// use fs_debug to verbose Sys_ListFiles
// returns -1 if directory was not found (the list is cleared)
int				Sys_ListFiles( const char *directory, const char *extension, idList<class idStr> &list );

// know early if we are performing a fatal error shutdown so the error message doesn't get lost
void			Sys_SetFatalError( const char *error );

// display perference dialog
void			Sys_DoPreferences( void );

/*
==============================================================

	Networking

==============================================================
*/

typedef enum {
	NA_BAD,					// an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
// jmarshall
	NA_BOT,
// jmarshall end
	NA_IP
} netadrtype_t;

typedef struct {
	netadrtype_t	type;
	unsigned char	ip[4];
	unsigned short	port;
} netadr_t;

/*
==============================================================

	idSys

==============================================================
*/

class idSys {
public:
	virtual void			DebugPrintf( const char *fmt, ... )id_attribute((format(printf,2,3))) = 0;
	virtual void			DebugVPrintf( const char *fmt, va_list arg ) = 0;

	virtual double			GetClockTicks( void ) = 0;
	virtual double			ClockTicksPerSecond( void ) = 0;
	virtual cpuid_t			GetProcessorId( void ) = 0;
	virtual const char *	GetProcessorString( void ) = 0;
	virtual const char *	FPU_GetState( void ) = 0;
	virtual bool			FPU_StackIsEmpty( void ) = 0;
	virtual void			FPU_SetFTZ( bool enable ) = 0;
	virtual void			FPU_SetDAZ( bool enable ) = 0;

	virtual void			FPU_EnableExceptions( int exceptions ) = 0;

	virtual bool			LockMemory( void *ptr, int bytes ) = 0;
	virtual bool			UnlockMemory( void *ptr, int bytes ) = 0;

	virtual void			GetCallStack( address_t *callStack, const int callStackSize ) = 0;
	virtual const char *	GetCallStackStr( const address_t *callStack, const int callStackSize ) = 0;
	virtual const char *	GetCallStackCurStr( int depth ) = 0;
	virtual void			ShutdownSymbols( void ) = 0;

	virtual INT_PTR			DLL_Load( const char *dllName ) = 0;
	virtual void *			DLL_GetProcAddress(INT_PTR dllHandle, const char *procName ) = 0;
	virtual void			DLL_Unload(INT_PTR dllHandle ) = 0;
	virtual void			DLL_GetFileName( const char *baseName, char *dllName, int maxLength ) = 0;

	virtual sysEvent_t		GenerateMouseButtonEvent( int button, bool down ) = 0;
	virtual sysEvent_t		GenerateMouseMoveEvent( int deltax, int deltay ) = 0;

	virtual void			OpenURL( const char *url, bool quit ) = 0;
// jmarshall
	virtual void			StartProcess( const char *exePath, bool quit, const char *startDirectory) = 0;
// jmarshall end
};

extern idSys *				sys;

bool Sys_LoadOpenAL( void );
void Sys_FreeOpenAL( void );

void Sys_SetDedicatedConsoleTitle(const char* txt);

#endif /* !__SYS_PUBLIC__ */
