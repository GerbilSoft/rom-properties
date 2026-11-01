/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataView.cpp: RomData viewer widget. (ROM operations)                *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RomDataView.hpp"
#include "RomDataView_p.hpp"
#include "RpGtk.h"

// RpFileGio
#include "RpFile_gio.hpp"

// Other rom-properties libraries
#include "libi18n/i18n.hpp"
#include "librpbase/TextOut.hpp"
#include "librpfile/FileSystem.hpp"
#include "librpfile/streambuf_IRpFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;

// C includes
#include "tcharx.h"

// C++ STL classes
#include <sstream>
using std::ostream;
using std::ostringstream;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;

// Custom widgets
#include "LanguageComboBox.hpp"
#include "MessageWidget.h"
#include "OptionsMenuButton.hpp"

// ENABLE_MESSAGESOUND is set by CMakeLists.txt.
#ifdef ENABLE_MESSAGESOUND
#  include "MessageSound.hpp"
#endif /* ENABLE_MESSAGESOUND */

// libfmt
#include "rp-libfmt.h"

/**
 * Update a field's value.
 * This is called after running a ROM operation.
 * @param page		[in] RpRomDataView
 * @param fieldIdx	[in] Field index
 * @return 0 on success; non-zero on error.
 */
int
rp_rom_data_view_update_field(RpRomDataView *page, int fieldIdx)
{
	assert(page != nullptr);
	assert(page->cxx != nullptr);
	_RpRomDataViewCxx *const cxx = page->cxx;

	const RomFields *const pFields = cxx->romData->fields();
	assert(pFields != nullptr);
	if (!pFields) {
		// No fields.
		// TODO: Show an error?
		return 1;
	}

	assert(fieldIdx >= 0);
	assert(fieldIdx < pFields->count());
	if (fieldIdx < 0 || fieldIdx >= pFields->count())
		return 2;

	const RomFields::Field *const field = pFields->at(fieldIdx);
	assert(field != nullptr);
	if (!field)
		return 3;

	// Lambda function to check a GObject's RFT_fieldIdx.
	auto checkFieldIdx = [](GtkWidget *widget, int fieldIdx) noexcept -> bool {
		// NOTE: RFT_fieldIdx starts at 1 to prevent conflicts with widgets
		// that don't have RFT_fieldIdx, which would return NULL here.
		const gint tmp_fieldIdx = GPOINTER_TO_INT(
			g_object_get_qdata(G_OBJECT(widget), RFT_fieldIdx_quark));
		return (tmp_fieldIdx != 0 && (tmp_fieldIdx - 1) == fieldIdx);
	};

	// Get the GtkWidget*.
	// NOTE: Linear search through all display objects, since
	// this function isn't used that often.
	GtkWidget *widget = nullptr;
	for (const auto &tab : cxx->tabs) {
		GtkWidget *const table = tab.table;	// GtkTable (2.x); GtkGrid (3.x)

#if GTK_CHECK_VERSION(4, 0, 0)
		// Enumerate the child widgets.
		// NOTE: Widgets are enumerated in forwards order.
		// TODO: Needs testing!
		for (GtkWidget *tmp_widget = gtk_widget_get_first_child(table);
		     tmp_widget != nullptr; tmp_widget = gtk_widget_get_next_sibling(tmp_widget))
		{
			// Check if the field index is correct.
			if (checkFieldIdx(tmp_widget, fieldIdx)) {
				// Found the field.
				widget = tmp_widget;
				break;
			}
		}
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
		// Get the list of child widgets.
		// NOTE: Widgets are enumerated in forwards order,
		// since the list head is the first item.
		GList *const widgetList = gtk_container_get_children(GTK_CONTAINER(table));
		if (!widgetList)
			continue;

		for (GList *widgetIter = widgetList; widgetIter != nullptr;
		     widgetIter = widgetIter->next)
		{
			GtkWidget *const tmp_widget = GTK_WIDGET(widgetIter->data);
			if (!tmp_widget)
				continue;

			// Check if the field index is correct.
			if (checkFieldIdx(tmp_widget, fieldIdx)) {
				// Found the field.
				widget = tmp_widget;
				break;
			}
		}
		g_list_free(widgetList);
#endif
	}

	// Update the value widget(s).
	int ret;
	switch (field->type) {
		case RomFields::RomFieldType::RFT_INVALID:
			assert(!"Cannot update an RFT_INVALID field.");
			ret = 5;
			break;
		default:
			assert(!"Unsupported field type.");
			ret = 6;
			break;

		case RomFields::RomFieldType::RFT_STRING: {
			// GtkWidget is a GtkLabel.
			assert(GTK_IS_LABEL(widget));
			if (!GTK_IS_LABEL(widget)) {
				ret = 7;
				break;
			}

			gtk_label_set_text(GTK_LABEL(widget), field->data.str);
			ret = 0;
			break;
		}

		case RomFields::RomFieldType::RFT_BITFIELD: {
			// GtkWidget is a GtkGrid/GtkTable GtkCheckButton widgets.
#ifdef USE_GTK_GRID
			assert(GTK_IS_GRID(widget));
			if (!GTK_IS_GRID(widget)) {
				ret = 8;
				break;
			}
#else /* !USE_GTK_GRID */
			assert(GTK_IS_TABLE(widget));
			if (!GTK_IS_TABLE(widget)) {
				ret = 8;
				break;
			}
#endif /* USE_GTK_GRID */

			// Bits with a blank name aren't included, so we'll need to iterate
			// over the bitfield description.
			const auto &bitfieldDesc = field->desc.bitfield;

#if GTK_CHECK_VERSION(4, 0, 0)
			// Get the first child.
			// NOTE: Widgets are enumerated in forwards order.
			// TODO: Needs testing!
			GtkWidget *checkBox = gtk_widget_get_first_child(widget);
			if (!checkBox) {
				ret = 9;
				break;
			}

			// Inhibit the "no-toggle" signal while updating.
			page->inhibit_checkbox_no_toggle = true;

			uint32_t bitfield = field->data.bitfield;
			const auto names_cend = bitfieldDesc.names->cend();
			for (auto iter = bitfieldDesc.names->cbegin(); iter != names_cend && checkBox != nullptr;
			     ++iter, checkBox = gtk_widget_get_next_sibling(checkBox), bitfield >>= 1)
			{
				const string &name = *iter;
				if (name.empty())
					continue;

				assert(GTK_IS_CHECK_BUTTON(checkBox));
				if (!GTK_IS_CHECK_BUTTON(checkBox))
					break;

				const bool value = (bitfield & 1);
				gtk_check_button_set_active(GTK_CHECK_BUTTON(checkBox), value);
				g_object_set_qdata(G_OBJECT(checkBox), RFT_BITFIELD_value_quark, GUINT_TO_POINTER((guint)value));
			}
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
			// Get the list of child widgets.
			// NOTE: Widgets are enumerated in reverse order.
			GList *const widgetList = gtk_container_get_children(GTK_CONTAINER(widget));
			if (!widgetList) {
				ret = 9;
				break;
			}
			GList *checkBoxIter = g_list_last(widgetList);

			// Inhibit the "no-toggle" signal while updating.
			page->inhibit_checkbox_no_toggle = true;

			uint32_t bitfield = field->data.bitfield;
			const auto names_cend = bitfieldDesc.names->cend();
			for (auto iter = bitfieldDesc.names->cbegin(); iter != names_cend && checkBoxIter != nullptr;
			     ++iter, checkBoxIter = checkBoxIter->prev, bitfield >>= 1)
			{
				const string &name = *iter;
				if (name.empty())
					continue;

				GtkWidget *const checkBox = GTK_WIDGET(checkBoxIter->data);
				assert(GTK_IS_CHECK_BUTTON(checkBox));
				if (!GTK_IS_CHECK_BUTTON(checkBox))
					break;

				const bool value = (bitfield & 1);
				gtk_check_button_set_active(GTK_CHECK_BUTTON(checkBox), value);
				g_object_set_qdata(G_OBJECT(checkBox), RFT_BITFIELD_value_quark, GUINT_TO_POINTER((guint)value));
			}
			g_list_free(widgetList);
#endif /* GTK_CHECK_VERSION(4, 0, 0) */

			// Done updating.
			page->inhibit_checkbox_no_toggle = false;
			ret = 0;
			break;
		}
	}

	return ret;
}

