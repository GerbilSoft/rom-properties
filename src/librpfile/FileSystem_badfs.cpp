/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * FileSystem_badfs.cpp: File system functions. (BadFS for POSIX systems)  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "FileSystem.hpp"

// C includes (C++ namespace)
#include <cassert>
#include <cstring>

// C++ STL classes
#include <algorithm>
#include <array>
using std::array;
using std::string;

#ifdef __linux__
// TODO: Remove once /proc/mounts parsing is implemented.
#  include <sys/vfs.h>
#  include <linux/magic.h>
// from `man 2 fstatfs`, but not present in linux/magic.h on 4.14-r1
// Virtual file systems
#  ifndef BPF_FS_MAGIC
#    define BPF_FS_MAGIC 0xcafe4a11
#  endif
#  ifndef CGROUP2_SUPER_MAGIC
#    define CGROUP2_SUPER_MAGIC 0x63677270
#  endif
#  ifndef COH_SUPER_MAGIC
#    define COH_SUPER_MAGIC 0x012ff7b7
#  endif
#  ifndef DEVMEM_MAGIC
#    define DEVMEM_MAGIC 0x454d444d	/* "DMEM" */
#  endif
#  ifndef DMA_BUF_MAGIC
#    define DMA_BUF_MAGIC 0x444d4142	/* "DMAB" */
#  endif
#  ifndef FUSE_SUPER_MAGIC
#    define FUSE_SUPER_MAGIC 0x65735546
#  endif
#  ifndef GUEST_MEMFD_MAGIC
#    define GUEST_MEMFD_MAGIC 0x474d454d	/* "GMEM" */
#  endif
#  ifndef MQUEUE_MAGIC
#    define MQUEUE_MAGIC 0x19800202
#  endif
#  ifndef NSFS_MAGIC
#    define NSFS_MAGIC 0x6e736673
#  endif
#  ifndef NULL_FS_MAGIC
#    define NULL_FS_MAGIC 0x4E554C4C	/* "NULL" */
#  endif
#  ifndef OCFS2_SUPER_MAGIC
#    define OCFS2_SUPER_MAGIC 0x7461636f
#  endif
#  ifndef PID_FS_MAGIC
#    define PID_FS_MAGIC 0x50494446	/* "PIDF" */
#  endif
#  ifndef TRACEFS_MAGIC
#    define TRACEFS_MAGIC 0x74726163
#  endif
#  ifndef SECRETMEM_MAGIC
#    define SECRETMEM_MAGIC 0x5345434d	/* "SECM" */
#  endif
#  ifndef SYSV2_SUPER_MAGIC
#    define SYSV2_SUPER_MAGIC 0x012ff7b6
#  endif
#  ifndef SYSV4_SUPER_MAGIC
#    define SYSV4_SUPER_MAGIC 0x012ff7b5
#  endif

// Network file systems
#  ifndef CEPH_SUPER_MAGIC
#    define CEPH_SUPER_MAGIC 0x00c36400
#  endif
#  ifndef CIFS_MAGIC_NUMBER
#    define CIFS_MAGIC_NUMBER 0xff534d42
#  endif
#endif /* __linux__ */

