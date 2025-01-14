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

#ifndef __COMMON_H__
#define __COMMON_H__

/*
==============================================================

  Common

==============================================================
*/

#define MAX_ASYNC_CLIENTS		8
struct usercmd_t;

typedef enum {
	EDITOR_NONE					= 0,
	EDITOR_RADIANT				= BIT(1),
	EDITOR_GUI					= BIT(2),
	EDITOR_DEBUGGER				= BIT(3),
	EDITOR_SCRIPT				= BIT(4),
	EDITOR_LIGHT				= BIT(5),
	EDITOR_SOUND				= BIT(6),
	EDITOR_DECL					= BIT(7),
	EDITOR_AF					= BIT(8),
	EDITOR_PARTICLE				= BIT(9),
	EDITOR_AAS					= BIT(10),
	EDITOR_MATERIAL				= BIT(11)
} toolFlag_t;

#define STRTABLE_ID				"#str_"
#define STRTABLE_ID_LENGTH		5

// jmarshall
class rvmScopedLoadContext {
public:
	rvmScopedLoadContext();
	~rvmScopedLoadContext();
private:
	bool contextSwitched;
};
// jmarshall end

extern idSysMutex	com_loadScreenMutex;

extern idCVar		com_version;
extern idCVar		com_skipRenderer;
extern idCVar		com_asyncInput;
extern idCVar		com_asyncSound;
extern idCVar		com_machineSpec;
extern idCVar		com_purgeAll;
extern idCVar		com_developer;
extern idCVar		com_allowConsole;
extern idCVar		com_speeds;
extern idCVar		com_showFPS;
extern idCVar		com_showMemoryUsage;
extern idCVar		com_showAsyncStats;
extern idCVar		com_showSoundDecoders;
extern idCVar		com_makingBuild;
extern idCVar		com_updateLoadSize;
extern idCVar		com_videoRam;

extern int			time_gameFrame;			// game logic time
extern int			time_gameDraw;			// game present time

extern int			com_frameTime;			// time for the current frame in milliseconds
extern volatile int	com_ticNumber;			// 60 hz tics, incremented by async function
extern int			com_editors;			// current active editor(s)

#ifdef _WIN32
const char			DMAP_MSGID[] = "DMAPOutput";
const char			DMAP_DONE[] = "DMAPDone";
extern HWND			com_hwndMsg;
extern bool			com_outputMsg;
#endif

struct MemInfo_t {
	idStr			filebase;

	int				total;
	int				assetTotals;

	// memory manager totals
	int				memoryManagerTotal;

	// subsystem totals
	int				gameSubsystemTotal;
	int				renderSubsystemTotal;

	// asset totals
	int				imageAssetsTotal;
	int				modelAssetsTotal;
	int				soundAssetsTotal;
};

class idCommon {
public:
	virtual						~idCommon( void ) {}

								// Initialize everything.
								// if the OS allows, pass argc/argv directly (without executable name)
								// otherwise pass the command line in a single string (without executable name)
	virtual void				Init( int argc, const char **argv, const char *cmdline ) = 0;

								// Shuts down everything.
	virtual void				Shutdown( void ) = 0;

								// Returns true if we are in production mode.
	virtual bool				InProductionMode(void) = 0;

								// Compresses a block of memory.
	virtual void				Compress(byte *uncompressedMem, byte *compressedMem, int length, int &compressedLength) = 0;

								// Decompress a block of memory.
	virtual void				Decompress(byte *compressedMem, byte *uncompressedMem, int compressedMemLength, int outputMemLength) = 0;

								// Shuts down everything.
	virtual void				Quit( void ) = 0;

								// Returns true if common initialization is complete.
	virtual bool				IsInitialized( void ) const = 0;

								// Called repeatedly as the foreground thread for rendering and game logic.
	virtual void				Frame( void ) = 0;

								// Called repeatedly by blocking function calls with GUI interactivity.
	virtual void				GUIFrame( bool execCmd, bool network ) = 0;

