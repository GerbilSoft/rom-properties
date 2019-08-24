/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_scsi.cpp: General SCSI functions.                                *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "../RpFile.hpp"
#include "../RpFile_p.hpp"
#ifdef _WIN32
# include "libwin32common/w32err.h"
#endif

#include "scsi_protocol.h"
#include "../../byteswap.h"
#include "../../bitstuff.h"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

namespace LibRpBase {

/**
 * Read one sector into the sector cache.
 * @param lba LBA to read.
 * @return 0 on success; non-zero on error.
 */
int RpFilePrivate::readOneLBA(uint32_t lba)
{
	if (!devInfo) {
		// Not a device.
		return -ENODEV;
	}

	// FIXME: On NetBSD and OpenBSD, the Kreon feature list command is failing
	// with EPERM, even as root. (Note that /dev/cd1c or /dev/rcd1c must be used;
	// the 'a' partition fails.)
	//
	// Therefore, we end up using OSAPI instead of SCSI READ, though Kreon
	// functionality *seems* to work in some cases...
	//
	// TODO: Not sure about NetBSD...
	RP_Q(RpFile);
	if (lba == devInfo->lba_cache) {
		// This LBA is already cached.
		// TODO: Special case for ~0U?
		if (!devInfo->isKreonUnlocked) {
			// OSAPI: Seek to the next sector.
			const int64_t seek_pos = (lba+1) * devInfo->sector_size;
#ifdef _WIN32
			LARGE_INTEGER liSeekPos;
			liSeekPos.QuadPart = seek_pos;
			BOOL bRet = SetFilePointerEx(file, liSeekPos, nullptr, FILE_BEGIN);
			if (!bRet) {
				// Seek error.
				q->m_lastError = w32err_to_posix(GetLastError());
				return -q->m_lastError;
			}
#else /* !_WIN32 */
			int ret = fseeko(file, seek_pos, SEEK_SET);
			if (ret != 0) {
				// Seek error.
				q->m_lastError = errno;
				return -q->m_lastError;
			}
#endif /* !_WIN32 */
		}
		return 0;
	}

	// Read the first block.
	if (devInfo->isKreonUnlocked) {
		// Kreon drive. Use SCSI commands.
		int sret = scsi_read(lba, 1, devInfo->sector_cache, devInfo->sector_size);
		if (sret != 0) {
			// Read error.
			// TODO: Handle this properly?
			devInfo->lba_cache = ~0U;
			q->m_lastError = sret;
			return sret;
		}
	} else {
		// Not a Kreon drive. Use the OS API.
		const int64_t seek_pos = lba * devInfo->sector_size;
#ifdef _WIN32
		LARGE_INTEGER liSeekPos;
		liSeekPos.QuadPart = seek_pos;
		BOOL bRet = SetFilePointerEx(file, liSeekPos, nullptr, FILE_BEGIN);
		if (!bRet) {
			// Seek error.
			devInfo->lba_cache = ~0U;
			q->m_lastError = w32err_to_posix(GetLastError());
			return -q->m_lastError;
		}
		DWORD bytesRead;
		BOOL bRet = ReadFile(file, devInfo->sector_cache, devInfo->sector_size, &bytesRead, nullptr);
		if (bRet == 0 || bytesRead != devInfo->sector_size) {
			// Read error.
			devInfo->lba_cache = ~0U;
			q->m_lastError = w32err_to_posix(GetLastError());
			return -q->m_lastError;
		}
#else /* !_WIN32 */
		int ret = fseeko(file, seek_pos, SEEK_SET);
		if (ret != 0) {
			// Seek error.
			devInfo->lba_cache = ~0U;
			q->m_lastError = errno;
			return -q->m_lastError;
		}
		size_t bytesRead = fread(devInfo->sector_cache, 1, devInfo->sector_size, file);
		if (ferror(file) || bytesRead != devInfo->sector_size) {
			// Read error.
			devInfo->lba_cache = ~0U;
			q->m_lastError = errno;
			return -q->m_lastError;
		}
#endif /* _WIN32 */
	}

	// Sector cache has been updated.
	devInfo->lba_cache = lba;
	return 0;
}

/**
 * Read using block reads.
 * Required for block devices.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t RpFilePrivate::readUsingBlocks(void *ptr, size_t size)
{
	assert(devInfo != nullptr);
	assert(devInfo->device_size > 0);
	assert(devInfo->sector_size >= 512);
	if (!devInfo) {
		// Not a block device...
		// NOTE: Not checking device_size or sector_size anymore.
		return 0;
	}

	assert(size != 0);
	if (size == 0) {
		// Why are you doing this?
		return 0;
	}

	uint8_t *ptr8 = static_cast<uint8_t*>(ptr);
	size_t ret = 0;

	RP_Q(RpFile);

	// Are we already at the end of the block device?
	if (devInfo->device_pos >= devInfo->device_size) {
		// End of the block device.
		return 0;
	}

	// Make sure device_pos + size <= d->device_size.
	// If it isn't, we'll do a short read.
	if (devInfo->device_pos + static_cast<int64_t>(size) >= devInfo->device_size) {
		size = static_cast<size_t>(devInfo->device_size - devInfo->device_pos);
	}

	// sector_size must be a power of two.
	assert(isPow2(devInfo->sector_size));
	// TODO: 64-bit LBAs?
	uint32_t lba_cur = static_cast<uint32_t>(devInfo->device_pos / devInfo->sector_size);

	// Make sure the sector cache is allocated.
	devInfo->alloc_sector_cache();

	// Check if we're not starting on a block boundary.
	const uint32_t blockStartOffset = devInfo->device_pos % devInfo->sector_size;
	if (blockStartOffset != 0) {
		// Not a block boundary.
		// Read the end of the first block.
		int lret = readOneLBA(lba_cur);
		if (lret != 0) {
			// Read error.
			// NOTE: q->m_lastError is set by readOneLBA().
			// TODO: readOneLBA() should return number of bytes read,
			// then return that value instead of 0?
			return 0;
		}

		// Copy the data from the sector buffer.
		uint32_t read_sz = devInfo->sector_size - blockStartOffset;
		if (size < static_cast<size_t>(read_sz)) {
			read_sz = static_cast<uint32_t>(size);
		}
		memcpy(ptr8, &devInfo->sector_cache[blockStartOffset], read_sz);

		// Starting block read.
		lba_cur++;
		devInfo->device_pos += read_sz;
		size -= read_sz;
		ptr8 += read_sz;
		ret += read_sz;
	}

	if (size == 0) {
		// Nothing else to read here.
		return ret;
	}

	// Must be on a sector boundary now.
	assert(devInfo->device_pos % devInfo->sector_size == 0);

	// Read contiguous blocks.
	int64_t lba_count = size / devInfo->sector_size;
	size_t contig_size = lba_count * devInfo->sector_size;
	if (devInfo->isKreonUnlocked) {
		// Kreon drive. Use SCSI commands.
		// NOTE: Reading up to 65535 LBAs at a time due to READ(10) limitations.
		// TODO: Move the 65535 LBA code down to RpFile::scsi_read()?
		// FIXME: Seems to have issues above a certain number of LBAs on Linux...
		// Reducing it to 64 KB maximum reads.
		// TODO: Use the sector cache for the first LBA if possible.
		uint32_t lba_increment = 65536 / devInfo->sector_size;
		for (; lba_count > 0; lba_count -= lba_increment) {
			const uint16_t lba_cur_count = (lba_count > lba_increment ? lba_increment : (uint16_t)lba_count);
			const size_t lba_cur_size = (size_t)lba_cur_count * devInfo->sector_size;
			int sret = scsi_read(lba_cur, lba_cur_count, ptr8, lba_cur_size);
			if (sret != 0) {
				// Read error.
				// TODO: Handle this properly?
				q->m_lastError = sret;
				return ret;
			}
			devInfo->device_pos += lba_cur_size;
			lba_cur += lba_cur_count;
			size -= lba_cur_size;
			ptr8 += lba_cur_size;
			ret += lba_cur_size;
		}
	} else {
		// Not a Kreon drive. Use the OS API.
		// TODO: Use the sector cache for the first LBA if possible.

		// Make sure we're at the correct address. The initial seek may
		// have been skipped if we started at the beginning of a block
		// or if the partial block was cached.
		const int64_t seek_pos = lba_cur * devInfo->sector_size;
#ifdef _WIN32
		LARGE_INTEGER liSeekPos;
		liSeekPos.QuadPart = seek_pos;
		BOOL bRet = SetFilePointerEx(file, liSeekPos, nullptr, FILE_BEGIN);
		if (!bRet) {
			// Seek error.
			q->m_lastError = w32err_to_posix(GetLastError());
			return ret;
		}
		DWORD bytesRead;
		BOOL bRet = ReadFile(file, ptr8, contig_size, &bytesRead, nullptr);
		if (bRet == 0 || bytesRead != contig_size) {
			// Read error.
			q->m_lastError = w32err_to_posix(GetLastError());
			return ret + bytesRead;
		}
#else /* !_WIN32 */
		int sret = fseeko(file, seek_pos, SEEK_SET);
		if (sret != 0) {
			// Seek error.
			q->m_lastError = errno;
			return ret;
		}
		size_t bytesRead = fread(ptr8, 1, contig_size, file);
		if (ferror(file) || bytesRead != contig_size) {
			// Read error.
			q->m_lastError = errno;
			return ret + bytesRead;
		}
#endif /* !_WIN32 */

