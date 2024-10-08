/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile.hpp: Standard file object.                                       *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "IRpFile.hpp"

// C++ includes
#include <vector>

// from scsi_protocol.h
struct _SCSI_RESP_INQUIRY_STD;
// from ata_protocol.h
struct _ATA_RESP_IDENTIFY_DEVICE;

namespace LibRpFile {

class RpFilePrivate;
class RpFile final : public IRpFile
{
	public:
		enum FileMode : uint8_t {
			FM_READ = 0,		// Read-only.
			FM_WRITE = 1,		// Read/write.
			FM_OPEN = 0,		// Open the file. (Must exist!)
			FM_CREATE = 2,		// Create the file. (Will overwrite!)

			// Combinations.
			FM_OPEN_READ = 0,	// Open for reading. (Must exist!)
			FM_OPEN_WRITE = 1,	// Open for reading/writing. (Must exist!)
			//FM_CREATE_READ = 2,	// Not valid; handled as FM_CREATE_WRITE.
			FM_CREATE_WRITE = 3,	// Create for reading/writing. (Will overwrite!)

			// Mask.
			FM_MODE_MASK = 3,	// Mode mask.

			// Extras.
			FM_GZIP_DECOMPRESS = 4,	// Transparent gzip decompression. (read-only!)
			FM_OPEN_READ_GZ = FM_READ | FM_GZIP_DECOMPRESS,
		};

		/**
		 * Open a file.
		 * NOTE: Files are always opened in binary mode.
		 * @param filename Filename (UTF-8)
		 * @param mode File mode
		 */
		RP_LIBROMDATA_PUBLIC
		RpFile(const char *filename, FileMode mode);
		RP_LIBROMDATA_PUBLIC
		RpFile(const std::string &filename, FileMode mode);

#ifdef _WIN32
		/**
		 * Open a file.
		 * NOTE: Files are always opened in binary mode.
		 * @param filenameW Filename (UTF-16)
		 * @param mode File mode
		 */
		RP_LIBROMDATA_PUBLIC
		RpFile(const wchar_t *filenameW, FileMode mode);
		RP_LIBROMDATA_PUBLIC
		RpFile(const std::wstring &filenameW, FileMode mode);
#endif /* _WIN32 */

	public:
		RP_LIBROMDATA_PUBLIC
		~RpFile() final;

	private:
		typedef IRpFile super;
		RP_DISABLE_COPY(RpFile)
	protected:
		friend class RpFilePrivate;
		RpFilePrivate *const d_ptr;

	public:
		/**
		 * Is the file open?
		 * This usually only returns false if an error occurred.
		 * @return True if the file is open; false if it isn't.
		 */
		RP_LIBROMDATA_PUBLIC
		bool isOpen(void) const final;

		/**
		 * Close the file.
		 */
		RP_LIBROMDATA_PUBLIC
		void close(void) final;

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		ATTR_ACCESS_SIZE(write_only, 2, 3)
		RP_LIBROMDATA_PUBLIC
		size_t read(void *ptr, size_t size) final;

		/**
		 * Write data to the file.
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		ATTR_ACCESS_SIZE(read_only, 2, 3)
		size_t write(const void *ptr, size_t size) final;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(off64_t pos) final;

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		off64_t tell(void) final;

		/**
		 * Truncate the file.
		 * @param size New size. (default is 0)
		 * @return 0 on success; -1 on error.
		 */
		int truncate(off64_t size = 0) final;

		/**
		 * Flush buffers.
		 * This operation only makes sense on writable files.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int flush(void) final;

	public:
		/** File properties **/

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		RP_LIBROMDATA_PUBLIC
		off64_t size(void) final;

		/**
		 * Get the filename.
		 * @return Filename (UTF-8) (May be nullptr if the filename is not available.)
		 */
		RP_LIBROMDATA_PUBLIC
		const char *filename(void) const final;

#ifdef _WIN32
		/**
		 * Get the filename. (Windows only: returns UTF-16.)
		 * @return Filename (UTF-16) (May be nullptr if the filename is not available.)
		 */
		RP_LIBROMDATA_PUBLIC
		const wchar_t *filenameW(void) const;
#endif /* _WIN32 */

	public:
		/** Extra functions **/

		/**
		 * Make the file writable.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int makeWritable(void) final;

	public:
		/** Device file functions **/

