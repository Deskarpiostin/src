//========= Copyright ThePixelMoon, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "gma_store.h"

CGMAStore::CGMAStore(char const *pFileBasename, char *pszFName, IBaseFileSystem *pFS)
	: m_bIsGMA(false)
	, m_pFileSystem(pFS)
	, m_pFileTracker(NULL)
	, m_PackFileID(-1)
{
	char fixedFullPath[MAX_PATH];
	char fixedBaseName[MAX_PATH];

	V_strncpy(fixedFullPath, pszFName, sizeof(fixedFullPath));
	V_strncpy(fixedBaseName, pFileBasename, sizeof(fixedBaseName));

	V_SetExtension(fixedFullPath, "gma", sizeof(fixedFullPath));
	V_SetExtension(fixedBaseName, "gma", sizeof(fixedBaseName));

	V_strncpy(m_pszFileBaseName, fixedBaseName, sizeof(m_pszFileBaseName));
	V_strncpy(m_pszFullPathName, fixedFullPath, sizeof(m_pszFullPathName));

	memset(&m_Header, 0, sizeof(m_Header));

	if (!ParseGMAHeader())
		return;
	
	m_bIsGMA = true;
}

CGMAStore::~CGMAStore()
{
}

bool CGMAStore::ReadCString(FileHandle_t hFile, CUtlString &outStr)
{
	CUtlVector<char> buffer;
	char c;
	
	while (m_pFileSystem->Read(&c, 1, hFile) == 1)
	{
		if (c == '\0')
		{
			buffer.AddToTail('\0');
			outStr = buffer.Base();
			return true;
		}
		buffer.AddToTail(c);
	}
	
	return false;
}

bool CGMAStore::ParseGMAHeader()
{
	FileHandle_t hFile = m_pFileSystem->Open(m_pszFullPathName, "rb");
	if (hFile == FILESYSTEM_INVALID_HANDLE)
		return false;
	
	// signature
	if (m_pFileSystem->Read(&m_Header, sizeof(GMAHeader_t), hFile) != sizeof(GMAHeader_t))
	{
		m_pFileSystem->Close(hFile);
		return false;
	}
	
	// signature
	if (memcmp(m_Header.signature, "GMAD", 4) != 0)
	{
		m_pFileSystem->Close(hFile);
		return false;
	}
	
	// dummy string block
	CUtlString dummyStr;
	while (true)
	{
		if (!ReadCString(hFile, dummyStr))
		{
			m_pFileSystem->Close(hFile);
			return false;
		}
		if (dummyStr.IsEmpty())
			break;
	}
	
	// metadata
	CUtlString name, description, author;
	ReadCString(hFile, name);
	ReadCString(hFile, description);
	ReadCString(hFile, author);
	
	uint32 addonVersion;
	m_pFileSystem->Read(&addonVersion, sizeof(uint32), hFile);
	
	// file entries
	if (!ParseFileEntries(hFile))
	{
		m_pFileSystem->Close(hFile);
		return false;
	}
	
	m_pFileSystem->Close(hFile);
	return true;
}

bool CGMAStore::ParseFileEntries(FileHandle_t hFile)
{
	while (true)
	{
		uint32 index;
		if (m_pFileSystem->Read(&index, sizeof(uint32), hFile) != sizeof(uint32))
			return false;
			
		if (index == 0)
			break;
		
		GMAFileEntry_t entry;
		CUtlString entryName;
		
		if (!ReadCString(hFile, entryName))
			return false;
			
		entry.name = entryName;
		
		if (m_pFileSystem->Read(&entry.size, sizeof(int64), hFile) != sizeof(int64))
			return false;
			
		if (m_pFileSystem->Read(&entry.crc, sizeof(uint32), hFile) != sizeof(uint32))
			return false;
		
		entry.offset = -1; // Will be calculated once all entries are read, ez
		
		int idx = m_FileEntries.AddToTail(entry);
		
		m_FileIndexByName.Insert(entry.name.Get(), idx);
	}
	
	int64 dataOffset = m_pFileSystem->Tell(hFile);
	
	for (int i = 0; i < m_FileEntries.Count(); i++)
	{
		m_FileEntries[i].offset = dataOffset;
		dataOffset += m_FileEntries[i].size;
	}
	
	return true;
}

GMAFileEntry_t* CGMAStore::FindGMAFileEntry(const char *pFileName)
{
	char normalizedPath[MAX_PATH];
	V_strncpy(normalizedPath, pFileName, sizeof(normalizedPath));
	V_FixSlashes(normalizedPath);
	
	int idx = m_FileIndexByName.Find(normalizedPath);
	if (idx == m_FileIndexByName.InvalidIndex())
		return NULL;
		
	int entryIdx = m_FileIndexByName[idx];
	return &m_FileEntries[entryIdx];
}

