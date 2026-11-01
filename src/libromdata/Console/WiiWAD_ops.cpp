/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiWAD_ops.cpp: Nintendo Wii WAD file reader. (ROM operations)          *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpbase.h"

#include "WiiWAD.hpp"
#include "WiiWAD_p.hpp"

// Other rom-properties libraries
#include "librpfile/RpFile.hpp"
using namespace LibRpFile;
using LibRpBase::RomData;

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
	vector<RomOp> ops;

	RP_D(const WiiWAD);
	if (be16_to_cpu(d->tmdHeader.title_id.sysID) != NINTENDO_SYSID_TWL) {
		// We only have a ROM operation for DSi TADs right now.
		return ops;
	}

	ops.emplace_back("E&xtract SRL...", RomOp::ROF_ENABLED | RomOp::ROF_SAVE_FILE);
	RomOp &op = ops.back();
	op.sfi.title = C_("Nintendo|RomOps", "Extract Nintendo DS SRL File");
	op.sfi.filter = C_("Nintendo|RomOps", "Nintendo DS SRL Files|*.nds;*.srl|application/x-nintendo-ds-rom;application/x-nintendo-dsi-rom");
	op.sfi.ext = ".nds";
#ifndef ENABLE_DECRYPTION
	op.flags &= ~RomOp::ROF_ENABLED;
#endif /* ENABLE_DECRYPTION */

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

	assert((bool)pParams->save_file);
	if (!pParams->save_file) {
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
	const bool wasMainContentOpen = (d->mainContent && d->mainContent->isOpen());

	// Check for a DSi SRL.
	int ret = d->openSRL();
	if (ret != 0) {
		// Unable to open the SRL.
		pParams->status = ret;
		switch (ret) {
			case -ENOENT:
				// Not a DSi SRL.
				pParams->msg = C_("RomData", "ROM operation ID is invalid for this object.");
				break;
			case -EIO:
				pParams->msg = C_("Nintendo", "Unable to open the SRL.");
				break;
			default:
				pParams->msg = C_("Nintendo", "An unknown error occurred attempting to open the SRL.");
				break;
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
		pParams->msg = C_("Nintendo", "Unable to open the SRL.");
		return -EIO;
	}

	// Get the source file.
	const IRpFilePtr srcFile = srl->ref_file();
	assert((bool)srcFile);
	if (!srcFile) {
		// No source file...
		// TODO: More useful message?
		pParams->status = -EIO;
		pParams->msg = C_("Nintendo", "Unable to open the SRL.");
		goto out;
	}

	// NOTE: Output file was already created by the calling function.
	// TODO: Delete on error?

	// Extract the file.
	srcFile->rewind();
	ret = srcFile->copyTo(pParams->save_file.get(), srcFile->size());
	pParams->status = ret;
	switch (ret) {
		case 0:
			pParams->msg = C_("Nintendo", "SRL file extracted successfully.");
			break;
		case -EPERM:
			pParams->msg = C_("Nintendo", "The output SRL file is not writable.");
			break;
		case -EIO:
			pParams->msg = C_("Nintendo", "An I/O error occurred while extracting the SRL.");
			break;
		default:
			pParams->msg = C_("Nintendo", "An unknown error occurred while extracting the SRL.");
			break;
	}

out:
	pParams->save_file->close();
	if (!wasMainContentOpen) {
		d->mainContent->close();
	}
	return pParams->status;
#else /* !ENABLE_DECRYPTION */
	pParams->status = -ENOTSUP;
	pParams->msg = C_("Nintendo", "SRL extraction is not supported in NoCrypto builds.");
	return -ENOTSUP;
#endif /* ENABLE_DECRYPTION */
}

} // namespace LibRomData
