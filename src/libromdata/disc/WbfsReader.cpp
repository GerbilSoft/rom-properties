/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WbfsReader.cpp: WBFS disc image reader.                                 *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "WbfsReader.hpp"
#include "libwbfs.h"

#include "byteswap.h"
#include "file/IRpFile.hpp"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

namespace LibRomData {

class WbfsReaderPrivate {
	public:
		explicit WbfsReaderPrivate(IRpFile *file);
		~WbfsReaderPrivate();

	private:
		WbfsReaderPrivate(const WbfsReaderPrivate &other);
		WbfsReaderPrivate &operator=(const WbfsReaderPrivate &other);

	public:
		IRpFile *file;
		int lastError;
		int64_t disc_size;		// Virtual disc image size.

		// WBFS structs.
		wbfs_t *m_wbfs;			// WBFS image.
		wbfs_disc_t *m_wbfs_disc;	// Current disc.
		int64_t m_wbfs_pos;		// Read position in m_wbfs_disc.

		/** WBFS functions. **/

		// WBFS magic number.
		static const uint8_t WBFS_MAGIC[4];

		/**
		 * Read the WBFS header.
		 * @return Allocated wbfs_t on success; nullptr on error.
		 */
		wbfs_t *readWbfsHeader(void);

		/**
		 * Free an allocated WBFS header.
		 * This frees all associated structs.
		 * All opened discs *must* be closed.
		 * @param p wbfs_t struct.
		 */
		void freeWbfsHeader(wbfs_t *p);

		/**
		 * Open a disc from the WBFS image.
		 * @param p wbfs_t struct.
		 * @param index Disc index.
		 * @return Allocated wbfs_disc_t on success; nullptr on error.
		 */
		wbfs_disc_t *openWbfsDisc(wbfs_t *p, uint32_t index);

		/**
		 * Close a WBFS disc.
		 * This frees all associated structs.
		 * @param disc wbfs_disc_t.
		 */
		void closeWbfsDisc(wbfs_disc_t *disc);