		devInfo->device_pos += contig_size;
		lba_cur += lba_count;
		size -= contig_size;
		ptr8 += contig_size;
		ret += contig_size;
	}

	// Check if we still have data left. (not a full block)
	if (size > 0) {
		// Must be on a sector boundary now.
		assert(devInfo->device_pos % devInfo->sector_size == 0);

		// Read the last block.
		int lret = readOneLBA(lba_cur);
		if (lret != 0) {
			// Read error.
			// NOTE: q->m_lastError is set by readOneLBA().
			// TODO: readOneLBA() should return number of bytes read,
			// then add it to ret?
			return ret;
		}

		// Copy the data from the sector buffer.
		memcpy(ptr8, devInfo->sector_cache, size);

		devInfo->device_pos += size;
		ret += size;
	}

	// Finished reading the data.
	return ret;
}

/** RpFilePrivate: SCSI commands **/

/**
 * Get the capacity of the device using SCSI commands.
 * @param pDeviceSize	[out] Retrieves the device size, in bytes.
 * @param pSectorSize	[out,opt] If not NULL, retrieves the sector size, in bytes.
 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
 */
int RpFilePrivate::scsi_read_capacity(int64_t *pDeviceSize, uint32_t *pSectorSize)
{
	assert(pDeviceSize != nullptr);
	if (!pDeviceSize)
		return -EINVAL;

	if (!devInfo) {
		// Not a device.
		return -ENODEV;
	}

#ifdef RP_OS_SCSI_SUPPORTED
	// SCSI command buffers.
	union {
		SCSI_CDB_READ_CAPACITY_10 cdb10;
		SCSI_CDB_READ_CAPACITY_16 cdb16;
	};
	union {
		SCSI_RESP_READ_CAPACITY_10 resp10;
		SCSI_RESP_READ_CAPACITY_16 resp16;
	};

	// NOTE: The returned LBA is the *last* LBA,
	// not the total number of LBAs. Hence, we'll
	// need to add one.

	// Try READ CAPACITY(10) first.
	cdb10.OpCode = SCSI_OP_READ_CAPACITY_10;
	cdb10.RelAdr = 0;
	cdb10.LBA = 0;
	cdb10.Reserved[0] = 0;
	cdb10.Reserved[1] = 0;
	cdb10.PMI = 0;
	cdb10.Control = 0;

	int ret = scsi_send_cdb(&cdb10, sizeof(cdb10), &resp10, sizeof(resp10), RpFilePrivate::SCSI_DIR_IN);
	if (ret != 0) {
		// SCSI command failed.
		return ret;
	}

	if (resp10.LBA != 0xFFFFFFFF) {
		// READ CAPACITY(10) has the full capacity.
		const uint32_t sector_size = be32_to_cpu(resp10.BlockLen);
		if (pSectorSize) {
			*pSectorSize = sector_size;
		}
		*pDeviceSize = (static_cast<int64_t>(be32_to_cpu(resp10.LBA)) + 1) *
				static_cast<int64_t>(sector_size);
		return 0;
	}

	// READ CAPACITY(10) is truncated.
	// Try READ CAPACITY(16).
	cdb16.OpCode = SCSI_OP_SERVICE_ACTION_IN_16;
	cdb16.SAIn_OpCode = SCSI_SAIN_OP_READ_CAPACITY_16;
	cdb16.LBA = 0;
	cdb16.AllocLen = 0;
	cdb16.Reserved = 0;
	cdb16.Control = 0;

	ret = scsi_send_cdb(&cdb16, sizeof(cdb16), &resp16, sizeof(resp16), RpFilePrivate::SCSI_DIR_IN);
	if (ret != 0) {
		// SCSI command failed.
		// TODO: Return 0xFFFFFFFF+1 blocks anyway?
		return ret;
	}

	const uint32_t sector_size = be32_to_cpu(resp16.BlockLen);
	if (pSectorSize) {
		*pSectorSize = sector_size;
	}
	*pDeviceSize = (static_cast<int64_t>(be64_to_cpu(resp16.LBA)) + 1) *
			static_cast<int64_t>(sector_size);
	return 0;
#else /* !RP_OS_SCSI_SUPPORTED */
	// No SCSI implementation for this OS.
	return -ENOSYS;
#endif /* RP_OS_SCSI_SUPPORTED */
}

