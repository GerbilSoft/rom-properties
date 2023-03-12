/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile_Kreon.cpp: Standard file object. (Kreon-specific functions)      *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpfile.h"

#include "../RpFile.hpp"
#include "../RpFile_p.hpp"

#include "scsi_protocol.h"

// C++ STL classes.
using std::array;
using std::vector;

namespace LibRpFile {

/**
 * Is this a supported Kreon drive?
 *
 * NOTE: This only checks the drive vendor and model.
 * Check the feature list to determine if it's actually
 * using Kreon firmware.
 *
 * @return True if the drive supports Kreon firmware; false if not.
 */
bool RpFile::isKreonDriveModel(void)
{
	RP_D(const RpFile);
	if (!d->devInfo) {
		// Not a device.
		return false;
	}

#ifdef RP_OS_SCSI_SUPPORTED
	// SCSI INQUIRY command.
	SCSI_RESP_INQUIRY_STD resp;
	int ret = scsi_inquiry(&resp);
	if (ret != 0) {
		// SCSI command failed.
		return false;
	}

	// Check the device type, vendor, and product ID.
	if ((resp.PeripheralDeviceType & 0x1F) != SCSI_DEVICE_TYPE_CDROM) {
		// Wrong type of device.
		return false;
	}

	// TODO: Optimize by removing pointers? Doing so would require
	// direct use of character arrays, which is ugly...

	// TSSTcorp (Toshiba/Samsung)
	static const char *const TSSTcorp_product_tbl[] = {
		// Kreon
		"DVD-ROM SH-D162C",
		"DVD-ROM TS-H353A",
		"DVD-ROM SH-D163B",

		// 360
		"DVD-ROM TS-H943A",
		nullptr
	};

	// Philips/BenQ Digital Storage
	static const char *const PBDS_product_tbl[] = {
		"VAD6038         ",
		"VAD6038-64930C  ",
		nullptr
	};

	// Hitachi-LG Data Storage
	static const char *const HLDTST_product_tbl[] = {
		"DVD-ROM GDR3120L",	// Phat
		nullptr
	};

	// Vendor table.
	// NOTE: Vendor strings MUST be 8 characters long.
	// NOTE: Strings in product ID tables MUST be 16 characters long.
	struct vendor_tbl_t {
		char vendor_id[8];
		const char *const *product_id_tbl;
	};
	static const vendor_tbl_t vendor_tbl[] = {
		{{'T','S','S','T','c','o','r','p'}, TSSTcorp_product_tbl},
		{{'P','B','D','S',' ',' ',' ',' '}, PBDS_product_tbl},
		{{'H','L','-','D','T','-','S','T'}, HLDTST_product_tbl},
	};
	static const vendor_tbl_t *const p_vendor_tbl_end = &vendor_tbl[ARRAY_SIZE(vendor_tbl)];

	// Find the vendor.
	auto vendor_iter = std::find_if(vendor_tbl, p_vendor_tbl_end,
		[&resp](const vendor_tbl_t &p) noexcept -> bool {
			return !memcmp(p.vendor_id, resp.vendor_id, 8);
		});
	if (vendor_iter == p_vendor_tbl_end) {
		// Not found.
		return false;
	}

	// Check if the product ID is supported.
	bool found = false;
	for (const char *const *pProdTbl = vendor_iter->product_id_tbl;
	     *pProdTbl != nullptr; pProdTbl++)
	{
		if (!memcmp(resp.product_id, *pProdTbl, 16)) {
			// Found a match.
			found = true;
			break;
		}
	}
	return found;
#else /* !RP_OS_SCSI_SUPPORTED */
	// No SCSI implementation for this OS.
	return -ENOSYS;
#endif /* _WIN32 */
}

/**
 * Get a list of supported Kreon features.
 * @return List of Kreon feature IDs, or empty vector if not supported.
 */
vector<RpFile::KreonFeature> RpFile::getKreonFeatureList(void)
{
	// NOTE: On Linux, this ioctl will fail if not running as root.
	RP_D(RpFile);
	vector<KreonFeature> vec;
	if (!d->devInfo) {
		// Not a device.
		return vec;
	}

#ifdef RP_OS_SCSI_SUPPORTED
	// Kreon "Get Feature List" command
	// Reference: https://github.com/saramibreak/DiscImageCreator/blob/cb9267da4877d32ab68263c25187cbaab3435ad5/DiscImageCreator/execScsiCmdforDVD.cpp#L1223
	static const uint8_t cdb[6] = {0xFF, 0x08, 0x01, 0x10, 0x00, 0x00};
	array<uint16_t, 13> feature_buf;
	int ret = d->scsi_send_cdb(cdb, sizeof(cdb), feature_buf.data(), sizeof(feature_buf), RpFilePrivate::ScsiDirection::In);
	if (ret != 0) {
		// SCSI command failed.
		return vec;
	}

	vec.reserve(feature_buf.size());
	for (const uint16_t feature : feature_buf) {
		if (feature == 0)
			break;
		vec.emplace_back(static_cast<KreonFeature>(be16_to_cpu(feature)));
	}

	if (vec.size() < 2 ||
	    vec[0] != KreonFeature::Header0 ||
	    vec[1] != KreonFeature::Header1)
	{
		// Kreon feature list is invalid.
		vec.clear();
		vec.shrink_to_fit();
	}
#endif /* RP_OS_SCSI_SUPPORTED */

	return vec;
}

/**
 * Set Kreon error skip state.
 * @param skip True to skip; false for normal operation.
 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
 */
int RpFile::setKreonErrorSkipState(bool skip)
{
	// NOTE: On Linux, this ioctl will fail if not running as root.
	RP_D(RpFile);
	if (!d->devInfo) {
		// Not a device.
		return -ENODEV;
	}

#ifdef RP_OS_SCSI_SUPPORTED
	// Kreon "Set Error Skip State" command
	// Reference: https://github.com/saramibreak/DiscImageCreator/blob/cb9267da4877d32ab68263c25187cbaab3435ad5/DiscImageCreator/execScsiCmdforDVD.cpp#L1341
	const uint8_t cdb[6] = {0xFF, 0x08, 0x01, 0x15, (uint8_t)skip, 0x00};
	return d->scsi_send_cdb(cdb, sizeof(cdb), nullptr, 0, RpFilePrivate::ScsiDirection::In);
#else /* !RP_OS_SCSI_SUPPORTED */
	// No SCSI implementation for this OS.
	RP_UNUSED(skip);
	return -ENOSYS;
#endif /* _WIN32 */
}

/**
 * Set Kreon lock state
 * @param lockState 0 == locked; 1 == Unlock State 1 (xtreme); 2 == Unlock State 2 (wxripper)
 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
 */
int RpFile::setKreonLockState(KreonLockState lockState)
{
	// NOTE: On Linux, this ioctl will fail if not running as root.
	RP_D(RpFile);
	if (!d->devInfo) {
		// Not a device.
		return -ENODEV;
	}

#ifdef RP_OS_SCSI_SUPPORTED
	// Kreon "Set Lock State" command
	// Reference: https://github.com/saramibreak/DiscImageCreator/blob/cb9267da4877d32ab68263c25187cbaab3435ad5/DiscImageCreator/execScsiCmdforDVD.cpp#L1309
	const uint8_t cdb[6] = {0xFF, 0x08, 0x01, 0x11, static_cast<uint8_t>(lockState), 0x00};
	int ret = d->scsi_send_cdb(cdb, sizeof(cdb), nullptr, 0, RpFilePrivate::ScsiDirection::In);
	if (ret == 0) {
		d->devInfo->isKreonUnlocked = (lockState != KreonLockState::Locked);
	}
	return ret;
#else /* !RP_OS_SCSI_SUPPORTED */
	// No SCSI implementation for this OS.
	RP_UNUSED(lockState);
	return -ENOSYS;
#endif /* _WIN32 */
}

}
