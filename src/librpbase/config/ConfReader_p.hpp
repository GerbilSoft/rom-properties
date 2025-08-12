/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ConfReader_p.hpp: Configuration reader base class.(Private class)       *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/config.librpbase.h"
#include "common.h"

// INI parser
#include "ini.h"

// C++ includes
#include <mutex>
#include <string>

namespace LibRpBase {

class ConfReader;
class NOVTABLE ConfReaderPrivate
{
public:
	/**
	 * Configuration reader.
	 * @param filename Configuration filename. Relative to ~/.config/rom-properties
	 */
	explicit ConfReaderPrivate(const char *filename);
	virtual ~ConfReaderPrivate() = default;

private:
	RP_DISABLE_COPY(ConfReaderPrivate)

public:
	// load() mutex
	std::mutex mtxLoad;

	// Configuration filename.
	const char *const conf_rel_filename;	// from ctor
	std::string conf_filename;		// alloc()'d in load()

	// rom-properties.conf status.
	time_t conf_mtime;
	time_t conf_last_checked;
	bool conf_was_found;

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
	static int processConfigLine_static(void *user,
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