namespace LibRpFile { namespace FileSystem {

#ifdef __linux__
/**
 * Get the filesystem name for the mount point for the specified filename.
 * @param filename Filename
 * @return Filesystem name for the Mount point, or empty string on error.
 */
string getFileSystemForMountPoint(const char *filename)
{
	string fs_name;

	// Read in `/proc/mounts` and find the longest mount point that matches the filename.
	// TODO: Cache it?
	FILE *f_mounts = fopen("/proc/mounts", "r");
	if (!f_mounts) {
		// Unable to read /proc/mounts?
		return fs_name;
	}

	string s_mnt("/", 1);
	while (!feof(f_mounts)) {
		// FIXME: Is 1024 bytes enough for a single line?
		char buf[1024];
		if (!fgets(buf, sizeof(buf), f_mounts)) {
			break;
		}

		// Token 0: Device name
		// Token 1: Mount point
		char *saveptr = nullptr;
		const char *token = strtok_r(buf, " ", &saveptr);
		token = strtok_r(nullptr, " ", &saveptr);
		assert(token != nullptr);
		if (!token) {
			continue;
		}

		// Is this mount point longer than the current mount point?
		const size_t mount_point_len = strlen(token);
		if (mount_point_len <= s_mnt.size()) {
			// Not longer. Skip it.
			continue;
		}

		// Does the start of the filename match the mount point?
		if (strncmp(token, filename, mount_point_len) != 0) {
			// Not a match.
			continue;
		}

		// Is the next character in the filename either '/' or '\0'?
		char next_chr = filename[mount_point_len];
		if (next_chr == '/' || next_chr == '\0') {
			// It is! This is a longer mount point.
			s_mnt = token;	// TODO: Only need to save the length?

			// Token 2: Filesystem name
			token = strtok_r(nullptr, " ", &saveptr);
			assert(token != nullptr);
			if (token) {
				fs_name.assign(token);
			} else {
				fs_name.clear();
			}
		}
	}

	fclose(f_mounts);
	return fs_name;
}
#endif /* __linux__ */

/**
 * Is a file located on a "bad" file system?
 *
 * We don't want to check files on e.g. procfs,
 * or on network file systems if the option is disabled.
 *
 * @param filename Filename (UTF-8)
 * @param allowNetFS If true, allow network file systems.
 *
 * @return True if this file is on a "bad" file system; false if not.
 */
bool isOnBadFS(const char *filename, bool allowNetFS)
{
#ifdef __linux__
	// TODO: Get the mount point, then look it up in /proc/mounts.

	struct statfs sfbuf;
	if (statfs(filename, &sfbuf) != 0) {
		// statfs() failed.
		// Assume this isn't a network file system.
		return false;
	}
	const uint32_t f_type = static_cast<uint32_t>(sfbuf.f_type);

	// Virtual file systems; ignore these completely
	static constexpr array<uint32_t, 30> vfs_types = {{
		ANON_INODE_FS_MAGIC,
		BDEVFS_MAGIC,
		BINFMTFS_MAGIC,
		BPF_FS_MAGIC,
		CGROUP_SUPER_MAGIC,
		CGROUP2_SUPER_MAGIC,
		DEBUGFS_MAGIC,
		DEVMEM_MAGIC,
		DEVPTS_SUPER_MAGIC,
		DMA_BUF_MAGIC,
		EFIVARFS_MAGIC,
		FUTEXFS_SUPER_MAGIC,
		GUEST_MEMFD_MAGIC,
		MQUEUE_MAGIC,
		NSFS_MAGIC,
		NULL_FS_MAGIC,
		OPENPROM_SUPER_MAGIC,
		PID_FS_MAGIC,
		PIPEFS_MAGIC,
		PROC_SUPER_MAGIC,
		PSTOREFS_MAGIC,
		SECRETMEM_MAGIC,
		SECURITYFS_MAGIC,
		SMACK_MAGIC,
		SOCKFS_MAGIC,
		SYSFS_MAGIC,
		SYSV2_SUPER_MAGIC,
		SYSV4_SUPER_MAGIC,
		TRACEFS_MAGIC,
		USBDEVICE_SUPER_MAGIC,
	}};

	// Network file systems; ignore only if !netFS
	static constexpr array<uint32_t, 10> netfs_types = {{
		AFS_SUPER_MAGIC,
		CEPH_SUPER_MAGIC,
		CIFS_MAGIC_NUMBER,
		CODA_SUPER_MAGIC,
		COH_SUPER_MAGIC,
		NCP_SUPER_MAGIC,
		NFS_SUPER_MAGIC,
		OCFS2_SUPER_MAGIC,
		SMB_SUPER_MAGIC,
		V9FS_MAGIC,
	}};

	// Search for a virtual file system.
	auto vfs_iter = std::find(vfs_types.cbegin(), vfs_types.cend(), f_type);
	if (vfs_iter != vfs_types.cend()) {
		// Found a virtual file system. Ignore it.
		return true;
	}

	// If network file systems are prohibited, check if this is one.
	if (!allowNetFS) {
		// Search for a network file system.
		auto netfs_iter = std::find(netfs_types.cbegin(), netfs_types.cend(), f_type);
		if (netfs_iter != netfs_types.cend()) {
			// Found a network file system. Ignore it.
			return true;
		}

		if (f_type == FUSE_SUPER_MAGIC) {
			// Get the actual filesystem name from the mount point.
			string fs_name = getFileSystemForMountPoint(filename);
			if (fs_name.empty()) {
				// findMountPoint() failed.
				// Assume it's a slow filesystem.
				return true;
			}

			// Check for known local FUSE filesystems.
			if (fs_name.find("exfat") != string::npos ||
			    fs_name.find("ntfs") != string::npos)
			{
				// It's a local filesystem.
				return false;
			}

			// It's not a known local FUSE filesystem.
			// Assume it's remote.
			return true;
		}
	}
#else
#  warning TODO: Implement "badfs" support for non-Linux systems.
	RP_UNUSED(filename);
	RP_UNUSED(allowNetFS);
#endif

	return false;
}

} } // namespace LibRpFile::FileSystem