/**
 * Read data from a device using SCSI commands.
 * @param lbaStart	[in] Starting LBA of the data to read.
 * @param lbaCount	[in] Number of LBAs to read.
 * @param pBuf		[out] Output buffer.
 * @param bufLen	[in] Output buffer length.
 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
 */
int RpFilePrivate::scsi_read(uint32_t lbaStart, uint16_t lbaCount, uint8_t *pBuf, size_t bufLen)
{
	assert(pBuf != nullptr);
	if (!pBuf)
		return -EINVAL;

	if (!devInfo) {
		// Not a device.
		return -ENODEV;
	}

#ifdef RP_OS_SCSI_SUPPORTED
	const size_t req_buf_size = static_cast<size_t>(
		static_cast<int64_t>(lbaCount) * static_cast<int64_t>(devInfo->sector_size));
	assert(bufLen >= req_buf_size);
	if (bufLen < req_buf_size) {
		// TODO: Better error code?
		return -EIO;
	}

	// SCSI command buffers.
	// NOTE: Using READ(10), which has 32-bit LBA and transfer length.
	// READ(6) has 21-bit LBA and 8-bit transfer length.
	// TODO: May need to use READ(32) for large devices.
	SCSI_CDB_READ_10 cdb10;

	// SCSI READ(10)
	cdb10.OpCode = SCSI_OP_READ_10;
	cdb10.Flags = 0;
	cdb10.LBA = cpu_to_be32(lbaStart);
	cdb10.Reserved = 0;
	cdb10.TransferLen = cpu_to_be16(lbaCount);
	cdb10.Control = 0;

	return scsi_send_cdb(&cdb10, sizeof(cdb10), pBuf, req_buf_size, SCSI_DIR_IN);
#else /* !RP_OS_SCSI_SUPPORTED */
	// No SCSI implementation for this OS.
	return -ENOSYS;
#endif /* RP_OS_SCSI_SUPPORTED */
}