								// Checks for and removes command line "+set var arg" constructs.
								// If match is NULL, all set commands will be executed, otherwise
								// only a set with the exact name.  Only used during startup.
								// set once to clear the cvar from +set for early init code
	virtual void				StartupVariable( const char *match, bool once ) = 0;

								// Initializes a tool with the given dictionary.
	virtual void				InitTool( const toolFlag_t tool, const idDict *dict ) = 0;

								// Activates or deactivates a tool.
	virtual void				ActivateTool( bool active ) = 0;

								// Writes the user's configuration to a file
	virtual void				WriteConfigToFile( const char *filename ) = 0;

								// Writes cvars with the given flags to a file.
	virtual void				WriteFlaggedCVarsToFile( const char *filename, int flags, const char *setCmd ) = 0;

								// Begins redirection of console output to the given buffer.
	virtual void				BeginRedirect( char *buffer, int buffersize, void (*flush)( const char * ) ) = 0;

								// Stops redirection of console output.
	virtual void				EndRedirect( void ) = 0;

								// Update the screen with every message printed.
	virtual void				SetRefreshOnPrint( bool set ) = 0;

								// Prints message to the console, which may cause a screen update if com_refreshOnPrint is set.
	virtual void				Printf( const char *fmt, ... )id_attribute((format(printf,2,3))) = 0;

								// Same as Printf, with a more usable API - Printf pipes to this.
	virtual void				VPrintf( const char *fmt, va_list arg ) = 0;

								// Prints message that only shows up if the "developer" cvar is set,
								// and NEVER forces a screen update, which could cause reentrancy problems.
	virtual void				DPrintf( const char *fmt, ... ) id_attribute((format(printf,2,3))) = 0;

								// Prints WARNING %s message and adds the warning message to a queue for printing later on.
	virtual void				Warning( const char *fmt, ... ) id_attribute((format(printf,2,3))) = 0;

								// Prints WARNING %s message in yellow that only shows up if the "developer" cvar is set.
	virtual void				DWarning( const char *fmt, ...) id_attribute((format(printf,2,3))) = 0;

								// Prints all queued warnings.
	virtual void				PrintWarnings( void ) = 0;

								// Removes all queued warnings.
	virtual void				ClearWarnings( const char *reason ) = 0;

								// Issues a C++ throw. Normal errors just abort to the game loop,
								// which is appropriate for media or dynamic logic errors.
	virtual void				Error( const char *fmt, ... ) id_attribute((format(printf,2,3))) = 0;

								// Fatal errors quit all the way to a system dialog box, which is appropriate for
								// static internal errors or cases where the system may be corrupted.
	virtual void				FatalError( const char *fmt, ... ) id_attribute((format(printf,2,3))) = 0;

								// Returns a pointer to the dictionary with language specific strings.
	virtual const idLangDict *	GetLanguageDict( void ) = 0;

								// Returns key bound to the command
	virtual const char *		KeysFromBinding( const char *bind ) = 0;

								// Returns the binding bound to the key
	virtual const char *		BindingFromKey( const char *key ) = 0; 

								// Directly sample a button.
	virtual int					ButtonState( int key ) = 0;

								// Directly sample a keystate.
	virtual int					KeyState( int key ) = 0;

								// Returns true if a editor is running.
	virtual bool				IsEditorRunning(void) = 0;

								// Returns true if the game is running in editor mode.
	virtual bool				IsEditorGameRunning(void) = 0;

								// Sets editor running.
	virtual void				SetEditorRunning(bool isRunning) = 0;

								// Gets the editor game window dinemsions.
	virtual void				GetEditorGameWindow(int& width, int& height) = 0;

								// Returns true if we are running as a dedicated server.
								// This means we are headless.
	virtual bool				IsDedicatedServer(void) = 0;

								// Begins the load screen.
	virtual void				BeginLoadScreen(void) = 0;

								// Ends the load screen.
	virtual void				EndLoadScreen(void) = 0;

								// Allocates a client slot for a bot.
	virtual int					AllocateClientSlotForBot(int maxPlayersOnServer) = 0;

								// Sets the current bot user command.
	virtual int					ServerSetBotUserCommand(int clientNum, int frameNum, const usercmd_t& cmd) = 0;