		/**
		 * Get the non-sparse size of an open WBFS disc, in bytes.
		 * This scans the block table to find the first block
		 * from the end of wlba_table[] that has been allocated.
		 * @param disc wbfs_disc_t struct.
		 * @return Non-sparse size, in bytes.
		 */
		int64_t getWbfsDiscSize(const wbfs_disc_t *disc);
};

/** WbfsReaderPrivate **/

// WBFS magic number.
const uint8_t WbfsReaderPrivate::WBFS_MAGIC[4] = {'W','B','F','S'};

WbfsReaderPrivate::WbfsReaderPrivate(IRpFile *file)
	: file(nullptr)
	, lastError(0)
	, disc_size(0)
	, m_wbfs(nullptr)
	, m_wbfs_disc(nullptr)
	, m_wbfs_pos(0)
{
	if (!file) {
		lastError = EBADF;
		return;
	}
	this->file = file->dup();

	// Read the WBFS header.
	m_wbfs = readWbfsHeader();
	if (!m_wbfs) {
		// Error reading the WBFS header.
		delete this->file;
		this->file = nullptr;
		lastError = EIO;
		return;
	}

	// Open the first disc.
	m_wbfs_disc = openWbfsDisc(m_wbfs, 0);
	if (!m_wbfs_disc) {
		// Error opening the WBFS disc.
		freeWbfsHeader(m_wbfs);
		m_wbfs = nullptr;
		delete this->file;
		this->file = nullptr;
		lastError = EIO;
		return;
	}

	// Get the size of the WBFS disc.
	disc_size = getWbfsDiscSize(m_wbfs_disc);
}

WbfsReaderPrivate::~WbfsReaderPrivate()
{
	if (m_wbfs_disc) {
		closeWbfsDisc(m_wbfs_disc);
	}
	if (m_wbfs) {
		freeWbfsHeader(m_wbfs);
	}

	delete file;
}

// from libwbfs.c
// TODO: Optimize this?
static uint8_t size_to_shift(uint32_t size)
{
	uint8_t ret = 0;
	while (size) {
		ret++;
		size >>= 1;
	}
	return ret-1;
}

#define ALIGN_LBA(x) (((x)+p->hd_sec_sz-1)&(~(p->hd_sec_sz-1)))

/**
 * Read the WBFS header.
 * @return Allocated wbfs_t on success; nullptr on error.
 */
wbfs_t *WbfsReaderPrivate::readWbfsHeader(void)
{
	// Assume 512-byte sectors initially.
	unsigned int hd_sec_sz = 512;
	wbfs_head_t *head = (wbfs_head_t*)malloc(hd_sec_sz);

	// Read the WBFS header.
	file->rewind();
	size_t size = file->read(head, hd_sec_sz);
	if (size != hd_sec_sz) {
		// Read error.
		free(head);
		return nullptr;
	}

	// Check the WBFS magic.
	if (memcmp(&head->magic, WBFS_MAGIC, sizeof(WBFS_MAGIC)) != 0) {
		// Invalid WBFS magic.
		// TODO: Better error code?
		free(head);
		return nullptr;
	}

	// Parse the header.
	// Based on libwbfs.c's wbfs_open_partition().

	// wbfs_t struct.
	wbfs_t *p = (wbfs_t*)malloc(sizeof(wbfs_t));

	// Since this is a disc image, we don't know the HDD sector size.
	// Use the value present in the header.
	if (head->hd_sec_sz_s < 0x09) {
		// Sector size is less than 512 bytes.
		// This isn't possible unless you're using
		// a Commodore 64 or an Apple ][.
		free(head);
		free(p);
		return nullptr;
	}
	p->hd_sec_sz = (1 << head->hd_sec_sz_s);
	p->hd_sec_sz_s = head->hd_sec_sz_s;
	p->n_hd_sec = be32_to_cpu(head->n_hd_sec);	// TODO: Use file size?

	// If the sector size is wrong, reallocate and reread.
	if (p->hd_sec_sz != hd_sec_sz) {
		// TODO: realloc()?
		hd_sec_sz = p->hd_sec_sz;
		free(head);
		head = (wbfs_head_t*)malloc(hd_sec_sz);

		// Re-read the WBFS header.
		file->rewind();
		size_t size = file->read(head, hd_sec_sz);
		if (size != hd_sec_sz) {
			// Read error.
			// TODO: Return errno?
			free(head);
			free(p);
			return nullptr;
		}
	}

	// Save the wbfs_head_t in the wbfs_t struct.
	p->head = head;

	// Constants.
	p->wii_sec_sz = 0x8000;
	p->wii_sec_sz_s = size_to_shift(0x8000);
	p->n_wii_sec = (p->n_hd_sec/0x8000)*p->hd_sec_sz;
	p->n_wii_sec_per_disc = 143432*2;	// support for dual-layer discs

	// WBFS sector size.
	p->wbfs_sec_sz_s = head->wbfs_sec_sz_s;
	p->wbfs_sec_sz = (1 << head->wbfs_sec_sz_s);

	// Disc size.
	p->n_wbfs_sec = p->n_wii_sec >> (p->wbfs_sec_sz_s - p->wii_sec_sz_s);
	p->n_wbfs_sec_per_disc = p->n_wii_sec_per_disc >> (p->wbfs_sec_sz_s - p->wii_sec_sz_s);
	p->disc_info_sz = ALIGN_LBA(sizeof(wbfs_disc_info_t) + p->n_wbfs_sec_per_disc*2);

	// Free blocks table.
	p->freeblks_lba = (p->wbfs_sec_sz - p->n_wbfs_sec/8) >> p->hd_sec_sz_s;
	p->freeblks = nullptr;
	p->max_disc = (p->freeblks_lba-1) / (p->disc_info_sz >> p->hd_sec_sz_s);
	if (p->max_disc > (p->hd_sec_sz - sizeof(wbfs_head_t)))
		p->max_disc = p->hd_sec_sz - sizeof(wbfs_head_t);

	p->n_disc_open = 0;
	return p;
}

/**
 * Free an allocated WBFS header.
 * This frees all associated structs.
 * All opened discs *must* be closed.
 * @param p wbfs_t struct.
 */
void WbfsReaderPrivate::freeWbfsHeader(wbfs_t *p)
{
	assert(p != nullptr);
	assert(p->head != nullptr);
	assert(p->n_disc_open == 0);

	// Free everything.
	free(p->head);
	free(p);
}

/**
 * Open a disc from the WBFS image.
 * @param p wbfs_t struct.
 * @param index Disc index.
 * @return Allocated wbfs_disc_t on success; non-zero on error.
 */
wbfs_disc_t *WbfsReaderPrivate::openWbfsDisc(wbfs_t *p, uint32_t index)
{
	// Based on libwbfs.c's wbfs_open_disc()
	// and wbfs_get_disc_info().
	wbfs_head_t *const head = p->head;
	uint32_t count = 0;
	for (uint32_t i = 0; i < p->max_disc; i++) {
		if (head->disc_table[i]) {
			if (count++ == index) {
				// Found the disc table index.
				wbfs_disc_t *disc = (wbfs_disc_t*)malloc(sizeof(wbfs_disc_t));
				disc->p = p;
				disc->i = i;

				// Read the disc header.
				disc->header = (wbfs_disc_info_t*)malloc(p->disc_info_sz);
				file->seek(p->hd_sec_sz + (i*p->disc_info_sz));
				size_t size = file->read(disc->header, p->disc_info_sz);
				if (size != p->disc_info_sz) {
					// Error reading the disc information.
					free(disc->header);
					free(disc);
					return nullptr;
				}

				// TODO: Byteswap wlba_table[] here?
				// Removes unnecessary byteswaps when reading,
				// but may not be necessary if we're not reading
				// the entire disc.

				// Disc information read successfully.
				p->n_disc_open++;
				return disc;
			}
		}
	}

	// Disc not found.
	return nullptr;
}

/**
 * Close a WBFS disc.
 * This frees all associated structs.
 * @param disc wbfs_disc_t.
 */
void WbfsReaderPrivate::closeWbfsDisc(wbfs_disc_t *disc)
{
	assert(disc != nullptr);
	assert(disc->p != nullptr);
	assert(disc->p->n_disc_open > 0);
	assert(disc->header != nullptr);

	// Free the disc.
	disc->p->n_disc_open--;
	free(disc->header);
	free(disc);
}

/**
 * Get the non-sparse size of an open WBFS disc, in bytes.
 * This scans the block table to find the first block
 * from the end of wlba_table[] that has been allocated.
 * @param disc wbfs_disc_t struct.
 * @return Non-sparse size, in bytes.
 */
int64_t WbfsReaderPrivate::getWbfsDiscSize(const wbfs_disc_t *disc)
{
	// Find the last block that's used on the disc.
	// NOTE: This is in WBFS blocks, not Wii blocks.
	const be16_t *const wlba_table = disc->header->wlba_table;
	int lastBlock = disc->p->n_wbfs_sec_per_disc - 1;
	for (; lastBlock >= 0; lastBlock--) {
		if (be16_to_cpu(wlba_table[lastBlock]) != 0)
			break;
	}

	// lastBlock+1 * WBFS block size == filesize.
	const wbfs_t *p = disc->p;
	return (int64_t)(lastBlock + 1) * (int64_t)(p->wbfs_sec_sz);
}

/** WbfsReader **/

WbfsReader::WbfsReader(IRpFile *file)
	: d(new WbfsReaderPrivate(file))
{ }

WbfsReader::~WbfsReader()
{
	delete d;
}

/**
 * Is a disc image supported by this class?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
 */
int WbfsReader::isDiscSupported_static(const uint8_t *pHeader, size_t szHeader)
{
	if (szHeader < sizeof(wbfs_head_t))
		return -1;

	const wbfs_head_t *head = reinterpret_cast<const wbfs_head_t*>(pHeader);
	if (memcmp(&head->magic, WbfsReaderPrivate::WBFS_MAGIC, sizeof(WbfsReaderPrivate::WBFS_MAGIC)) != 0) {
		// Incorrect magic number.
		return -1;
	}

	// Make sure the sector size is at least 512 bytes.
	if (head->hd_sec_sz_s < 0x09) {
		// Sector size is less than 512 bytes.
		// This isn't possible unless you're using
		// a Commodore 64 or an Apple ][.
		return -1;
	}

	// This is a valid WBFS image.
	return 0;
}

/**
 * Is a disc image supported by this object?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WbfsReader::isDiscSupported(const uint8_t *pHeader, size_t szHeader) const
{
	return isDiscSupported_static(pHeader, szHeader);
}

/**
 * Is the disc image open?
 * This usually only returns false if an error occurred.
 * @return True if the disc image is open; false if it isn't.
 */
bool WbfsReader::isOpen(void) const
{
	return (d->file != nullptr);
}

/**
 * Get the last error.
 * @return Last POSIX error, or 0 if no error.
 */
int WbfsReader::lastError(void) const
{
	return d->lastError;
}

/**
 * Clear the last error.
 */
void WbfsReader::clearError(void)
{
	d->lastError = 0;
}

/**
 * Read data from the disc image.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t WbfsReader::read(void *ptr, size_t size)
{
	assert(d->file != nullptr);
	assert(d->m_wbfs != nullptr);
	assert(d->m_wbfs_disc != nullptr);
	if (!d->file || !d->m_wbfs || !d->m_wbfs_disc) {
		d->lastError = EBADF;
		return 0;
	}

	uint8_t *ptr8 = reinterpret_cast<uint8_t*>(ptr);
	size_t ret = 0;

	// Are we already at the end of the file?
	if (d->m_wbfs_pos >= d->disc_size)
		return 0;

	// Make sure m_wbfs_pos + size <= file size.
	// If it isn't, we'll do a short read.
	if (d->m_wbfs_pos + (int64_t)size >= d->disc_size) {
		size = (size_t)(d->disc_size - d->m_wbfs_pos);
	}

	// Check if we're not starting on a block boundary.
	const uint32_t wbfs_sec_sz = d->m_wbfs->wbfs_sec_sz;
	const uint16_t *const wlba_table = d->m_wbfs_disc->header->wlba_table;
	const uint32_t blockStartOffset = d->m_wbfs_pos % wbfs_sec_sz;
	if (blockStartOffset != 0) {
		// Not a block boundary.
		// Read the end of the block.
		uint32_t read_sz = d->m_wbfs->wbfs_sec_sz - blockStartOffset;
		if (size < read_sz)
			read_sz = size;

		// Get the physical block number first.
		uint16_t blockStart = (d->m_wbfs_pos / wbfs_sec_sz);
		uint16_t physBlockStartIdx = be16_to_cpu(wlba_table[blockStart]);
		if (physBlockStartIdx == 0) {
			// Empty block.
			memset(ptr8, 0, read_sz);
		} else {
			// Seek to the physical block address.
			int64_t physBlockStartAddr = (uint64_t)physBlockStartIdx * wbfs_sec_sz;
			d->file->seek(physBlockStartAddr + blockStartOffset);
			// Read read_sz bytes.
			size_t rd = d->file->read(ptr8, read_sz);
			if (rd != read_sz) {
				// Error reading the data.
				return rd;
			}
		}

		// Starting block read.
		size -= read_sz;
		ptr8 += read_sz;
		ret += read_sz;
		d->m_wbfs_pos += read_sz;
	}

	// Read entire blocks.
	for (; size >= wbfs_sec_sz;
	    size -= wbfs_sec_sz, ptr8 += wbfs_sec_sz,
	    ret += wbfs_sec_sz, d->m_wbfs_pos += wbfs_sec_sz)
	{
		assert(d->m_wbfs_pos % wbfs_sec_sz == 0);
		uint16_t physBlockIdx = be16_to_cpu(wlba_table[d->m_wbfs_pos / wbfs_sec_sz]);
		if (physBlockIdx == 0) {
			// Empty block.
			memset(ptr8, 0, wbfs_sec_sz);
		} else {
			// Seek to the physical block address.
			int64_t physBlockAddr = (uint64_t)physBlockIdx * wbfs_sec_sz;
			d->file->seek(physBlockAddr);
			// Read one WBFS sector worth of data.
			size_t rd = d->file->read(ptr8, wbfs_sec_sz);
			if (rd != wbfs_sec_sz) {
				// Error reading the data.
				return rd + ret;
			}
		}
	}

	// Check if we still have data left. (not a full block)
	if (size > 0) {
		// Not a full block.

		// Get the physical block number first.
		assert(d->m_wbfs_pos % wbfs_sec_sz == 0);
		uint16_t blockEnd = (d->m_wbfs_pos / wbfs_sec_sz);
		uint16_t physBlockEndIdx = be16_to_cpu(wlba_table[blockEnd]);
		if (physBlockEndIdx == 0) {
			// Empty block.
			memset(ptr8, 0, size);
		} else {
			// Seek to the physical block address.
			int64_t physBlockEndAddr = (uint64_t)physBlockEndIdx * wbfs_sec_sz;
			d->file->seek(physBlockEndAddr);
			// Read size bytes.
			size_t rd = d->file->read(ptr8, size);
			if (rd != size) {
				// Error reading the data.
				return rd + ret;
			}
		}

		ret += size;
		d->m_wbfs_pos += size;
	}

	// Finished reading the data.
	return ret;
}

/**
 * Set the disc image position.
 * @param pos Disc image position.
 * @return 0 on success; -1 on error.
 */
int WbfsReader::seek(int64_t pos)
{
	assert(d->file != nullptr);
	assert(d->m_wbfs != nullptr);
	assert(d->m_wbfs_disc != nullptr);
	if (!d->file || !d->m_wbfs || !d->m_wbfs_disc) {
		d->lastError = EBADF;
		return -1;
	}

	// Handle out-of-range cases.
	// TODO: How does POSIX behave?
	if (pos < 0)
		d->m_wbfs_pos = 0;
	else if (pos >= d->disc_size)
		d->m_wbfs_pos = d->disc_size;
	else
		d->m_wbfs_pos = pos;
	return 0;
}

/**
 * Seek to the beginning of the disc image.
 */
void WbfsReader::rewind(void)
{
	assert(d->file != nullptr);
	assert(d->m_wbfs != nullptr);
	assert(d->m_wbfs_disc != nullptr);
	if (!d->file || !d->m_wbfs || !d->m_wbfs_disc) {
		d->lastError = EBADF;
		return;
	}

	d->m_wbfs_pos = 0;
}

/**
 * Get the disc image size.
 * @return Disc image size, or -1 on error.
 */
int64_t WbfsReader::size(void) const
{
	assert(d->file != nullptr);
	assert(d->m_wbfs != nullptr);
	assert(d->m_wbfs_disc != nullptr);
	if (!d->file || !d->m_wbfs || !d->m_wbfs_disc) {
		d->lastError = EBADF;
		return -1;
	}

	return d->disc_size;
}

}
