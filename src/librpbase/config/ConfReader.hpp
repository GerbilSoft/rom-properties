/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ConfReader.hpp: Configuration reader base class.                        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

namespace LibRpBase {

class ConfReaderPrivate;
class RP_LIBROMDATA_PUBLIC ConfReader
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
