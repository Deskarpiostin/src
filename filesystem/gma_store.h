//========= Copyright ThePixelMoon, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef GMA_STORE_H
#define GMA_STORE_H
#ifdef _WIN32
#pragma once
#endif // _WIN32

#include "vpklib/packedstore.h"
#include "tier1/utldict.h"
#include "basefilesystem.h"

#pragma pack(push, 1)
struct GMAHeader_t
{
	char signature[4];
	uint8 version;
	uint64 steamid;
	uint64 timestamp;
};

struct GMAFileEntry_t
{
	CUtlString name;
	int64 size;
	uint32 crc;
	int64 offset;
};
#pragma pack(pop)

class CGMAStore
{
public:
	CGMAStore(char const *pFileBasename, char *pszFName, IBaseFileSystem *pFS);
	~CGMAStore();

	CPackedStoreFileHandle OpenFile(char const *pFile);
	int ReadData(CPackedStoreFileHandle &handle, void *pOutData, int nNumBytes);

	bool ParseGMAHeader();
	bool IsGMAFile() const { return m_bIsGMA; }
	bool IsEmpty() const { return m_FileEntries.Count() == 0; }

	char const *BaseName() const { return m_pszFileBaseName; }
	char const *FullPathName() const { return m_pszFullPathName; }
	
	void RegisterFileTracker(IThreadedFileMD5Processor *pFileTracker) { m_pFileTracker = pFileTracker; }

	int GetFileList(CUtlStringList &outFilenames, bool bFormattedOutput, bool bSortedOutput);
	int GetFileList(const char *pWildCard, CUtlStringList &outFilenames, bool bFormattedOutput, bool bSortedOutput);

	bool SimpleWildcardMatch(const char *pWildCard, const char *pFileName);
	
	void GetPackFileName(CPackedStoreFileHandle &handle, char *pchFileNameOut, int cchFileNameOut) const;
	
	int m_PackFileID;

	char m_pszFileBaseName[MAX_PATH];
	char m_pszFullPathName[MAX_PATH];
	
	bool m_bIsGMA;
	GMAHeader_t m_Header;
	CUtlVector<GMAFileEntry_t> m_FileEntries;
	CUtlDict<int> m_FileIndexByName;
	
	IBaseFileSystem *m_pFileSystem;
	IThreadedFileMD5Processor *m_pFileTracker;
	CThreadFastMutex m_Mutex;

	bool ReadCString(FileHandle_t hFile, CUtlString &outStr);
	bool ParseFileEntries(FileHandle_t hFile);
	GMAFileEntry_t* FindGMAFileEntry(const char *pFileName);
};

class CGMAStoreRefCount : public CPackedStoreRefCount
{
public:
	CGMAStoreRefCount(char const *pFileBasename, char *pszFName, IBaseFileSystem *pFS);
	virtual ~CGMAStoreRefCount();
	
	virtual CPackedStoreFileHandle OpenFile(char const *pFile) override;
	virtual int ReadData(CPackedStoreFileHandle &handle, void *pOutData, int nNumBytes) override;
	virtual bool IsEmpty() const override;
	virtual int GetFileList(CUtlStringList &outFilenames, bool bFormattedOutput, bool bSortedOutput) override;
	virtual int GetFileList(const char *pWildCard, CUtlStringList &outFilenames, bool bFormattedOutput, bool bSortedOutput) override;

	bool IsGMAFile() const { return m_bIsGMA; }

	bool m_bIsGMA;
	GMAHeader_t m_Header;
	CUtlVector<GMAFileEntry_t> m_FileEntries;
	CUtlDict<int> m_FileIndexByName;
	
	CThreadFastMutex m_GMAMutex;
	
	bool ParseGMAHeader();
	bool ParseFileEntries(FileHandle_t hFile);
	bool ReadCString(FileHandle_t hFile, CUtlString &outStr);
	GMAFileEntry_t* FindGMAFileEntry(const char *pFileName);
	bool WildcardMatch(const char *pWildCard, const char *pString);
};

#endif // GMA_STORE_H