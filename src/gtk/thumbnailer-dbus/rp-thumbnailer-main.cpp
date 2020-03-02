/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * rp-thumbnailer-main.cpp: D-Bus thumbnailerer service: main()            *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "librpbase/common.h"
#include "libunixcommon/userdirs.hpp"
#include "libunixcommon/dll-search.h"
#include "rp-thumbnailer-dbus.h"

// OS-specific security options.
#include "librpsecure/os-secure.h"

// C includes. (C++ namespace)
#include <cstdarg>
#include <cstdio>

// C++ includes.
#include <string>
using std::string;

// dlopen()
#include <dlfcn.h>

// Shutdown request.
static bool stop_main_loop = false;

// Cache directory.
static string cache_dir;

/**
 * Initialize the cache directory.
 * @return 0 on success; non-zero on error.
 */
static int init_cache_dir(void)
{
	cache_dir = LibUnixCommon::getCacheDirectory();
	if (cache_dir.empty()) {
		g_critical("Unable to determine the XDG cache directory.");
		return -1;
	}
	g_debug("Cache directory: %s", cache_dir.c_str());
	return 0;
}

/**
 * Debug print function for rp_dll_search().
 * @param level Debug level.
 * @param format Format string.
 * @param ... Format arguments.
 * @return vfprintf() return value.
 */
static int ATTR_PRINTF(2, 3) fnDebug(int level, const char *format, ...)
{
	// g_warning() may be using g_log_structured(),
	// and there's no variant of g_log_structured()
	// that takes va_list, so we'll print it to a
	// buffer first.
	char buf[512];

	va_list args;
	va_start(args, format);
	int ret = vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	if (level < LEVEL_ERROR) {
		// G_MESSAGES_DEBUG must be set to rom-properties-xfce
		// in order to print these messages.
		g_debug("%s", buf);
	} else {
		g_warning("%s", buf);
	}
	return ret;
}

/**
 * Shutdown callback.
 * @param thumbnailer RpThumbnail object.
 * @param main_loop GMainLoop.
 */
static void
shutdown_rp_thumbnailer_dbus(RpThumbnailer *thumbnailer, GMainLoop *main_loop)
{
	g_return_if_fail(IS_RP_THUMBNAILER(thumbnailer));
	g_return_if_fail(main_loop != nullptr);

	// Exit the main loop.
	stop_main_loop = true;
	if (g_main_loop_is_running(main_loop)) {
		g_main_loop_quit(main_loop);
	}
}

/**
 * The D-Bus name was either lost or could not be acquired.
 * @param main_loop GMainLoop
 */
static void
on_dbus_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	RP_UNUSED(connection);
	RP_UNUSED(name);
	GMainLoop *const main_loop = (GMainLoop*)user_data;
	g_return_if_fail(main_loop != nullptr);

	stop_main_loop = true;
	if (g_main_loop_is_running(main_loop)) {
		g_debug("D-Bus name was lost; exiting.");
		g_main_loop_quit(main_loop);
	}
}

