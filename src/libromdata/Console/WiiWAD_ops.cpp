/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiWAD_ops.cpp: Nintendo Wii WAD file reader. (ROM operations)          *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "WiiWAD.hpp"
#include "WiiWAD_p.hpp"

// librpbase, librpfile
using LibRpBase::RomData;
using LibRpFile::IRpFile;
using LibRpFile::RpFile;

// For sections delegated to other RomData subclasses.
#include "Handheld/NintendoDS.hpp"

// C++ STL classes
using std::string;
using std::vector;

namespace LibRomData {

/** WiiWADPrivate **/

/**
 * Get the list of operations that can be performed on this ROM.
 * Internal function; called by RomData::romOps().
 * @return List of operations.
 */
vector<RomData::RomOp> WiiWAD::romOps_int(void) const
{
	RP_D(const WiiWAD);
	vector<RomOp> ops;

	if (be16_to_cpu(d->tmdHeader.title_id.sysID) != NINTENDO_SYSID_TWL) {
		// We only have a ROM operation for DSi TADs right now.
		return ops;
	}

	RomOp op("E&xtract SRL...", RomOp::ROF_ENABLED | RomOp::ROF_SAVE_FILE);
	op.sfi.title = C_("WiiWAD|RomOps", "Extract Nintendo DS SRL File");
	op.sfi.filter = C_("WiiWAD|RomOps", "Nintendo DS SRL Files|*.nds;*.srl|application/x-nintendo-ds-rom;application/x-nintendo-dsi-rom");
	op.sfi.ext = ".nds";
#ifndef ENABLE_DECRYPTION
	op.flags &= ~RomOp::ROF_ENABLED;
#endif /* ENABLE_DECRYPTION */

	ops.emplace_back(std::move(op));
	return ops;
}

/**
 * Perform a ROM operation.
 * Internal function; called by RomData::doRomOp().
 * @param id		[in] Operation index.
 * @param pParams	[in/out] Parameters and results. (for e.g. UI updates)
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiWAD::doRomOp_int(int id, RomOpParams *pParams)
{
	RP_D(WiiWAD);

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

	if (be16_to_cpu(d->tmdHeader.title_id.sysID) != NINTENDO_SYSID_TWL) {
		// We only have a ROM operation for DSi TADs right now.
		pParams->status = -EINVAL;
		pParams->msg = C_("WiiWAD", "SRL extraction is only supported for DSi TAD packages.");
		return -EINVAL;
	}

#ifdef ENABLE_DECRYPTION
	// If the DSi SRL isn't open right now, make sure we close it later.
	bool wasMainContentOpen = (d->mainContent && d->mainContent->isOpen());

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
			pParams->msg = C_("WiiWAD", "Unable to open the SRL.");
		} else {
			// Unknown error...
			pParams->msg = C_("WiiWAD", "An unknown error occurred attempting to open the SRL.");
		}
		return ret;
	}

	NintendoDS *const srl = dynamic_cast<NintendoDS*>(d->mainContent);
	assert(srl != nullptr);
	if (!srl) {
		// This shouldn't have happened...
		if (!wasMainContentOpen) {
			d->mainContent->close();
		}
		pParams->status = -EIO;
		pParams->msg = C_("WiiWAD", "Unable to open the SRL.");
		return -EIO;
	}

	// Get the source file.
	RpFile *destFile = nullptr;
	IRpFile *const srcFile = srl->ref_file();
	assert(srcFile != nullptr);
	if (!srcFile) {
		// No source file...
		// TODO: More useful message? (may need std::string)
		pParams->status = -EIO;
		pParams->msg = C_("WiiWAD", "Unable to open the SRL.");
		goto out;
	}

	// Create the output file.
	destFile = new RpFile(pParams->save_filename, RpFile::FM_CREATE_WRITE);
	if (!destFile->isOpen()) {
		// TODO: More useful message? (may need std::string)
		pParams->status = -destFile->lastError();
		pParams->msg = C_("WiiWAD", "Could not open output SRL file.");
		goto out;
	}

	// Extract the file.
	srcFile->rewind();
	ret = srcFile->copyTo(destFile, srcFile->size());
	pParams->status = ret;
	switch (ret) {
		case 0:
			pParams->msg = C_("WiiWAD", "SRL file extracted successfully.");
			break;
		case -EIO:
			pParams->msg = C_("WiiWAD", "An I/O error occurred while extracting the SRL.");
			break;
		default:
			pParams->msg = C_("WiiWAD", "An unknown error occurred while extracting the SRL.");
			break;
	}

out:
	UNREF(destFile);
	UNREF(srcFile);
	if (!wasMainContentOpen) {
		d->mainContent->close();
	}
	return pParams->status;
#else /* !ENABLE_DECRYPTION */
	pParams->status = -ENOTSUP;
	pParams->msg = C_("WiiWAD", "SRL extraction is not supported in NoCrypto builds.");
	return -ENOTSUP;
#endif /* ENABLE_DECRYPTION */
}

}