		/**
		 * Re-read device size using the native OS API.
		 * @param pDeviceSize	[out,opt] If not NULL, retrieves the device size, in bytes.
		 * @param pSectorSize	[out,opt] If not NULL, retrieves the sector size, in bytes.
		 * @return 0 on success, negative for POSIX error code.
		 */
		int rereadDeviceSizeOS(off64_t *pDeviceSize = nullptr, uint32_t *pSectorSize = nullptr);

		/**
		 * Re-read device size using SCSI commands.
		 * This may be needed for Kreon devices.
		 * @param pDeviceSize	[out,opt] If not NULL, retrieves the device size, in bytes.
		 * @param pSectorSize	[out,opt] If not NULL, retrieves the sector size, in bytes.
		 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
		 */
		int rereadDeviceSizeScsi(off64_t *pDeviceSize = nullptr, uint32_t *pSectorSize = nullptr);

	public:
		/** Public SCSI command wrapper functions **/

		/**
		 * SCSI INQUIRY command.
		 * @param pResp Response buffer.
		 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
		 */
		RP_LIBROMDATA_PUBLIC
		int scsi_inquiry(struct _SCSI_RESP_INQUIRY_STD *pResp);

		/**
		 * ATA IDENTIFY DEVICE command. (via SCSI-ATA pass-through)
		 * @param pResp Response buffer.
		 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
		 */
		RP_LIBROMDATA_PUBLIC
		int ata_identify_device(struct _ATA_RESP_IDENTIFY_DEVICE *pResp);

		/**
		 * ATA IDENTIFY PACKET DEVICE command. (via SCSI-ATA pass-through)
		 * @param pResp Response buffer.
		 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
		 */
		RP_LIBROMDATA_PUBLIC
		int ata_identify_packet_device(struct _ATA_RESP_IDENTIFY_DEVICE *pResp);

	private:
		/**
		 * ATA IDENTIFY (PACKET) DEVICE command. (internal function)
		 * @param pResp Response buffer.
		 * @param packet True for IDENTIFY PACKET DEVICE; false for IDENTIFY DEVICE.
		 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
		 */
		int ata_identify_device_int(struct _ATA_RESP_IDENTIFY_DEVICE *pResp, bool packet);

	public:
		/**
		 * Is this a supported Kreon drive?
		 *
		 * NOTE: This only checks the drive vendor and model.
		 * Check the feature list to determine if it's actually
		 * using Kreon firmware.
		 *
		 * @return True if the drive supports Kreon firmware; false if not.
		 */
		bool isKreonDriveModel(void);

		// Kreon features.
		enum class KreonFeature : uint16_t {
			Header0			= 0xA55A,	// always the first feature
			Header1			= 0x5AA5,	// always the second feature
			Unlock1_X360		= 0x0100,	// Unlock state 1 (xtreme) for Xbox 360
			Unlock2_X360		= 0x0101,	// Unlock state 2 (wxripper) for Xbox 360
			Unlock1a_X360		= 0x0120,	// Unlock state 1 (xtreme) for Xbox 360
			FullChallenge_X360	= 0x0121,	// Full challenge functionality for Xbox 360
			Unlock1_Xbox		= 0x0200,	// Unlock state 1 (xtreme) for Xbox
			Unlock2_Xbox		= 0x0201,	// Unlock state 2 (wxripper) for Xbox
			Unlock1a_Xbox		= 0x0220,	// Unlock state 1 (xtreme) for Xbox
			FullChallenge_Xbox	= 0x0221,	// Full challenge functionality for Xbox
			LockCommand		= 0xF000,	// Lock (cancel unlock state) command
			ErrorSkipping		= 0xF001,	// Error skipping
		};

		/**
		 * Get a list of supported Kreon features.
		 * @return List of Kreon feature IDs, or empty vector if not supported.
		 */
		std::vector<KreonFeature> getKreonFeatureList(void);

		/**
		 * Set Kreon error skip state.
		 * @param skip True to skip; false for normal operation.
		 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
		 */
		int setKreonErrorSkipState(bool skip);

		enum class KreonLockState : uint8_t {
			Locked = 0,
			State1Xtreme = 1,
			State2WxRipper = 2,
		};

		/**
		 * Set Kreon lock state
		 * @param lockState 0 == locked; 1 == Unlock State 1 (xtreme); 2 == Unlock State 2 (wxripper)
		 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
		 */
		int setKreonLockState(KreonLockState lockState);
};

} // namespace LibRpFile
