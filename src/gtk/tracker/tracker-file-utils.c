/***************************************************************************
 * ROM Properties Page shell extension. (GNOME Tracker)                    *
 * tracker-file-utils.h: File utilities from tracker-miners-3.6.2.         *
 *                                                                         *
 * Copyright (C) 2006, Jamie McCracken <jamiemcc@gnome.org>                *
 * Copyright (C) 2008, Nokia <ivan.frade@nokia.com>                        *
 * SPDX-License-Identifier: LGPL-2.1-or-later                              *
 ***************************************************************************/

// These functions are used by Tracker's own extractor modules,
// but are not exported by libtracker_extractor. They're part of
// libtracker-miners-common, which is statically-linked.
#include "config.tracker.h"
#include "tracker-file-utils.h"

#include <gio/gunixmounts.h>
//#include <blkid.h>

#ifdef HAVE_BTRFS_IOCTL
#  include <linux/btrfs.h>
#  include <sys/ioctl.h>
#  define BTRFS_ROOT_INODE 256
#endif

// C includes
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

// NOTE: The following features have been disabled compatibility with older glib:
// - g_autofree (2.44)
// - g_unix_mount_monitor_get() (2.44)

/** blkid/blkid.h **/

// dlopen() pointers for libblkid
// NOTE: Not dlclose()'d.
static void *libblkid_so = NULL;

typedef struct blkid_struct_cache *blkid_cache;

extern int blkid_get_cache(blkid_cache *cache, const char *filename);
extern char *blkid_get_tag_value(blkid_cache cache, const char *tagname, const char *devname);

static __typeof__(blkid_get_cache) *pfn_blkid_get_cache = NULL;
static __typeof__(blkid_get_tag_value) *pfn_blkid_get_tag_value = NULL;

/** Original libtracker-miners-common code starts here **/

typedef struct {
	GFile *file;
	gchar *mount_point;
	gchar *id;
} UnixMountInfo;

typedef struct {
	//GUnixMountMonitor *monitor;
	blkid_cache id_cache;
	GArray *mounts;
	GRWLock lock;
} TrackerUnixMountCache;

static gint
sort_by_mount (gconstpointer a,
	       gconstpointer b)
{
	const UnixMountInfo *info_a = (const UnixMountInfo*)a, *info_b = (const UnixMountInfo*)b;

	return g_strcmp0 (info_a->mount_point, info_b->mount_point);
}

static void
clear_mount_info (gpointer user_data)
{
	UnixMountInfo *info = (UnixMountInfo*)user_data;

	g_object_unref (info->file);
	g_free (info->mount_point);
	g_free (info->id);
}

static void
update_mounts (TrackerUnixMountCache *cache)
{
	GList *mounts;
	const GList *l;

	g_rw_lock_writer_lock (&cache->lock);

	g_array_set_size (cache->mounts, 0);

	mounts = g_unix_mounts_get (NULL);

	for (l = mounts; l; l = l->next) {
		GUnixMountEntry *entry = (GUnixMountEntry*)l->data;
		const gchar *devname;
		/*g_autofree*/ gchar *id = NULL/*, *subvol = NULL*/;
		UnixMountInfo mount;

		devname = g_unix_mount_get_device_path (entry);
		id = pfn_blkid_get_tag_value (cache->id_cache, "UUID", devname);
		if (!id && strchr (devname, G_DIR_SEPARATOR) != NULL)
			id = g_strdup (devname);

		if (!id)
			continue;

		mount.mount_point = g_strdup (g_unix_mount_get_mount_path (entry));
		mount.file = g_file_new_for_path (mount.mount_point);
		//mount.id = g_steal_pointer (&id);
		mount.id = id;
		g_array_append_val (cache->mounts, mount);
	}

	g_array_sort (cache->mounts, sort_by_mount);

	g_rw_lock_writer_unlock (&cache->lock);
	g_list_free_full (mounts, (GDestroyNotify) g_unix_mount_free);
}

static TrackerUnixMountCache *
tracker_unix_mount_cache_get (void)
{
	static TrackerUnixMountCache *cache = NULL;
	TrackerUnixMountCache *obj;

	if (cache == NULL) {
		obj = g_new0 (TrackerUnixMountCache, 1);
		g_rw_lock_init (&obj->lock);
		//obj->monitor = g_unix_mount_monitor_get ();
		obj->mounts = g_array_new (FALSE, FALSE, sizeof (UnixMountInfo));
		g_array_set_clear_func (obj->mounts, clear_mount_info);

		pfn_blkid_get_cache (&obj->id_cache, NULL);

		/*g_signal_connect (obj->monitor, "mounts-changed",
				  G_CALLBACK (on_mounts_changed), obj);*/
		update_mounts (obj);
		cache = obj;
	}

	return cache;
}

