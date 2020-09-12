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
using LibRpFile::RpFile;

// For sections delegated to other RomData subclasses.
#include "NintendoDS.hpp"

// C++ STL classes.
using std::string;
using std::unique_ptr;
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

	// SRL read buffer. (Will be allocated later.)
#define SRL_BUFFER_SIZE (64*1024)
	unique_ptr<uint8_t[]> buf;

	// Get the source SRL.
	RpFile *destFile = nullptr;
	IRpFile *const srcFile = srl->ref_file();
	assert(srcFile != nullptr);
	if (!srcFile) {
		// No source file...
		// TODO: More useful message? (may need std::string)
		pParams->status = -EIO;
		pParams->msg = C_("Nintendo3DS", "Unable to open the SRL.");
		goto out;
	}

	// Create the output file.
	destFile = new RpFile(pParams->save_filename, RpFile::FM_CREATE_WRITE);
	if (!destFile->isOpen()) {
		// TODO: More useful message? (may need std::string)
		pParams->status = -destFile->lastError();
		pParams->msg = C_("Nintendo3DS", "Could not open output SRL file.");
		goto out;
	}

	// Extract the file.
	buf.reset(new uint8_t[SRL_BUFFER_SIZE]);
	srcFile->rewind();
	for (int64_t fileSize = srcFile->size(); fileSize > 0; fileSize -= SRL_BUFFER_SIZE) {
		const size_t szRead = srcFile->read(buf.get(), SRL_BUFFER_SIZE);
		if (szRead != SRL_BUFFER_SIZE &&
		    (fileSize < SRL_BUFFER_SIZE && szRead != (size_t)fileSize))
		{
			// Short read.
			int ret = -srcFile->lastError();
			if (ret == 0) {
				ret = -EIO;
			}
			// TODO: More useful message? (may need std::string)
			pParams->status = ret;
			pParams->msg = C_("Nintendo3DS", "A read error occurred while extracting the SRL.");
			goto out;
		}

		const size_t szWrite = destFile->write(buf.get(), szRead);
		if (szWrite != szRead) {
			// Short write.
			int ret = -destFile->lastError();
			if (ret == 0) {
				ret = -EIO;
			}
			// TODO: More useful message? (may need std::string)
			pParams->status = ret;
			pParams->msg = C_("Nintendo3DS", "A write error occurred while extracting the SRL.");
			goto out;
		}
	}

	// SRL extracted.
	// TODO: Include the basename here? (may need std::string)
	pParams->status = 0;
	pParams->msg = "SRL file extracted successfully.";

out:
	UNREF(destFile);
	UNREF(srcFile);
	if (!isAlreadyOpen) {
		d->mainContent->close();
	}
	return pParams->status;
}

}
