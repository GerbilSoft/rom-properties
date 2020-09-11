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

// librpbase
using LibRpBase::RomData;
using LibRpBase::RomFields;

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
		// TODO: Store a default filename.
		const RomOpSaveFileInfo sfi = {"Nintendo DS SRL Files (*.srl)|*.srl||", string()};
		RomOp op("E&xtract SRL...", RomOp::ROF_ENABLED, &sfi);
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
	RP_D(const Nintendo3DS);

	// Currently only one ROM operation.
	if (id != 0) {
		pParams->status = -EINVAL;
		pParams->msg = C_("RomData", "ROM operation ID is invalid for this object.");
		return -EINVAL;
	}

	// Check for a DSi SRL.
	NintendoDS *const srl = dynamic_cast<NintendoDS*>(d->mainContent);
	if (!srl) {
		// Not a DSi SRL.
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

	// TODO: Extract the SRL.
	// NOTE: May need to reopen the SRL.
	pParams->status = -ENOTSUP;
	pParams->msg = "TODO: Extract the SRL.";
	return -EINVAL;
}

}
