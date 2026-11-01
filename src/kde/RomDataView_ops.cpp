/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomDataView.cpp: RomData viewer. (ROM operations)                       *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RomDataView.hpp"
#include "RomDataView_p.hpp"

// Custom widgets
#include "LanguageComboBox.hpp"
#include "OptionsMenuButton.hpp"

// MessageSound (always enabled on KDE)
#include "MessageSound.hpp"

// Other rom-properties libraries
#include "libi18n/i18n.hpp"
#include "librpbase/TextOut.hpp"
#include "librpfile/FileSystem.hpp"
#include "librpfile/RpFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;

// C++ STL classes
#include <fstream>
#include <sstream>
#include "rp-string_view.hpp"
using std::ofstream;
using std::ostringstream;
using std::string;
using std::string_view;
using std::vector;

// Qt includes
#include <QtCore/QFileInfo>
#include <QtGui/QClipboard>
#include <QCheckBox>
#include <QFileDialog>

// libfmt
#include "rp-libfmt.h"

/**
 * Update a field's value.
 * This is called after running a ROM operation.
 * @param fieldIdx Field index.
 * @return 0 on success; non-zero on error.
 */
int RomDataViewPrivate::updateField(int fieldIdx)
{
	const RomFields *const pFields = romData->fields();
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

	// Lambda function to check a QObject's RFT_fieldIdx.
	auto checkFieldIdx = [](QObject *qObj, int fieldIdx) -> bool {
		const QVariant qVar = qObj->property("RFT_fieldIdx");
		bool ok = false;
		const int tmp_fieldIdx = qVar.toInt(&ok);
		return (ok && tmp_fieldIdx == fieldIdx);
	};

	// Get the QObject*.
	// NOTE: Linear search through all display objects, since
	// this function isn't used that often.
	QObject *qObj = nullptr;
	for (const auto &tab : tabs) {
		QFormLayout *const form = tab.form;
		const int rowCount = form->rowCount();
		for (int row = 0; row < rowCount && qObj == nullptr; row++) {
			// TODO: Also check LabelRole in some cases?
			QLayoutItem *const item = form->itemAt(row, QFormLayout::FieldRole);

			// Check for QWidget.
			QObject *qObjTmp = item->widget();
			if (qObjTmp && checkFieldIdx(qObjTmp, fieldIdx)) {
				// Found the field.
				qObj = qObjTmp;
				break;
			}

			// Check for QLayout.
			qObjTmp = item->layout();
			if (qObjTmp && checkFieldIdx(qObjTmp, fieldIdx)) {
				// Found the field.
				qObj = qObjTmp;
				break;
			}
		}
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
			// QObject is a QLabel.
			QLabel *const label = qobject_cast<QLabel*>(qObj);
			assert(label != nullptr);
			if (!label) {
				ret = 7;
				break;
			}

			if (field->data.str) {
				label->setText(U82Q(field->data.str));
			} else {
				label->clear();
			}
			ret = 0;
			break;
		}

		case RomFields::RomFieldType::RFT_BITFIELD: {
			// QObject is a QGridLayout with QCheckBox widgets.
			QGridLayout *const layout = qobject_cast<QGridLayout*>(qObj);
			assert(layout != nullptr);
			if (!layout) {
				ret = 8;
				break;
			}

			// Bits with a blank name aren't included, so we'll need to iterate
			// over the bitfield description.
			const auto &bitfieldDesc = field->desc.bitfield;
			uint32_t bitfield = field->data.bitfield;
			const auto names_cend = bitfieldDesc.names->cend();
			int layoutIdx = 0;
			for (auto iter = bitfieldDesc.names->cbegin(); iter != names_cend; ++iter, bitfield >>= 1) {
				const string &name = *iter;
				if (name.empty())
					continue;

				// Get the widget.
				QLayoutItem *const subLayoutItem = layout->itemAt(layoutIdx);
				assert(subLayoutItem != nullptr);
				if (!subLayoutItem)
					break;

				QCheckBox *const checkBox = qobject_cast<QCheckBox*>(subLayoutItem->widget());
				assert(checkBox != nullptr);
				if (!checkBox)
					break;

				const bool value = (bitfield & 1);
				checkBox->setChecked(value);
				checkBox->setProperty("RFT_BITFIELD_value", value);

				// Next layout item.
				layoutIdx++;
			}
			ret = 0;
			break;
		}
	}

	return ret;
}

/**
 * ROM operation: Standard Operations
 * Dispatched by RomDataView::btnOptions_triggered().
 * @param id Standard action ID
 */