CPackedStoreFileHandle CGMAStore::OpenFile(char const *pFile)
{
	AUTO_LOCK_FM(m_Mutex);
	
	if (!m_bIsGMA)
		return CPackedStoreFileHandle(); // Invalid handle
	
	GMAFileEntry_t *pEntry = FindGMAFileEntry(pFile);
	if (!pEntry)
		return CPackedStoreFileHandle(); // Invalid handle
	
	CPackedStoreFileHandle handle;
	handle.m_nFileNumber = 0;
	handle.m_nFileOffset = (int)pEntry->offset;
	handle.m_nFileSize = (int)pEntry->size;
	handle.m_nCurrentFileOffset = 0;
	handle.m_pMetaData = NULL;
	handle.m_nMetaDataSize = 0;
	handle.m_pOwner = NULL;
	handle.m_pHeaderData = (CFileHeaderFixedData*)&pEntry->crc;
	handle.m_pDirFileNamePtr = (uint8*)pEntry->name.Get();
	
	return handle;
}

int CGMAStore::ReadData(CPackedStoreFileHandle &handle, void *pOutData, int nNumBytes)
{
	AUTO_LOCK_FM(m_Mutex);
	
	if (!m_bIsGMA)
		return 0;

	FileHandle_t hFile = m_pFileSystem->Open(m_pszFullPathName, "rb");
	if (hFile == FILESYSTEM_INVALID_HANDLE)
		return 0;
	
	int nBytesToRead = MIN(nNumBytes, handle.m_nFileSize - handle.m_nCurrentFileOffset);
	if (nBytesToRead <= 0)
	{
		m_pFileSystem->Close(hFile);
		return 0;
	}
	
	m_pFileSystem->Seek(hFile, handle.m_nFileOffset + handle.m_nCurrentFileOffset, FILESYSTEM_SEEK_HEAD);
	int nBytesRead = m_pFileSystem->Read(pOutData, nBytesToRead, hFile);
	
	handle.m_nCurrentFileOffset += nBytesRead;
	
	m_pFileSystem->Close(hFile);
	
	return nBytesRead;
}

int CGMAStore::GetFileList(CUtlStringList &outFilenames, bool bFormattedOutput, bool bSortedOutput)
{
	for (int i = 0; i < m_FileEntries.Count(); i++)
	{
		outFilenames.CopyAndAddToTail(m_FileEntries[i].name.Get());
	}
	
	if (bSortedOutput)
	{
		outFilenames.Sort();
	}
	
	return outFilenames.Count();
}

int CGMAStore::GetFileList(const char *pWildCard, CUtlStringList &outFilenames, bool bFormattedOutput, bool bSortedOutput)
{
	for (int i = 0; i < m_FileEntries.Count(); i++)
	{
		const char *pFileName = m_FileEntries[i].name.Get();
		
		if (V_strcmp(pWildCard, "*") == 0 || V_strcmp(pWildCard, "*.*") == 0)
		{
			outFilenames.CopyAndAddToTail(pFileName);
		}
		else if (V_stristr(pFileName, pWildCard) != NULL || 
				 SimpleWildcardMatch(pWildCard, pFileName))
		{
			outFilenames.CopyAndAddToTail(pFileName);
		}
	}
	
	if (bSortedOutput)
	{
		outFilenames.Sort();
	}
	
	return outFilenames.Count();
}

bool CGMAStore::SimpleWildcardMatch(const char *pWildCard, const char *pFileName)
{
	if (pWildCard[0] == '*' && pWildCard[1] == '.')
	{
		const char *pExt = V_GetFileExtension(pFileName);
		if (pExt)
		{
			return V_stricmp(pExt, pWildCard + 2) == 0;
		}
		return false;
	}

	size_t len = V_strlen(pWildCard);
	if (len > 0 && pWildCard[len - 1] == '*')
	{
		return V_strnicmp(pFileName, pWildCard, len - 1) == 0;
	}

	return V_stricmp(pWildCard, pFileName) == 0;
}

void CGMAStore::GetPackFileName(CPackedStoreFileHandle &handle, char *pchFileNameOut, int cchFileNameOut) const
{
	V_strncpy(pchFileNameOut, m_pszFullPathName, cchFileNameOut);
}

