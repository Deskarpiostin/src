//========= Copyleft iSquad Software, Some rights reserved.============//
//
// Purpose: not much
//
//=======================================================================//

#include "cbase.h"
#include "proto_version.h"
#include "filesystem.h"
#include "tier0/platform.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

Color clr = Color( 255, 255, 0, 255 );

#ifdef ANDROID
#define OpenGL "OpenGLES"
#else
#define OpenGL "OpenGL"
#endif

const char *GetArch()
{
#if defined( __x86_64__ ) || defined( _M_X64 )
	return "amd64";
#elif defined( __i386__ ) || defined( _X86_ ) || defined( _M_IX86 )
	return "i386";
#elif defined __aarch64__
	return "aarch64";
#elif defined __arm__ || defined _M_ARM
	return "arm";
#elif defined __e2k__ || defined E2K
	return "e2k ( Elbrus )";
#else
	return "Unknown";
#endif
}

const char *GetProtoVersion()
{
#if defined CSTRIKE15 || defined SWARM_DLL
	return "Source Engine (Left 4 Dead Branch)";
#else
	if ( PROTOCOL_VERSION == 25 )
		return "Source Engine 1.17";
	else if ( PROTOCOL_VERSION == 24 )
		return "Source Engine 24 / 2013";
	else if ( PROTOCOL_VERSION == 14 )
		return "Source Engine 12 / 2007";
	else if ( PROTOCOL_VERSION == 2 )
		return "Source Engine 2 / 2003";
	else
		return "What are you even playing on?";
#endif
}

const char *GetPlatform()
{
#ifdef LINUX
	return "Linux";
#elif ANDROID
	return "Android";
#elif PLATFORM_FBSD
	return "FreeBSD";
#elif PLATFORM_BSD
	return "*BSD";
#elif WIN32
	return "Windows";
#elif OSX
	return "OSX";
#elif PLATFORM_HAIKU
	return "Haiku";
#else
	return "Unknown Platform"
#endif
}

const char *GetGame()
{
	const char *var;
	KeyValues  *kv = new KeyValues( "KeyValues" );
	kv->LoadFromFile( g_pFullFileSystem, "gameinfo.txt" );

	if ( kv )
	{
		for ( KeyValues *control = kv->GetFirstSubKey(); control != NULL; control = control->GetNextKey() )
		{
			if ( !Q_strcasecmp( control->GetName(), "game" ) )
			{
				var = control->GetString();
			}
		}
	}
	return var;
}

const char *GetArt( const char *game )
{
	return "f//////////////////}W@@@@@ci/ffjtjrrjt\n"
		   "Xnjt///////////////} Q)@B@j'|rtrrfjrrf\n"
		   "_-jcrf//////////////'b)]@@z '-|tjjjjrn\n"
		   "){]_/znjt|((|////-    ]};.r>` .tfjxffx\n"
		   "(()1[+_+jh@@MJ]l>lZ@@@    @@@@ ft/jrjj\n"
		   "(((((~]@@    .p@a' @@@@@@@@YJ; |tftjrx\n"
		   "(((([l@  /@@@B.Io,]z '    p  @[{(/jt/r\n"
		   "((()'h@ @W`.`!|nj-Z@ @Zl]`@@ @{1/((rft\n"
		   "((()'@  @.    [-+)l@ @@@@@@  @^[//()/f\n"
		   "((()'@  @@p@@ `]1] @        w@@c]//|1t\n"
		   ")(((<<@B    @@@I^[ $  @@@@@@   @_1//|(\n"
		   "))(()+;t@@@    @o; @@@c   @@|@ @*]///|\n"
		   "()((([>   z@@@  @# @  @@@>: .@ @#[////\n"
		   "11)([ +   1@. @  @ @  @   ')@  @~xnjt/\n"
		   "/1)([X@@@k@W@@@  @ @  @a@@@^  @B,+{jzv\n"
		   "||{)[.~@@      @@>.@       j@@[,[){]+-\n"
		   "/|/))-' [@@@@@@1![+X@@@@@@@/lI[)((((){\n"
		   "(||)1))]i'. '^~[)(1-:`.'`I-1((((((((((\n"
		   "//||())(((((((((((((((((((((((((((((((\n";
}

CON_COMMAND_F( sourcefetch, "Print info about engine", FCVAR_NONE )
{
	//Use a monospaced fonts for this ASCII art!!!!!
	ConColorMsg( clr, GetArt( GetGame() ) );
	Msg( "Half-Life 2: Sandbox++\n" );
	Msg( "----------\n" );
	Msg( "Engine Version: %s\n", GetProtoVersion() );
	Msg( "Platform: %s\n", GetPlatform() );
	Msg( "Arch: %s\n", GetArch() );
	Msg( "Game: %s\n", GetGame() );
#ifdef LUA_SDK
	Msg( "Lua Version: %s\n", LUA_RELEASE );
#endif

	if ( IsPlatformOpenGL() )
		Msg( "Renderer: %s\n", OpenGL );
	else
		Msg( "Renderer Direct3D\n" );

#ifdef MAPBASE
	Msg( "Mapbase: %s\n", MAPBASE_VERSION );
#endif
}