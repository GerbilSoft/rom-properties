/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDrive.hpp: Sega Mega Drive ROM reader.                              *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_MEGADRIVE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_MEGADRIVE_HPP__

#include <stdint.h>
#include <string>

namespace LibRomData {

class MegaDrive
{
	public:
		// TODO: Some abstraction to read the file directory
		// using a wrapper around FILE*, QFile, etc.
		// For now, just check the header.

		/**
		 * Read a Sega Mega Drive ROM.
		 * @param header ROM header. (Should be at least 65536+512 bytes.)
		 * @param size Header size.
		 *
		 * Check isValid() to determine if this is a valid ROM.
		 */
		MegaDrive(const uint8_t *header, size_t size);

		~MegaDrive();

	private:
		MegaDrive(const MegaDrive &);
		MegaDrive &operator=(const MegaDrive &);

	public:
		/**
		 * Is this ROM recognized as a Sega Mega Drive ROM?
		 * @return True if it is; false if it isn't.
		 */
		bool isValid(void) const;

	private:
		bool m_isValid;

	public:
		/** MD ROM properties. **/

		enum SysID {
			MD_MEGA_DRIVE,		// Mega Drive
			MD_MEGA_CD_DISC,	// Mega CD (disc)
			MD_MEGA_CD_BOOT,	// Mega CD (boot ROM)
			MD_32X,			// Sega 32X
			MD_PICO,		// Sega Pico
		};
		SysID m_sysID;

		// I/O support bitfield.
		enum IOSupport {
			IO_JOYPAD_3	= (1 << 0),	// 3-button joypad
			IO_JOYPAD_6	= (1 << 1),	// 6-button joypad
			IO_JOYPAD_SMS	= (1 << 2),	// 2-button joypad (SMS)
			IO_TEAM_PLAYER	= (1 << 3),	// Team Player
			IO_KEYBOARD	= (1 << 4),	// Keyboard
			IO_SERIAL	= (1 << 5),	// Serial (RS-232C)
			IO_PRINTER	= (1 << 6),	// Printer
			IO_TABLET	= (1 << 7),	// Tablet
			IO_TRACKBALL	= (1 << 8),	// Trackball
			IO_PADDLE	= (1 << 9),	// Paddle
			IO_FDD		= (1 << 10),	// Floppy Drive
			IO_CDROM	= (1 << 11),	// CD-ROM
			IO_ACTIVATOR	= (1 << 12),	// Activator
			IO_MEGA_MOUSE	= (1 << 13),	// Mega Mouse
		};

		// TODO: Make accessor functions.
		std::string m_system;
		std::string m_copyright;
		std::string m_title_domestic;
		std::string m_title_export;
		std::string m_serial;
		std::string m_company;
		uint16_t m_checksum;
		uint32_t m_io_support;
		uint32_t m_rom_start;
		uint32_t m_rom_end;
		uint32_t m_ram_start;
		uint32_t m_ram_end;
		// TODO: SRAM type.
		uint32_t m_sram_start;
		uint32_t m_sram_end;
		uint32_t m_entry_point;
		uint32_t m_initial_sp;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_MEGADRIVE_HPP__ */