CGMAStoreRefCount::CGMAStoreRefCount(char const *pFileBasename, char *pszFName, IBaseFileSystem *pFS)
	: CPackedStoreRefCount(pFileBasename, pszFName, pFS)
	, m_bIsGMA(false)
{
	char fixedFullPath[MAX_PATH];
	char fixedBaseName[MAX_PATH];

	V_strncpy(fixedFullPath, pszFName, sizeof(fixedFullPath));
	V_strncpy(fixedBaseName, pFileBasename, sizeof(fixedBaseName));

	V_SetExtension(fixedFullPath, "gma", sizeof(fixedFullPath));
	V_SetExtension(fixedBaseName, "gma", sizeof(fixedBaseName));

	V_strncpy(m_pszFileBaseName, fixedBaseName, sizeof(m_pszFileBaseName));
	V_strncpy(m_pszFullPathName, fixedFullPath, sizeof(m_pszFullPathName));

	if (!ParseGMAHeader())
		return;
	
	m_bIsGMA = true;
	m_bSignatureValid = false; // GMA doesn't use VPK signatures
}

CGMAStoreRefCount::~CGMAStoreRefCount()
{
}

bool CGMAStoreRefCount::IsEmpty() const
{
	if (!m_bIsGMA)
		return CPackedStoreRefCount::IsEmpty();
	
	return m_FileEntries.Count() == 0;
}

CPackedStoreFileHandle CGMAStoreRefCount::OpenFile(char const *pFile)
{
	if (!m_bIsGMA)
		return CPackedStoreRefCount::OpenFile(pFile);
	
	AUTO_LOCK_FM(m_GMAMutex);
	
	GMAFileEntry_t *pEntry = FindGMAFileEntry(pFile);
	if (!pEntry)
		return CPackedStoreFileHandle(); // Invalid handle

	CPackedStoreFileHandle handle;
	handle.m_nFileNumber = 0;
	handle.m_nFileOffset = (int)pEntry->offset;
	handle.m_nFileSize = (int)pEntry->size;
	handle.m_nCurrentFileOffset = 0;
	handle.m_pMetaData = NULL;
	handle.m_nMetaDataSize = 0;
	handle.m_pOwner = this;
	handle.m_pHeaderData = (CFileHeaderFixedData*)&pEntry->crc;
	handle.m_pDirFileNamePtr = (uint8*)pEntry->name.Get();
	
	return handle;
}

int CGMAStoreRefCount::ReadData(CPackedStoreFileHandle &handle, void *pOutData, int nNumBytes)
{
	if (!m_bIsGMA)
		return CPackedStoreRefCount::ReadData(handle, pOutData, nNumBytes);
	
	AUTO_LOCK_FM(m_GMAMutex);
	
	IBaseFileSystem *pFS = m_pFileSystem;
	if (!pFS)
		return 0;
	
	FileHandle_t hFile = pFS->Open(FullPathName(), "rb");
	if (hFile == FILESYSTEM_INVALID_HANDLE)
		return 0;
	
	int nBytesToRead = MIN(nNumBytes, handle.m_nFileSize - handle.m_nCurrentFileOffset);
	if (nBytesToRead <= 0)
	{
		pFS->Close(hFile);
		return 0;
	}
	
	pFS->Seek(hFile, handle.m_nFileOffset + handle.m_nCurrentFileOffset, FILESYSTEM_SEEK_HEAD);
	int nBytesRead = pFS->Read(pOutData, nBytesToRead, hFile);
	
	handle.m_nCurrentFileOffset += nBytesRead;
	
	pFS->Close(hFile);

	return nBytesRead;
}

int CGMAStoreRefCount::GetFileList(CUtlStringList &outFilenames, bool bFormattedOutput, bool bSortedOutput)
{
	if (!m_bIsGMA)
		return CPackedStoreRefCount::GetFileList(outFilenames, bFormattedOutput, bSortedOutput);
	
	for (int i = 0; i < m_FileEntries.Count(); i++)
		outFilenames.CopyAndAddToTail(m_FileEntries[i].name.Get());
	
	if (bSortedOutput)
		outFilenames.Sort();
	
	return outFilenames.Count();
}

int CGMAStoreRefCount::GetFileList(const char *pWildCard, CUtlStringList &outFilenames, bool bFormattedOutput, bool bSortedOutput)
{
	if (!m_bIsGMA)
		return CPackedStoreRefCount::GetFileList(pWildCard, outFilenames, bFormattedOutput, bSortedOutput);
	
	for (int i = 0; i < m_FileEntries.Count(); i++)
	{
		if (WildcardMatch(pWildCard, m_FileEntries[i].name.Get()))
			outFilenames.CopyAndAddToTail(m_FileEntries[i].name.Get());
	}
	
	if (bSortedOutput)
		outFilenames.Sort();
	
	return outFilenames.Count();
}