/**
 * Show a message in the Message Widget.
 * @param page RomDataView
 * @param messageType GtkMessageType
 * @param msg Message
 */
static void
rp_rom_data_view_show_message(RpRomDataView *page, GtkMessageType messageType, const char *msg)
{
#ifdef ENABLE_MESSAGESOUND
	MessageSound::play(messageType, msg, GTK_WIDGET(page));
#endif /* ENABLE_MESSAGESOUND */

	// Show the MessageWidget.
	if (!page->messageWidget) {
		page->messageWidget = rp_message_widget_new();
		rp_message_widget_set_transition_type(RP_MESSAGE_WIDGET(page->messageWidget), GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
#if GTK_CHECK_VERSION(4, 0, 0)
		gtk_box_append(GTK_BOX(page), page->messageWidget);
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
		gtk_box_pack_end(GTK_BOX(page), page->messageWidget, false, false, 0);
#endif /* GTK_CHECK_VERSION(4, 0, 0) */
	}

	RpMessageWidget *const messageWidget = RP_MESSAGE_WIDGET(page->messageWidget);
	rp_message_widget_set_message_type(messageWidget, messageType);
	rp_message_widget_set_text(messageWidget, msg);
	rp_message_widget_set_reveal_child(messageWidget, true);
}

// Simple struct for passing both RomDataView and the operation ID.
struct save_data_t {
	RpRomDataView *page;
	int id;
	bool isFileRequired;	// Is a filename required?
				// True for standard ops.
				// For ROM-specific, only true if ROF_SAVE_FILE is set.
};

/**
 * File dialog callback function.
 * @param file (in) (transfer full): Selected file, or nullptr if no file was selected
 * @param save_data (in) (transfer full): open_data_t specified as user_data when rpGtk_getSaveFileName() was called.
 */
static void
rp_rom_data_view_getSaveFileDialog_callback(GFile *file, save_data_t *save_data)
{
	if (!file && save_data->isFileRequired) {
		// No file selected, but a file is required.
		g_free(save_data);
		return;
	}

	// NOTE: Only using RpFileGio for file writing here.
	// RpFile isn't needed because the file write capabilities should be the same.

	// Get the URI.
	gchar *const uri = g_file_get_uri(file);
	if (!uri && save_data->isFileRequired) {
		// No URI, but a file is required...
		g_free(save_data);
		return;
	}

	// for convenience purposes
	RpRomDataView *const page = save_data->page;
	RomData *const romData = page->cxx->romData.get();
	const int id = save_data->id;
	g_free(save_data);

	// Save the previous export directory.
	// FIXME: Handle this with non-local files?
	{
		gchar *const filename = g_file_get_path(file);
		if (filename) {
			g_free(page->prevExportDir);
			page->prevExportDir = g_path_get_dirname(filename);
			g_free(filename);
		}
	}

	// Open the output file using RpFileGio.
	shared_ptr<RpFileGio> out_file = std::make_shared<RpFileGio>(uri, RpFileGio::FM_CREATE_WRITE);
	g_free(uri);
	if (!out_file->isOpen()) {
		// Show an error message.
		// TODO: Include the filename?
		// TODO: Thread-safe strerror()?
		const string msg = fmt::format(FRUN(C_("RomDataView", "Error opening output file: {:s}")),
			strerror(out_file->lastError()));

		// Delete the file if it was created.
		out_file.reset();
		g_file_delete(file, nullptr, nullptr);
		g_object_unref(file);
		
		rp_rom_data_view_show_message(page, GTK_MESSAGE_WARNING, msg.c_str());
		return;
	}

	if (id < 0) {
		// Standard operation.
		unique_ptr<streambuf_IRpFile> streambuf(new streambuf_IRpFile(out_file, std::ios_base::out));
		ostream os(streambuf.get());
		if (os.fail()) {
			// TODO: Show an error message?
			out_file.reset();
			streambuf.reset();
			g_file_delete(file, nullptr, nullptr);
			g_object_unref(file);
			return;
		}

		switch (id) {
			case OPTION_EXPORT_TEXT: {
				const uint32_t sel_lc = page->cboLanguage
					? rp_language_combo_box_get_selected_lc(RP_LANGUAGE_COMBO_BOX(page->cboLanguage))
					: 0;

				os << "== " << fmt::format(FRUN(C_("RomDataView", "File: '{:s}'")), romData->filename()) << '\n';
				ROMOutput ro(romData, sel_lc);
				os << ro;
				break;
			}

			case OPTION_EXPORT_JSON: {
				JSONROMOutput jsro(romData);
				os << jsro << '\n';
				break;
			}

			default:
				assert(!"Invalid ID for an Export Standard ROM Operation.");
				break;
		}

		os.flush();
		streambuf->pubsync();
		g_object_unref(file);
		return;
	}

	// Run the ROM operation.
	RomData::RomOpParams params;
	params.save_file = std::move(out_file);
	const int ret = romData->doRomOp(id, &params);

	GtkMessageType messageType;
	if (ret == 0) {
		// ROM operation completed.

		// Update fields.
		for (const int fieldIdx : params.fieldIdx) {
			rp_rom_data_view_update_field(page, fieldIdx);
		}

		// Update the RomOp menu entry in case it changed.
		// TODO: Don't keep rebuilding this vector...
		// NOTE: Assuming the RomOps vector order hasn't changed.
		vector<RomData::RomOp> ops = romData->romOps();
		assert(id < static_cast<int>(ops.size()));
		if (id < static_cast<int>(ops.size())) {
			rp_options_menu_button_update_op(RP_OPTIONS_MENU_BUTTON(page->btnOptions), id, &ops[id]);
		}

		messageType = GTK_MESSAGE_INFO;
	} else {
		// An error occurred...
		messageType = GTK_MESSAGE_WARNING;

		// Delete the file, since it's likely only 0 bytes.
		params.save_file.reset();
		g_file_delete(file, nullptr, nullptr);
	}

	g_object_unref(file);
	if (!params.msg.empty()) {
		rp_rom_data_view_show_message(page, messageType, params.msg.c_str());
	}
}

/**
 * ROM operation: Standard Operations
 * Dispatched by btnOptions_triggered_signal_handler().
 * @param page RpRomDataView
 * @param id Standard action ID
 */
static void
rp_rom_data_view_doRomOp_stdop(RpRomDataView *page, int id)
{
	// Prevent unused variable warnings for some quarks.
	RP_UNUSED(RFT_STRING_warning_quark);
	RP_UNUSED(RFT_LISTDATA_rows_visible_quark);

	// for convenience purposes
	const RomData *const romData = page->cxx->romData.get();

	const char *const rom_filename = romData->filename();
	if (!rom_filename)
		return;

	const char *title = nullptr;
	const char *filter = nullptr;
	const char *default_ext = nullptr;

	// Check the standard operation.
	switch (id) {
		case OPTION_COPY_TEXT: {
			const uint32_t sel_lc = page->cboLanguage
				? rp_language_combo_box_get_selected_lc(RP_LANGUAGE_COMBO_BOX(page->cboLanguage))
				: 0;

			ostringstream oss;
			oss << "== " << fmt::format(FRUN(C_("RomDataView", "File: '{:s}'")), rom_filename) << '\n';
			ROMOutput ro(romData, sel_lc);
			oss << ro;
			oss.flush();
			rp_gtk_main_clipboard_set_text(oss.str().c_str());
			// Nothing else to do here.
			return;
		}

		case OPTION_COPY_JSON: {
			ostringstream oss;
			JSONROMOutput jsro(romData);
			oss << jsro << '\n';
			oss.flush();
			rp_gtk_main_clipboard_set_text(oss.str().c_str());
			// Nothing else to do here.
			return;
		}

		case OPTION_EXPORT_TEXT:
			title = C_("RomDataView", "Export to Text File");
			// tr: "Text Files" filter (RP format)
			filter = C_("RomDataView", "Text Files|*.txt|text/plain|All Files|*|-");
			default_ext = ".txt";
			break;

		case OPTION_EXPORT_JSON:
			title = C_("RomDataView", "Export to JSON File");
			// tr: "JSON Files" filter (RP format)
			filter = C_("RomDataView", "JSON Files|*.json|application/json|All Files|*|-");
			default_ext = ".json";
			break;

		default:
			assert(!"Invalid ID for a Standard ROM Operation.");
			return;
	}

	GtkWindow *const parent = gtk_widget_get_toplevel_window(GTK_WIDGET(page));

	// Get the initial name.
	gchar *const basename = g_path_get_basename(rom_filename);
	string defaultName = basename;
	g_free(basename);
	// Remove the extension, if present.
	const size_t extpos = defaultName.rfind('.');
	if (extpos != string::npos) {
		defaultName.resize(extpos);
	}
	defaultName += default_ext;

	save_data_t *const save_data = static_cast<save_data_t*>(g_malloc(sizeof(*save_data)));
	save_data->page = page;
	save_data->id = id;
	save_data->isFileRequired = true;

	const rpGtk_getFileName_t gfndata = {
		(rpGtk_fileDialogCallback)rp_rom_data_view_getSaveFileDialog_callback,	// callback
		save_data,		// user_data

		parent,			// parent
		title,			// title
		filter,			// filter
		page->prevExportDir,	// init_dir
		defaultName.c_str(),	// init_name

		FALSE,			// bLocalOnly
	};
	int ret = rpGtk_getSaveFileName(&gfndata);
	if (ret != 0) {
		// rpGtk_getSaveFileName() failed.
		// g_free() the save_data_t because the callback won't be run.
		g_free(save_data);
	}

	// rpGtk_getSaveFileName() will call rp_rom_data_view_doRomOp_stdop_callback()
	// when the dialog is closed.
}

/**
 * An "Options" menu action was triggered.
 * @param menuButton	RpOptionsMenuButton
 * @param id		Menu options ID
 * @param page		RpRomDataView
 */
void
btnOptions_triggered_signal_handler(RpOptionsMenuButton *menuButton,
				    gint id,
				    RpRomDataView *page)
{
	RP_UNUSED(menuButton);

	if (id < 0) {
		// Standard operation.
		rp_rom_data_view_doRomOp_stdop(page, id);
		return;
	}

	// for convenience purposes
	const RomData *const romData = page->cxx->romData.get();

	// Run a ROM operation.
	// TODO: Don't keep rebuilding this vector...
	vector<RomData::RomOp> ops = romData->romOps();
	assert(id < static_cast<int>(ops.size()));
	if (id >= static_cast<int>(ops.size())) {
		// ID is out of range.
		return;
	}

	save_data_t *const save_data = static_cast<save_data_t*>(g_malloc(sizeof(*save_data)));
	save_data->page = page;
	save_data->id = id;

	const RomData::RomOp *op = &ops[id];
	if (op->flags & RomData::RomOp::ROF_SAVE_FILE) {
		// Prompt for a save file.
		save_data->isFileRequired = true;

		// Need to add "All Files" to the filters.
		string filter;
		if (op->sfi.filter) {
			filter.assign(op->sfi.filter);
			filter += '|';
		}
		// tr: "All Files" filter (RP format)
		filter += C_("RomData", "All Files|*|-");

		// Initial file and directory, based on the current file.
		// NOTE: Not checking if it's a file or a directory. Assuming it's a file.
		string fullFilename = FileSystem::replace_ext(romData->filename(), op->sfi.ext);
		string init_dir, init_name;
		if (!fullFilename.empty()) {
			// Split the directory and basename.
			const size_t slash_pos = fullFilename.rfind(DIR_SEP_CHR);
			if (slash_pos != string::npos) {
				// Full path. Set the directory and filename separately.
				init_name.assign(&fullFilename[slash_pos + 1]);
				init_dir.assign(fullFilename, 0, slash_pos);
			} else {
				// Not a full path. We can only set the filename.
				init_name = std::move(fullFilename);
			}
		}

		GtkWindow *const parent = gtk_widget_get_toplevel_window(GTK_WIDGET(page));
		const rpGtk_getFileName_t gfndata = {
			(rpGtk_fileDialogCallback)rp_rom_data_view_getSaveFileDialog_callback,	// callback
			save_data,		// user_data

			parent,			// parent
			op->sfi.title,		// title
			filter.c_str(),		// filter
			!init_dir.empty() ? init_dir.c_str() : nullptr,		// init_dir
			!init_name.empty() ? init_name.c_str() : nullptr,	// init_name

			FALSE,			// bLocalOnly
		};
		int ret = rpGtk_getSaveFileName(&gfndata);
		if (ret != 0) {
			// rpGtk_getSaveFileName() failed.
			// g_free() the save_data_t because the callback won't be run.
			g_free(save_data);
		}
	} else {
		// No filename is needed.
		// Run the callback directly.
		save_data->isFileRequired = false;
		rp_rom_data_view_getSaveFileDialog_callback(nullptr, save_data);
	}
}
