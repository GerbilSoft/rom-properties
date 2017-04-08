/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WbfsReader.cpp: WBFS disc image reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
#include "SparseDiscReader_p.hpp"
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

class WbfsReaderPrivate : public SparseDiscReaderPrivate {
	public:
		WbfsReaderPrivate(WbfsReader *q, IRpFile *file);
		virtual ~WbfsReaderPrivate();

	private:
		typedef SparseDiscReaderPrivate super;
		RP_DISABLE_COPY(WbfsReaderPrivate)

	public:
		// WBFS structs.
		wbfs_t *m_wbfs;			// WBFS image.
		wbfs_disc_t *m_wbfs_disc;	// Current disc.

		const be16_t *wlba_table;	// Pointer to m_wbfs_disc->disc->header->wlba_table.

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
		int64_t getWbfsDiscSize(const wbfs_disc_t *disc) const;
};

/** WbfsReaderPrivate **/

// WBFS magic number.
const uint8_t WbfsReaderPrivate::WBFS_MAGIC[4] = {'W','B','F','S'};

WbfsReaderPrivate::WbfsReaderPrivate(WbfsReader *q, IRpFile *file)
	: super(q, file)
	, m_wbfs(nullptr)
	, m_wbfs_disc(nullptr)
	, wlba_table(nullptr)
{
	if (!this->file) {
		// File could not be dup()'d.
		return;
	}

	// Read the WBFS header.
	m_wbfs = readWbfsHeader();
	if (!m_wbfs) {
		// Error reading the WBFS header.
		delete this->file;
		this->file = nullptr;
		q->m_lastError = EIO;
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
		q->m_lastError = EIO;
		return;
	}

	// Save important values for later.
	wlba_table = m_wbfs_disc->header->wlba_table;
	block_size = m_wbfs->wbfs_sec_sz;
	pos = 0;	// Reset the read position.

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
}

// from libwbfs.c
// TODO: Optimize this?
static inline uint8_t size_to_shift(uint32_t size)
{
	uint8_t ret = 0;
	while (size) {
		ret++;
		size >>= 1;
	}
	return ret-1;
}

#define ALIGN_LBA(x) (((x)+p->hd_sec_sz-1)&(~(size_t)(p->hd_sec_sz-1)))

/**
 * Read the WBFS header.
 * @return Allocated wbfs_t on success; nullptr on error.
 */
wbfs_t *WbfsReaderPrivate::readWbfsHeader(void)
{
	// Assume 512-byte sectors initially.
	unsigned int hd_sec_sz = 512;
	wbfs_head_t *head = (wbfs_head_t*)malloc(hd_sec_sz);
	if (!head) {
		// ENOMEM
		return nullptr;
	}

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
	if (!p) {
		// ENOMEM
		free(head);
		return nullptr;
	}

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
		if (!head) {
			// ENOMEM
			free(p);
			return nullptr;
		}

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
	p->disc_info_sz = (uint16_t)ALIGN_LBA(sizeof(wbfs_disc_info_t) + p->n_wbfs_sec_per_disc*2);

	// Free blocks table.
	p->freeblks_lba = (p->wbfs_sec_sz - p->n_wbfs_sec/8) >> p->hd_sec_sz_s;
	p->freeblks = nullptr;
	p->max_disc = (p->freeblks_lba-1) / (p->disc_info_sz >> p->hd_sec_sz_s);
	if (p->max_disc > (p->hd_sec_sz - sizeof(wbfs_head_t)))
		p->max_disc = (uint16_t)(p->hd_sec_sz - sizeof(wbfs_head_t));

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
	const wbfs_head_t *const head = p->head;
	uint32_t count = 0;
	for (uint32_t i = 0; i < p->max_disc; i++) {
		if (head->disc_table[i]) {
			if (count++ == index) {
				// Found the disc table index.
				wbfs_disc_t *disc = (wbfs_disc_t*)malloc(sizeof(wbfs_disc_t));
				if (!disc) {
					// ENOMEM
					return nullptr;
				}
				disc->p = p;
				disc->i = i;

				// Read the disc header.
				disc->header = (wbfs_disc_info_t*)malloc(p->disc_info_sz);
				if (!disc->header) {
					// ENOMEM
					free(disc);
					return nullptr;
				}
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
int64_t WbfsReaderPrivate::getWbfsDiscSize(const wbfs_disc_t *disc) const
{
	// Find the last block that's used on the disc.
	// NOTE: This is in WBFS blocks, not Wii blocks.
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
	: super(new WbfsReaderPrivate(this, file))
{ }

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

/** SparseDiscReader functions. **/

/**
 * Read the specified block.
 *
 * This can read either a full block or a partial block.
 * For a full block, set pos = 0 and size = block_size.
 *
 * @param blockIdx	[in] Block index.
 * @param ptr		[out] Output data buffer.
 * @param pos		[in] Starting position. (Must be >= 0 and <= the block size!)
 * @param size		[in] Amount of data to read, in bytes. (Must be <= the block size!)
 * @return Number of bytes read, or -1 if the block index is invalid.
 */
int WbfsReader::readBlock(uint32_t blockIdx, void *ptr, int pos, size_t size)
{
	// Read 'size' bytes of block 'blockIdx', starting at 'pos'.
	// NOTE: This can only be called by SparseDiscReader,
	// so the main assertions are already checked there.
	RP_D(WbfsReader);
	assert(pos >= 0 && pos < (int)d->block_size);
	assert(size <= d->block_size);
	// TODO: Make sure overflow doesn't occur.
	assert((int64_t)(pos + size) <= (int64_t)d->block_size);
	if (pos < 0 || (int64_t)(pos + size) > (int64_t)d->block_size)
	{
		// pos+size is out of range.
		return -1;
	}

	// Get the physical block number first.
	assert(blockIdx < d->m_wbfs_disc->p->n_wbfs_sec_per_disc);
	if (blockIdx >= d->m_wbfs_disc->p->n_wbfs_sec_per_disc) {
		// Out of range.
		return -1;
	}

	const unsigned int physBlockIdx = be16_to_cpu(d->wlba_table[blockIdx]);
	if (physBlockIdx == 0) {
		// Empty block.
		memset(ptr, 0, size);
		return (int)size;
	}

	// Go to the block.
	const int64_t phys_pos = ((int64_t)physBlockIdx * d->block_size) + pos;
	int ret = d->file->seek(phys_pos);
	if (ret != 0) {
		// Seek failed.
		m_lastError = d->file->lastError();
		return -1;
	}

	// Read the first 'size' bytes of the block.
	ret = (int)d->file->read(ptr, size);
	m_lastError = d->file->lastError();
	return ret;
}

}
