/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile.hpp: Standard file object.                                       *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_RPFILE_HPP__
#define __ROMPROPERTIES_LIBRPBASE_RPFILE_HPP__

#include "IRpFile.hpp"

// C includes. (C++ namespace)
#include <cstdio>

// C++ includes.
#include <memory>
#include <vector>

namespace LibRpBase {

class RpFilePrivate;
class RpFile : public IRpFile
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
		 * @param filename Filename.
		 * @param mode File mode.
		 */
		RpFile(const char *filename, FileMode mode);
		RpFile(const std::string &filename, FileMode mode);
	private:
		void init(void);
	protected:
		virtual ~RpFile();	// call unref() instead

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
		bool isOpen(void) const final;

		/**
		 * Close the file.
		 */
		void close(void) final;

		/**
		 * Read data from the file.
		 * @param ptr Output data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes read.
		 */
		size_t read(void *ptr, size_t size) final;

		/**
		 * Write data to the file.
		 * @param ptr Input data buffer.
		 * @param size Amount of data to read, in bytes.
		 * @return Number of bytes written.
		 */
		size_t write(const void *ptr, size_t size) final;

		/**
		 * Set the file position.
		 * @param pos File position.
		 * @return 0 on success; -1 on error.
		 */
		int seek(int64_t pos) final;

		/**
		 * Get the file position.
		 * @return File position, or -1 on error.
		 */
		int64_t tell(void) final;

		/**
		 * Truncate the file.
		 * @param size New size. (default is 0)
		 * @return 0 on success; -1 on error.
		 */
		int truncate(int64_t size = 0) final;

	public:
		/** File properties **/

		/**
		 * Get the file size.
		 * @return File size, or negative on error.
		 */
		int64_t size(void) final;

		/**
		 * Get the filename.
		 * @return Filename. (May be empty if the filename is not available.)
		 */
		std::string filename(void) const final;

	public:
		/** Device file functions **/

		/**
		 * Is this a device file?
		 * @return True if this is a device file; false if not.
		 */
		bool isDevice(void) const final;

	protected:
		enum ScsiDirection {
			SCSI_DIR_NONE,
			SCSI_DIR_IN,
			SCSI_DIR_OUT,
		};

		/**
		 * Send a SCSI command to the device.
		 * @param cdb		[in] SCSI command descriptor block
		 * @param cdb_len	[in] Length of cdb
		 * @param data		[in/out] Data buffer, or nullptr for SCSI_DIR_NONE operations
		 * @param data_len	[in] Length of data
		 * @param direction	[in] Data direction
		 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
		 */
		int scsi_send_cdb(const void *cdb, uint8_t cdb_len,
			void *data, size_t data_len,
			ScsiDirection direction);

		/**
		 * Get the capacity of the device using SCSI commands.
		 * @param pDeviceSize	[out] Retrieves the device size, in bytes.
		 * @param pBlockSize	[out,opt] If not NULL, retrieves the block size.
		 * @return 0 on success, positive for SCSI sense key, negative for POSIX error code.
		 */
		int scsi_read_capacity(int64_t *pDeviceSize, uint32_t *pBlockSize = nullptr);

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
		enum KreonFeatures : uint16_t {
			KREON_FEATURE_HEADER_0		= 0xA55A,	// always the first feature
			KREON_FEATURE_HEADER_1		= 0x5AA5,	// always the second feature
			KREON_FEATURE_UNLOCK_1_X360	= 0x0100,	// Unlock state 1 (xtreme) for Xbox 360
			KREON_FEATURE_UNLOCK_2_X360	= 0x0101,	// Unlock state 2 (wxripper) for Xbox 360
			KREON_FEATURE_UNLOCK_1a_X360	= 0x0120,	// Unlock state 1 (xtreme) for Xbox 360
			KREON_FEATURE_FULL_CHLNG_X360	= 0x0121,	// Full challenge functionality for Xbox 360
			KREON_FEATURE_UNLOCK_1_XBOX	= 0x0200,	// Unlock state 1 (xtreme) for Xbox
			KREON_FEATURE_UNLOCK_2_XBOX	= 0x0201,	// Unlock state 2 (wxripper) for Xbox
			KREON_FEATURE_UNLOCK_1a_XBOX	= 0x0220,	// Unlock state 1 (xtreme) for Xbox
			KREON_FEATURE_FULL_CHLNG_XBOX	= 0x0221,	// Full challenge functionality for Xbox
			KREON_FEATURE_LOCK_COMMAND	= 0xF000,	// Lock (cancel unlock state) command
			KREON_FEATURE_ERROR_SKIPPING	= 0xF001,	// Error skipping
		};

		/**
		 * Get a list of supported Kreon features.
		 * @return List of Kreon feature IDs, or empty vector if not supported.
		 */
		std::vector<uint16_t> getKreonFeatureList(void);

		/**
		 * Set Kreon error skip state.
		 * @param skip True to skip; false for normal operation.
		 * @return 0 on success; non-zero on error.
		 */
		int setKreonErrorSkipState(bool skip);

		/**
		 * Set Kreon lock state
		 * @param lockState 0 == locked; 1 == Unlock State 1 (xtreme); 2 == Unlock State 2 (wxripper)
		 * @return 0 on success; non-zero on error.
		 */
		int setKreonLockState(uint8_t lockState);
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IRPFILE_HPP__ */