void RomDataViewPrivate::doRomOp_stdop(int id)
{
	const char *const rom_filename = romData->filename();
	if (!rom_filename)
		return;
	const uint32_t sel_lc = (cboLanguage ? cboLanguage->selectedLC() : 0);

	const char *title = nullptr;
	const char *filter = nullptr;
	const char *default_ext = nullptr;

	// Check the standard operation.
	switch (id) {
		case OPTION_COPY_TEXT: {
			ostringstream oss;
			oss << "== " << fmt::format(FRUN(C_("RomDataView", "File: '{:s}'")), rom_filename) << '\n';
			ROMOutput ro(romData.get(), sel_lc);
			oss << ro;
			oss.flush();
			QApplication::clipboard()->setText(U82Q(oss.str()));
			// Nothing else to do here.
			return;
		}

		case OPTION_COPY_JSON: {
			ostringstream oss;
			JSONROMOutput jsro(romData.get());
			oss << jsro << '\n';
			oss.flush();
			QApplication::clipboard()->setText(U82Q(oss.str()));
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

	// Export/copy to text or JSON.
	const QFileInfo fi(U82Q(rom_filename));

	if (prevExportDir.isEmpty()) {
		prevExportDir = fi.path();
	}

	QString defaultFileName = prevExportDir + QLatin1Char('/') + fi.completeBaseName();
	defaultFileName += QLatin1String(default_ext);

	// TODO: Rework so it's not application-modal.
	Q_Q(RomDataView);
	const QString out_filename = QFileDialog::getSaveFileName(q,
		U82Q(title), defaultFileName, rpFileDialogFilterToQt(filter));
	if (out_filename.isEmpty())
		return;

	// Save the previous export directory.
	const QFileInfo fi2(out_filename);
	prevExportDir = fi2.path();

	// TODO: QTextStream wrapper for ostream.
	// For now, we'll use ofstream.
	ofstream ofs;
	ofs.open(Q2U8(out_filename), ofstream::out);
	if (ofs.fail()) {
		// Delete the file, since it's likely only 0 bytes.
		// TODO: Only if it was created by us...
		ofs.close();
		//QFile::remove(out_filename);

		// TODO: Better error message?
		showMessageWidget(QMessageBox::Warning, QC_("RomDataView", "Could not open the output file."));
		return;
	}

	switch (id) {
		case OPTION_EXPORT_TEXT: {
			ofs << "== " << fmt::format(FRUN(C_("RomDataView", "File: '{:s}'")), rom_filename) << '\n';
			ROMOutput ro(romData.get(), sel_lc);
			ofs << ro;
			ofs.flush();
			break;
		}

		case OPTION_EXPORT_JSON: {
			JSONROMOutput jsro(romData.get());
			ofs << jsro << '\n';
			ofs.flush();
			break;
		}

		default:
			assert(!"Invalid ID for an Export Standard ROM Operation.");
			ofs.close();
			QFile::remove(out_filename);
			showMessageWidget(QMessageBox::Critical, QC_("RomDataView", "Invalid ID for an Export Standard ROM Operation."));
			return;
	}

	ofs.close();
}

/**
 * Show a message in the Message Widget.
 * @param messageType QMessageBox::Icon
 * @param msg Message
 */
void RomDataViewPrivate::showMessageWidget(QMessageBox::Icon messageType, const QString &msg)
{
#ifdef HAVE_KMESSAGEWIDGET
	Q_Q(RomDataView);
	MessageSound::play(messageType, msg, q);

	if (!messageWidget) {
		messageWidget = new KMessageWidget(q);
		messageWidget->setObjectName(QLatin1String("messageWidget"));
		messageWidget->setCloseButtonVisible(true);
		messageWidget->setWordWrap(true);
		ui.vboxLayout->addWidget(messageWidget);
	}

#  ifdef HAVE_KMESSAGEWIDGET_SETICON
#    define KMessageWidget_SetIcon(messageWidget, iconID) \
	(messageWidget)->setIcon((messageWidget)->style()->standardIcon(iconID))
#  else /* !HAVE_KMESSAGEWIDGET_SETICON */
#    define KMessageWidget_SetIcon(messageWidget, iconID) do { } while (0)
#  endif /* HAVE_KMESSAGEWIDGET_SETICON */

	switch (messageType) {
		default:
		case QMessageBox::NoIcon:
			break;

		case QMessageBox::Information:
			messageWidget->setMessageType(KMessageWidget::Information);
			KMessageWidget_SetIcon(messageWidget, QStyle::SP_MessageBoxInformation);
			break;

		case QMessageBox::Warning:
			messageWidget->setMessageType(KMessageWidget::Warning);
			KMessageWidget_SetIcon(messageWidget, QStyle::SP_MessageBoxWarning);
			break;

		case QMessageBox::Critical:
			messageWidget->setMessageType(KMessageWidget::Error);
			KMessageWidget_SetIcon(messageWidget, QStyle::SP_MessageBoxCritical);
			break;

		case QMessageBox::Question:
			// Doesn't exactly map to "question"...
			messageWidget->setMessageType(KMessageWidget::Positive);
			KMessageWidget_SetIcon(messageWidget, QStyle::SP_MessageBoxQuestion);
			break;
	}

	messageWidget->setText(msg);
	messageWidget->animatedShow();
#else /* !HAVE_KMESSAGEWIDGET */
	// No KMessageWidget...
	Q_UNUSED(messageType);
	Q_UNUSED(msg);
#endif /* HAVE_KMESSAGEWIDGET */
}

/**
 * An "Options" menu action was triggered.
 * @param id Options ID.
 */
void RomDataView::btnOptions_triggered(int id)
{
	// IDs below 0 are for built-in actions.
	// IDs >= 0 are for RomData-specific actions.
	Q_D(RomDataView);

	if (id < 0) {
		// Standard operation.
		d->doRomOp_stdop(id);
		return;
	}

	// Run a ROM operation.
	// TODO: Don't keep rebuilding this vector...
	vector<RomData::RomOp> ops = d->romData->romOps();
	assert(id < static_cast<int>(ops.size()));
	if (id >= static_cast<int>(ops.size())) {
		// ID is out of range.
		return;
	}

	QString save_filename;
	RomData::RomOpParams params;
	const RomData::RomOp *op = &ops[id];
	if (op->flags & RomData::RomOp::ROF_SAVE_FILE) {
		// Add "All Files" to the filter.
		QString filter = rpFileDialogFilterToQt(op->sfi.filter);
		if (!filter.isEmpty()) {
			filter += QLatin1String(";;");
		}
		// tr: "All Files" filter (RP format)
		filter += rpFileDialogFilterToQt(C_("RomData", "All Files|*|-"));

		// Initial file and directory, based on the current file.
		const QString initialFile = U82Q(FileSystem::replace_ext(d->romData->filename(), op->sfi.ext));

		// Prompt for a save file.
		save_filename = QFileDialog::getSaveFileName(this,
			U82Q(op->sfi.title), initialFile, filter);
		if (save_filename.isEmpty()) {
			return;
		}
		QByteArray ba_save_filename = save_filename.toUtf8();

		// Open the save file.
		// TODO: RpFileKio?
		params.save_file = std::make_shared<RpFile>(
			string_view(ba_save_filename.constData(), ba_save_filename.size()), RpFile::FM_CREATE_WRITE);
		if (!params.save_file->isOpen()) {
			// Show an error message.
			// TODO: Include the filename?
			// TODO: Thread-safe strerror()?
			const QString msg = U82Q(fmt::format(FRUN(C_("RomDataView", "Error opening output file: {:s}")),
				strerror(params.save_file->lastError())));

			// Delete the file if it was created.
			// TODO: Only if it was created by us...
			params.save_file.reset();
			//QFile::remove(save_filename);
			
			d->showMessageWidget(QMessageBox::Warning, msg);
			return;
		}
	}

	const int ret = d->romData->doRomOp(id, &params);
	const QString qs_msg = U82Q(params.msg);
	QMessageBox::Icon messageType;
	if (ret == 0) {
		// ROM operation completed.

		// Update fields.
		for (const int fieldIdx : params.fieldIdx) {
			d->updateField(fieldIdx);
		}

		// Update the RomOp menu entry in case it changed.
		// NOTE: Assuming the RomOps vector order hasn't changed.
		ops = d->romData->romOps();
		assert(id < static_cast<int>(ops.size()));
		if (id < static_cast<int>(ops.size())) {
			d->btnOptions->updateOp(id, &ops[id]);
		}

		// Show the message and play the sound.
		messageType = QMessageBox::Information;
	} else {
		// An error occurred...
		messageType = QMessageBox::Warning;

		// Delete the file, since it's likely only 0 bytes.
		params.save_file.reset();
		QFile::remove(save_filename);
	}

	if (!qs_msg.isEmpty()) {
		d->showMessageWidget(messageType, qs_msg);
	}
}