static const gchar *
tracker_unix_mount_cache_lookup_filesystem_id (GFile *file)
{
	TrackerUnixMountCache *cache;
	const gchar *id = NULL;
	gint i;

	cache = tracker_unix_mount_cache_get ();

	g_rw_lock_reader_lock (&cache->lock);

	for (i = (gint) cache->mounts->len - 1; i >= 0; i--) {
		UnixMountInfo *info = &g_array_index (cache->mounts, UnixMountInfo, i);

		if (g_file_equal (file, info->file) ||
		    g_file_has_prefix (file, info->file)) {
			id = info->id;
			break;
		}
	}

	g_rw_lock_reader_unlock (&cache->lock);

	return id;
}

#ifdef HAVE_BTRFS_IOCTL
static gchar *
tracker_file_get_btrfs_subvolume_id (GFile *file)
{
	struct btrfs_ioctl_ino_lookup_args args = {
		.treeid = 0,
		.objectid = BTRFS_ROOT_INODE,
	};
	/*g_autofree*/ gchar *path = NULL;
	int fd, ret;

	path = g_file_get_path (file);
	if (!path)
		return NULL;

	fd = open (path, O_RDONLY);
	g_free (path);
	if (fd < 0)
		return NULL;

	ret = ioctl (fd, BTRFS_IOC_INO_LOOKUP, &args);
	close (fd);
	if (ret < 0)
		return NULL;

	return g_strdup_printf ("%" G_GUINT64_FORMAT, (guint64) args.treeid);
}
#endif

/**
 * Attempt to initialize libblkid.so.
 * @return 0 on success; negative POSIX error code on error.
 */
static int
init_libblkid_so(void)
{
	if (libblkid_so) {
		// Already initialized.
		return 0;
	}

	// Attempt to dlopen() libblkid.so.
	// NOTE: Not dlclose()'d.
	libblkid_so = dlopen("libblkid.so.1", RTLD_NOW | RTLD_LOCAL);
	if (!libblkid_so) {
		// Not found...
		// TODO: Other error?
		return -ENOENT;
	}

	// Attempt to dlsym() the required symbols.
	pfn_blkid_get_cache = dlsym(libblkid_so, "blkid_get_cache");
	pfn_blkid_get_tag_value = dlsym(libblkid_so, "blkid_get_tag_value");
	if (!pfn_blkid_get_cache || !pfn_blkid_get_tag_value) {
		// One (or both) symbols are missing?
		dlclose(libblkid_so);
		pfn_blkid_get_cache = NULL;
		pfn_blkid_get_tag_value = NULL;
		libblkid_so = NULL;
		return -EIO;
	}

	// Symbols loaded.
	return 0;
}

gchar *
tracker_file_get_content_identifier (GFile *file, GFileInfo *info, const gchar *suffix)
{
	const gchar *id;
	/*g_autofree*/ gchar *inode = NULL, *str = NULL, *subvolume = NULL;

	// Make sure libblkid.so is initialized.
	if (init_libblkid_so() != 0) {
		// Cannot initialize libblkid.so.
		return NULL;
	}

	if (info) {
		g_object_ref (info);
	} else {
		info = g_file_query_info (file,
		                          G_FILE_ATTRIBUTE_ID_FILESYSTEM ","
		                          G_FILE_ATTRIBUTE_UNIX_INODE,
		                          G_FILE_QUERY_INFO_NONE,
		                          NULL,
		                          NULL);
		if (!info)
			return NULL;
	}

	id = tracker_unix_mount_cache_lookup_filesystem_id (file);

	if (!id) {
		id = g_file_info_get_attribute_string (info,
		                                       G_FILE_ATTRIBUTE_ID_FILESYSTEM);
	}

	inode = g_file_info_get_attribute_as_string (info, G_FILE_ATTRIBUTE_UNIX_INODE);

#ifdef HAVE_BTRFS_IOCTL
	subvolume = tracker_file_get_btrfs_subvolume_id (file);
#endif

	/* Format:
	 * 'urn:fileid:' [fsid] (':' [subvolumeid])? ':' [inode] ('/' [suffix])?
	 */
	str = g_strconcat ("urn:fileid:", id,
	                   subvolume ? ":" : "",
	                   subvolume ? subvolume : "",
	                   ":", inode,
	                   suffix ? "/" : NULL,
	                   suffix, NULL);

	g_object_unref (info);
	g_free (inode);
	g_free (subvolume);

	//return g_steal_pointer(&str);
	return str;
}
