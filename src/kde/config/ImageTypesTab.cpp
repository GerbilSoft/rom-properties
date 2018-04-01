/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ImageTypesTab.cpp: Image Types tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "ImageTypesTab.hpp"
#include "RpQt.hpp"

// Qt includes.
#include <QComboBox>
#include <QLabel>
#include <QSignalMapper>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

// TImageTypesConfig is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/config/TImageTypesConfig.cpp"
using LibRomData::TImageTypesConfig;

#include "ui_ImageTypesTab.h"
class ImageTypesTabPrivate : public TImageTypesConfig<QComboBox*>
{
	public:
		explicit ImageTypesTabPrivate(ImageTypesTab *q);
		~ImageTypesTabPrivate();

	private:
		ImageTypesTab *const q_ptr;
		Q_DECLARE_PUBLIC(ImageTypesTab)
		Q_DISABLE_COPY(ImageTypesTabPrivate)

	public:
		Ui::ImageTypesTab ui;

	protected:
		/** TImageTypesConfig functions. (protected) **/

		/**
		 * Create the labels in the grid.
		 */
		void createGridLabels(void) final;

		/**
		 * Create a ComboBox in the grid.
		 * @param cbid ComboBox ID.
		 */
		void createComboBox(unsigned int cbid) final;

		/**
		 * Add strings to a ComboBox in the grid.
		 * @param cbid ComboBox ID.
		 * @param max_prio Maximum priority value. (minimum is 1)
		 */
		void addComboBoxStrings(unsigned int cbid, int max_prio) final;

		/**
		 * Finish adding the ComboBoxes.
		 */
		void finishComboBoxes(void) final;

		/**
		 * Initialize the Save subsystem.
		 * This is needed on platforms where the configuration file
		 * must be opened with an appropriate writer class.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int saveStart(void) final;

		/**
		 * Write an ImageType configuration entry.
		 * @param sysName System name.
		 * @param imageTypeList Image type list, comma-separated.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int saveWriteEntry(const char *sysName, const char *imageTypeList) final;

		/**
		 * Close the Save subsystem.
		 * This is needed on platforms where the configuration file
		 * must be opened with an appropriate writer class.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int saveFinish(void) final;

	protected:
		/** TImageTypesConfig functions. (public) **/

		/**
		 * Set a ComboBox's current index.
		 * This will not trigger cboImageType_priorityValueChanged().
		 * @param cbid ComboBox ID.
		 * @param prio New priority value. (0xFF == no)
		 */
		void cboImageType_setPriorityValue(unsigned int cbid, unsigned int prio) final;

	public:
		// Last ComboBox added.
		// Needed in order to set the correct
		// tab order for the credits label.
		QComboBox *cboImageType_lastAdded;

		// QSignalMapper for the QComboBoxes.
		QSignalMapper *mapperCboImageType;

		// Temporary QSettings object.
		// Set and cleared by ImageTypesTab::save();
		QSettings *pSettings;
};

/** ImageTypesTabPrivate **/

ImageTypesTabPrivate::ImageTypesTabPrivate(ImageTypesTab* q)
	: q_ptr(q)
	, cboImageType_lastAdded(nullptr)
	, mapperCboImageType(new QSignalMapper(q))
	, pSettings(nullptr)
{
	// Connect the QSignalMapper to the ImageTypesTab.
	QObject::connect(mapperCboImageType, SIGNAL(mapped(int)),
		q, SLOT(cboImageType_currentIndexChanged(int)));
}

ImageTypesTabPrivate::~ImageTypesTabPrivate()
{
	// cboImageType_lastAdded should be nullptr.
	// (Cleared by finishComboBoxes().)
	assert(cboImageType_lastAdded == nullptr);

	// pSettings should be nullptr,
	// since it's only used when saving.
	assert(pSettings == nullptr);
}

/** TImageTypesConfig functions. (protected) **/

/**
 * Create the labels in the grid.
 */
