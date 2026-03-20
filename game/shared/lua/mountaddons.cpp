//========== Copyleft © 2011, Team Sandbox, Some rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "luamanager.h"
#include "tier0/icommandline.h"
#include "zip/miniz.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static void ExtractZIP( const char *zipPath, const char *cacheBase, const char *gamePath )
{
	if ( !zipPath || !cacheBase )
		return;

	const char *baseNamePtr = V_UnqualifiedFileName( zipPath ); // "popcorn_swep.zip"
	char		baseName[MAX_PATH];
	Q_strncpy( baseName, baseNamePtr ? baseNamePtr : zipPath, sizeof( baseName ) );

	char addonName[MAX_PATH];
	Q_strncpy( addonName, baseName, sizeof( addonName ) );
	char *dot = Q_strrchr( addonName, '.' );
	if ( dot )
		*dot = '\0';

	char relOutBase[MAX_PATH];
	Q_snprintf( relOutBase, sizeof( relOutBase ), "%s/zips/%s", cacheBase, addonName );
	Q_FixSlashes( relOutBase );

	filesystem->CreateDirHierarchy( relOutBase, "MOD" );

	mz_zip_archive zip;
	memset( &zip, 0, sizeof( zip ) );
	if ( !mz_zip_reader_init_file( &zip, zipPath, 0 ) )
	{
		Warning( "Failed to open ZIP: %s\n", zipPath );
		return;
	}

	int numFiles = (int)mz_zip_reader_get_num_files( &zip );
	for ( int i = 0; i < numFiles; ++i )
	{
		mz_zip_archive_file_stat fileStat;
		if ( !mz_zip_reader_file_stat( &zip, i, &fileStat ) )
			continue;

		if ( mz_zip_reader_is_file_a_directory( &zip, i ) )
			continue;

		const char *fileName = fileStat.m_filename;
		const char *firstSlash = strchr( fileName, '/' );
		if ( firstSlash )
		{
			fileName = firstSlash + 1;
		}

		if ( !fileName || fileName[0] == '\0' )
		{
			DevMsg( "Skipping empty path from: %s\n", fileStat.m_filename );
			continue;
		}

		DevMsg( "Extracting: %s -> %s/%s\n", fileStat.m_filename, relOutBase, fileName );

		char relOutPath[MAX_PATH];
		Q_snprintf( relOutPath, sizeof( relOutPath ), "%s/%s", relOutBase, fileName );
		Q_FixSlashes( relOutPath );

		char dirBuf[MAX_PATH];
		Q_strncpy( dirBuf, relOutPath, sizeof( dirBuf ) );
		char *lastSlash = Q_strrchr( dirBuf, '/' );
		if ( lastSlash )
		{
			*lastSlash = '\0'; // keep directory only
			filesystem->CreateDirHierarchy( dirBuf, "MOD" );
		}
		else
			filesystem->CreateDirHierarchy( relOutBase, "MOD" );

		size_t uncomp_size = 0;
		void  *p = mz_zip_reader_extract_to_heap( &zip, i, &uncomp_size, 0 );
		if ( !p )
		{
			Warning( "Failed to extract (to heap) %s from ZIP %s\n", fileStat.m_filename, zipPath );
			continue;
		}

		FileHandle_t fh = g_pFullFileSystem->Open( relOutPath, "wb", "MOD" );
		if ( fh == FILESYSTEM_INVALID_HANDLE )
		{
			Warning( "Failed to open output '%s' for writing in Source FS\n", relOutPath );
			mz_free( p );
			continue;
		}

		if ( uncomp_size > INT_MAX )
		{
			Warning( "File too large to write via Source FS: %s (size=%zu)\n", relOutPath, uncomp_size );
			g_pFullFileSystem->Close( fh );
			mz_free( p );
			continue;
		}

		int written = g_pFullFileSystem->Write( p, (int)uncomp_size, fh );
		if ( written != (int)uncomp_size )
		{
			Warning( "Write mismatch for %s (wrote %d, expected %zu)\n", relOutPath, written, uncomp_size );
		}
		g_pFullFileSystem->Close( fh );

		// free buffer from miniz
		mz_free( p );
	}

	mz_zip_reader_end( &zip );

	char absOutBase[MAX_PATH];
	Q_snprintf( absOutBase, sizeof( absOutBase ), "%s/%s", gamePath, relOutBase );
	Q_FixSlashes( absOutBase );

	filesystem->AddSearchPath( absOutBase, "MOD", PATH_ADD_TO_HEAD );
	filesystem->AddSearchPath( absOutBase, "GAME", PATH_ADD_TO_HEAD );

	DevMsg( "Mounted ZIP addon: %s -> %s\n", zipPath, absOutBase );
}

