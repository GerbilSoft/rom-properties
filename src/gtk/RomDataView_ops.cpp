/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * RomDataView.cpp: RomData viewer widget. (ROM operations)                *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "RomDataView.hpp"
#include "RomDataView_p.hpp"
#include "RpGtk.hpp"

// Custom widgets
#include "LanguageComboBox.hpp"
#include "OptionsMenuButton.hpp"
#include "MessageWidget.h"

// ENABLE_MESSAGESOUND is set by CMakeLists.txt.
#ifdef ENABLE_MESSAGESOUND
#  include "MessageSound.hpp"
#endif /* ENABLE_MESSAGESOUND */

// Other rom-properties libraries
#include "librpbase/TextOut.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
#include <fstream>
#include <sstream>
using std::ofstream;
using std::ostringstream;
using std::string;
using std::vector;

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

	const RomFields *const pFields = page->romData->fields();
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
	const auto tabs_cend = cxx->tabs.cend();
	for (auto iter = cxx->tabs.cbegin(); iter != tabs_cend && widget == nullptr; ++iter) {
		GtkWidget *const table = iter->table;	// GtkTable (2.x); GtkGrid (3.x)

#if GTK_CHECK_VERSION(4,0,0)
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
#else /* !GTK_CHECK_VERSION(4,0,0) */
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
		case RomFields::RFT_INVALID:
			assert(!"Cannot update an RFT_INVALID field.");
			ret = 5;
			break;
		default:
			assert(!"Unsupported field type.");
			ret = 6;
			break;

		case RomFields::RFT_STRING: {
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

		case RomFields::RFT_BITFIELD: {
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

#if GTK_CHECK_VERSION(4,0,0)
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
#else /* !GTK_CHECK_VERSION(4,0,0) */
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
#endif /* GTK_CHECK_VERSION(4,0,0) */

			// Done updating.
			page->inhibit_checkbox_no_toggle = false;
			ret = 0;
			break;
		}
	}

	return ret;
}

static void
rp_rom_data_view_doRomOp_stdop_response(GtkFileChooserDialog *fileDialog, gint response_id, RpRomDataView *page);

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

	const char *const rom_filename = page->romData->filename();
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
			oss << "== " << rp_sprintf(C_("RomDataView", "File: '%s'"), rom_filename) << std::endl;
			ROMOutput ro(page->romData, sel_lc);
			oss << ro;
			rp_gtk_main_clipboard_set_text(oss.str().c_str());
			// Nothing else to do here.
			return;
		}

		case OPTION_COPY_JSON: {
			ostringstream oss;
			JSONROMOutput jsro(page->romData);
			oss << jsro << std::endl;
			rp_gtk_main_clipboard_set_text(oss.str().c_str());
			// Nothing else to do here.
			return;
		}

		case OPTION_EXPORT_TEXT:
			title = C_("RomDataView", "Export to Text File");
			filter = C_("RomDataView", "Text Files|*.txt|text/plain|All Files|*.*|-");
			default_ext = ".txt";
			break;

		case OPTION_EXPORT_JSON:
			title = C_("RomDataView", "Export to JSON File");
			filter = C_("RomDataView", "JSON Files|*.json|application/json|All Files|*.*|-");
			default_ext = ".json";
			break;

		default:
			assert(!"Invalid ID for a Standard ROM Operation.");
			return;
	}

	GtkWindow *const parent = gtk_widget_get_toplevel_window(GTK_WIDGET(page));
	GtkWidget *const fileDialog = gtk_file_chooser_dialog_new(
		title, parent, GTK_FILE_CHOOSER_ACTION_SAVE,
		_("_Cancel"), GTK_RESPONSE_CANCEL,
		_("_Save"), GTK_RESPONSE_ACCEPT,
		nullptr);
	gtk_widget_set_name(fileDialog, "fileDialog");

#if GTK_CHECK_VERSION(4,0,0)
	// NOTE: GTK4 has *mandatory* overwrite confirmation.
	// Reference: https://gitlab.gnome.org/GNOME/gtk/-/commit/063ad28b1a06328e14ed72cc4b99cd4684efed12

	// TODO: URI?
	if (page->prevExportDir) {
		GFile *const set_file = g_file_new_for_path(page->prevExportDir);
		if (set_file) {
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileDialog), set_file, nullptr);
			g_object_unref(set_file);
		}
	}
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(fileDialog), TRUE);
	if (page->prevExportDir) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileDialog), page->prevExportDir);
	}
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Set the filters.
	rpFileDialogFilterToGtk(GTK_FILE_CHOOSER(fileDialog), filter);

	// Set the operation ID in the dialog.
	g_object_set_qdata(G_OBJECT(fileDialog), RomDataView_romOp_quark, GINT_TO_POINTER(id));

	gchar *const basename = g_path_get_basename(rom_filename);
	string defaultName = basename;
	g_free(basename);
	// Remove the extension, if present.
	size_t extpos = defaultName.rfind('.');
	if (extpos != string::npos) {
		defaultName.resize(extpos);
	}
	defaultName += default_ext;
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(fileDialog), defaultName.c_str());

	// Prompt for a save file.
	g_signal_connect(fileDialog, "response", G_CALLBACK(rp_rom_data_view_doRomOp_stdop_response), page);
	gtk_window_set_transient_for(GTK_WINDOW(fileDialog), parent);
	gtk_window_set_modal(GTK_WINDOW(fileDialog), true);