void ImageTypesTabPrivate::createGridLabels(void)
{
	Q_Q(ImageTypesTab);

	// TODO: Make sure that all columns except 0 have equal sizes.

	// Create the image type labels.
	const QString cssImageType = QLatin1String(
		"QLabel { margin-left: 0.2em; margin-right: 0.2em; margin-bottom: 0.1em; }");
	for (unsigned int i = 0; i < IMG_TYPE_COUNT; i++) {
		QLabel *const lblImageType = new QLabel(U82Q(imageTypeName(i)), q);
		lblImageType->setAlignment(Qt::AlignTop|Qt::AlignHCenter);
		lblImageType->setStyleSheet(cssImageType);
		ui.gridImageTypes->addWidget(lblImageType, 0, i+1);
	}

	// Create the system name labels.
	const QString cssSysName = QLatin1String(
		"QLabel { margin-right: 0.25em; }");
	for (unsigned int sys = 0; sys < SYS_COUNT; sys++) {
		QLabel *const lblSysName = new QLabel(U82Q(sysName(sys)), q);
		lblSysName->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
		lblSysName->setStyleSheet(cssSysName);
		ui.gridImageTypes->addWidget(lblSysName, sys+1, 0);
	}
}

/**
 * Create a ComboBox in the grid.
 * @param cbid ComboBox ID.
 */
void ImageTypesTabPrivate::createComboBox(unsigned int cbid)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return;

	// Create the ComboBox.
	Q_Q(ImageTypesTab);
	QComboBox *const cbo = new QComboBox(q);
	ui.gridImageTypes->addWidget(cbo, sys+1, imageType+1);
	cboImageType[sys][imageType] = cbo;

	// Connect the signal to the QSignalMapper.
	QObject::connect(cbo, SIGNAL(currentIndexChanged(int)),
		mapperCboImageType, SLOT(map()));
	mapperCboImageType->setMapping(cbo, (int)cbid);

	// Adjust the tab order.
	if (cboImageType_lastAdded) {
		q->setTabOrder(cboImageType_lastAdded, cbo);
	}
	cboImageType_lastAdded = cbo;
}

/**
 * Add strings to a ComboBox in the grid.
 * @param cbid ComboBox ID.
 * @param max_prio Maximum priority value. (minimum is 1)
 */
void ImageTypesTabPrivate::addComboBoxStrings(unsigned int cbid, int max_prio)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return;

	QComboBox *const cbo = cboImageType[sys][imageType];
	assert(cbo != nullptr);
	if (!cbo)
		return;

	// Dropdown strings.
	// NOTE: One more string than the total number of image types,
	// since we have a string for "No".
	static const char s_values[][4] = {
		NOP_C_("ImageTypesTab|Values", "No"),
		NOP_C_("ImageTypesTab|Values", "1"),
		NOP_C_("ImageTypesTab|Values", "2"),
		NOP_C_("ImageTypesTab|Values", "3"),
		NOP_C_("ImageTypesTab|Values", "4"),
		NOP_C_("ImageTypesTab|Values", "5"),
		NOP_C_("ImageTypesTab|Values", "6"),
		NOP_C_("ImageTypesTab|Values", "7"),
		NOP_C_("ImageTypesTab|Values", "8"),
		NOP_C_("ImageTypesTab|Values", "9"),
	};
	static_assert(ARRAY_SIZE(s_values) == IMG_TYPE_COUNT+1, "s_values[] is the wrong size.");

	// NOTE: Need to add one more than the total number,
	// since "No" counts as an entry.
	for (int i = 0; i <= max_prio; i++) {
		cbo->addItem(U82Q(
			dpgettext_expr(RP_I18N_DOMAIN, "ImageTypesTab|Values", s_values[i])));
	}
	cbo->setCurrentIndex(0);
}

/**
 * Finish adding the ComboBoxes.
 */
void ImageTypesTabPrivate::finishComboBoxes(void)
{
	if (!cboImageType_lastAdded) {
		// Nothing to do here.
		return;
	}

	// Set the tab order for the credits label.
	Q_Q(ImageTypesTab);
	q->setTabOrder(cboImageType_lastAdded, ui.lblCredits);
	cboImageType_lastAdded = nullptr;
}

/**
 * Initialize the Save subsystem.
 * This is needed on platforms where the configuration file
 * must be opened with an appropriate writer class.
 * @return 0 on success; negative POSIX error code on error.
 */
