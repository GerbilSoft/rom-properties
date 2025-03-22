/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Nintendo3DS_ops.cpp: Nintendo 3DS ROM reader. (ROM operations)          *
 * Handles CCI/3DS, CIA, and SMDH files.                                   *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "Nintendo3DS.hpp"
#include "Nintendo3DS_p.hpp"

// Other rom-properties libraries
using namespace LibRpFile;
using LibRpBase::RomData;

// For sections delegated to other RomData subclasses.
#include "NintendoDS.hpp"

// C++ STL classes
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
	NintendoDS *const srl = dynamic_cast<NintendoDS*>(d->mainContent.get());
	if (srl) {
		RomOp op("E&xtract SRL...", RomOp::ROF_SAVE_FILE | RomOp::ROF_ENABLED);
		op.sfi.title = C_("Nintendo3DS|RomOps", "Extract Nintendo DS SRL File");
		op.sfi.filter = C_("Nintendo3DS|RomOps", "Nintendo DS SRL Files|*.nds;*.srl|application/x-nintendo-ds-rom;application/x-nintendo-dsi-rom");
		op.sfi.ext = ".nds";
		ops.push_back(std::move(op));
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
	const bool wasMainContentOpen = (d->mainContent && d->mainContent->isOpen());

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

	NintendoDS *const srl = dynamic_cast<NintendoDS*>(d->mainContent.get());
	assert(srl != nullptr);
	if (!srl) {
		// This shouldn't have happened...
		if (!wasMainContentOpen) {
			d->mainContent->close();
		}
		pParams->status = -EIO;
		pParams->msg = C_("Nintendo3DS", "Unable to open the SRL.");
		return -EIO;
	}

	// Get the source file.
	RpFile *destFile = nullptr;
	const IRpFilePtr srcFile(srl->ref_file());
	assert((bool)srcFile);
	if (!srcFile) {
		// No source file...
		// TODO: More useful message?
		pParams->status = -EIO;
		pParams->msg = C_("Nintendo3DS", "Unable to open the SRL.");
		goto out;
	}

	// Create the output file.
	destFile = new RpFile(pParams->save_filename, RpFile::FM_CREATE_WRITE);
	if (!destFile->isOpen()) {
		// TODO: More useful message?
		pParams->status = -destFile->lastError();
		pParams->msg = C_("Nintendo3DS", "Could not open output SRL file.");
		goto out;
	}

	// Extract the file.
	srcFile->rewind();
	ret = srcFile->copyTo(destFile, srcFile->size());
	pParams->status = ret;
	switch (ret) {
		case 0:
			pParams->msg = C_("Nintendo3DS", "SRL file extracted successfully.");
			break;
		case -EIO:
			pParams->msg = C_("Nintendo3DS", "An I/O error occurred while extracting the SRL.");
			break;
		default:
			pParams->msg = C_("Nintendo3DS", "An unknown error occurred while extracting the SRL.");
			break;
	}

out:
	delete destFile;
	if (!wasMainContentOpen) {
		d->mainContent->close();
	}
	return pParams->status;
}

}