static void MountAddonFolder( const char *folder, const char *gamePath )
{
	FileFindHandle_t fh;
	char			 searchPath[MAX_PATH];
	Q_snprintf( searchPath, sizeof( searchPath ), "%s/*", folder );

	const char *fn = g_pFullFileSystem->FindFirstEx( searchPath, "MOD", &fh );
	while ( fn )
	{
		if ( fn[0] != '.' )
		{
			char fullPath[MAX_PATH];
			Q_snprintf( fullPath, sizeof( fullPath ), "%s/%s/%s", gamePath, folder, fn );
			Q_FixSlashes( fullPath );

			if ( g_pFullFileSystem->FindIsDirectory( fh ) )
			{
				DevMsg( "Mounting %s dir: %s\n", folder, fullPath );
				filesystem->AddSearchPath( fullPath, "MOD", PATH_ADD_TO_HEAD );
				filesystem->AddSearchPath( fullPath, "GAME", PATH_ADD_TO_HEAD );
			}
			else if ( Q_stristr( fn, ".vpk" ) )
			{
				DevMsg( "Mounting %s VPK: %s\n", folder, fullPath );
				filesystem->AddSearchPath( fullPath, "MOD", PATH_ADD_TO_HEAD );
				filesystem->AddSearchPath( fullPath, "GAME", PATH_ADD_TO_HEAD );
			}
			else if ( Q_stristr( fn, ".gma" ) )
			{
				DevMsg( "Mounting %s GMA: %s\n", folder, fullPath );
				filesystem->AddSearchPath( fullPath, "MOD", PATH_ADD_TO_HEAD );
				filesystem->AddSearchPath( fullPath, "GAME", PATH_ADD_TO_HEAD );
			}
			else if ( Q_stristr( fn, ".zip" ) )
			{
				DevMsg( "Mounting %s ZIP: %s\n", folder, fullPath );
				ExtractZIP( fullPath, "cache", gamePath );
			}
		}

		fn = g_pFullFileSystem->FindNext( fh );
	}

	g_pFullFileSystem->FindClose( fh );
}

void MountAddons()
{
	if ( CommandLine()->CheckParm( "-noaddons" ) )
	{
		DevMsg( "Addons mounting skipped due to -noaddons parameter.\n" );
		return;
	}

	char gamePath[MAX_PATH] = { 0 };
#ifdef CLIENT_DLL
	Q_strncpy( gamePath, engine->GetGameDirectory(), sizeof( gamePath ) );
#else
	engine->GetGameDir( gamePath, sizeof( gamePath ) );
#endif

	// NOO! I DON'T WANT TO DIE!
	filesystem->CreateDirHierarchy( "cache/zips", "MOD" );

	filesystem->AddSearchPath( LUA_PATH_CACHE, "MOD", PATH_ADD_TO_HEAD );
	filesystem->AddSearchPath( LUA_PATH_CACHE, "GAME", PATH_ADD_TO_HEAD );

	MountAddonFolder( "addons", gamePath );
	MountAddonFolder( "custom", gamePath );
	MountAddonFolder( "mods", gamePath );
}

extern void lcf_recursivedeletefile( const char *current );

void UnMountAddons()
{
	lcf_recursivedeletefile( "cache/zips" );
}