								// Sets the bot user name.
	virtual int					ServerSetBotUserName(int clientNum, const char* playerName) = 0;

								// Sends a reliable message to this client or all the clients if passed -1.
	virtual void				ServerSendReliableMessage(int clientNum, const idBitMsg& msg) = 0;

								// Sends a message to all clients except for clientNum
	virtual void				ServerSendReliableMessageExcluding(int clientNum, const idBitMsg& msg) = 0;

								// Sends a reliable message to the server.
	virtual void				ClientSendReliableMessage(const idBitMsg& msg) = 0;

								// On the client, returns the last time since the last packet.
	virtual int					ClientGetTimeSinceLastPacket(void) = 0;

								// Returns true if we are the local client.
	virtual bool				IsClient(void) = 0;

								// Returns true if we are the server.
	virtual bool				IsServer(void) = 0;

								// Returns true if this is a multiplayer game.
	virtual bool				IsMultiplayer(void) = 0;

								// Disconnects from the server.
	virtual void				DisconnectFromServer(void) = 0;

								// Kills the server.
	virtual void				KillServer(void) = 0;

								// Returns the last time we recieved a packet from this client.
	virtual int					ServerGetClientTimeSinceLastPacket(int clientNum) = 0;

								// Returns the ping of the requested client.
	virtual int					ServerGetClientPing(int clientNum) = 0;

								// Returns the local client number.
	virtual int					GetLocalClientNum(void) = 0;

	virtual float				Get_com_engineHz_latched(void) = 0;
	virtual int64_t				Get_com_engineHz_numerator(void ) = 0;
	virtual int64_t				Get_com_engineHz_denominator(void) = 0;

	virtual void				SetUserInfoKey(int clientNum, const char* key, const char* value) = 0;
	virtual void				BindUserInfo(int clientNum) = 0;

	virtual void				ExecuteClientMapChange(const char* mapName, const char* gameType) = 0;

	virtual int					GetGameFrame(void) = 0;

	virtual void				ServerSetUserCmdForClient(int clientNum, struct usercmd_t& cmd) = 0;
};



//
// rvmNetworkPacket
//
class rvmNetworkPacket {
public:
	rvmNetworkPacket(int clientNum, int maxMessageSize = 256);
	~rvmNetworkPacket();

	idBitMsg	msg;
private:
	byte*		net_buffer;
};

/*
==================
rvmNetworkPacket::rvmNetworkPacket
==================
*/
ID_INLINE rvmNetworkPacket::rvmNetworkPacket(int clientNum, int maxMessageSize) {
	net_buffer = new byte[maxMessageSize];

	msg.Init(net_buffer, maxMessageSize);
	msg.WriteUShort(clientNum);
}

/*
==================
rvmNetworkPacket::~rvmNetworkPacket
==================
*/
ID_INLINE rvmNetworkPacket::~rvmNetworkPacket() {
	if(net_buffer) {
		delete net_buffer;
		net_buffer = NULL;
	}
}

extern idCommon* common;

// Returns the msec the frame starts on
ID_INLINE int FRAME_TO_MSEC(int64_t frame) {
	return (int)((frame * common->Get_com_engineHz_numerator()) / common->Get_com_engineHz_denominator());
}
// Rounds DOWN to the nearest frame
ID_INLINE int MSEC_TO_FRAME_FLOOR(int msec) {
	return (int)((((int64_t)msec * common->Get_com_engineHz_denominator()) + (common->Get_com_engineHz_denominator() - 1)) / common->Get_com_engineHz_numerator());
}
// Rounds UP to the nearest frame
ID_INLINE int MSEC_TO_FRAME_CEIL(int msec) {
	return (int)((((int64_t)msec * common->Get_com_engineHz_denominator()) + (common->Get_com_engineHz_numerator() - 1)) / common->Get_com_engineHz_numerator());
}
// Aligns msec so it starts on a frame bondary
ID_INLINE int MSEC_ALIGN_TO_FRAME(int msec) {
	return FRAME_TO_MSEC(MSEC_TO_FRAME_CEIL(msec));
}

#endif /* !__COMMON_H__ */
