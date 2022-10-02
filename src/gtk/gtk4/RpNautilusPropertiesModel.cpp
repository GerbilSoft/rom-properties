/*****************************************************************************
 * ROM Properties Page shell extension. (GTK4)                               *
 * RpNautilusPropertiesModel.hpp: Nautilus properties model                  *
 *                                                                           *
 * NOTE: Nautilus 43 only accepts key/value pairs for properties, instead of *
 * arbitrary GtkWidgets. As such, the properties returned will be more       *
 * limited than in previous versions.                                        *
 *                                                                           *
 * Copyright (c) 2017-2022 by David Korth.                                   *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#include "stdafx.h"
#include "RpNautilusPropertiesModel.hpp"
#include "libi18n/i18n.h"

// librpbase
#include "librpbase/RomData.hpp"
using namespace LibRpBase;

// C++ STL classes
using std::string;

// Internal data.
// Based on Nautilus 43's image-extension.
// Reference: https://github.com/GNOME/nautilus/blob/43.0/extensions/image-properties/nautilus-image-properties-model.c
typedef struct _RpNautilusPropertiesModel {
	GListStore *listStore;
} RpNautilusPropertiesModel;

static void
rp_nautilus_properties_model_free(RpNautilusPropertiesModel *self)
{
	g_free(self);
}

static void
append_item(RpNautilusPropertiesModel *self,
            const char                *name,
            const char                *value)
{
	NautilusPropertiesItem *const item = nautilus_properties_item_new(name, value);
	g_list_store_append(self->listStore, item);
	g_object_unref(item);
}

static void
rp_nautilus_properties_model_init(RpNautilusPropertiesModel *self)
{
	self->listStore = g_list_store_new(NAUTILUS_TYPE_PROPERTIES_ITEM);
}

/**
 * Initialize a string field.
 * @param self		[in] RpNautilusPropertiesModel
 * @param field		[in] RomFields::Field
 * @param str		[in,opt] String data (If nullptr, field data is used)
 */
static void
rp_nautilus_properties_model_init_string(RpNautilusPropertiesModel *self,
	const RomFields::Field &field, const char *str = nullptr)
{
	if (!str) {
		str = field.data.str;
	}

	if (field.type == RomFields::RFT_STRING &&
	    (field.flags & RomFields::STRF_CREDITS))
	{
		// TODO: Handle credits.
		return;
	}

	// TODO: Other formatting options?
	append_item(self, field.name, str);
}

/**
 * Initialize a bitfield.
 * @param self		[in] RpNautilusPropertiesModel
 * @param field		[in] RomFields::Field
 */
static void
rp_nautilus_properties_model_init_bitfield(RpNautilusPropertiesModel *self, const RomFields::Field &field)
{
	// Using Unicode symbols to simulate checkboxes.
	const auto &bitfieldDesc = field.desc.bitfield;
	assert(bitfieldDesc.names->size() <= 32);

	string str;
	str.reserve(bitfieldDesc.names->size() * 24);

	int col = 0;
	uint32_t bitfield = field.data.bitfield;
	const auto names_cend = bitfieldDesc.names->cend();
	for (auto iter = bitfieldDesc.names->cbegin(); iter != names_cend; ++iter, bitfield >>= 1) {
		const string &name = *iter;
		if (name.empty())
			continue;

		// FIXME: Fall back if a color emoji font can't be loaded.
		gboolean value = (bitfield & 1);
		str += (value) ? "✅ " : "🟩 ";
		//str += (value) ? "☑ " : "☐ ";
		str += name;

		if ((iter + 1) != names_cend) {
			col++;
			if (col == bitfieldDesc.elemsPerRow) {
				str += "\n";
				col = 0;
			} else if (!str.empty()) {
				// FIXME: Better alignment. (markup isn't supported)
				str += "    ";
			}
		}
	}

	rp_nautilus_properties_model_init_string(self, field, str.c_str());
}

/**
 * Initialize a Date/Time field.
 * @param self		[in] RpNautilusPropertiesModel
 * @param field		[in] RomFields::Field
 */
