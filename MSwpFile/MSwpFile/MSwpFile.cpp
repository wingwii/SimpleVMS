#include "pch.h"
#include <malloc.h>
#include <stdio.h>

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define FFERRTAG(a, b, c, d) (-(int)MKTAG(a, b, c, d))
#define AVERROR_EOF FFERRTAG( 'E','O','F',' ')

typedef struct _MSWPFILE
{
	LPVOID  storage;
	DWORD   maxSize;
	DWORD   size;
	LPBYTE  data;
	__int64 pos;
} MSWPFILE, *PMSWPFILE;

static HANDLE _stdinHandle = 0;
static BYTE _dummyStdinBuf[8];

void* CreateSwapStorage(const char*, bool);
void RotateSharedMemFile(void*);
DWORD LockSharedMemFileReader(void*, bool locked);
void GetSharedMemFileInfo(void*, int& index, LPBYTE& data, DWORD& maxSize);
void UpdateSharedMemFileSize(void*, DWORD size);

void InitMemSwapEngine()
{
	_stdinHandle = ::GetStdHandle(STD_INPUT_HANDLE);
}

VOID WINAPI CloseMemSwapFile(LPVOID handle)
{
	auto ctx = (PMSWPFILE)handle;
	if (ctx != 0)
	{
		auto storage = ctx->storage;
		free(ctx);

		LockSharedMemFileReader(storage, false);
		RotateSharedMemFile(storage);
	}
}

LPVOID WINAPI OpenMemSwapFile(LPCSTR name, INT flags, INT* pfd)
{
	auto readerMode = ((flags & 1) == 1);
	auto storage = CreateSwapStorage(name, readerMode);
	if (storage == 0)
	{
		return 0;
	}

	auto ctx = new MSWPFILE();
	memset(ctx, 0, sizeof(MSWPFILE));
	ctx->storage = storage;

	if (readerMode)
	{
		ctx->size = LockSharedMemFileReader(storage, true);
	}

	int fd = -1;
	GetSharedMemFileInfo(storage, fd, ctx->data, ctx->maxSize);
	if (readerMode)
	{
		fd += 21;
	}
	else
	{
		fd += 11;
	}

	printf("FD: %d\n", fd);
	if (pfd != 0)
	{
		(*pfd) = fd;
	}
	return (LPVOID)ctx;
}

INT WINAPI SeekMemSwapFile(LPVOID handle, INT whence, LONGLONG pos, PLONGLONG pNewPos)
{
	auto ctx = (PMSWPFILE)handle;
	//printf("FSeek %d %ld %d %lu\n", whence, (long)pos, (int)ctx->pos, ctx->size);
	if (SEEK_SET == whence)
	{
		ctx->pos = pos;
	}
	else if (SEEK_CUR == whence)
	{
		ctx->pos += pos;
	}
	else if (SEEK_END == whence)
	{
		ctx->pos = ctx->size;
	}
	if (pNewPos != 0)
	{
		(*pNewPos) = ctx->pos;
	}
	return 0;
}

INT WINAPI WriteMemSwapFile(LPVOID handle, LPBYTE buf, INT size)
{
	auto ctx = (PMSWPFILE)handle;
	auto pos = (DWORD)ctx->pos;
	auto newPos = pos + size;
	if (ctx->maxSize >= newPos)
	{
		memcpy(ctx->data + pos, buf, size);
		ctx->pos = newPos;
		if (newPos > ctx->size)
		{
			ctx->size = newPos;
			UpdateSharedMemFileSize(ctx->storage, newPos);
		}
	}

	DWORD byteRead = 0;
	if (!PeekNamedPipe(_stdinHandle, _dummyStdinBuf, 1, &byteRead, NULL, NULL))
	{
		ExitProcess(0);
	}

	return size;
}

INT WINAPI ReadMemSwapFile(LPVOID handle, LPBYTE buf, INT size)
{
	auto ctx = (PMSWPFILE)handle;
	auto pos = (int)ctx->pos;
	int actualSize = 0;
	if (ctx->size > pos)
	{
		actualSize = (int)(ctx->size - pos);
	}
	if (actualSize > size)
	{
		actualSize = size;
	}
	memcpy(buf, ctx->data + pos, actualSize);
	ctx->pos += actualSize;
	if (actualSize == 0)
	{
		actualSize = AVERROR_EOF;
	}
	return actualSize;
}
