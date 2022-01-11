#include "pch.h"
#include <stdio.h>
#include <string>
#include <map>

#pragma pack(push, 1)
typedef struct _SharedMemFile
{
	DWORD readerLock;
	DWORD size;
	BYTE  data[4];
} SharedMemFile, *PSharedMemFile;
#pragma pack(pop)

typedef struct _SwapStorage
{
	bool    readerMode;
	int     readerFileIndex;
	HANDLE  hMutex;
	HANDLE  hFileMapping;
	DWORD   size;
	LPBYTE  data;
	LPDWORD pFileIndex;
	PSharedMemFile fileArray[2];
} SwapStorage, *PSwapStorage;


static CRITICAL_SECTION _storagesLck;
static std::map<std::string, PSwapStorage> _mpStorages;


static char* ParseURL(char* url, LPDWORD pSize)
{
	auto name = url;
	auto token = (char*)strstr(name, "://");
	if (token != 0)
	{
		name = token + 3;
	}

	token = (char*)strchr(name, '/');
	if (token == 0)
	{
		return 0;
	}
	(*token) = 0;

	sscanf(name, "%lu", pSize);

	name = token + 1;
	token = (char*)strchr(name, '/');
	if (token == 0)
	{
		return 0;
	}
	(*token) = 0;

	return name;
}

void InitSwapStorages()
{
	::InitializeCriticalSection(&_storagesLck);
}

void LockSwapStorages()
{
	::EnterCriticalSection(&_storagesLck);
}

void UnlockSwapStorages()
{
	::LeaveCriticalSection(&_storagesLck);
}

void DestroySwapStorage(void* handle)
{
	auto storage = (PSwapStorage)handle;
	if (storage != 0)
	{
		if (storage->data != NULL)
		{
			::UnmapViewOfFile(storage->data);
		}
		if (storage->hFileMapping != NULL)
		{
			::CloseHandle(storage->hFileMapping);
		}
		if (storage->hMutex != NULL)
		{
			::CloseHandle(storage->hMutex);
		}
		free(storage);
	}
}

void* CreateSwapStorage(const char* url, bool readerMode)
{
	char tmpStr[MAX_PATH];
	strcpy(tmpStr, url);

	DWORD dataSize = 0;
	auto name = ParseURL(tmpStr, &dataSize);
	if (name == 0)
	{
		return 0;
	}

	PSwapStorage storage = 0;
	LockSwapStorages();
	auto it = _mpStorages.find(name);
	if (it != _mpStorages.end())
	{
		storage = it->second;
	}
	UnlockSwapStorages();
	if (storage != 0)
	{
		return storage;
	}

	storage = new SwapStorage();
	memset(storage, 0, sizeof(SwapStorage));
	storage->size = dataSize;
	storage->readerMode = readerMode;

	char mappingName[MAX_PATH];
	strcpy(mappingName, "Local\\");
	strcat(mappingName, name);
	storage->hFileMapping = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, mappingName);
	if (storage->hFileMapping == NULL)
	{
		DestroySwapStorage(storage);
		return 0;
	}

	strcat(mappingName, ".lck");
	storage->hMutex = ::OpenMutexA(SYNCHRONIZE, FALSE, mappingName);
	if (storage->hMutex == NULL)
	{
		DestroySwapStorage(storage);
		return 0;
	}

	auto fileSize = sizeof(SharedMemFile) + dataSize;
	auto mappingSize = sizeof(DWORD) + (2 * fileSize);

	storage->data = (LPBYTE)::MapViewOfFile(storage->hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, mappingSize);
	if (storage->data == NULL)
	{
		DestroySwapStorage(storage);
		return 0;
	}

	storage->pFileIndex = (LPDWORD)storage->data;
	storage->fileArray[0] = (PSharedMemFile)(storage->pFileIndex + 1);
	storage->fileArray[1] = (PSharedMemFile)((LPBYTE)storage->fileArray[0] + fileSize);

	LockSwapStorages();
	_mpStorages[name] = storage;
	UnlockSwapStorages();

	return storage;
}

static void LockSharedStorage(void* handle)
{
	auto storage = (PSwapStorage)handle;
	::WaitForSingleObject(storage->hMutex, INFINITE);
}

static void UnlockSharedStorage(void* handle)
{
	auto storage = (PSwapStorage)handle;
	::ReleaseMutex(storage->hMutex);
}

DWORD LockSharedMemFileReader(void* handle, bool locked)
{
	DWORD size = 0;
	auto storage = (PSwapStorage)handle;
	if (storage->readerMode)
	{
		LockSharedStorage(storage);
		auto index = (int)(*storage->pFileIndex);
		if (locked)
		{		
			storage->readerFileIndex = ((index + 1) % 2);
		}

		auto file = storage->fileArray[storage->readerFileIndex];
		file->readerLock = (locked ? 1 : 0);
		size = file->size;
		UnlockSharedStorage(storage);
	}
	return size;
}

static int GetCurrentSharedMemFileIndex(void* handle)
{
	int index;
	auto storage = (PSwapStorage)handle;
	if (storage->readerMode)
	{
		index = storage->readerFileIndex;
	}
	else
	{
		index = (int)(*storage->pFileIndex);
	}
	return index;
}

void GetSharedMemFileInfo(void* handle,	int &index, LPBYTE &data, DWORD &maxSize)
{
	auto storage = (PSwapStorage)handle;
	LockSharedStorage(storage);
	index = GetCurrentSharedMemFileIndex(storage);
	data = storage->fileArray[index]->data;
	maxSize = storage->size;
	UnlockSharedStorage(storage);
}

void UpdateSharedMemFileSize(void* handle, DWORD size)
{
	auto storage = (PSwapStorage)handle;
	if (!storage->readerMode)
	{
		LockSharedStorage(storage);
		storage->fileArray[(*storage->pFileIndex)]->size = size;
		UnlockSharedStorage(storage);
	}
}

void RotateSharedMemFile(void* handle)
{
	auto storage = (PSwapStorage)handle;
	if (!storage->readerMode)
	{
		LockSharedStorage(storage);
		auto nextIndex = (((*storage->pFileIndex) + 1) % 2);
		if (!storage->fileArray[nextIndex]->readerLock)
		{
			(*storage->pFileIndex) = nextIndex;
		}
		storage->fileArray[(*storage->pFileIndex)]->size = 0;
		UnlockSharedStorage(storage);
	}
}
