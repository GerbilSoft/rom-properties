/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ELF.cpp: Executable and Linkable Format reader.                         *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "ELF.hpp"
#include "librpbase/RomData_p.hpp"

#include "data/ELFData.hpp"
#include "elf_structs.h"

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

// cinttypes was added in MSVC 2013.
// For older versions, we'll need to manually define PRIX64.
// TODO: Split into a separate header file?
#if !defined(_MSC_VER) || _MSC_VER >= 1800
# include <cinttypes>
#else
# ifndef PRIx64
#  define PRIx64 "I64x"
# endif
# ifndef PRIX64
#  define PRIX64 "I64X"
# endif
#endif

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRomData {

ROMDATA_IMPL(ELF)

class ELFPrivate : public LibRpBase::RomDataPrivate
{
	public:
		ELFPrivate(ELF *q, LibRpBase::IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(ELFPrivate)

	public:
		// ELF format.
		enum Elf_Format {
			ELF_FORMAT_UNKNOWN	= -1,
			ELF_FORMAT_32LSB	= 0,
			ELF_FORMAT_64LSB	= 1,
			ELF_FORMAT_32MSB	= 2,
			ELF_FORMAT_64MSB	= 3,

			// Host/swap endian formats.

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
			#define ELFDATAHOST ELFDATA2LSB
			ELF_FORMAT_32HOST	= ELF_FORMAT_32LSB,
			ELF_FORMAT_64HOST	= ELF_FORMAT_64LSB,
			ELF_FORMAT_32SWAP	= ELF_FORMAT_32MSB,
			ELF_FORMAT_64SWAP	= ELF_FORMAT_64MSB,
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
			#define ELFDATAHOST ELFDATA2MSB
			ELF_FORMAT_32HOST	= ELF_FORMAT_32MSB,
			ELF_FORMAT_64HOST	= ELF_FORMAT_64MSB,
			ELF_FORMAT_32SWAP	= ELF_FORMAT_32LSB,
			ELF_FORMAT_64SWAP	= ELF_FORMAT_64LSB,
#endif

			ELF_FORMAT_MAX
		};
		int elfFormat;

		// ELF header.
		union {
			Elf_PrimaryEhdr primary;
			Elf32_Ehdr elf32;
			Elf64_Ehdr elf64;
		} Elf_Header;

		// Program Header information.
		bool hasCheckedPH;	// Have we checked program headers yet?
		bool isPie;		// Is this a position-independent executable?
		bool isDynamic;		// Is this program dynamically linked?
		bool isWiiU;		// Is this a Wii U executable?

		string interpreter;	// PT_INTERP value

		// Section Header information.
		bool hasCheckedSH;	// Have we checked section headers yet?
		string osVersion;	// Operating system version.

		ao::uvector<uint8_t> build_id;	// GNU `ld` build ID. (raw data)
		const char *build_id_type;	// Build ID type.

		/**
		 * Byteswap a uint32_t value from ELF to CPU.
		 * @param x Value to swap.
		 * @return Swapped value.
		 */
		inline uint32_t elf32_to_cpu(uint32_t x);

		/**
		 * Check program headers.
		 * @return 0 on success; non-zero on error.
		 */
		int checkProgramHeaders(void);

		/**
		 * Check section headers.
		 * @return 0 on success; non-zero on error.
		 */
		int checkSectionHeaders(void);
};

/** ELFPrivate **/

ELFPrivate::ELFPrivate(ELF *q, IRpFile *file)
	: super(q, file)
	, elfFormat(ELF_FORMAT_UNKNOWN)
	, hasCheckedPH(false)
	, isPie(false)
	, isDynamic(false)
	, isWiiU(false)
	, hasCheckedSH(false)
	, build_id_type(nullptr)
{
	// Clear the structs.
	memset(&Elf_Header, 0, sizeof(Elf_Header));
}

/**
 * Byteswap a uint32_t value from ELF to CPU.
 * @param x Value to swap.
 * @return Swapped value.
 */
inline uint32_t ELFPrivate::elf32_to_cpu(uint32_t x)
{
	if (Elf_Header.primary.e_data == ELFDATAHOST) {
		return x;
	} else {
		return __swab32(x);
	}

	// Should not get here...
	assert(!"Should not get here...");
	return ~0;
}

/**
 * Check program headers.
 * @return 0 on success; non-zero on error.
 */
int ELFPrivate::checkProgramHeaders(void)
{
	if (hasCheckedPH) {
		// Already checked.
		return 0;
	}

	// Now checking...
	hasCheckedPH = true;

	// Read the program headers.
	// PIE executables have a PT_INTERP header.
	// Shared libraries do not.
	// (NOTE: glibc's libc.so.6 *does* have PT_INTERP...)
	int64_t e_phoff;
	unsigned int e_phnum;
	unsigned int phsize;
	uint8_t phbuf[sizeof(Elf64_Phdr)];

	if (Elf_Header.primary.e_class == ELFCLASS64) {
		e_phoff = (int64_t)Elf_Header.elf64.e_phoff;
		e_phnum = Elf_Header.elf64.e_phnum;
		phsize = sizeof(Elf64_Phdr);
	} else {
		e_phoff = (int64_t)Elf_Header.elf32.e_phoff;
		e_phnum = Elf_Header.elf32.e_phnum;
		phsize = sizeof(Elf32_Phdr);
	}

	if (e_phoff == 0 || e_phnum == 0) {
		// No program headers. Can't determine anything...
		return 0;
	}

	int ret = file->seek(e_phoff);
	if (ret != 0) {
		// Seek error.
		return ret;
	}

	// Read all of the program header entries.
	const bool isHostEndian = (Elf_Header.primary.e_data == ELFDATAHOST);
	for (; e_phnum > 0; e_phnum--) {
		size_t size = file->read(phbuf, phsize);
		if (size != phsize) {
			// Read error.
			break;
		}

		// Check the type.
		uint32_t p_type;
		memcpy(&p_type, phbuf, sizeof(p_type));
		if (!isHostEndian) {
			p_type = __swab32(p_type);
		}

		switch (p_type) {
			case PT_INTERP: {
				// If the file type is ET_DYN, this is a PIE executable.
				isPie = (Elf_Header.primary.e_type == ET_DYN);

				// Get the interpreter name.
				int64_t int_addr;
				uint64_t int_size;
				if (Elf_Header.primary.e_class == ELFCLASS64) {
					const Elf64_Phdr *const phdr = reinterpret_cast<const Elf64_Phdr*>(phbuf);
					if (Elf_Header.primary.e_data == ELFDATAHOST) {
						int_addr = phdr->p_offset;
						int_size = phdr->p_filesz;
					} else {
						int_addr = __swab64(phdr->p_offset);
						int_size = __swab64(phdr->p_filesz);
					}
				} else {
					const Elf32_Phdr *const phdr = reinterpret_cast<const Elf32_Phdr*>(phbuf);
					if (Elf_Header.primary.e_data == ELFDATAHOST) {
						int_addr = phdr->p_offset;
						int_size = phdr->p_filesz;
					} else {
						int_addr = __swab32(phdr->p_offset);
						int_size = __swab32(phdr->p_filesz);
					}
				}

				// Sanity check: Interpreter must be 256 characters or less.
				// NOTE: Interpreter should be NULL-terminated.
				if (int_size <= 256) {
					char buf[256];
					const int64_t prevoff = file->tell();
					size = file->seekAndRead(int_addr, buf, int_size);
					if (size != int_size) {
						// Seek and/or read error.
						return -EIO;
					}
					ret = file->seek(prevoff);
					if (ret != 0) {
						// Seek error.
						return ret;
					}

					// Remove trailing NULLs.
					while (int_size > 0 && buf[int_size-1] == 0) {
						int_size--;
					}

					if (int_size > 0) {
						interpreter.assign(buf, int_size);
					}
				}

				break;
			}

			case PT_DYNAMIC:
				// Executable is dynamically linked.
				isDynamic = true;
				break;

			default:
				break;
		}
	}

	// Program headers checked.
	return 0;
}

/**
 * Check section headers.
 * @return 0 on success; non-zero on error.
 */
int ELFPrivate::checkSectionHeaders(void)
{
	if (hasCheckedSH) {
		// Already checked.
		return 0;
	}

	// Now checking...
	hasCheckedSH = true;

	// Read the section headers.
	int64_t e_shoff;
	unsigned int e_shnum;
	unsigned int shsize;
	uint8_t shbuf[sizeof(Elf64_Shdr)];

	if (Elf_Header.primary.e_class == ELFCLASS64) {
		e_shoff = (int64_t)Elf_Header.elf64.e_shoff;
		e_shnum = Elf_Header.elf64.e_shnum;
		shsize = sizeof(Elf64_Shdr);
	} else {
		e_shoff = (int64_t)Elf_Header.elf32.e_shoff;
		e_shnum = Elf_Header.elf32.e_shnum;
		shsize = sizeof(Elf32_Shdr);
	}

	if (e_shoff == 0 || e_shnum == 0) {
		// No section headers. Can't determine anything...
		return 0;
	}

	int ret = file->seek(e_shoff);
	if (ret != 0) {
		// Seek error.
		return ret;
	}

	// Read all of the section header entries.
	const bool isHostEndian = (Elf_Header.primary.e_data == ELFDATAHOST);
	for (; e_shnum > 0; e_shnum--) {
		size_t size = file->read(shbuf, shsize);
		if (size != shsize) {
			// Read error.
			break;
		}

		// Check the type.
		uint32_t s_type;
		memcpy(&s_type, &shbuf[4], sizeof(s_type));
		if (!isHostEndian) {
			s_type = __swab32(s_type);
		}

		// Only NOTEs are supported right now.
		if (s_type != SHT_NOTE)
			continue;

		// Get the note address and size.
		int64_t int_addr;
		uint64_t int_size;
		if (Elf_Header.primary.e_class == ELFCLASS64) {
			const Elf64_Shdr *const shdr = reinterpret_cast<const Elf64_Shdr*>(shbuf);
			if (Elf_Header.primary.e_data == ELFDATAHOST) {
				int_addr = shdr->sh_offset;
				int_size = shdr->sh_size;
			} else {
				int_addr = __swab64(shdr->sh_offset);
				int_size = __swab64(shdr->sh_size);
			}
		} else {
			const Elf32_Shdr *const shdr = reinterpret_cast<const Elf32_Shdr*>(shbuf);
			if (Elf_Header.primary.e_data == ELFDATAHOST) {
				int_addr = shdr->sh_offset;
				int_size = shdr->sh_size;
			} else {
				int_addr = __swab32(shdr->sh_offset);
				int_size = __swab32(shdr->sh_size);
			}
		}

		// Sanity check: Note must be 256 bytes or less,
		// and must be greater than sizeof(Elf32_Nhdr).
		// NOTE: Elf32_Nhdr and Elf64_Nhdr are identical.
		if (int_size < sizeof(Elf32_Nhdr) || int_size > 256) {
			// Out of range. Ignore it.
			continue;
		}

		uint8_t buf[256];
		const int64_t prevoff = file->tell();
		size = file->seekAndRead(int_addr, buf, int_size);
		if (size != int_size) {
			// Seek and/or read error.
			return -EIO;
		}
		ret = file->seek(prevoff);
		if (ret != 0) {
			// Seek error.
			return ret;
		}

		// Parse the note.
		Elf32_Nhdr *const nhdr = reinterpret_cast<Elf32_Nhdr*>(buf);
		if (Elf_Header.primary.e_data != ELFDATAHOST) {
			// Byteswap the fields.
			nhdr->n_namesz = __swab32(nhdr->n_namesz);
			nhdr->n_descsz = __swab32(nhdr->n_descsz);
			nhdr->n_type   = __swab32(nhdr->n_type);
		}

		if (nhdr->n_namesz == 0 || nhdr->n_descsz == 0) {
			// No name or description...
			continue;
		}

		if (int_size < sizeof(Elf32_Nhdr) + nhdr->n_namesz + nhdr->n_descsz) {
			// Section is too small.
			continue;
		}

		const char *const pName = reinterpret_cast<const char*>(&buf[sizeof(Elf32_Nhdr)]);
		const uint8_t *const pData = &buf[sizeof(Elf32_Nhdr) + nhdr->n_namesz];
		switch (nhdr->n_type) {
			case NT_GNU_ABI_TAG:
				// GNU ABI tag.
				if (nhdr->n_namesz == 5 && !strcmp(pName, "SuSE")) {
					// SuSE Linux
					if (nhdr->n_descsz < 2) {
						// Header is too small...
						break;
					}
					osVersion = rp_sprintf("SuSE Linux %u.%u", pData[0], pData[1]);
				} else if (nhdr->n_namesz == 4 && !strcmp(pName, ELF_NOTE_GNU)) {
					// GNU system
					if (nhdr->n_descsz < sizeof(uint32_t)*4) {
						// Header is too small...
						break;
					}
					uint32_t desc[4];
					memcpy(desc, pData, sizeof(desc));

					const uint32_t os_id = elf32_to_cpu(desc[0]);
					const char *const os_tbl[] = {
						"Linux", "Hurd", "Solaris", "kFreeBSD", "kNetBSD"
					};

					const char *s_os;
					if (os_id < ARRAY_SIZE(os_tbl)) {
						s_os = os_tbl[os_id];
					} else {
						s_os = "<unknown>";
					}

					osVersion = rp_sprintf("GNU/%s %u.%u.%u",
						s_os, elf32_to_cpu(desc[1]),
						elf32_to_cpu(desc[2]), elf32_to_cpu(desc[3]));
				} else if (nhdr->n_namesz == 7 && !strcmp(pName, "NetBSD")) {
					// Check if the version number is valid.
					// Older versions kept this as 199905.
					// Newer versions use __NetBSD_Version__.
					if (nhdr->n_descsz < sizeof(uint32_t)) {
						// Header is too small...
						break;
					}

					uint32_t desc;
					memcpy(&desc, pData, sizeof(desc));
					desc = elf32_to_cpu(desc);

					if (desc > 100000000U) {
						const uint32_t ver_patch = (desc / 100) % 100;
						uint32_t ver_rel = (desc / 10000) % 100;
						const uint32_t ver_min = (desc / 1000000) % 100;
						const uint32_t ver_maj = desc / 100000000;
						osVersion = rp_sprintf("NetBSD %u.%u", ver_maj, ver_min);
						if (ver_rel == 0 && ver_patch != 0) {
							osVersion += rp_sprintf(".%u", ver_patch);
						} else if (ver_rel != 0) {
							while (ver_rel > 26) {
								osVersion += 'Z';
								ver_rel -= 26;
							}
							osVersion += ('A' + ver_rel - 1);
						}
					} else {
						// No version number.
						osVersion = "NetBSD";
					}
				} else if (nhdr->n_namesz == 8 && !strcmp(pName, "FreeBSD")) {
					if (nhdr->n_descsz < sizeof(uint32_t)) {
						// Header is too small...
						break;
					}

					uint32_t desc;
					memcpy(&desc, pData, sizeof(desc));
					desc = elf32_to_cpu(desc);

					if (desc == 460002) {
						osVersion = "FreeBSD 4.6.2";
					} else if (desc < 460100) {
						osVersion = rp_sprintf("FreeBSD %u.%u",
							desc / 100000, desc / 10000 % 10);
						if (desc / 1000 % 10 > 0) {
							osVersion += rp_sprintf(".%u", desc / 1000 % 10);
						}
						if ((desc % 1000 > 0) || (desc % 100000 == 0)) {
							osVersion += rp_sprintf(" (%u)", desc);
						}
					} else if (desc < 500000) {
						osVersion = rp_sprintf("FreeBSD %u.%u",
							desc / 100000, desc / 10000 % 10 + desc / 1000 % 10);
						if (desc / 100 % 10 > 0) {
							osVersion += rp_sprintf(" (%u)", desc);
						} else if (desc / 10 % 10 > 0) {
							osVersion += rp_sprintf(".%u", desc / 10 % 10);
						}
					} else {
						osVersion = rp_sprintf("FreeBSD %u.%u",
							desc / 100000, desc / 1000 % 100);
						if ((desc / 100 % 10 > 0) || (desc % 100000 / 100 == 0)) {
							osVersion += rp_sprintf(" (%u)", desc);
						} else if (desc / 10 % 10 > 0) {
							osVersion += rp_sprintf(".%u", desc / 10 % 10);
						}
					}
				} else if (nhdr->n_namesz == 8 && !strcmp(pName, "OpenBSD")) {
					osVersion = "OpenBSD";
				} else if (nhdr->n_namesz == 10 && !strcmp(pName, "DragonFly")) {
					if (nhdr->n_descsz < sizeof(uint32_t)) {
						// Header is too small...
						break;
					}

					uint32_t desc;
					memcpy(&desc, pData, sizeof(desc));
					desc = elf32_to_cpu(desc);

					osVersion = rp_sprintf("DragonFlyBSD %u.%u.%u",
						desc / 100000, desc / 10000 % 10, desc % 10000);
				}
				break;

			case NT_GNU_BUILD_ID:
				if (nhdr->n_namesz != 4 || strcmp(pName, ELF_NOTE_GNU) != 0) {
					// Not a GNU note.
					break;
				}

				// Build ID.
				switch (nhdr->n_descsz) {
					case 8:
						build_id_type = "xxHash";
						break;
					case 16:
						build_id_type = "md5/uuid";
						break;
					case 20:
						build_id_type = "sha1";
						break;
					default:
						build_id_type = nullptr;
						break;
				}

				// Hexdump will be done when parsing the data.
				build_id.resize(nhdr->n_descsz);
				memcpy(build_id.data(), pData, nhdr->n_descsz);
				break;

			default:
				break;
		}
	}

	// Program headers checked.
	return 0;
}

/** ELF **/

/**
 * Read an ELF executable.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
ELF::ELF(IRpFile *file)
	: super(new ELFPrivate(this, file))
{
	// This class handles different types of files.
	// d->fileType will be set later.
	RP_D(ELF);
	d->className = "ELF";
	d->fileType = FTYPE_UNKNOWN;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Assume this is a 64-bit ELF executable and read a 64-bit header.
	// 32-bit executables have a smaller header, but they should have
	// more data than just the header.
	d->file->rewind();
	size_t size = d->file->read(&d->Elf_Header, sizeof(d->Elf_Header));
	if (size != sizeof(d->Elf_Header))
		return;

	// Check if this executable is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->Elf_Header);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->Elf_Header);
	info.ext = nullptr;	// Not needed for ELF.
	info.szFile = 0;	// Not needed for ELF.
	d->elfFormat = isRomSupported_static(&info);

	d->isValid = (d->elfFormat >= 0);
	if (!d->isValid) {
		// Not an ELF executable.
		return;
	}

	// Swap endianness if needed.
	switch (d->elfFormat) {
		default:
			// Unsupported...
			d->isValid = false;
			d->elfFormat = ELFPrivate::ELF_FORMAT_UNKNOWN;
			return;

		case ELFPrivate::ELF_FORMAT_32HOST:
		case ELFPrivate::ELF_FORMAT_64HOST:
			// Host-endian. Nothing to do.
			break;

		case ELFPrivate::ELF_FORMAT_32SWAP: {
			// 32-bit, swapped endian.
			Elf32_Ehdr *const elf32 = &d->Elf_Header.elf32;
			elf32->e_type		= __swab16(elf32->e_type);
			elf32->e_machine	= __swab16(elf32->e_machine);
			elf32->e_version	= __swab32(elf32->e_version);
			elf32->e_entry		= __swab32(elf32->e_entry);
			elf32->e_phoff		= __swab32(elf32->e_phoff);
			elf32->e_shoff		= __swab32(elf32->e_shoff);
			elf32->e_flags		= __swab32(elf32->e_flags);
			elf32->e_ehsize		= __swab16(elf32->e_ehsize);
			elf32->e_phentsize	= __swab16(elf32->e_phentsize);
			elf32->e_phnum		= __swab16(elf32->e_phnum);
			elf32->e_shentsize	= __swab16(elf32->e_shentsize);
			elf32->e_shnum		= __swab16(elf32->e_shnum);
			elf32->e_shstrndx	= __swab16(elf32->e_shstrndx);
			break;
		}

		case ELFPrivate::ELF_FORMAT_64SWAP: {
			// 64-bit, swapped endian.
			Elf64_Ehdr *const elf64 = &d->Elf_Header.elf64;
			elf64->e_type		= __swab16(elf64->e_type);
			elf64->e_machine	= __swab16(elf64->e_machine);
			elf64->e_version	= __swab32(elf64->e_version);
			elf64->e_entry		= __swab64(elf64->e_entry);
			elf64->e_phoff		= __swab64(elf64->e_phoff);
			elf64->e_shoff		= __swab64(elf64->e_shoff);
			elf64->e_flags		= __swab32(elf64->e_flags);
			elf64->e_ehsize		= __swab16(elf64->e_ehsize);
			elf64->e_phentsize	= __swab16(elf64->e_phentsize);
			elf64->e_phnum		= __swab16(elf64->e_phnum);
			elf64->e_shentsize	= __swab16(elf64->e_shentsize);
			elf64->e_shnum		= __swab16(elf64->e_shnum);
			elf64->e_shstrndx	= __swab16(elf64->e_shstrndx);
			break;
		}
	}

	// Primary ELF header.
	const Elf_PrimaryEhdr *const primary = &d->Elf_Header.primary;

	// Is this a Wii U executable?
	if (primary->e_osabi == ELFOSABI_CAFEOS &&
	    primary->e_osabiversion == 0xFE &&
	    d->elfFormat == ELFPrivate::ELF_FORMAT_32MSB &&
	    primary->e_machine == EM_PPC)
	{
		// OS ABI and version is 0xCAFE.
		// Assuming this is a Wii U executable.
		// TODO: Also verify that there's no program headers?
		d->isWiiU = true;
		d->isDynamic = true;	// TODO: Properly check this.

		// TODO: Determine different RPX/RPL file types.
		switch (primary->e_type) {
			case 0xFE01:
				// This matches some homebrew software.
				d->fileType = RomData::FTYPE_EXECUTABLE;
				break;
			default:
				break;
		}
	} else {
		// Standard ELF executable.
		// Check program and section headers.
		d->checkProgramHeaders();
		d->checkSectionHeaders();

		// Determine the file type.
		switch (d->Elf_Header.primary.e_type) {
			default:
				// Should not happen...
				break;
			case ET_REL:
				d->fileType = FTYPE_RELOCATABLE_OBJECT;
				break;
			case ET_EXEC:
				d->fileType = FTYPE_EXECUTABLE;
				break;
			case ET_DYN:
				// This may either be a shared library or a
				// position-independent executable.
				d->fileType = (d->isPie ? FTYPE_EXECUTABLE : FTYPE_SHARED_LIBRARY);
				break;
			case ET_CORE:
				d->fileType = FTYPE_CORE_DUMP;
				break;
		}
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int ELF::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(Elf_PrimaryEhdr))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// TODO: Use 32-bit and/or 16-bit reads to improve performance.
	// (Manual vectorization.)

	const Elf_PrimaryEhdr *const pHdr =
		reinterpret_cast<const Elf_PrimaryEhdr*>(info->header.pData);

	// Check the magic number.
	if (!memcmp(pHdr->e_magic, ELF_MAGIC, sizeof(pHdr->e_magic))) {
		// Verify the bitness and endianness fields.
		switch (pHdr->e_data) {
			case ELFDATA2LSB:
				// Little-endian.
				switch (pHdr->e_class) {
					case ELFCLASS32:
						// 32-bit LSB.
						return ELFPrivate::ELF_FORMAT_32LSB;
					case ELFCLASS64:
						// 64-bit LSB.
						return ELFPrivate::ELF_FORMAT_64LSB;
					default:
						// Unknown bitness.
						break;
				}
				break;

			case ELFDATA2MSB:
				// Big-endian.
				switch (pHdr->e_class) {
					case ELFCLASS32:
						// 32-bit MSB.
						return ELFPrivate::ELF_FORMAT_32MSB;
					case ELFCLASS64:
						// 64-bit MSB.
						return ELFPrivate::ELF_FORMAT_64MSB;
					default:
						// Unknown bitness.
						break;
				}
				break;

			default:
				// Unknown endianness.
				break;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *ELF::systemName(unsigned int type) const
{
	RP_D(const ELF);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// TODO: Identify the OS, or list that in the fields instead?
	static_assert(SYSNAME_TYPE_MASK == 3,
		"ELF::systemName() array index optimization needs to be updated.");

	type &= SYSNAME_TYPE_MASK;

	if (d->isWiiU) {
		// This is a Wii U RPX/RPL executable.
		if (d->fileType == FTYPE_SHARED_LIBRARY) {
			static const char *const sysNames_RPL[4] = {
				"Nintendo Wii U RPL", "RPL", "RPL", nullptr
			};
			return sysNames_RPL[type];
		} else {
			static const char *const sysNames_RPX[4] = {
				"Nintendo Wii U RPX", "RPX", "RPX", nullptr
			};
			return sysNames_RPX[type];
		}
	}

	// Standard ELF executable.
	static const char *const sysNames[4] = {
		"Executable and Linkable Format", "ELF", "ELF", nullptr
	};

	return sysNames[type];
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
const char *const *ELF::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".",		// FIXME: Does this work for files with no extension?
		".elf",		// Common for Wii homebrew.
		".so",		// Shared libraries. (TODO: Versioned .so files.)
		".o",		// Relocatable object files.
		".core",	// Core dumps.
		".debug",	// Split debug files.

		// Wii U
		".rpx",		// Cafe OS executable
		".rpl",		// Cafe OS library

		nullptr
	};
	return exts;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int ELF::loadFieldData(void)
{
	RP_D(ELF);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unsupported file.
		return -EIO;
	}

	// Primary ELF header.
	const Elf_PrimaryEhdr *const primary = &d->Elf_Header.primary;
	d->fields->reserve(10);	// Maximum of 10 fields.

	// NOTE: Executable type is used as File Type.

	// CPU.
	const char *const cpu = ELFData::lookup_cpu(primary->e_machine);
	if (cpu) {
		d->fields->addField_string(C_("ELF", "CPU"), cpu);
	} else {
		d->fields->addField_string(C_("ELF", "CPU"),
			rp_sprintf(C_("ELF", "Unknown (0x%04X)"), primary->e_machine));
	}

	// CPU flags.
	// TODO: Needs testing.
	const Elf32_Word flags = (primary->e_class == ELFCLASS64
			? d->Elf_Header.elf64.e_flags
			: d->Elf_Header.elf32.e_flags);
	switch (primary->e_machine) {
		case EM_68K: {
			if (primary->e_class != ELFCLASS32) {
				// M68K is 32-bit only.
				break;
			}

			// Instruction set.
			// NOTE: `file` can show both 68000 and CPU32
			// at the same time, but that doesn't make sense.
			const char *m68k_insn;
			if (d->Elf_Header.elf32.e_flags == 0) {
				m68k_insn = "68020";
			} else if (d->Elf_Header.elf32.e_flags & 0x01000000) {
				m68k_insn = "68000";
			} else if (d->Elf_Header.elf32.e_flags & 0x00810000) {
				m68k_insn = "CPU32";
			} else {
				m68k_insn = nullptr;
			}

			if (m68k_insn) {
				d->fields->addField_string(C_("ELF", "Instruction Set"), m68k_insn);
			}
			break;
		}

		case EM_SPARC32PLUS:
		case EM_SPARCV9: {
			// Verify bitness.
			if (primary->e_machine == EM_SPARC32PLUS &&
			    primary->e_class != ELFCLASS32)
			{
				// SPARC32PLUS must be 32-bit.
				break;
			}
			else if (primary->e_machine == EM_SPARCV9 &&
				 primary->e_class != ELFCLASS64)
			{
				// SPARCV9 must be 64-bit.
				break;
			}

			// SPARC memory ordering.
			static const char *const sparc_mm[] = {
				NOP_C_("ELF|SPARC_MM", "Total Store Ordering"),
				NOP_C_("ELF|SPARC_MM", "Partial Store Ordering"),
				NOP_C_("ELF|SPARC_MM", "Relaxed Memory Ordering"),
				NOP_C_("ELF|SPARC_MM", "Invalid"),
			};
			d->fields->addField_string(C_("ELF", "Memory Ordering"),
				dpgettext_expr(RP_I18N_DOMAIN, "ELF|SPARC_MM", sparc_mm[flags & 3]));

			// SPARC CPU flags.
			static const char *const sparc_flags_names[] = {
				// 0x1-0x8
				nullptr, nullptr, nullptr, nullptr,
				// 0x10-0x80
				nullptr, nullptr, nullptr, nullptr,
				// 0x100-0x800
				NOP_C_("ELF|SPARCFlags", "SPARC V8+"),
				NOP_C_("ELF|SPARCFlags", "UltraSPARC I"),
				NOP_C_("ELF|SPARCFlags", "HaL R1"),
				NOP_C_("ELF|SPARCFlags", "UltraSPARC III"),
				// 0x1000-0x8000
				nullptr, nullptr, nullptr, nullptr,
				// 0x10000-0x80000
				nullptr, nullptr, nullptr, nullptr,
				// 0x100000-0x800000
				nullptr, nullptr, nullptr,
				// tr: Little-Endian Data
				NOP_C_("ELF|SPARCFlags", "LE Data")
			};
			vector<string> *v_sparc_flags_names = RomFields::strArrayToVector_i18n(
				"ELF|SPARCFlags", sparc_flags_names, ARRAY_SIZE(sparc_flags_names));
			d->fields->addField_bitfield(C_("ELF", "CPU Flags"),
				v_sparc_flags_names, 4, flags);
			break;
		}

		case EM_MIPS:
		case EM_MIPS_RS3_LE: {
			// 32-bit: O32 vs. N32
			if (primary->e_class == ELFCLASS32) {
				d->fields->addField_string(C_("ELF", "MIPS ABI"),
					(d->Elf_Header.elf32.e_flags & 0x20) ? "N32" : "O32");
			}

			// MIPS architecture level.
			static const char *const mips_levels[] = {
				"MIPS-I", "MIPS-II", "MIPS-III", "MIPS-IV",
				"MIPS-V", "MIPS32", "MIPS64", "MIPS32 rel2",
				"MIPS64 rel2", "MIPS32 rel6", "MIPS64 rel6"
			};
			const unsigned int level = (flags >> 28);
			if (level < ARRAY_SIZE(mips_levels)) {
				d->fields->addField_string(C_("ELF", "CPU Level"), mips_levels[level]);
			} else {
				d->fields->addField_string(C_("ELF", "CPU Level"),
					rp_sprintf(C_("ELF", "Unknown (0x%02X)"), level));
			}

			// MIPS CPU flags.
			static const char *const mips_flags_names[] = {
				// 0x1-0x8
				NOP_C_("ELF|MIPSFlags", "No Reorder"),
				NOP_C_("ELF|MIPSFlags", "PIC"),
				NOP_C_("ELF|MIPSFlags", "CPIC"),
				NOP_C_("ELF|MIPSFlags", "XGOT"),
				// 0x10-0x80
				NOP_C_("ELF|MIPSFlags", "64-bit Whirl"),
				NOP_C_("ELF|MIPSFlags", "ABI2"),
				NOP_C_("ELF|MIPSFlags", "ABI ON32"),
				nullptr,
				// 0x100-0x400
				nullptr,
				NOP_C_("ELF|MIPSFlags", "FP64"),
				NOP_C_("ELF|MIPSFlags", "NaN 2008"),
			};
			vector<string> *v_mips_flags_names = RomFields::strArrayToVector_i18n(
				"ELF|MIPSFlags", mips_flags_names, ARRAY_SIZE(mips_flags_names));
			d->fields->addField_bitfield(C_("ELF", "CPU Flags"),
				v_mips_flags_names, 4, (flags & ~0xF0000000));
			break;
		}

		case EM_PARISC: {
			// Flags indicate PA-RISC version.
			d->fields->addField_string(C_("ELF", "PA-RISC Version"),
				rp_sprintf("%s%s",
					(flags >> 16 == 0x0214) ? "2.0" : "1.0",
					(flags & 0x0008) ? " (LP64)" : ""));
			break;
		}

		case EM_ARM: {
			if (primary->e_class != ELFCLASS32) {
				// 32-bit only.
				break;
			}

			// ARM EABI
			string arm_eabi;
			switch (d->Elf_Header.elf32.e_flags >> 24) {
				case 0x04:
					arm_eabi = "EABI4";
					break;
				case 0x05:
					arm_eabi = "EABI5";
					break;
				default:
					break;
			}

			if (d->Elf_Header.elf32.e_flags & 0x00800000) {
				if (!arm_eabi.empty()) {
					arm_eabi += ' ';
				}
				arm_eabi += "BE8";
			}

			if (d->Elf_Header.elf32.e_flags & 0x00400000) {
				if (!arm_eabi.empty()) {
					arm_eabi += ' ';
				}
				arm_eabi += "LE8";
			}

			if (!arm_eabi.empty()) {
				d->fields->addField_string(C_("ELF", "ARM EABI"), arm_eabi);
			}
			break;
		}

		default:
			// No flags.
			break;
	}

	// OS ABI.
	const char *const osabi = ELFData::lookup_osabi(primary->e_osabi);
	if (osabi) {
		d->fields->addField_string(C_("ELF", "OS ABI"), osabi);
	} else {
		d->fields->addField_string(C_("ELF", "OS ABI"),
			rp_sprintf(C_("ELF", "Unknown (%u)"), primary->e_osabi));
	}

	// ABI version.
	if (!d->isWiiU) {
		d->fields->addField_string_numeric(C_("ELF", "ABI Version"),
			primary->e_osabiversion);
	}

	// Bitness/Endianness. (consolidated as "format")
	static const char *const elf_formats[] = {
		"32-bit Little-Endian",
		"64-bit Little-Endian",
		"32-bit Big-Endian",
		"64-bit Big-Endian",
	};
	if (d->elfFormat > ELFPrivate::ELF_FORMAT_UNKNOWN &&
	    d->elfFormat < ARRAY_SIZE(elf_formats))
	{
		d->fields->addField_string(C_("ELF", "Format"), elf_formats[d->elfFormat]);
	}
	else
	{
		// TODO: Show individual values.
		// NOTE: This shouldn't happen...
		d->fields->addField_string(C_("ELF", "Format"), C_("ELF", "Unknown"));
	}

	// Linkage.
	d->fields->addField_string(C_("ELF", "Linkage"),
		d->isDynamic
			? C_("ELF|Linkage", "Dynamic")
			: C_("ELF|Linkage", "Static"));

	// Interpreter.
	if (!d->interpreter.empty()) {
		d->fields->addField_string(C_("ELF", "Interpreter"), d->interpreter);
	}

	// Operating system.
	if (!d->osVersion.empty()) {
		d->fields->addField_string(C_("ELF", "OS Version"), d->osVersion);
	}

	// Entry point.
	// Also indicates PIE.
	// NOTE: Formatting using 8 digits, since 64-bit executables
	// usually have entry points within the first 4 GB.
	if (d->fileType == FTYPE_EXECUTABLE) {
		string entry_point;
		if (primary->e_class == ELFCLASS64) {
			entry_point = rp_sprintf("0x%08" PRIX64, d->Elf_Header.elf64.e_entry);
		} else {
			entry_point = rp_sprintf("0x%08X", d->Elf_Header.elf32.e_entry);
		}
		if (d->isPie) {
			// tr: Entry point, then "Position-Independent".
			entry_point = rp_sprintf(C_("ELF", "%s (Position-Independent)"),
				entry_point.c_str());
		}
		d->fields->addField_string(C_("ELF", "Entry Point"), entry_point);
	}

	// Build ID.
	if (!d->build_id.empty()) {
		// TODO: Put the build ID type in the field itself.
		// Using field name for now.
		const string fieldName = rp_sprintf("BuildID[%s]", (d->build_id_type ? d->build_id_type : "unknown"));
		d->fields->addField_string_hexdump(fieldName.c_str(),
			d->build_id.data(), d->build_id.size(),
			RomFields::STRF_HEX_LOWER | RomFields::STRF_HEXDUMP_NO_SPACES);
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

}