/** RpFile **/

/**
 * Re-read device size using SCSI commands.
 * This may be needed for Kreon devices.
 * @param pDeviceSize	[out,opt] If not NULL, retrieves the device size, in bytes.
 * @param pSectorSize	[out,opt] If not NULL, retrieves the sector size, in bytes.
 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
 */
int RpFile::rereadDeviceSizeScsi(int64_t *pDeviceSize, uint32_t *pSectorSize)
{
	RP_D(RpFile);
	if (!d->devInfo) {
		// Not a device.
		return -ENODEV;
	}

#ifdef RP_OS_SCSI_SUPPORTED
	int64_t device_size;
	uint32_t sector_size;
	int ret = d->scsi_read_capacity(&device_size, &sector_size);
	if (ret != 0) {
		// Error reading capacity.
		return ret;
	}

	// Sector size should match.
	assert(d->devInfo->sector_size == sector_size);

	// Update the device size.
	d->devInfo->device_size = device_size;

	// Return the values.
	if (pDeviceSize) {
		*pDeviceSize = device_size;
	}
	if (pSectorSize) {
		*pSectorSize = sector_size;
	}
	return 0;
#else /* !RP_OS_SCSI_SUPPORTED */
	// No SCSI implementation for this OS.
	return -ENOSYS;
#endif /* RP_OS_SCSI_SUPPORTED */
}

/** RpFile: Public SCSI command wrapper functions **/

/**
 * SCSI INQUIRY command.
 * @param pResp Response buffer.
 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
 */
int RpFile::scsi_inquiry(SCSI_RESP_INQUIRY_STD *pResp)
{
	SCSI_CDB_INQUIRY cdb;
	cdb.OpCode = SCSI_OP_INQUIRY;
	cdb.EVPD = 0;
	cdb.PageCode = 0;
	cdb.AllocLen = cpu_to_be16(sizeof(SCSI_RESP_INQUIRY_STD));
	cdb.Control = 0;

	RP_D(RpFile);
	return d->scsi_send_cdb(&cdb, sizeof(cdb), pResp, sizeof(*pResp), RpFilePrivate::SCSI_DIR_IN);
}

}
