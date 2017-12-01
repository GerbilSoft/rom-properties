/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TImageTypesConfig.hpp: Image Types editor template.                     *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_HPP__
#define __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_HPP__

/**
 * NOTE: TImageTypesConfig.cpp MUST be #included by a file in
 * the UI frontend. Otherwise, the instantiated template won't
 * be compiled correctly.
 */

#include "librpbase/common.h"
#include "librpbase/RomData.hpp"

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

// NOTE: SysData_t is defined here because it causes
// problems if it's embedded inside of a templated class.
typedef uint32_t (*pFnSupportedImageTypes)(void);
struct SysData_t {
	const char *className;			// Class name in Config. (ASCII)
	pFnSupportedImageTypes getTypes;	// Get supported image types.
};
#define SysDataEntry(klass) \
	{#klass, LibRomData::klass::supportedImageTypes_static}

template<typename ComboBox>
class TImageTypesConfig
{
	public:
		TImageTypesConfig();
		virtual ~TImageTypesConfig();
	private:
		RP_DISABLE_COPY(TImageTypesConfig)

	public:
		// Number of image types. (columns)
		static const int IMG_TYPE_COUNT = LibRpBase::RomData::IMG_EXT_MAX+1;
		// Number of systems. (rows)
		static const int SYS_COUNT = 10;

		/**
		 * Create the grid of labels and ComboBoxes.
		 */
		void createGrid(void);

	protected:
		/**
		 * (Re-)Load the configuration into the grid.
		 * @param loadDefaults If true, use the default configuration instead of the user configuration.
		 * @return True if anything was modified; false if not.
		 */
		bool reset_int(bool loadDefaults);

	public:
		/**
		 * (Re-)Load the configuration into the grid.
		 */
		void reset(void);

		/**
		 * Load the default configuration.
		 * This does NOT save and will not clear 'changed'.
		 * @return True if anything was modified; false if not.
		 */
		bool loadDefaults(void);

		/**
		 * Save the configuration from the grid.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int save(void);

	public:
		/**
		 * Get an image type name.
		 * @param imageType Image type ID.
		 * @return Image type name, or nullptr if invalid.
		 */
		static const char *imageTypeName(unsigned int imageType);

		// System data. (SYS_COUNT)
		static const SysData_t sysData[];

		/**
		 * Get a system name.
		 * @param sys System ID.
		 * @return System name, or nullptr if invalid.
		 */
		static const char *sysName(unsigned int sys);

	public:
		/**
		 * Convert image type/system to ComboBox ID.
		 * @param sys System. (column)
		 * @param imageType Image type. (row)
		 * @return ComboBox ID.
		 */
		static inline unsigned int sysAndImageTypeToCbid(unsigned int sys, unsigned int imageType)
		{
			assert(sys < SYS_COUNT);
			assert(imageType < IMG_TYPE_COUNT);
			return (sys << 4) | imageType;
		}

		/**
		 * Get the image type from a ComboBox ID.
		 * @param cbid ComboBox ID.
		 * @return Image type. (column)
		 */
		static inline unsigned int imageTypeFromCbid(unsigned int cbid)
		{
			assert(cbid < (SYS_COUNT << 4));
			assert((cbid & 15) < IMG_TYPE_COUNT);
			return (cbid & 15);
		}

		/**
		 * Get the system from a ComboBox ID.
		 * @param cbid ComboBox ID.
		 * @return System. (row)
		 */
		static inline unsigned int sysFromCbid(unsigned int cbid)
		{
			assert(cbid < (SYS_COUNT << 4));
			assert((cbid & 15) < IMG_TYPE_COUNT);
			return (cbid >> 4);
		}

		/**
		 * Validate system and image types.
		 * @param sys System.
		 * @param imageType Image type.
		 * @return True if valid; false if not.
		 */
		static inline bool validateSysImageType(unsigned int sys, unsigned int imageType)
		{
			assert(sys < SYS_COUNT);
			assert(imageType < IMG_TYPE_COUNT);
			return (sys < SYS_COUNT && imageType < IMG_TYPE_COUNT);
		}

		/**
		 * Validate a ComboBox ID.
		 * @param cbid ComboBox ID.
		 * @return True if valid; false if not.
		 */
		static inline bool validateCbid(unsigned int cbid)
		{
			assert(sysFromCbid(cbid) < SYS_COUNT);
			assert(imageTypeFromCbid(cbid) < IMG_TYPE_COUNT);
			return (sysFromCbid(cbid) < SYS_COUNT &&
				imageTypeFromCbid(cbid) < IMG_TYPE_COUNT);
		}

	public:
		/** UI signals. **/

		/**
		 * A ComboBox index was changed by the user.
		 * @param cbid ComboBox ID.
		 * @param prio New priority value. (0xFF == no)
		 * @return True if changed; false if not.
		 */
		bool cboImageType_priorityValueChanged(unsigned int cbid, unsigned int prio);

	protected:
		/** Pure virtual functions. (protected) **/

		/**
		 * Create the labels in the grid.
		 */
		virtual void createGridLabels(void) = 0;

		/**
		 * Create a ComboBox in the grid.
		 * @param cbid ComboBox ID.
		 */
		virtual void createComboBox(unsigned int cbid) = 0;

		/**
		 * Add strings to a ComboBox in the grid.
		 * @param cbid ComboBox ID.
		 * @param max_prio Maximum priority value. (minimum is 1)
		 */
		virtual void addComboBoxStrings(unsigned int cbid, int max_prio) = 0;

		/**
		 * Finish adding the ComboBoxes.
		 */
		virtual void finishComboBoxes(void) = 0;

		/**
		 * Initialize the Save subsystem.
		 * This is needed on platforms where the configuration file
		 * must be opened with an appropriate writer class.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int saveStart(void) = 0;

		/**
		 * Write an ImageType configuration entry.
		 * @param sysName System name.
		 * @param imageTypeList Image type list, comma-separated.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int saveWriteEntry(const char *sysName, const char *imageTypeList) = 0;

		/**
		 * Close the Save subsystem.
		 * This is needed on platforms where the configuration file
		 * must be opened with an appropriate writer class.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		virtual int saveFinish(void) = 0;

	public:
		/** Pure virtual functions. (public) **/

		/**
		 * Set a ComboBox's current index.
		 * This will not trigger cboImageType_priorityValueChanged().
		 * @param cbid ComboBox ID.
		 * @param prio New priority value. (0xFF == no)
		 */
		virtual void cboImageType_setPriorityValue(unsigned int cbid, unsigned int prio) = 0;

	public:
		// Has the user changed anything?
		bool changed;

		// Combo box array.
		// NOTE: This is a square array, but no system supports
		// *all* image types, so most of these will be nullptr.
		// NOTE: Elements can be checked for nullptr, but the
		// pure virtual functions must be used to check the
		// actual contents.
		ComboBox cboImageType[SYS_COUNT][IMG_TYPE_COUNT];

		// Image types. (0xFF == No; others == priority)
		// NOTE: We need to store them here in order to handle
		// duplicate prevention, since ComboBox signals usually
		// don't include the "previous" index.
		uint8_t imageTypes[SYS_COUNT][IMG_TYPE_COUNT];

		// Number of valid image types per system.
		uint8_t validImageTypes[SYS_COUNT];

		// Which systems have the default configuration?
		// These ones will be saved with a blank value.
		bool sysIsDefault[SYS_COUNT];
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_CONFIG_TIMAGETYPESCONFIG_HPP__ */
