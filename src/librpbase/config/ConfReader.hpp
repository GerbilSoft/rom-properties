/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ConfReader.hpp: Configuration reader base class.                        *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_CONFIG_CONFREADER_HPP__
#define __ROMPROPERTIES_LIBRPBASE_CONFIG_CONFREADER_HPP__

#include "../common.h"

namespace LibRpBase {

class ConfReaderPrivate;
class ConfReader
{
	protected:
		/**
		 * Configuration reader.
		 * @param d ConfReaderPrivate subclass.
		 */
		explicit ConfReader(ConfReaderPrivate *d);
	public:
		virtual ~ConfReader();

	private:
		RP_DISABLE_COPY(ConfReader)
	protected:
		ConfReaderPrivate *const d_ptr;

	public:
		/**
		 * Has the configuration been loaded yet?
		 *
		 * This function will *not* load the configuration.
		 * To load the configuration, call load().
		 *
		 * If this function returns false after calling get(),
		 * rom-properties.conf is probably missing.
		 *
		 * @return True if the configuration have been loaded; false if not.
		 */
		bool isLoaded(void) const;

		/**
		 * Load the configuration.
		 *
		 * If the configuration has been modified since the last
		 * load, it will be reloaded. Otherwise, this function
		 * won't do anything.
		 *
		 * @param force If true, force a reload, even if the file hasn't been modified.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int load(bool force = false);

		/**
		 * Get the configuration filename.
		 *
		 * If the configuration's directory does not exist, this
		 * will return nullptr. Otherwise, the filename will be
		 * returned, even if the file doesn't exist yet.
		 *
		 * @return Configuration filename, or nullptr on error.
		 */
		const char *filename(void) const;
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_CONFIG_CONFREADER_HPP__ */
