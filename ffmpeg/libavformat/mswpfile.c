#include "config.h"
#if CONFIG_MSWPFILE_PROTOCOL
#include "internal.h"
#include "libavutil/avstring.h"
#include "libavutil/internal.h"
#include "libavutil/opt.h"
#include "avformat.h"
#include "url.h"
#ifdef _WIN32
#include <windows.h>

typedef struct FileContext {
	const AVClass *class;
	void* handle;
	int fd;
} FileContext;

typedef void* (WINAPI * FN_OPEN_MSWPFILE)(const char*, int, int*);
typedef int (WINAPI * FN_READWRITE_MSWPFILE)(void*, void*, int);
typedef int (WINAPI * FN_SEEK_MSWPFILE)(void*, int, int64_t, int64_t*);
typedef void (WINAPI * FN_CLOSE_MSWPFILE)(void*);


static void* _mswpfileModuleHandle = 0;
static FN_OPEN_MSWPFILE _fnOpenMSwpFile = 0;
static FN_READWRITE_MSWPFILE _fnReadMSwpFile = 0;
static FN_READWRITE_MSWPFILE _fnWriteMSwpFile = 0;
static FN_SEEK_MSWPFILE _fnSeekMSwapFile = 0;
static FN_CLOSE_MSWPFILE _fnCloseMSwapFile = 0;


static void InitMemSwapFileModule(void)
{
	void* hmod;
	if (_mswpfileModuleHandle != 0)
	{
		return;
	}

	_mswpfileModuleHandle = (void*)LoadLibraryA("mswpfile.dll");
	hmod = _mswpfileModuleHandle;
	
	_fnOpenMSwpFile  = (FN_OPEN_MSWPFILE)GetProcAddress(hmod, "OpenMemSwapFile");
	_fnReadMSwpFile  = (FN_READWRITE_MSWPFILE)GetProcAddress(hmod, "ReadMemSwapFile");
	_fnWriteMSwpFile = (FN_READWRITE_MSWPFILE)GetProcAddress(hmod, "WriteMemSwapFile");
	_fnSeekMSwapFile = (FN_SEEK_MSWPFILE)GetProcAddress(hmod, "SeekMemSwapFile");
	_fnCloseMSwapFile = (FN_CLOSE_MSWPFILE)GetProcAddress(hmod, "CloseMemSwapFile");
}

static int file_open(URLContext *h, const char *filename, int flags)
{
	FileContext *ctx = h->priv_data;
	int fd = -1;

	InitMemSwapFileModule();
	
	ctx->handle = _fnOpenMSwpFile(filename, flags, &fd);
	ctx->fd = fd;
	return 0;
}

static int file_read(URLContext *h, unsigned char *buf, int size)
{
	FileContext *ctx = h->priv_data;
	void* handle = ctx->handle;
	int result = _fnReadMSwpFile(handle, buf, size);
	return result;
}

static int file_write(URLContext *h, const unsigned char *buf, int size)
{
	FileContext *ctx = h->priv_data;
	void* handle = ctx->handle;
	int result = _fnWriteMSwpFile(handle, (void*)buf, size);
	return result;
}

static int64_t file_seek(URLContext *h, int64_t pos, int whence)
{
	FileContext *ctx = h->priv_data;
	void* handle = ctx->handle;
	int64_t result = 0;
	_fnSeekMSwapFile(handle, whence, pos, &result);
	return result;
}

static int file_close(URLContext *h)
{
	FileContext *ctx = h->priv_data;
	void* handle = ctx->handle;
	_fnCloseMSwapFile(handle);
	return 0;
}

static int file_get_handle(URLContext *h)
{
    FileContext *c = h->priv_data;
    return c->fd;
}

static int file_check(URLContext *h, int mask)
{
	return 1;
}

static int file_delete(URLContext *h)
{
	return AVERROR(ENOSYS);
}

static int file_move(URLContext *h_src, URLContext *h_dst)
{
	return AVERROR(ENOSYS);
}

static int file_open_dir(URLContext *h)
{
	return AVERROR(ENOSYS);
}

static int file_read_dir(URLContext *h, AVIODirEntry **next)
{
	return AVERROR(ENOSYS);
}

static int file_close_dir(URLContext *h)
{
	return AVERROR(ENOSYS);
}

static const AVOption file_options[] = {
    //{ "truncate", "truncate existing files on write", offsetof(FileContext, trunc), AV_OPT_TYPE_BOOL, { .i64 = 1 }, 0, 1, AV_OPT_FLAG_ENCODING_PARAM },
    //{ "blocksize", "set I/O operation maximum block size", offsetof(FileContext, blocksize), AV_OPT_TYPE_INT, { .i64 = INT_MAX }, 1, INT_MAX, AV_OPT_FLAG_ENCODING_PARAM },
    //{ "follow", "Follow a file as it is being written", offsetof(FileContext, follow), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, AV_OPT_FLAG_DECODING_PARAM },
    //{ "seekable", "Sets if the file is seekable", offsetof(FileContext, seekable), AV_OPT_TYPE_INT, { .i64 = -1 }, -1, 0, AV_OPT_FLAG_DECODING_PARAM | AV_OPT_FLAG_ENCODING_PARAM },
    { NULL }
};

static const AVClass file_class = {
    .class_name = "mswpfile",
    .item_name  = av_default_item_name,
    .option     = file_options,
    .version    = LIBAVUTIL_VERSION_INT,
};

const URLProtocol ff_mswpfile_protocol = {
    .name                = "mswpfile",
    .url_open            = file_open,
    .url_read            = file_read,
    .url_write           = file_write,
    .url_seek            = file_seek,
    .url_close           = file_close,
    .url_get_file_handle = file_get_handle,
    .url_check           = file_check,
    .url_delete          = file_delete,
    .url_move            = file_move,
    .priv_data_size      = sizeof(FileContext),
    .priv_data_class     = &file_class,
    .url_open_dir        = file_open_dir,
    .url_read_dir        = file_read_dir,
    .url_close_dir       = file_close_dir,
    .default_whitelist   = "file,crypto,data"
};

#endif
#endif