static void
rp_nautilus_properties_model_init_datetime(RpNautilusPropertiesModel *self, const RomFields::Field &field)
{
	// Date/Time.
	if (field.data.date_time == -1) {
		// tr: Invalid date/time.
		rp_nautilus_properties_model_init_string(self, field, C_("RomDataView", "Unknown"));
		return;
	}

	GDateTime *dateTime;
	if (field.flags & RomFields::RFT_DATETIME_IS_UTC) {
		dateTime = g_date_time_new_from_unix_utc(field.data.date_time);
	} else {
		dateTime = g_date_time_new_from_unix_local(field.data.date_time);
	}
	assert(dateTime != nullptr);
	if (!dateTime) {
		// Unable to convert the timestamp.
		rp_nautilus_properties_model_init_string(self, field, C_("RomDataView", "Unknown"));
		return;
	}

	static const char formats_strtbl[] =
		"\0"		// [0] No date or time
		"%x\0"		// [1] Date
		"%X\0"		// [4] Time
		"%x %X\0"	// [7] Date Time

		// TODO: Better localization here.
		"\0"		// [13] No date or time
		"%b %d\0"	// [14] Date (no year)
		"%X\0"		// [20] Time
		"%b %d %X\0";	// [23] Date Time (no year)
	static const uint8_t formats_offtbl[8] = {0, 1, 4, 7, 13, 14, 20, 23};
	static_assert(sizeof(formats_strtbl) == 33, "formats_offtbl[] needs to be recalculated");

	const unsigned int offset = (field.flags & RomFields::RFT_DATETIME_HAS_DATETIME_NO_YEAR_MASK);
	const char *const format = &formats_strtbl[formats_offtbl[offset]];
	assert(format[0] != '\0');

	GtkWidget *widget = nullptr;
	if (format[0] != '\0') {
		gchar *const str = g_date_time_format(dateTime, format);
		if (str) {
			rp_nautilus_properties_model_init_string(self, field, str);
			g_free(str);
		}
	}

	g_date_time_unref(dateTime);
}

/**
 * Initialize an Age Ratings field.
 * @param self		[in] RpNautilusPropertiesModel
 * @param field		[in] RomFields::Field
 */
static void
rp_nautilus_properties_model_init_age_ratings(RpNautilusPropertiesModel *self, const RomFields::Field &field)
{
	const RomFields::age_ratings_t *const age_ratings = field.data.age_ratings;
	assert(age_ratings != nullptr);
	if (!age_ratings) {
		// tr: No age ratings data.
		rp_nautilus_properties_model_init_string(self, field, C_("RomDataView", "ERROR"));
		return;
	}

	// Convert the age ratings field to a string.
	const string str = RomFields::ageRatingsDecode(age_ratings);
	rp_nautilus_properties_model_init_string(self, field, str.c_str());
}

/**
 * Initialize a Dimensions field.
 * @param self		[in] RpNautilusPropertiesModel
 * @param field		[in] RomFields::Field
 */
static void
rp_nautilus_properties_model_init_dimensions(RpNautilusPropertiesModel *self, const RomFields::Field &field)
{
	// TODO: 'x' or '×'? Using 'x' for now.
	const int *const dimensions = field.data.dimensions;
	char buf[64];
	if (dimensions[1] > 0) {
		if (dimensions[2] > 0) {
			snprintf(buf, sizeof(buf), "%dx%dx%d",
				dimensions[0], dimensions[1], dimensions[2]);
		} else {
			snprintf(buf, sizeof(buf), "%dx%d",
				dimensions[0], dimensions[1]);
		}
	} else {
		snprintf(buf, sizeof(buf), "%d", dimensions[0]);
	}

	rp_nautilus_properties_model_init_string(self, field, buf);
}

/**
 * Initialize a multi-language string field.
 * @param self		[in] RpNautilusPropertiesModel
 * @param field		[in] RomFields::Field
 * @param def_lc	[in] RomFields default language code
 */