#if !GTK_CHECK_VERSION(4,0,0)
	gtk_widget_show(GTK_WIDGET(fileDialog));
#endif /* !GTK_CHECK_VERSION(4,0,0) */

	// GtkFileChooserDialog will send the "response" signal when the dialog is closed.
}

/**
 * The Save dialog for a Standard ROM Operation has been closed.
 * @param fileDialog GtkFileChooserDialog
 * @param response_id Response ID
 * @param page RpRomDataView
 */
static void
rp_rom_data_view_doRomOp_stdop_response(GtkFileChooserDialog *fileDialog, gint response_id, RpRomDataView *page)
{
	if (response_id != GTK_RESPONSE_ACCEPT) {
		// User cancelled the dialog.
#if GTK_CHECK_VERSION(4,0,0)
		gtk_window_destroy(GTK_WINDOW(fileDialog));
#else /* !GTK_CHECK_VERSION(4,0,0) */
		gtk_widget_destroy(GTK_WIDGET(fileDialog));
#endif /* GTK_CHECK_VERSION(4,0,0) */
		return;
	}

	// Get the operation ID from the dialog.
	const int id = GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(fileDialog), RomDataView_romOp_quark));

#if GTK_CHECK_VERSION(4,0,0)
	// TODO: URIs?
	gchar *out_filename = nullptr;
	GFile *const get_file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(fileDialog));
	if (get_file) {
		out_filename = g_file_get_path(get_file);
		g_object_unref(get_file);
	}
	gtk_window_destroy(GTK_WINDOW(fileDialog));
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gchar *const out_filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileDialog));
	gtk_widget_destroy(GTK_WIDGET(fileDialog));
#endif /* GTK_CHECK_VERSION(4,0,0) */

	if (!out_filename) {
		// No filename...
		return;
	}

	// Save the previous export directory.
	g_free(page->prevExportDir);
	page->prevExportDir = g_path_get_dirname(out_filename);

	// TODO: QTextStream wrapper for ostream.
	// For now, we'll use ofstream.
	ofstream ofs;
	ofs.open(out_filename, ofstream::out);
	if (ofs.fail()) {
		// TODO: Show an error message?
		return;
	}

	switch (id) {
		case OPTION_EXPORT_TEXT: {
			const uint32_t sel_lc = page->cboLanguage
				? rp_language_combo_box_get_selected_lc(RP_LANGUAGE_COMBO_BOX(page->cboLanguage))
				: 0;

			ofs << "== " << rp_sprintf(C_("RomDataView", "File: '%s'"), page->romData->filename()) << std::endl;
			ROMOutput ro(page->romData, sel_lc);
			ofs << ro;
			break;
		}

		case OPTION_EXPORT_JSON: {
			JSONROMOutput jsro(page->romData);
			ofs << jsro << std::endl;
			break;
		}

		default:
			assert(!"Invalid ID for an Export Standard ROM Operation.");
			return;
	}

	ofs.close();
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
	GtkWindow *const parent = gtk_widget_get_toplevel_window(GTK_WIDGET(page));

	if (id < 0) {
		// Standard operation.
		rp_rom_data_view_doRomOp_stdop(page, id);
		return;
	}

	// Run a ROM operation.
	// TODO: Don't keep rebuilding this vector...
	vector<RomData::RomOp> ops = page->romData->romOps();
	assert(id < (int)ops.size());
	if (id >= (int)ops.size()) {
		// ID is out of range.
		return;
	}

	gchar *save_filename = nullptr;
	RomData::RomOpParams params;
	const RomData::RomOp *op = &ops[id];
	if (op->flags & RomData::RomOp::ROF_SAVE_FILE) {
		GtkWidget *const fileDialog = gtk_file_chooser_dialog_new(
			op->sfi.title, parent, GTK_FILE_CHOOSER_ACTION_SAVE,
			_("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_Save"), GTK_RESPONSE_ACCEPT,
			nullptr);
		gtk_widget_set_name(fileDialog, "fileDialog");

#if !GTK_CHECK_VERSION(4,0,0)
		// NOTE: GTK4 has *mandatory* overwrite confirmation.
		// Reference: https://gitlab.gnome.org/GNOME/gtk/-/commit/063ad28b1a06328e14ed72cc4b99cd4684efed12
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(fileDialog), TRUE);
#endif /* !GTK_CHECK_VERSION(4,0,0) */

		// Set the filters.
		rpFileDialogFilterToGtk(GTK_FILE_CHOOSER(fileDialog), op->sfi.filter);

		// Add the "All Files" filter.
		GtkFileFilter *const allFilesFilter = gtk_file_filter_new();
		// tr: "All Files" filter (GTK+ file filter)
		gtk_file_filter_set_name(allFilesFilter, C_("RomData", "All Files"));
		gtk_file_filter_add_pattern(allFilesFilter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fileDialog), allFilesFilter);

		// Initial file and directory, based on the current file.
		// NOTE: Not checking if it's a file or a directory. Assuming it's a file.
		string initialFile = FileSystem::replace_ext(page->romData->filename(), op->sfi.ext);
		if (!initialFile.empty()) {
			// Split the directory and basename.
			size_t slash_pos = initialFile.rfind(DIR_SEP_CHR);
			if (slash_pos != string::npos) {
				// Full path. Set the directory and filename separately.
				gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(fileDialog), &initialFile[slash_pos + 1]);
				initialFile.resize(slash_pos);

#if GTK_CHECK_VERSION(4,0,0)
				// TODO: URI?
				GFile *const set_file = g_file_new_for_path(initialFile.c_str());
				if (set_file) {
					gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileDialog), set_file, nullptr);
					g_object_unref(set_file);
				}
