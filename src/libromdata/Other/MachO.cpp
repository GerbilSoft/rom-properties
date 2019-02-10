/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MachO.cpp: Mach-O executable format.                                    *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "MachO.hpp"
#include "librpbase/RomData_p.hpp"

#include "data/MachOData.hpp"
#include "macho_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <string>
#include <vector>
using std::string;
using std::vector;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

ROMDATA_IMPL(MachO)

class MachOPrivate : public LibRpBase::RomDataPrivate
{
	public:
		MachOPrivate(MachO *q, LibRpBase::IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(MachOPrivate)

	public:
		// Executable format.
		enum Exec_Format {
			EXEC_FORMAT_UNKNOWN	= -1,
			EXEC_FORMAT_MACH	= 0,
			EXEC_FORMAT_FAT		= 1,
		};
		int execFormat;

		// Mach-O format.
		enum Mach_Format {
			MACH_FORMAT_UNKNOWN	= -1,
			MACH_FORMAT_32LSB	= 0,
			MACH_FORMAT_64LSB	= 1,
			MACH_FORMAT_32MSB	= 2,
			MACH_FORMAT_64MSB	= 3,

			// Host/swap endian formats.

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
			MACH_FORMAT_32HOST	= MACH_FORMAT_32LSB,
			MACH_FORMAT_64HOST	= MACH_FORMAT_64LSB,
			MACH_FORMAT_32SWAP	= MACH_FORMAT_32MSB,
			MACH_FORMAT_64SWAP	= MACH_FORMAT_64MSB,
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
			MACH_FORMAT_32HOST	= MACH_FORMAT_32MSB,
			MACH_FORMAT_64HOST	= MACH_FORMAT_64MSB,
			MACH_FORMAT_32SWAP	= MACH_FORMAT_32LSB,
			MACH_FORMAT_64SWAP	= MACH_FORMAT_64LSB,
#endif

			MACHO_FORMAT_MAX
		};

		// Maximum number of Mach-O headers to read.
		#define MAX_MACH_HEADERS 16U

		// Mach-O formats and headers.
		vector<int8_t> machFormats;
		ao::uvector<mach_header> machHeaders;

		/**
		 * Check the Mach-O magic number.
		 * @param magic Magic number as read directly from disk.
		 * @return Mach_Format value.
		 */
		static int checkMachMagicNumber(uint32_t magic);
};

/** MachOPrivate **/

MachOPrivate::MachOPrivate(MachO *q, IRpFile *file)
	: super(q, file)
	, execFormat(EXEC_FORMAT_UNKNOWN)
{ }

/**
 * Check the Mach-O magic number.
 * @param magic Magic number as read directly from disk.
 * @return Mach_Format value.
 */
int MachOPrivate::checkMachMagicNumber(uint32_t magic)
{
	// NOTE: Checking in order of Mac OS X usage as of 2019.
	int ret = MACH_FORMAT_UNKNOWN;
	if (magic == cpu_to_le32(MH_MAGIC_64)) {
		// LE64 magic number.
		ret = MACH_FORMAT_64LSB;
	} else if (magic == cpu_to_le32(MH_MAGIC)) {
		// LE32 magic number.
		ret = MACH_FORMAT_32LSB;
	} else if (magic == cpu_to_be32(MH_MAGIC)) {
		// BE32 magic number.
		ret = MACH_FORMAT_32MSB;
	} else if (magic == cpu_to_be32(MH_MAGIC_64)) {
		// BE64 magic number.
		ret = MACH_FORMAT_64MSB;
	}
	return ret;
}

/** MachO **/

/**
 * Read a Mach-O executable.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
MachO::MachO(IRpFile *file)
	: super(new MachOPrivate(this, file))
{
	// This class handles different types of files.
	// d->fileType will be set later.
	RP_D(MachO);
	d->className = "MachO";
	d->fileType = FTYPE_UNKNOWN;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the file header.
	// - Mach-O header: 7 DWORDs
	// - Universal header: 2 DWORDs, plus 5 DWORDs per architecture.
	// Assuming up to 16 architectures, read 2+(5*16) = 82 DWORDs, or 328 bytes.
	uint8_t header[(2+(5*MAX_MACH_HEADERS))*sizeof(uint32_t)];
	d->file->rewind();
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this executable is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for ELF.
	info.szFile = 0;	// Not needed for ELF.
	d->execFormat = isRomSupported_static(&info);

	// Load the Mach header.
	switch (d->execFormat) {
		case MachOPrivate::EXEC_FORMAT_MACH:
			// Standard Mach executable.
			d->machFormats.resize(1);
			d->machHeaders.resize(1);
			memcpy(&d->machHeaders[0], header, sizeof(mach_header));
			d->machFormats[0] = d->checkMachMagicNumber(d->machHeaders[0].magic);
			d->isValid = (d->machFormats[0] >= 0);
			break;

		case MachOPrivate::EXEC_FORMAT_FAT: {
			// Read up to 16 architectures.
			const fat_header *const fatHeader =
				reinterpret_cast<const fat_header*>(info.header.pData);
			const unsigned int nfat_arch = std::min<uint32_t>(
				MAX_MACH_HEADERS, be32_to_cpu(fatHeader->nfat_arch));
			d->machFormats.reserve(nfat_arch);
			d->machHeaders.reserve(nfat_arch);

			const fat_arch *fatArch =
				reinterpret_cast<const fat_arch*>(info.header.pData + 8);
			for (unsigned int i = 0; i < nfat_arch; i++, fatArch++) {
				const uint32_t offset = be32_to_cpu(fatArch->offset);
				if (offset < sizeof(fat_header)) {
					continue;
				}

				size_t idx = d->machFormats.size();
				d->machFormats.resize(idx+1);
				d->machHeaders.resize(idx+1);
				size_t size = d->file->seekAndRead(offset, &d->machHeaders[i], sizeof(mach_header));
				if (size == sizeof(mach_header)) {
					d->machFormats[idx] = d->checkMachMagicNumber(d->machHeaders[idx].magic);
				} else {
					// Unable to read this header.
					// TODO: Show an error?
					d->machFormats.resize(idx);
					d->machHeaders.resize(idx);
				}
			}

			d->isValid = !d->machFormats.empty();
			break;
		}

		default:
			// Not supported.
			d->isValid = false;
			break;
	}

	if (d->machFormats.empty() || d->machHeaders.empty()) {
		// No headers...
		d->isValid = false;
	}

	if (!d->isValid) {
		d->execFormat = MachOPrivate::EXEC_FORMAT_UNKNOWN;
		d->machFormats.clear();
		d->machHeaders.clear();
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Swap endianness if needed.
	assert(d->machFormats.size() == d->machHeaders.size());
	auto hdrIter = d->machHeaders.begin();
	for (auto fmtIter = d->machFormats.cbegin();
	     fmtIter != d->machFormats.cend(); ++fmtIter, ++hdrIter)
	{
		switch (*fmtIter) {
			default:
				// Invalid format. Continue anyway...
				break;

			case MachOPrivate::MACH_FORMAT_32HOST:
			case MachOPrivate::MACH_FORMAT_64HOST:
				// Host-endian. Nothing to do.
				break;

			case MachOPrivate::MACH_FORMAT_32SWAP:
			case MachOPrivate::MACH_FORMAT_64SWAP: {
				// Swapped endian.
				// NOTE: Not swapping the magic number.
				mach_header *const machHeader = &(*hdrIter);
				machHeader->cputype	= __swab32(machHeader->cputype);
				machHeader->cpusubtype	= __swab32(machHeader->cpusubtype);
				machHeader->filetype	= __swab32(machHeader->filetype);
				machHeader->ncmds	= __swab32(machHeader->ncmds);
				machHeader->sizeofcmds	= __swab32(machHeader->sizeofcmds);
				machHeader->flags	= __swab32(machHeader->flags);
				break;
			}
		}
	}

	// Determine the file type.
	// NOTE: This assumes all architectures have the same file type.
	switch (d->machHeaders[0].filetype) {
		default:
			// Should not happen...
			d->fileType = FTYPE_UNKNOWN;
			break;
		case MH_OBJECT:
			d->fileType = FTYPE_RELOCATABLE_OBJECT;
			break;
		case MH_EXECUTE:
		case MH_PRELOAD:
			// TODO: Special FTYPE for MH_PRELOAD?
			d->fileType = FTYPE_EXECUTABLE;
			break;
		case MH_FVMLIB:
			// "Fixed VM" library file.
			// TODO: Add a separate FTYPE?
			d->fileType = FTYPE_SHARED_LIBRARY;
			break;
		case MH_CORE:
			d->fileType = FTYPE_CORE_DUMP;
			break;
		case MH_DYLIB:
			d->fileType = FTYPE_SHARED_LIBRARY;
			break;
		case MH_DYLINKER:
			// TODO
			d->fileType = FTYPE_UNKNOWN;
			break;
		case MH_BUNDLE:
			d->fileType = FTYPE_BUNDLE;
			break;
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int MachO::isRomSupported_static(const DetectInfo *info)
{
	// Read the file header.
	// - Mach-O header: 7 DWORDs
	// - Universal header: 2 DWORDs, plus 5 DWORDs per architecture.
	// Only the first two DWORDs are needed for identification.
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 8)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const uint32_t *const pu32 =
		reinterpret_cast<const uint32_t*>(info->header.pData);

	// Check the magic number.
	// NOTE: Checking in order of Mac OS X usage as of 2019.
	if (pu32[0] == cpu_to_be32(FAT_MAGIC)) {
		// Universal binary.
		// Note that this is the same magic number as Java classes,
		// so check the second value (number of architectures)
		// to verify. We're assuming a maximum of 16 architectures
		// per executable.
		if (be32_to_cpu(pu32[1]) <= 16) {
			// Mach-O universal binary.
			return MachOPrivate::EXEC_FORMAT_FAT;
		}
	} else if (MachOPrivate::checkMachMagicNumber(pu32[0]) !=
		   MachOPrivate::MACH_FORMAT_UNKNOWN)
	{
		// Mach-O executable.
		return MachOPrivate::EXEC_FORMAT_MACH;
	}

	// Not supported.
	return MachOPrivate::EXEC_FORMAT_UNKNOWN;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *MachO::systemName(unsigned int type) const
{
	RP_D(const MachO);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Mach-O has the same name worldwide, so we can
	// ignore the region selection.
	// TODO: Identify the OS, or list that in the fields instead?
	static_assert(SYSNAME_TYPE_MASK == 3,
		"MachO::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Mach Microkernel", "Mach", "Mach", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *MachO::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		//".",		// FIXME: Does this work for files with no extension?
		".bin",
		".so",		// Shared libraries. (TODO: Versioned .so files.)
		".dylib"	// Dynamic libraries. (TODO: Versioned .dylib files.)
		".bundle",	// Bundles.
		// TODO: More?

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *MachO::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.

		// FIXME: Defining the magic numbers for Mach-O
		// executables in rom-properties.xml causes
		// KDE to lock up due to a conflict with the
		// standard definitions. Hence, we're using our
		// own types.

		// TODO: Upstream the Mach-O definitions.

		"application/x-mach-executable",
		"application/x-mach-sharedlib",
		"application/x-mach-core",
		"application/x-mach-bundle",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int MachO::loadFieldData(void)
{
	RP_D(MachO);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unsupported file.
		return -EIO;
	}

	// Mach-O header.
	// TODO: Show multiple headers.
	if (d->machHeaders.empty()) {
		// No headers at all...
		return 0;
	}

	// Maximum of 4 fields per architecture.
	const int n_tabs = static_cast<int>(d->machHeaders.size());
	d->fields->reserve(4*n_tabs);
	d->fields->reserveTabs(n_tabs);

	auto fmtIter = d->machFormats.cbegin();
	int i = 0;
	for (auto hdrIter = d->machHeaders.cbegin();
	     hdrIter != d->machHeaders.cend(); ++hdrIter, ++fmtIter, i++)
	{
		const mach_header *const machHeader = &(*hdrIter);
		const int machFormat = *fmtIter;

		// Use the CPU name for the tab title.
		const char *const s_cpu = MachOData::lookup_cpu_type(machHeader->cputype);

		// TODO: Change addTab() behavior to set the first tab's name?
		if (i == 0) {
			if (s_cpu) {
				d->fields->setTabName(i, s_cpu);
			} else {
				d->fields->setTabName(i,
					rp_sprintf("0x%08X", machHeader->cputype).c_str());
			}
		} else {
			if (s_cpu) {
				d->fields->addTab(s_cpu);
			} else {
				d->fields->addTab(rp_sprintf("0x%08X", machHeader->cputype).c_str());
			}
		}

		// Executable format.
		static const char *const exec_type_tbl[] = {
			NOP_C_("RomData|ExecType", "32-bit Little-Endian"),
			NOP_C_("RomData|ExecType", "64-bit Little-Endian"),
			NOP_C_("RomData|ExecType", "32-bit Big-Endian"),
			NOP_C_("RomData|ExecType", "64-bit Big-Endian"),
		};
		const char *const format_title = C_("MachO", "Format");
		if (machFormat > MachOPrivate::MACH_FORMAT_UNKNOWN &&
		    machFormat < ARRAY_SIZE(exec_type_tbl))
		{
			d->fields->addField_string(format_title,
				dpgettext_expr(RP_I18N_DOMAIN, "RomData|ExecType", exec_type_tbl[machFormat]));
		} else {
			// TODO: Show individual values.
			// NOTE: This shouldn't happen...
			d->fields->addField_string(format_title,
				C_("RomData", "Unknown"));
		}

		// CPU type.
		const char *const cpu_title = C_("MachO", "CPU");
		if (s_cpu) {
			d->fields->addField_string(cpu_title, s_cpu);
		} else {
			d->fields->addField_string(cpu_title,
				rp_sprintf(C_("ELF", "Unknown (%u)"), machHeader->cputype & 0xFFFFFF));
		}

		// CPU subtype.
		const char *const s_cpu_subtype = MachOData::lookup_cpu_subtype(
			machHeader->cputype, machHeader->cpusubtype);
		if (s_cpu_subtype) {
			d->fields->addField_string(C_("MachO", "CPU Subtype"), s_cpu_subtype);
		}

		// Flags.
		// I/O support bitfield.
		static const char *const flags_bitfield_names[] = {
			// 0x00000000
			"NoUndefs", "IncrLink", "DyldLink", "BindAtLoad",
			// 0x00000010
			"Prebound", "SplitSegs", "LazyInit", "TwoLevel",
			// 0x00000100
			"ForceFlat", "NoMultiDefs", "NoFixPrebinding", "Prebindable",
			// 0x00001000
			"AllModsBound", "Subsections", "Canonical", "WeakDefines",
			// 0x00010000
			"BindsToWeak", "StackExec", "RootSafe", "SetuidSafe",
			// 0x00100000
			"NoReexport", "PIE", "DeadStrip", "TLVDescriptors",
			// 0x01000000
			"NoHeapExec", "AppExtSafe"
		};
		vector<string> *const v_flags_bitfield_names = RomFields::strArrayToVector(
			flags_bitfield_names, ARRAY_SIZE(flags_bitfield_names));
		d->fields->addField_bitfield(C_("MachO", "Flags"),
			v_flags_bitfield_names, 3, machHeader->flags);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

}
