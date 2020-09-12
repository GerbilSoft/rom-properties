/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DS.hpp: Nintendo 3DS ROM reader. (ROM operations)              *
 * Handles CCI/3DS, CIA, and SMDH files.                                   *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "Nintendo3DS.hpp"
#include "Nintendo3DS_p.hpp"

// librpbase, librpfile
using LibRpBase::RomData;
using LibRpBase::RomFields;
using LibRpFile::IRpFile;

// For sections delegated to other RomData subclasses.
#include "NintendoDS.hpp"

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

/** Nintendo3DSPrivate **/

/**
 * Get the list of operations that can be performed on this ROM.
 * Internal function; called by RomData::romOps().
 * @return List of operations.
 */
vector<RomData::RomOp> Nintendo3DS::romOps_int(void) const
{
	RP_D(const Nintendo3DS);
	vector<RomOp> ops;

	// TMD needs to be loaded so we can check if it's a DSiWare SRL.
	if (!(d->headers_loaded & Nintendo3DSPrivate::HEADER_TMD)) {
		const_cast<Nintendo3DSPrivate*>(d)->loadTicketAndTMD();
	}

	// Check for a DSi SRL.
	NintendoDS *const srl = dynamic_cast<NintendoDS*>(d->mainContent);
	if (srl) {
		RomOp op("E&xtract SRL...", RomOp::ROF_ENABLED | RomOp::ROF_SAVE_FILE);
		op.sfi.title = C_("Nintendo3DS|RomOps", "Extract Nintendo DS SRL File");
		op.sfi.filter = C_("Nintendo3DS|RomOps", "Nintendo DS SRL Files|*.srl|application/x-nintendo-ds-rom;application/x-nintendo-dsi-rom");

		// Get the basename and change the extension to ".srl".
		// TODO: Split into another function?
		if (!d->filename.empty()) {
			const size_t slash_pos = d->filename.rfind(DIR_SEP_CHR);
			if (slash_pos != string::npos) {
				op.filename = d->filename.substr(slash_pos + 1);
			} else {
				op.filename = d->filename;
			}

			// Find the dot.
			const size_t dot_pos = op.filename.rfind('.');
			if (dot_pos != string::npos) {
				// Remove the existing extension.
				op.filename.resize(dot_pos);
			}
			op.filename += ".srl";
		}

		ops.emplace_back(std::move(op));
	}

	return ops;
}

/**
 * Perform a ROM operation.
 * Internal function; called by RomData::doRomOp().
 * @param id		[in] Operation index.
 * @param pParams	[in/out] Parameters and results. (for e.g. UI updates)
 * @return 0 on success; negative POSIX error code on error.
 */
int Nintendo3DS::doRomOp_int(int id, RomOpParams *pParams)
{
	RP_D(Nintendo3DS);

	// Currently only one ROM operation.
	if (id != 0) {
		pParams->status = -EINVAL;
		pParams->msg = C_("RomData", "ROM operation ID is invalid for this object.");
		return -EINVAL;
	}

	assert(pParams->save_filename != nullptr);
	if (!pParams->save_filename) {
		pParams->status = -EINVAL;
		pParams->msg = C_("RomData", "Save filename was not specified.");
		return -EINVAL;
	}

	// If the DSi SRL isn't open right now, make sure we close it later.
	bool isAlreadyOpen = (d->mainContent && d->mainContent->isOpen());

	// Check for a DSi SRL.
	int ret = d->openSRL();
	if (ret != 0) {
		// Unable to open the SRL.
		pParams->status = ret;
		if (ret == -ENOENT) {
			// Not a DSi SRL.
			pParams->msg = C_("RomData", "ROM operation ID is invalid for this object.");
		} else if (ret == -EIO) {
			// Unable to open the DSi SRL.
			pParams->msg = C_("Nintendo3DS", "Unable to open the SRL.");
		} else {
			// Unknown error...
			pParams->msg = C_("Nintendo3DS", "An unknown error occurred attempting to open the SRL.");
		}
		return ret;
	}

	NintendoDS *const srl = dynamic_cast<NintendoDS*>(d->mainContent);
	assert(srl != nullptr);
	if (!srl) {
		// This shouldn't have happened...
		if (!isAlreadyOpen) {
			d->mainContent->close();
		}
		pParams->status = -EIO;
		pParams->msg = C_("Nintendo3DS", "Unable to open the SRL.");
		return -EIO;
	}

	// Extract the SRL.
	IRpFile *const file = srl->ref_file();
	assert(file != nullptr);
	if (!file) {
		// No file...
		if (!isAlreadyOpen) {
			d->mainContent->close();
		}
		pParams->status = -EIO;
		pParams->msg = C_("Nintendo3DS", "Unable to open the SRL.");
		return -EIO;
	}

	// TODO: Extract the SRL.
	// NOTE: May need to reopen the SRL.
	pParams->status = -ENOTSUP;
	pParams->msg = "TODO: Extract the SRL.";

	file->unref();

	// Close the SRL if it wasn't previously opened.
	if (!isAlreadyOpen) {
		d->mainContent->close();
	}

	return pParams->status;
}

}
