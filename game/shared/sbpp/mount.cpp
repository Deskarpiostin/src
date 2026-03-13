//========== Copyright (C) 2026, Team HL2SB++, All rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//

#include "cbase.h"
#include "mount.h"
#include "filesystem.h"
#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar hl2_mounted( "hl2_mounted", "0", FCVAR_DEVELOPMENTONLY );
ConVar portal_mounted( "portal_mounted", "0", FCVAR_DEVELOPMENTONLY );
ConVar css_mounted( "css_mounted", "0", FCVAR_DEVELOPMENTONLY );
ConVar hl1_mounted( "hl1_mounted", "0", FCVAR_DEVELOPMENTONLY );
ConVar hl2mp_mounted( "hl2mp_mounted", "0", FCVAR_DEVELOPMENTONLY );
ConVar ep2_mounted( "ep2_mounted", "0", FCVAR_DEVELOPMENTONLY );
ConVar episodic_mounted( "episodic_mounted", "0", FCVAR_DEVELOPMENTONLY );
ConVar dod_mounted( "dod_mounted", "0", FCVAR_DEVELOPMENTONLY );

struct GameMount
{
    const char* name;
    ConVar* var;
};

static GameMount mounts[] = { { "hl2", &hl2_mounted }, { "cstrike", &css_mounted }, { "portal", &portal_mounted }, { "hl1", &hl1_mounted }, { "hl2mp", &hl2mp_mounted }, { "ep2", &ep2_mounted }, { "episodic", &episodic_mounted },
	{ "dod", &dod_mounted } };

static void ConcatPaths( char *dest, const char *basePath, const char *fileName, size_t destSize )
{
	snprintf( dest, destSize, "%s/%s", basePath, fileName );
}

static void StripDirFromFileName( char *fileName )
{
	size_t len = strlen( fileName );
	if ( len > 8 && strcmp( fileName + len - 8, "_dir.vpk" ) == 0 )
	{
		fileName[len - 8] = '\0';
		strcat( fileName, ".vpk" );
	}
}

static void AddDirectoryAndVPks( const char *directoryPath )
{
	// Dirty hack to get sourcemods mounting too
	g_pFullFileSystem->AddSearchPath( directoryPath, "GAME" );

	char searchPattern[MAX_PATH];
	Q_snprintf( searchPattern, sizeof( searchPattern ), "%s/*_dir.vpk", directoryPath );

	FileFindHandle_t findHandle;
	const char		*fileName = g_pFullFileSystem->FindFirst( searchPattern, &findHandle );

	if ( fileName )
	{
		do
		{
			char vpkPath[MAX_PATH];
			char modifiedFileName[MAX_PATH];

			Q_strncpy( modifiedFileName, fileName, sizeof( modifiedFileName ) );

			StripDirFromFileName( modifiedFileName );

			ConcatPaths( vpkPath, directoryPath, modifiedFileName, sizeof( vpkPath ) );
			DevMsg( "Adding VPK: %s\n", vpkPath );
			g_pFullFileSystem->AddSearchPath( vpkPath, "GAME" );

		} while ( ( fileName = g_pFullFileSystem->FindNext( findHandle ) ) );

		g_pFullFileSystem->FindClose( findHandle );
	}
	else
	{
		DevWarning( "No .vpk files found in directory: %s\n", directoryPath );
	}
}

void loadMount()
{
	KeyValues *pKeyValues = new KeyValues( "mounts" );
	pKeyValues->LoadFromFile( g_pFullFileSystem, "cfg/mounts.kv", "MOD", true );

	if ( pKeyValues )
	{
		KeyValues *pSubKey = pKeyValues->GetFirstSubKey();
		while ( pSubKey )
		{
			const char *gameName = pSubKey->GetName();
			const char *path = pSubKey->GetString();

			// Folder doesn't exist
			if ( !filesystem->IsDirectory( path, "GAME" ) )
			{
				Warning( "Mount path does not exist: %s\n", path );
				pSubKey = pSubKey->GetNextKey();
				continue;
			}

			for ( const auto &m : mounts )
			{
				if ( !Q_stricmp( gameName, m.name ) )
				{
					m.var->SetValue( 1 );
					break;
				}
			}

			Msg( "Mounting %s from %s\n", gameName, path );

			AddDirectoryAndVPks( path );

			pSubKey = pSubKey->GetNextKey();
		}
	}

	pKeyValues->deleteThis();
	return;
}
