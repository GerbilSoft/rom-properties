/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomDataView.cpp: RomData viewer. (ROM operations)                       *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "RomDataView.hpp"
#include "RomDataView_p.hpp"

// Custom widgets
#include "LanguageComboBox.hpp"
#include "OptionsMenuButton.hpp"

// MessageSound (always enabled on KDE)
#include "MessageSound.hpp"

// Other rom-properties libraries
#include "librpbase/TextOut.hpp"
using namespace LibRpBase;
using namespace LibRpFile;

// Qt includes
#include <QtGui/QClipboard>

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
		case RomFields::RFT_INVALID:
			assert(!"Cannot update an RFT_INVALID field.");
			ret = 5;
			break;
		default:
			assert(!"Unsupported field type.");
			ret = 6;
			break;

		case RomFields::RFT_STRING: {
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

		case RomFields::RFT_BITFIELD: {
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
			oss << "== " << fmt::format(C_("RomDataView", "File: '{:s}'"), rom_filename) << '\n';
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

	QString defaultFileName = prevExportDir + QChar(L'/') + fi.completeBaseName();
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
	ofs.open(out_filename.toUtf8().constData(), ofstream::out);
	if (ofs.fail()) {
		// TODO: Show an error message?
		return;
	}

	switch (id) {
		case OPTION_EXPORT_TEXT: {
			ofs << "== " << fmt::format(C_("RomDataView", "File: '{:s}'"), rom_filename) << '\n';
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
			return;
	}

	ofs.close();
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

	QByteArray ba_save_filename;
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
		const QString filename = QFileDialog::getSaveFileName(this,
			U82Q(op->sfi.title), initialFile, filter);
		if (filename.isEmpty())
			return;
		ba_save_filename = filename.toUtf8();
		params.save_filename = ba_save_filename.constData();
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
	}

	if (!qs_msg.isEmpty()) {
#ifdef HAVE_KMESSAGEWIDGET
		MessageSound::play(messageType, qs_msg, this);

		if (!d->messageWidget) {
			d->messageWidget = new KMessageWidget(this);
			d->messageWidget->setObjectName(QLatin1String("messageWidget"));
			d->messageWidget->setCloseButtonVisible(true);
			d->messageWidget->setWordWrap(true);
			d->ui.vboxLayout->addWidget(d->messageWidget);
		}

		if (ret == 0) {
			d->messageWidget->setMessageType(KMessageWidget::Information);
#  ifdef HAVE_KMESSAGEWIDGET_SETICON
			d->messageWidget->setIcon(
				d->messageWidget->style()->standardIcon(QStyle::SP_MessageBoxInformation));
#  endif /* HAVE_KMESSAGEWIDGET_SETICON */
		} else {
			d->messageWidget->setMessageType(KMessageWidget::Warning);
#  ifdef HAVE_KMESSAGEWIDGET_SETICON
			d->messageWidget->setIcon(
				d->messageWidget->style()->standardIcon(QStyle::SP_MessageBoxWarning));
#  endif /* HAVE_KMESSAGEWIDGET_SETICON */
		}
		d->messageWidget->setText(qs_msg);
		d->messageWidget->animatedShow();
#endif /* HAVE_KMESSAGEWIDGET */
	}
}
