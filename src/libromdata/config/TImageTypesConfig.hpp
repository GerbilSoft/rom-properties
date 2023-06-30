/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TImageTypesConfig.hpp: Image Types editor template.                     *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

/**
 * NOTE: TImageTypesConfig.cpp MUST be #included by a file in
 * the UI frontend. Otherwise, the instantiated template won't
 * be compiled correctly.
 */

#include "common.h"
#include "ImageTypesConfig.hpp"

// C includes (C++ namespace)
#include <cassert>

// C++ includes
#include <vector>

namespace LibRomData {

template<typename ComboBox>
class TImageTypesConfig
{
	public:
		TImageTypesConfig();
		virtual ~TImageTypesConfig() = default;
	private:
		RP_DISABLE_COPY(TImageTypesConfig)

	public:
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
		/** ImageTypesConfig wrapper functions **/

		/**
		 * Get an image type name.
		 * @param imageType Image type ID.
		 * @return Image type name, or nullptr if invalid.
		 */
		static inline const char *imageTypeName(unsigned int imageType)
		{
			return ImageTypesConfig::imageTypeName(imageType);
		}

		/**
		 * Get a system name.
		 * @param sys System ID
		 * @return System name, or nullptr if invalid.
		 */
		static inline const char *sysName(unsigned int sys)
		{
			return ImageTypesConfig::sysName(sys);
		}

	public:
		/**
		 * Convert image type/system to ComboBox ID.
		 * @param sys System. (column)
		 * @param imageType Image type. (row)
		 * @return ComboBox ID.
		 */
		static inline unsigned int sysAndImageTypeToCbid(unsigned int sys, unsigned int imageType)
		{
			// NOTE: Shifting may need changes if either index exceeds 15.
			assert(sys < ImageTypesConfig::sysCount());
			assert(imageType < ImageTypesConfig::imageTypeCount());
			return (sys << 4) | imageType;
		}

		/**
		 * Get the image type from a ComboBox ID.
		 * @param cbid ComboBox ID.
		 * @return Image type. (column)
		 */
		static inline unsigned int imageTypeFromCbid(unsigned int cbid)
		{
			assert(cbid < (ImageTypesConfig::sysCount() << 4));
			assert((cbid & 15) < ImageTypesConfig::imageTypeCount());
			return (cbid & 15);
		}

		/**
		 * Get the system from a ComboBox ID.
		 * @param cbid ComboBox ID.
		 * @return System. (row)
		 */
		static inline unsigned int sysFromCbid(unsigned int cbid)
		{
			assert(cbid < (ImageTypesConfig::sysCount() << 4));
			assert((cbid & 15) < ImageTypesConfig::imageTypeCount());
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
			assert(sys < ImageTypesConfig::sysCount());
			assert(imageType < ImageTypesConfig::imageTypeCount());
			// TODO: Make this not inline so it doesn't have to call functions?
			return (sys < ImageTypesConfig::sysCount() && imageType < ImageTypesConfig::imageTypeCount());
		}

		/**
		 * Validate a ComboBox ID.
		 * @param cbid ComboBox ID.
		 * @return True if valid; false if not.
		 */
		static inline bool validateCbid(unsigned int cbid)
		{
			// TODO: Make this not inline so it doesn't have to call functions?
			assert(sysFromCbid(cbid) < ImageTypesConfig::sysCount());
			assert(imageTypeFromCbid(cbid) < ImageTypesConfig::imageTypeCount());
			return (sysFromCbid(cbid) < ImageTypesConfig::sysCount() &&
				imageTypeFromCbid(cbid) < ImageTypesConfig::imageTypeCount());
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
		/** Pure virtual functions (protected) **/

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
		virtual int saveStart(void) { return 0; }

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
		virtual int saveFinish(void) { return 0; }

	public:
		/** Pure virtual functions (public) **/

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

		// Array of items for systems.
		// NOTE: Although the system count and image type count are
		// static, we're using a dynamic vector because ImageTypesConfig
		// does not publicly declare the static size to prevent ABI
		// breakages later on.
		struct SysData_t {
			// Vector of comboboxes for image types.
			// NOTE: No system supports *all* image types,
			// so most of these will be nullptr.
			// NOTE: Elements can be checked for nullptr, but the
			// pure virtual functions must be used to check the
			// actual contents.
			std::vector<ComboBox> cboImageType;

			// Image types. (0xFF = No; others = priority)
			// NOTE: We need to store them here in order to handle
			// duplicate prevention, since ComboBox signals usually
			// don't include the "previous" index.
			std::vector<uint8_t> imageTypes;

			// Number of valid image types.
			uint8_t validImageTypes;

			// Does this system have the default configuration?
			// If it does, it will be saved with a blank value.
			bool sysIsDefault;
		};

		std::vector<SysData_t> v_sysData;
};

}