int main(int argc, char *argv[])
{
	RP_UNUSED(argc);
	RP_UNUSED(argv);

	if (getuid() == 0 || geteuid() == 0) {
		fprintf(stderr, "*** %s does not support running as root.", argv[0]);
		return EXIT_FAILURE;
	}

	// Set OS-specific security options.
	rp_secure_param_t param;
#if defined(_WIN32)
	param.bHighSec = FALSE;
#elif defined(HAVE_SECCOMP)
	static const int syscall_wl[] = {
		// Syscalls used by rp-download.
		// TODO: Add more syscalls.
		// FIXME: glibc-2.31 uses 64-bit time syscalls that may not be
		// defined in earlier versions, including Ubuntu 14.04.

		// NOTE: Special case for clone(). If it's the first syscall
		// in the list, it has a parameter restriction added that
		// ensures it can only be used to create threads.
		SCMP_SYS(clone),

#if 0
		SCMP_SYS(clock_gettime),
#ifdef __SNR_clock_gettime64
		SCMP_SYS(clock_gettime64),
#endif /* __SNR_clock_gettime64 */
		SCMP_SYS(close), SCMP_SYS(fcntl), SCMP_SYS(fsetxattr),
		SCMP_SYS(fstat), SCMP_SYS(getdents),
		SCMP_SYS(getrusage), SCMP_SYS(getuid), SCMP_SYS(lseek),
		SCMP_SYS(mkdir), SCMP_SYS(mmap2),
		SCMP_SYS(munmap),
		SCMP_SYS(poll), SCMP_SYS(select),
		SCMP_SYS(utimensat),
#endif

		SCMP_SYS(access),	// LibUnixCommon::isWritableDirectory()
		SCMP_SYS(close),
		SCMP_SYS(fstat),	// __GI___fxstat() [printf()]
		SCMP_SYS(ftruncate),	// LibRpBase::RpFile::truncate() [from LibRpBase::RpPngWriterPrivate::init()]
		SCMP_SYS(futex),	// iconv_open(), dlopen()
		SCMP_SYS(getppid),	// dll-search.c: walk_proc_tree()
		SCMP_SYS(getuid),	// TODO: Only use geteuid()?
		SCMP_SYS(lseek),
		SCMP_SYS(lstat),	// LibRpBase::FileSystem::is_symlink()
		SCMP_SYS(mkdir),	// g_mkdir_with_parents() [rp_thumbnailer_process()]
		SCMP_SYS(mmap),		// iconv_open(), dlopen()
		SCMP_SYS(mmap2),	// iconv_open(), dlopen() [might only be needed on i386...]
		SCMP_SYS(munmap),	// dlopen(), free() [in some cases]
		SCMP_SYS(mprotect),	// iconv_open()
		SCMP_SYS(mmap),		// iconv_open()
		SCMP_SYS(open),		// Ubuntu 16.04
		SCMP_SYS(openat),	// glibc-2.31
#ifdef __SNR_openat2
		SCMP_SYS(openat2),	// Linux 5.6
#endif /* __SNR_openat2 */
		SCMP_SYS(readlink),	// realpath() [LibRpBase::FileSystem::resolve_symlink()]
		SCMP_SYS(stat),		// LibUnixCommon::isWritableDirectory()
#ifdef __SNR_statx
		SCMP_SYS(statx),	// unsure?
#endif /* __SNR_statx */
		SCMP_SYS(statfs),	// LibRpBase::FileSystem::isOnBadFS()
		SCMP_SYS(statfs64),	// LibRpBase::FileSystem::isOnBadFS()

		// glib / D-Bus
		SCMP_SYS(connect), SCMP_SYS(eventfd2), SCMP_SYS(fcntl),
		SCMP_SYS(getdents),	// g_file_new_for_uri() [rp_create_thumbnail()]
		SCMP_SYS(getdents64),	// g_file_new_for_uri() [rp_create_thumbnail()]
		SCMP_SYS(getegid), SCMP_SYS(geteuid), SCMP_SYS(poll),
		SCMP_SYS(recvfrom), SCMP_SYS(recvmsg), SCMP_SYS(set_robust_list),
		SCMP_SYS(sendmsg), SCMP_SYS(sendto), SCMP_SYS(socket),

		// only if G_MESSAGES_DEBUG=all [on Gentoo, but not Ubuntu 14.04]
		SCMP_SYS(getpeername),	// g_log_writer_is_journald() [g_log()]
		SCMP_SYS(ioctl),	// isatty() [g_log()]

		// TODO: Parameter filtering for prctl().
		SCMP_SYS(prctl),	// pthread_setname_np() [g_thread_proxy(), start_thread()]

		-1	// End of whitelist
	};
	param.syscall_wl = syscall_wl;
#elif defined(HAVE_PLEDGE)
	// Promises:
	// - stdio: General stdio functionality.
	// - rpath: Read from ~/.config/rom-properties/ and ~/.cache/rom-properties/
	// - wpath: Write to ~/.cache/rom-properties/
	// - cpath: Create ~/.cache/rom-properties/ if it doesn't exist.
	// - getpw: Get user's home directory if HOME is empty.
	param.promises = "stdio rpath wpath cpath getpw";
#elif defined(HAVE_TAME)
	// NOTE: stdio includes fattr, e.g. utimes().
	param.tame_flags = TAME_STDIO | TAME_RPATH | TAME_WPATH | TAME_CPATH | TAME_GETPW;
#else
	param.dummy = 0;
#endif
	rp_secure_enable(param);

#if !GLIB_CHECK_VERSION(2,36,0)
	// g_type_init() is automatic as of glib-2.36.0
	// and is marked deprecated.
	g_type_init();
#endif /* !GLIB_CHECK_VERSION(2,36,0) */
#if !GLIB_CHECK_VERSION(2,32,0)
	// g_thread_init() is automatic as of glib-2.32.0
	// and is marked deprecated.
	if (!g_thread_supported()) {
		g_thread_init(nullptr);
	}
#endif /* !GLIB_CHECK_VERSION(2,32,0) */

	// Initialize the cache directory.
	if (init_cache_dir() != 0) {
		return EXIT_FAILURE;
	}

	// Attempt to open a ROM Properties Page library.
	void *pDll = nullptr;
	PFN_RP_CREATE_THUMBNAIL pfn_rp_create_thumbnail = nullptr;
	int ret = rp_dll_search("rp_create_thumbnail", &pDll, (void**)&pfn_rp_create_thumbnail, fnDebug);
	if (ret != 0) {
		return EXIT_FAILURE;
	}

	GError *error = nullptr;
	GDBusConnection *const connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
	if (error) {
		g_critical("Unable to connect to the session bus: %s", error->message);
		g_error_free(error);
		dlclose(pDll);
		return EXIT_FAILURE;
	}

	GMainLoop *main_loop = g_main_loop_new(nullptr, false);

	// Create the RpThumbnail service object.
	RpThumbnailer *const thumbnailer = rp_thumbnailer_new(
		connection, cache_dir.c_str(), pfn_rp_create_thumbnail);

	// Register the D-Bus service.
	g_bus_own_name_on_connection(connection,
		"com.gerbilsoft.rom-properties.SpecializedThumbnailer1",
		G_BUS_NAME_OWNER_FLAGS_NONE, nullptr, on_dbus_name_lost, main_loop, nullptr);

	if (rp_thumbnailer_is_exported(thumbnailer)) {
		// Service object is exported.

		// Make sure we quit after the RpThumbnail server is idle for long enough.
		g_signal_connect(thumbnailer, "shutdown", G_CALLBACK(shutdown_rp_thumbnailer_dbus), main_loop);

		// Run the main loop.
		if (!stop_main_loop) {
			g_debug("Starting the D-Bus service.");
			g_main_loop_run(main_loop);
		}
	}
	dlclose(pDll);
	return 0;
}