static void
rp_nautilus_properties_model_init_string_multi(RpNautilusPropertiesModel *self, const RomFields::Field &field, uint32_t def_lc = 0)
{
	// Mutli-language string.
	// FIXME: We can't easily change the language, so only the
	// system default language will be used for now.

	const auto *const pStr_multi = field.data.str_multi;
	assert(pStr_multi != nullptr);
	assert(!pStr_multi->empty());
	const string *pStr = nullptr;
	if (pStr_multi && !pStr_multi->empty()) {
		// Get the string.
		pStr = RomFields::getFromStringMulti(pStr_multi, def_lc, 0);
		assert(pStr != nullptr);
	} else {
		// Empty string.
		pStr = nullptr;
	}

	rp_nautilus_properties_model_init_string(self, field, (pStr ? pStr->c_str() : nullptr));
}

static void
rp_nautilus_properties_model_load_from_romData(RpNautilusPropertiesModel *self,
                                               const RomData             *romData)
{
	// NOTE: Not taking a reference to RomData.

	// TODO: Asynchronous field loading. (separate thread?)
	// If implemented, check Nautilus 43's image-extension for cancellable.

	// System name and file type.
	// TODO: System logo and/or game title?
	const char *systemName = romData->systemName(
		RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);
	const char *fileType = romData->fileType_string();
	assert(systemName != nullptr);
	assert(fileType != nullptr);
	if (!systemName) {
		systemName = C_("RomDataView", "(unknown system)");
	}
	if (!fileType) {
		fileType = C_("RomDataView", "(unknown filetype)");
	}

	// Add a "File Type" field with the system name and file type.
	// Other UI frontends have dedicated widgets for this.
	// NOTE: Using " | " separator; other UI frontends use "\n". (rpcli uses a single space)
	const string sysInfo = rp_sprintf_p(
		// tr: %1$s == system name, %2$s == file type
		C_("RomDataView", "%1$s | %2$s"), systemName, fileType);
	append_item(self, C_("RomDataView", "File Type"), sysInfo.c_str());

	// Process RomData fields.
	// NOTE: Not all field types can be handled here,
	// and we can't do tabs.
	const RomFields *const pFields = romData->fields();
	assert(pFields != nullptr);
	if (!pFields) {
		// No fields.
		// TODO: Show an error?
		return;
	}

	const uint32_t def_lc = pFields->defaultLanguageCode();

	const auto pFields_cend = pFields->cend();
	for (auto iter = pFields->cbegin(); iter != pFields_cend; ++iter) {
		const RomFields::Field &field = *iter;
		assert(field.isValid);
		if (!field.isValid)
			continue;

		switch (field.type) {
			case RomFields::RFT_INVALID:
				// No data here.
				break;
			default:
				// Unsupported right now.
				assert(!"Unsupported RomFields::RomFieldsType.");
				break;

			case RomFields::RFT_STRING:
				rp_nautilus_properties_model_init_string(self, field);
				break;
			case RomFields::RFT_BITFIELD:
				rp_nautilus_properties_model_init_bitfield(self, field);
				break;
			case RomFields::RFT_LISTDATA:
				// Can't easily do RFT_LISTDATA here.
				break;
			case RomFields::RFT_DATETIME:
				rp_nautilus_properties_model_init_datetime(self, field);
				break;
			case RomFields::RFT_AGE_RATINGS:
				rp_nautilus_properties_model_init_age_ratings(self, field);
				break;
			case RomFields::RFT_DIMENSIONS:
				rp_nautilus_properties_model_init_dimensions(self, field);
				break;
			case RomFields::RFT_STRING_MULTI:
				rp_nautilus_properties_model_init_string_multi(self, field, def_lc);
				break;
		}
	}
}

NautilusPropertiesModel *
rp_nautilus_properties_model_new(const RomData *romData)
{
	RpNautilusPropertiesModel *const self = g_new0 (RpNautilusPropertiesModel, 1);

	rp_nautilus_properties_model_init(self);
	rp_nautilus_properties_model_load_from_romData(self, romData);

	NautilusPropertiesModel *model = nautilus_properties_model_new(
		C_("RomDataView", "ROM Properties"), G_LIST_MODEL(self->listStore));

	g_object_weak_ref(G_OBJECT(model), (GWeakNotify)rp_nautilus_properties_model_free, self);

	return model;
}
