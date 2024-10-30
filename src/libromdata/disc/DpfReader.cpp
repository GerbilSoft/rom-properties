/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DpfReader.cpp: GameCube/Wii DPF/RPF sparse disc image reader.           *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: This class does NOT derive from DpfReader because
// DPF/RPF uses byte-granularity, not block-granularity.

#include "stdafx.h"
#include "config.librpbase.h"

#include "DpfReader.hpp"
#include "dpf_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;

// C++ STL classes
using std::unique_ptr;

namespace LibRomData {

class DpfReaderPrivate
{
public:
	explicit DpfReaderPrivate();

private:
	RP_DISABLE_COPY(DpfReaderPrivate)

public:
	// DPF/RPF header
	DpfHeader dpfHeader;

	// RPF entries (DPF entries are converted to RPF on load)
	rp::uvector<RpfEntry> entries;

public:
	// Disc size
	off64_t disc_size;

	// Current position
	off64_t pos;
};

/** DpfReaderPrivate **/

DpfReaderPrivate::DpfReaderPrivate()
	: disc_size(0)
	, pos(0)
{
	// Clear the DPF header struct.
	memset(&dpfHeader, 0, sizeof(dpfHeader));
}

/** DpfReader **/

DpfReader::DpfReader(const IRpFilePtr &file)
	: super(file)
	, d_ptr(new DpfReaderPrivate())
{
	if (!m_file) {
		// File could not be ref()'d.
		return;
	}

	// Read the DPF header.
	RP_D(DpfReader);
	m_file->rewind();
	size_t sz = m_file->read(&d->dpfHeader, sizeof(d->dpfHeader));
	if (sz != sizeof(d->dpfHeader)) {
		// Error reading the DPF header.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

	// Check the DPF magic.
	if (d->dpfHeader.magic != cpu_to_le32(DPF_MAGIC) &&
	    d->dpfHeader.magic != cpu_to_le32(RPF_MAGIC))
	{
		// Invalid magic.
		m_file.reset();
		m_lastError = EIO;
		return;
	}

#if SYS_BYTEORDER != SYS_LIL_ENDIAN
	// Byteswap the header.
	d->dpfHeader.magic		= le32_to_cpu(d->dpfHeader.magic);
	d->dpfHeader.version		= le32_to_cpu(d->dpfHeader.version);
	d->dpfHeader.header_size	= le32_to_cpu(d->dpfHeader.header_size);
	d->dpfHeader.unknown_0C		= le32_to_cpu(d->dpfHeader.unknown_0C);
	d->dpfHeader.entry_table_offset	= le32_to_cpu(d->dpfHeader.entry_table_offset);
	d->dpfHeader.entry_count	= le32_to_cpu(d->dpfHeader.entry_count);
	d->dpfHeader.data_offset	= le32_to_cpu(d->dpfHeader.data_offset);
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */

	// Allow up to 65,535 entries.
	assert(d->dpfHeader.entry_count > 0);
	assert(d->dpfHeader.entry_count <= 65535);
	if (d->dpfHeader.entry_count == 0) {
		// No entries...
		m_file.reset();
		m_lastError = EIO;
		return;
	} else if (d->dpfHeader.entry_count > 65535) {
		// Too many entries.
		m_file.reset();
		m_lastError = ENOMEM;
		return;
	}

	// Load the entry table.
	d->entries.resize(d->dpfHeader.entry_count);
	if (d->dpfHeader.magic == RPF_MAGIC) {
		// RPF: Load it directly.
		const size_t load_size = d->dpfHeader.entry_count * sizeof(RpfEntry);
		size_t size = m_file->seekAndRead(d->dpfHeader.entry_table_offset, d->entries.data(), load_size);
		if (size != load_size) {
			// Load error.
			d->entries.clear();
			m_file.reset();
			m_lastError = EIO;
			return;
		}

		// FIXME: If the first entry has virt=0 and phys!=0,
		// anything before that non-zero physical address should be
		// the *real* virt=0. Mostly affects RPFs.

#if SYS_BYTEORDER != SYS_LIL_ENDIAN
		// Value are stored in little-endian on disk.
		// Convert to host-endian.
		for (auto &p : d->entries) {
			p.virt_offset = le64_to_cpu(p.virt_offset);
			p.phys_offset = le64_to_cpu(p.phys_offset);
			p.size = le32_to_cpu(p.size);
			p.unknown_14 = le32_to_cpu(p.unknown_14);
		}
#endif /* SYS_BYTEORDER != SYS_LIL_ENDIAN */
	} else /*if (d->dpfHeader.magic == DPF_MAGIC)*/ {
		// DPF: Load into a buffer, then convert from DPF to RPF.
		unique_ptr<DpfEntry[]> dpf_entry_buf(new DpfEntry[d->dpfHeader.entry_count]);
		const size_t load_size = d->dpfHeader.entry_count * sizeof(DpfEntry);
		size_t size = m_file->seekAndRead(d->dpfHeader.entry_table_offset, dpf_entry_buf.get(), load_size);
		if (size != load_size) {
			// Load error.
			d->entries.clear();
			m_file.reset();
			m_lastError = EIO;
			return;
		}

		// TODO: Use pointer arithmetic?
		for (unsigned int i = 0; i < d->dpfHeader.entry_count; i++) {
			d->entries[i].virt_offset = static_cast<uint64_t>(le32_to_cpu(dpf_entry_buf[i].virt_offset));
			d->entries[i].phys_offset = static_cast<uint64_t>(le32_to_cpu(dpf_entry_buf[i].phys_offset));
			d->entries[i].size = le32_to_cpu(dpf_entry_buf[i].size);
			d->entries[i].unknown_14 = le32_to_cpu(dpf_entry_buf[i].unknown_0C);
		}
	}

	// Make sure entries are sorted by virtual address.
	// TODO: Remove zero-length entries?
	std::sort(d->entries.begin(), d->entries.end(),
		[](const RpfEntry &a, const RpfEntry &b) {
			return (a.virt_offset < b.virt_offset);
		});

	// The first entry should be virt=0, phys=0.
	// If it isn't, we'll need to adjust offsets in order to read the beginning of the disc.
	// TODO: Currently only virt=0, phys!=0.
	if (d->entries[0].virt_offset == 0 && d->entries[0].phys_offset != 0) {
		// Need to add an extra entry.
		const uint32_t entry_size = static_cast<uint32_t>(d->entries[0].phys_offset);
		const RpfEntry first_entry = {
			0,		// virt_offset
			0,		// phys_offset
			entry_size,	// size
			0		// unknown_14
		};
		d->entries.insert(d->entries.begin(), first_entry);

		// Adjust the virtual address for the remaining entries.
		for (auto &p : d->entries) {
			p.virt_offset += entry_size;
		}
	}

	// Disc size is the highest virtual address, plus size.
	const auto &last_entry = *(d->entries.crbegin());
	d->disc_size = last_entry.virt_offset + last_entry.size;

	// Reset the disc position.
	d->pos = 0;
}

/**
 * Is a disc image supported by this class?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific disc format ID (>= 0) if supported; -1 if not.
 */
int DpfReader::isDiscSupported_static(const uint8_t *pHeader, size_t szHeader)
{
	if (szHeader < sizeof(DpfHeader)) {
		// Not enough data to check.
		return -1;
	}

	// Check the DPF/RPF magic.
	const DpfHeader *const dpfHeader = reinterpret_cast<const DpfHeader*>(pHeader);
	if (dpfHeader->magic != cpu_to_le32(DPF_MAGIC) &&
	    dpfHeader->magic != cpu_to_le32(RPF_MAGIC))
	{
		// Invalid magic.
		return -1;
	}

	// Allow up to 65,535 entries.
	const uint32_t entry_count = le32_to_cpu(dpfHeader->entry_count);
	assert(entry_count > 0);
	assert(entry_count <= 65535);
	if (entry_count == 0 || entry_count > 65535) {
		// No entries, or too many entries.
		return -1;
	}

	// This is a valid DPF or RPF image.
	// TODO: More checks.
	return 0;
}

/**
 * Is a disc image supported by this object?
 * @param pHeader Disc image header.
 * @param szHeader Size of header.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int DpfReader::isDiscSupported(const uint8_t *pHeader, size_t szHeader) const
{
	return isDiscSupported_static(pHeader, szHeader);
}

/** IDiscReader functions **/

/**
 * Read data from the disc image.
 * @param ptr Output data buffer.
 * @param size Amount of data to read, in bytes.
 * @return Number of bytes read.
 */
size_t DpfReader::read(void *ptr, size_t size)
{
	RP_D(DpfReader);
	assert(m_file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	if (!m_file || d->disc_size <= 0 || d->pos < 0) {
		// Disc image wasn't initialized properly.
		m_lastError = EBADF;
		return -1;
	}

	uint8_t *ptr8 = static_cast<uint8_t*>(ptr);
	size_t ret = 0;

	// Are we already at the end of the disc?
	if (d->pos >= d->disc_size) {
		// End of the disc.
		return 0;
	}

	// Make sure d->pos + size <= d->disc_size.
	// If it isn't, we'll do a short read.
	if (d->pos + static_cast<off64_t>(size) >= d->disc_size) {
		size = static_cast<size_t>(d->disc_size - d->pos);
	}

	while (size > 0) {
		uint64_t virt_start = 0;
		off64_t phys_offset = -1;
		size_t virt_size = 0;

		// Find the sparse entry for the current position.
		// NOTE: Sparse entries are sorted by virtual offset.
		// TODO: Cache it for d->pos?
		for (const auto &p : d->entries) {
			if (p.size == 0)
				continue;

			if (d->pos < static_cast<off64_t>(p.virt_offset)) {
				// Requested position is before this entry. This means we don't have a valid entry...
				phys_offset = -1;
				break;
			}

			const off64_t virt_end = static_cast<off64_t>(p.virt_offset) + p.size;
			if (d->pos >= static_cast<off64_t>(p.virt_offset) && d->pos < virt_end) {
				// Requested position starts within this entry.
				virt_start = p.virt_offset;
				virt_size = static_cast<size_t>(p.size);
				phys_offset = p.phys_offset;

				// Seek to the current physical position.
				m_file->seek(phys_offset + (d->pos - virt_start) + d->dpfHeader.data_offset);
				break;
			}

			// Requested position is after this entry.
			// Keep going.
		}
		if (virt_size == 0) {
			// Section not found...
			break;
		}

		// Read up to virt_size bytes.
		if (phys_offset < 0) {
			// Zero section
			const size_t to_zero = std::min(size, virt_size);
			// FIXME: memset() here is causing an ICE in gcc-13.2.0.
			//memset(ptr8, 0, to_zero);
			//ptr8 += to_zero;
			for (size_t i = to_zero; i > 0; i--) {
				*ptr8++ = 0;
			}
			size -= to_zero;
			d->pos += to_zero;
			ret += to_zero;
		} else {
			// Data section
			size_t to_read = std::min(size, virt_size - static_cast<size_t>(d->pos - virt_start));
			size_t has_read = m_file->read(ptr8, to_read);
			size -= has_read;
			ptr8 += has_read;
			d->pos += has_read;
			ret += has_read;
			if (has_read != to_read) {
				// Short read?
				return ret;
			}
		}
	}

	// Finished reading the data.
	return ret;
}

/**
 * Set the disc image position.
 * @param pos disc image position.
 * @return 0 on success; -1 on error.
 */
int DpfReader::seek(off64_t pos)
{
	RP_D(DpfReader);
	assert(m_file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	if (!m_file || d->disc_size <= 0 || d->pos < 0) {
		// Disc image wasn't initialized properly.
		m_lastError = EBADF;
		return -1;
	}

	// Handle out-of-range cases.
	if (pos < 0) {
		// Negative is invalid.
		m_lastError = EINVAL;
		return -1;
	} else if (pos >= d->disc_size) {
		d->pos = d->disc_size;
	} else {
		d->pos = pos;
	}
	return 0;
}

/**
 * Get the disc image position.
 * @return Disc image position on success; -1 on error.
 */
off64_t DpfReader::tell(void)
{
	RP_D(const DpfReader);
	assert(m_file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	if (!m_file || d->disc_size <= 0 || d->pos < 0) {
		// Disc image wasn't initialized properly.
		m_lastError = EBADF;
		return -1;
	}

	return d->pos;
}

/**
 * Get the disc image size.
 * @return Disc image size, or -1 on error.
 */
off64_t DpfReader::size(void)
{
	RP_D(const DpfReader);
	assert(m_file != nullptr);
	assert(d->disc_size > 0);
	assert(d->pos >= 0);
	if (!m_file || d->disc_size <= 0 || d->pos < 0) {
		// Disc image wasn't initialized properly.
		m_lastError = EBADF;
		return -1;
	}

	return d->disc_size;
}

}
