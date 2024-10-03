/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiPartition.hpp: Wii partition reader.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "librpbase/config.librpbase.h"
#include "GcnPartition.hpp"
#include "../Console/wii_structs.h"

// librpbase
#include "librpbase/crypto/KeyManager.hpp"

// WiiTicket for EncryptionKeys
#include "../Console/WiiTicket.hpp"

namespace LibRomData {

class WiiPartitionPrivate;
class WiiPartition final : public GcnPartition
{
public:
	// Bitfield enum indicating the encryption type.
	enum CryptoMethod {
		CM_ENCRYPTED = 0,	// Data is encrypted.
		CM_UNENCRYPTED = 1,	// Data is not encrypted.
		CM_MASK_ENCRYPTED = 1,

		CM_1K_31K = 0,		// 1k hashes, 31k data
		CM_32K = 2,		// 32k data
		CM_MASK_SECTOR = 2,

		CM_STANDARD = (CM_ENCRYPTED | CM_1K_31K),	// Standard encrypted Wii disc
		CM_RVTH = (CM_UNENCRYPTED | CM_32K),		// Unencrypted RVT-H disc image
		CM_NASOS = (CM_UNENCRYPTED | CM_1K_31K),	// NASOS compressed retail disc image
	};

	/**
	 * Construct a WiiPartition with the specified IDiscReader.
	 *
	 * NOTE: The IDiscReader *must* remain valid while this
	 * WiiPartition is open.
	 *
	 * @param discReader		[in] IDiscReader
	 * @param partition_offset	[in] Partition start offset
	 * @param partition_size	[in] Calculated partition size. Used if the size in the header is 0.
	 * @param cryptoMethod		[in] Crypto method
	 */
	WiiPartition(const LibRpBase::IDiscReaderPtr &discReader, off64_t partition_offset,
		off64_t partition_size, CryptoMethod crypto = CM_STANDARD);

private:
	typedef GcnPartition super;
	RP_DISABLE_COPY(WiiPartition)

protected:
	friend class WiiPartitionPrivate;
	// d_ptr is used from the subclass.
	//WiiPartitionPrivate *const d_ptr;

public:
	/** IDiscReader **/

	/**
	 * Read data from the partition.
	 * @param ptr Output data buffer.
	 * @param size Amount of data to read, in bytes.
	 * @return Number of bytes read.
	 */
	ATTR_ACCESS_SIZE(write_only, 2, 3)
	size_t read(void *ptr, size_t size) final;

	/**
	 * Set the partition position.
	 * @param pos Partition position.
	 * @return 0 on success; -1 on error.
	 */
	int seek(off64_t pos) final;

	/**
	 * Get the partition position.
	 * @return Partition position on success; -1 on error.
	 */
	off64_t tell(void) final;

public:
	/**
	 * Get the used partition size.
	 * This size includes the partition header and hashes,
	 * but does not include "empty" sectors.
	 * @return Used partition size, or -1 on error.
	 */
	off64_t partition_size_used(void) const final;

public:
	/** WiiPartition **/

	/**
	 * Encryption key verification result.
	 * @return Encryption key verification result.
	 */
	LibRpBase::KeyManager::VerifyResult verifyResult(void) const;

	/**
	 * Get the encryption key in use.
	 * @return Encryption key in use.
	 */
	WiiTicket::EncryptionKeys encKey(void) const;

	/**
	 * Get the encryption key that would be in use if the partition was encrypted.
	 * This is only needed for NASOS images.
	 * @return "Real" encryption key in use.
	 */
	WiiTicket::EncryptionKeys encKeyReal(void) const;

	/**
	 * Get the ticket.
	 * @return Ticket, or nullptr if unavailable.
	 */
	const RVL_Ticket *ticket(void) const;

	/**
	 * Get the TMD header.
	 * @return TMD header, or nullptr if unavailable.
	 */
	const RVL_TMD_Header *tmdHeader(void) const;

	/**
	 * Get the title ID. (NOT BYTESWAPPED)
	 * @return Title ID. (0-0 if unavailable)
	 */
	Nintendo_TitleID_BE_t titleID(void) const;
};

typedef std::shared_ptr<WiiPartition> WiiPartitionPtr;

}
