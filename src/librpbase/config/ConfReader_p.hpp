/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ConfReader_p.hpp: Configuration reader base class.(Private class)       *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_CONFIG_CONFREADER_P_HPP__
#define __ROMPROPERTIES_LIBRPBASE_CONFIG_CONFREADER_P_HPP__

#include "librpbase/config.librpbase.h"
#include "librpbase/common.h"

// load() mutex.
#include "../threads/Mutex.hpp"

// INI parser.
#include "ini.h"

// C++ includes.
#include <string>

namespace LibRpBase {

class ConfReader;
class ConfReaderPrivate
{
	public:
		/**
		 * Configuration reader.
		 * @param filename Configuration filename. Relative to ~/.config/rom-properties
		 */
		explicit ConfReaderPrivate(const char *filename);
		virtual ~ConfReaderPrivate();

	private:
		RP_DISABLE_COPY(ConfReaderPrivate)

	public:
		// load() mutex.
		Mutex mtxLoad;

		// Configuration filename.
		const char *const conf_rel_filename;	// from ctor
		std::string conf_filename;		// alloc()'d in load()

		// rom-properties.conf status.
		bool conf_was_found;
		time_t conf_mtime;
		time_t conf_last_checked;

	public:
		/**
		 * Reset the configuration to the default values.
		 */
		virtual void reset(void) = 0;

		/**
		 * Process a configuration line.
		 * Static function; used by inih as a C-style callback function.
		 *
		 * @param user ConfReaderPrivate object.
		 * @param section Section.
		 * @param name Key.
		 * @param value Value.
		 * @return 1 on success; 0 on error.
		 */
		static int INIHCALL processConfigLine_static(void *user,
			const char *section, const char *name, const char *value);

		/**
		 * Process a configuration line.
		 * Virtual function; must be reimplemented by subclasses.
		 *
		 * @param section Section.
		 * @param name Key.
		 * @param value Value.
		 * @return 1 on success; 0 on error.
		 */
		virtual int processConfigLine(const char *section,
			const char *name, const char *value) = 0;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_CONFIG_CONFREADER_P_HPP__ */