int ImageTypesTabPrivate::saveStart(void)
{
	assert(pSettings != nullptr);
	if (!pSettings) {
		return -ENOENT;
	}
	pSettings->beginGroup(QLatin1String("ImageTypes"));
	return 0;
}

/**
 * Write an ImageType configuration entry.
 * @param sysName System name.
 * @param imageTypeList Image type list, comma-separated.
 * @return 0 on success; negative POSIX error code on error.
 */
int ImageTypesTabPrivate::saveWriteEntry(const char *sysName, const char *imageTypeList)
{
	assert(pSettings != nullptr);
	if (!pSettings) {
		return -ENOENT;
	}

	// NOTE: QSettings stores comma-separated strings with
	// double-quotes, which may be a bit confusing.
	// Config will simply ignore the double-quotes.
	pSettings->setValue(U82Q(sysName), U82Q(imageTypeList));
	return 0;
}

/**
 * Close the Save subsystem.
 * This is needed on platforms where the configuration file
 * must be opened with an appropriate writer class.
 * @return 0 on success; negative POSIX error code on error.
 */
int ImageTypesTabPrivate::saveFinish(void)
{
	assert(pSettings != nullptr);
	if (!pSettings) {
		return -ENOENT;
	}
	pSettings->endGroup();
	return 0;
}

/** TImageTypesConfig functions. (public) **/

/**
 * Set a ComboBox's current index.
 * This will not trigger cboImageType_priorityValueChanged().
 * @param cbid ComboBox ID.
 * @param prio New priority value. (0xFF == no)
 */
void ImageTypesTabPrivate::cboImageType_setPriorityValue(unsigned int cbid, unsigned int prio)
{
	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return;

	QComboBox *const cbo = cboImageType[sys][imageType];
	assert(cbo != nullptr);
	if (cbo) {
		cbo->setCurrentIndex(prio < IMG_TYPE_COUNT ? prio+1 : 0);
	}
}

/** Other ImageTypesTabPrivate functions. **/

// TODO

/** ImageTypesTab **/

ImageTypesTab::ImageTypesTab(QWidget *parent)
	: super(parent)
	, d_ptr(new ImageTypesTabPrivate(this))
{
	Q_D(ImageTypesTab);
	d->ui.setupUi(this);

	// Create the control grid.
	d->createGrid();
}

ImageTypesTab::~ImageTypesTab()
{
	delete d_ptr;
}

/**
 * Widget state has changed.
 * @param event State change event.
 */
void ImageTypesTab::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		// Retranslate the UI.
		Q_D(ImageTypesTab);
		d->ui.retranslateUi(this);
	}

	// Pass the event to the base class.
	super::changeEvent(event);
}

/**
 * Reset the configuration.
 */
void ImageTypesTab::reset(void)
{
	Q_D(ImageTypesTab);
	d->reset();
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void ImageTypesTab::loadDefaults(void)
{
	Q_D(ImageTypesTab);
	bool bRet = d->loadDefaults();
	if (bRet) {
		// Configuration has been changed.
		emit modified();
	}
}

/**
 * Save the configuration.
 * @param pSettings QSettings object.
 */
void ImageTypesTab::save(QSettings *pSettings)
{
	assert(pSettings != nullptr);
	if (!pSettings)
		return;

	// Save the configuration.
	Q_D(ImageTypesTab);
	if (d->changed) {
		d->pSettings = pSettings;
		d->save();
		d->pSettings = nullptr;
	}
}

/**
 * A QComboBox index has changed.
 * @param cbid ComboBox ID.
 */
void ImageTypesTab::cboImageType_currentIndexChanged(int cbid)
{
	Q_D(ImageTypesTab);
	const unsigned int sys = d->sysFromCbid((unsigned int)cbid);
	const unsigned int imageType = d->imageTypeFromCbid((unsigned int)cbid);
	if (!d->validateSysImageType(sys, imageType))
		return;

	QComboBox *cbo = d->cboImageType[sys][imageType];
	assert(cbo != nullptr);
	if (!cbo)
		return;

	const int idx = cbo->currentIndex();
	const unsigned int prio = (unsigned int)(idx <= 0 ? 0xFF : idx-1);
	if (d->cboImageType_priorityValueChanged(cbid, prio)) {
		// Configuration has been changed.
		emit modified();
	}
}