bool CGMAStoreRefCount::ParseGMAHeader()
{
	IBaseFileSystem *pFS = m_pFileSystem;
	if (!pFS)
		return false;

	FileHandle_t hFile = pFS->Open(FullPathName(), "rb");
	if (hFile == FILESYSTEM_INVALID_HANDLE)
		return false;

	// signature
	int bytesRead = pFS->Read(&m_Header, sizeof(GMAHeader_t), hFile);
	if (bytesRead != sizeof(GMAHeader_t))
	{
		pFS->Close(hFile);
		return false;
	}

	// signature
	if (memcmp(m_Header.signature, "GMAD", 4) != 0)
	{
		pFS->Close(hFile);
		return false;
	}

	CUtlString dummyStr;
	int dummyCount = 0;
	while (true)
	{
		if (!ReadCString(hFile, dummyStr))
		{
			pFS->Close(hFile);
			return false;
		}
		if (dummyStr.IsEmpty())
			break;

		dummyCount++;
	}
	
	// metadata
	CUtlString name, description, author;
	if (!ReadCString(hFile, name))
	{
		pFS->Close(hFile);
		return false;
	}
	if (!ReadCString(hFile, description))
	{
		pFS->Close(hFile);
		return false;
	}
	if (!ReadCString(hFile, author))
	{
		pFS->Close(hFile);
		return false;
	}

	uint32 addonVersion;
	if (pFS->Read(&addonVersion, sizeof(uint32), hFile) != sizeof(uint32))
	{
		pFS->Close(hFile);
		return false;
	}

	if (!ParseFileEntries(hFile))
	{
		pFS->Close(hFile);
		return false;
	}

	pFS->Close(hFile);
	return true;
}

bool CGMAStoreRefCount::ReadCString(FileHandle_t hFile, CUtlString &outStr)
{
	IBaseFileSystem *pFS = m_pFileSystem;
	CUtlVector<char> buffer;
	char c;
	
	while (pFS->Read(&c, 1, hFile) == 1)
	{
		if (c == '\0')
		{
			buffer.AddToTail('\0');
			outStr = buffer.Base();
			return true;
		}
		buffer.AddToTail(c);
	}
	
	return false;
}

bool CGMAStoreRefCount::ParseFileEntries(FileHandle_t hFile)
{
	IBaseFileSystem *pFS = m_pFileSystem;

	while (true)
	{
		uint32 index;
		if (pFS->Read(&index, sizeof(uint32), hFile) != sizeof(uint32))
			return false;
			
		if (index == 0)
			break;
		
		GMAFileEntry_t entry;
		CUtlString entryName;
		
		if (!ReadCString(hFile, entryName))
			return false;
			
		entry.name = entryName;
		
		if (pFS->Read(&entry.size, sizeof(int64), hFile) != sizeof(int64))
			return false;
			
		if (pFS->Read(&entry.crc, sizeof(uint32), hFile) != sizeof(uint32))
			return false;
		
		entry.offset = -1;
		
		int idx = m_FileEntries.AddToTail(entry);
		m_FileIndexByName.Insert(entry.name.Get(), idx);
	}
	
	int64 dataOffset = pFS->Tell(hFile);

	for (int i = 0; i < m_FileEntries.Count(); i++)
	{
		m_FileEntries[i].offset = dataOffset;
		dataOffset += m_FileEntries[i].size;
	}
	
	return true;
}

GMAFileEntry_t* CGMAStoreRefCount::FindGMAFileEntry(const char *pFileName)
{
	char normalizedPath[MAX_PATH];
	V_strncpy(normalizedPath, pFileName, sizeof(normalizedPath));
	V_FixSlashes(normalizedPath);
	
	int idx = m_FileIndexByName.Find(normalizedPath);
	if (idx == m_FileIndexByName.InvalidIndex())
		return NULL;
		
	int entryIdx = m_FileIndexByName[idx];
	return &m_FileEntries[entryIdx];
}

bool CGMAStoreRefCount::WildcardMatch(const char *pWildCard, const char *pString)
{
	const char *w = pWildCard;
	const char *s = pString;
	const char *star = NULL;
	const char *ss = NULL;
	
	while (*s)
	{
		if (*w == '*')
		{
			star = w++;
			ss = s;
		}
							// todo: is there a Q_tolower?
		else if (*w == '?' || _tolower(*w) == _tolower(*s))
		{
			w++;
			s++;
		}
		else if (star)
		{
			w = star + 1;
			s = ++ss;
		}
		else
		{
			return false;
		}
	}
	
	while (*w == '*')
	{
		w++;
	}
	
	return *w == '\0';
}
