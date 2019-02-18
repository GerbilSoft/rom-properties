/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_stdio_kreon.cpp: Standard file object. (stdio implementation)    *
 * Kreon-specific functions.                                               *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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

#include "RpFile.hpp"
#include "RpFile_stdio_p.hpp"

// SCSI and CD-ROM IOCTLs.
#ifdef __linux__
# include <sys/ioctl.h>
# include <scsi/sg.h>
# include <scsi/scsi.h>
# include <linux/cdrom.h>
#endif /* __linux__ */

// C++ includes.
#include <vector>
using std::vector;

namespace LibRpBase {

/**
 * Is this a supported Kreon drive?
 *
 * NOTE: This only checks the drive vendor and model.
 * Check the feature list to determine if it's actually
 * using Kreon firmware.
 *
 * @return True if the drive supports Kreon firmware; false if not.
 */
bool RpFile::isKreonDriveModel(void) const
{
	RP_D(const RpFile);

#ifdef __linux__
	// NOTE: On Linux, this ioctl will fail if not running as root.
	if (!d->isDevice) {
		// Not a device.
		return false;
	}

	/* INQUIRY response for Standard Inquiry Data. (EVPD == 0, PageCode == 0) */
	typedef struct PACKED _SCSI_RESP_INQUIRY_STD {
		uint8_t PeripheralDeviceType;	/* High 3 bits == qualifier; low 5 bits == type */
		uint8_t RMB_DeviceTypeModifier;
		uint8_t Version;
		uint8_t ResponseDataFormat;
		uint8_t AdditionalLength;
		uint8_t Reserved1[2];
		uint8_t Flags;
		char vendor_id[8];
		char product_id[16];
		char product_revision_level[4];
		uint8_t VendorSpecific[20];
		uint8_t Reserved2[40];
	} SCSI_RESP_INQUIRY_STD;

	// SCSI command buffers.
	struct sg_io_hdr sg_io;
	union {
		struct request_sense s;
		uint8_t u[18];
	} _sense;

	// TODO: Consolidate this.
	memset(&sg_io, 0, sizeof(sg_io));
	sg_io.interface_id = 'S';
	sg_io.mx_sb_len = sizeof(_sense);
	sg_io.sbp = _sense.u;
	sg_io.flags = SG_FLAG_LUN_INHIBIT | SG_FLAG_DIRECT_IO;

	// SCSI INQUIRY command.
	uint8_t cdb[6] = {0x12, 0x00, 0x00,
		sizeof(SCSI_RESP_INQUIRY_STD) >> 8,
		sizeof(SCSI_RESP_INQUIRY_STD) & 0xFF,
		0x00};
	sg_io.cmdp = cdb;
	sg_io.cmd_len = sizeof(cdb);

	SCSI_RESP_INQUIRY_STD resp;
	sg_io.dxfer_direction = SG_DXFER_FROM_DEV;
	sg_io.dxferp = &resp;
	sg_io.dxfer_len = sizeof(resp);

	if (ioctl(fileno(d->file), SG_IO, &sg_io) != 0) {
		// ioctl failed.
		return false;
	}

	// Check the drive vendor and product ID.
	if (!memcmp(resp.vendor_id, "TSSTcorp", 8)) {
		// Correct vendor ID.
		// Check for supported product IDs.
		// NOTE: More drive models are supported, but the
		// Kreon firmware only uses these product IDs.
		static const char *const product_id_tbl[] = {
			"DVD-ROM SH-D162C",
			"DVD-ROM TS-H353A",
			"DVD-ROM SH-D163B",
		};
		for (int i = 0; i < ARRAY_SIZE(product_id_tbl); i++) {
			if (!memcmp(resp.product_id, product_id_tbl[i], sizeof(resp.product_id))) {
				// Found a match.
				return true;
			}
		}
	}
#endif /* __linux__ */

	// Not supported.
	return false;
}

/**
 * Get a list of supported Kreon features.
 * @return List of Kreon feature IDs, or empty vector if not supported.
 */
vector<uint16_t> RpFile::getKreonFeatureList(void) const
{
	RP_D(const RpFile);
	vector<uint16_t> vec;

#ifdef __linux__
	// NOTE: On Linux, this ioctl will fail if not running as root.
	if (!d->isDevice) {
		// Not a device.
		return vec;
	}

	// SCSI command buffers.
	struct sg_io_hdr sg_io;
	union {
		struct request_sense s;
		uint8_t u[18];
	} _sense;

	// TODO: Consolidate this.
	memset(&sg_io, 0, sizeof(sg_io));
	sg_io.interface_id = 'S';
	sg_io.mx_sb_len = sizeof(_sense);
	sg_io.sbp = _sense.u;
	sg_io.flags = SG_FLAG_LUN_INHIBIT | SG_FLAG_DIRECT_IO;

	// Kreon "Get Feature List" command
	// Reference: https://github.com/saramibreak/DiscImageCreator/blob/cb9267da4877d32ab68263c25187cbaab3435ad5/DiscImageCreator/execScsiCmdforDVD.cpp#L1223
	uint8_t cdb[6] = {0xFF, 0x08, 0x01, 0x10, 0x00, 0x00};
	sg_io.cmdp = cdb;
	sg_io.cmd_len = sizeof(cdb);

	uint8_t feature_buf[26];
	sg_io.dxfer_direction = SG_DXFER_FROM_DEV;
	sg_io.dxferp = feature_buf;
	sg_io.dxfer_len = sizeof(feature_buf);

	if (ioctl(fileno(d->file), SG_IO, &sg_io) != 0) {
		// ioctl failed.
		return vec;
	}

	vec.reserve(sizeof(feature_buf)/sizeof(uint16_t));
	for (size_t i = 0; i < sizeof(feature_buf); i += 2) {
		const uint16_t feature = (feature_buf[i] << 8) | feature_buf[i+1];
		if (feature == 0)
			break;
		vec.push_back(feature);
	}

	if (vec.size() < 2 || vec[0] != KREON_FEATURE_HEADER_0 ||
	    vec[1] != KREON_FEATURE_HEADER_1)
	{
		// Kreon feature list is invalid.
		vec.clear();
		vec.shrink_to_fit();
	}
#endif /* __linux__ */

	return vec;
}

/**
 * Set Kreon error skip state.
 * @param skip True to skip; false for normal operation.
 * @return 0 on success; non-zero on error.
 */
int RpFile::setKreonErrorSkipState(bool skip)
{
	RP_D(const RpFile);
	vector<uint16_t> vec;

#ifdef __linux__
	// NOTE: On Linux, this ioctl will fail if not running as root.
	if (!d->isDevice) {
		// Not a device.
		return -ENODEV;
	}

	// SCSI command buffers.
	struct sg_io_hdr sg_io;
	union {
		struct request_sense s;
		uint8_t u[18];
	} _sense;

	// TODO: Consolidate this.
	memset(&sg_io, 0, sizeof(sg_io));
	sg_io.interface_id = 'S';
	sg_io.mx_sb_len = sizeof(_sense);
	sg_io.sbp = _sense.u;
	sg_io.flags = SG_FLAG_LUN_INHIBIT | SG_FLAG_DIRECT_IO;

	// Kreon "Set Error Skip State" command
	// Reference: https://github.com/saramibreak/DiscImageCreator/blob/cb9267da4877d32ab68263c25187cbaab3435ad5/DiscImageCreator/execScsiCmdforDVD.cpp#L1341
	uint8_t cdb[6] = {0xFF, 0x08, 0x01, 0x15, (uint8_t)skip, 0x00};
	sg_io.cmdp = cdb;
	sg_io.cmd_len = sizeof(cdb);

	sg_io.dxfer_direction = SG_DXFER_NONE;

	if (ioctl(fileno(d->file), SG_IO, &sg_io) != 0) {
		// ioctl failed.
		return -errno;
	}

	// Command succeeded.
	return 0;
#endif /* __linux__ */

	// Not supported.
	return -ENOTSUP;
}

/**
 * Set Kreon lock state
 * @param lockState 0 == locked; 1 == Unlock State 1 (xtreme); 2 == Unlock State 2 (wxripper)
 * @return 0 on success; non-zero on error.
 */
int RpFile::setKreonLockState(uint8_t lockState)
{
	RP_D(const RpFile);
	vector<uint16_t> vec;

#ifdef __linux__
	// NOTE: On Linux, this ioctl will fail if not running as root.
	if (!d->isDevice) {
		// Not a device.
		return -ENODEV;
	}

	// SCSI command buffers.
	struct sg_io_hdr sg_io;
	union {
		struct request_sense s;
		uint8_t u[18];
	} _sense;

	// TODO: Consolidate this.
	memset(&sg_io, 0, sizeof(sg_io));
	sg_io.interface_id = 'S';
	sg_io.mx_sb_len = sizeof(_sense);
	sg_io.sbp = _sense.u;
	sg_io.flags = SG_FLAG_LUN_INHIBIT | SG_FLAG_DIRECT_IO;

	// Kreon "Set Lock State" command
	// Reference: https://github.com/saramibreak/DiscImageCreator/blob/cb9267da4877d32ab68263c25187cbaab3435ad5/DiscImageCreator/execScsiCmdforDVD.cpp#L1309
	uint8_t cdb[6] = {0xFF, 0x08, 0x01, 0x11, (uint8_t)lockState, 0x00};
	sg_io.cmdp = cdb;
	sg_io.cmd_len = sizeof(cdb);

	sg_io.dxfer_direction = SG_DXFER_NONE;

	if (ioctl(fileno(d->file), SG_IO, &sg_io) != 0) {
		// ioctl failed.
		return -errno;
	}

	// Command succeeded.
	return 0;
#endif /* __linux__ */

	// Not supported.
	return -ENOTSUP;
}

}