#else /* !GTK_CHECK_VERSION(4,0,0) */
				// FIXME: Do we need to prepend "file://"?
				gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(fileDialog), initialFile.c_str());
#endif /* GTK_CHECK_VERSION(4,0,0) */
			} else {
				// Not a full path. We can only set the filename.
				gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(fileDialog), initialFile.c_str());
			}
		}

		// Prompt for a save file.
#if GTK_CHECK_VERSION(4,0,0)
		// FIXME: Need to add a response signal like standard operations.
		assert(!"GTK4 doesn't support gtk_dialog_run().");
		// TODO: URIs?
		GFile *const get_file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(fileDialog));
		if (get_file) {
			save_filename = (get_file ? g_file_get_path(get_file) : nullptr);
			g_object_unref(get_file);
		}
		gtk_window_destroy(GTK_WINDOW(fileDialog));
#else /* !GTK_CHECK_VERSION(4,0,0) */
		gint res = gtk_dialog_run(GTK_DIALOG(fileDialog));
		save_filename = (res == GTK_RESPONSE_ACCEPT
			? gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileDialog))
			: nullptr);
		gtk_widget_destroy(fileDialog);
#endif /* !GTK_CHECK_VERSION(4,0,0) */

		params.save_filename = save_filename;
	}

	GtkMessageType messageType;
	int ret = page->romData->doRomOp(id, &params);
	g_free(save_filename);
	if (ret == 0) {
		// ROM operation completed.

		// Update fields.
		for (int fieldIdx : params.fieldIdx) {
			rp_rom_data_view_update_field(page, fieldIdx);
		}

		// Update the RomOp menu entry in case it changed.
		// NOTE: Assuming the RomOps vector order hasn't changed.
		ops = page->romData->romOps();
		assert(id < (int)ops.size());
		if (id < (int)ops.size()) {
			rp_options_menu_button_update_op(RP_OPTIONS_MENU_BUTTON(page->btnOptions), id, &ops[id]);
		}

		messageType = GTK_MESSAGE_INFO;
	} else {
		// An error occurred...
		// TODO: Show an error message.
		messageType = GTK_MESSAGE_WARNING;
	}

	if (!params.msg.empty()) {
#ifdef ENABLE_MESSAGESOUND
		MessageSound::play(messageType, params.msg.c_str(), GTK_WIDGET(page));
#endif /* ENABLE_MESSAGESOUND */

		// Show the MessageWidget.
		if (!page->messageWidget) {
			page->messageWidget = rp_message_widget_new();
#if GTK_CHECK_VERSION(4,0,0)
			gtk_box_append(GTK_BOX(page), page->messageWidget);
#else /* !GTK_CHECK_VERSION(4,0,0) */
			gtk_box_pack_end(GTK_BOX(page), page->messageWidget, false, false, 0);
#endif /* GTK_CHECK_VERSION(4,0,0) */
		}

		RpMessageWidget *const messageWidget = RP_MESSAGE_WIDGET(page->messageWidget);
		rp_message_widget_set_message_type(messageWidget, messageType);
		rp_message_widget_set_text(messageWidget, params.msg.c_str());
#if !GTK_CHECK_VERSION(4,0,0)
		gtk_widget_show(page->messageWidget);
#endif /* !GTK_CHECK_VERSION(4,0,0) */
	}
}
