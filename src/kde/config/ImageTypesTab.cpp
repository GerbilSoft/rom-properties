/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ImageTypesTab.cpp: Image Types tab for rp-config.                       *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ImageTypesTab.hpp"
#include "RpQt.hpp"

// TImageTypesConfig is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/config/ImageTypesConfig.hpp"
#include "libromdata/config/TImageTypesConfig.cpp"
using namespace LibRomData;

// Other rom-properties libraries
using LibRpBase::RomData;
using namespace LibRpText;

// C++ STL classes
using std::string;

#include "ui_ImageTypesTab.h"
class ImageTypesTabPrivate final : public TImageTypesConfig<QComboBox*>
{
public:
	explicit ImageTypesTabPrivate(ImageTypesTab *q);
	~ImageTypesTabPrivate() final;

private:
	ImageTypesTab *const q_ptr;
	Q_DECLARE_PUBLIC(ImageTypesTab)
	Q_DISABLE_COPY(ImageTypesTabPrivate)

public:
	Ui::ImageTypesTab ui;

protected:
	/** TImageTypesConfig functions (protected) **/

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
	/** TImageTypesConfig functions (public) **/

	/**
	 * Set a ComboBox's current index.
	 * This will not trigger cboImageType_priorityValueChanged().
	 * @param cbid ComboBox ID.
	 * @param prio New priority value. (0xFF == no)
	 */
	void cboImageType_setPriorityValue(unsigned int cbid, unsigned int prio) final;

public:
	/** Other ImageTypesTabPrivate functions **/

	/**
	 * Initialize the Credits label.
	 */
	void initCreditsLabel(void);

public:
	// Credits label
	QLabel *lblCredits;

	// Temporary QSettings object.
	// Set and cleared by ImageTypesTab::save();
	QSettings *pSettings;
};

/** ImageTypesTabPrivate **/

ImageTypesTabPrivate::ImageTypesTabPrivate(ImageTypesTab* q)
	: q_ptr(q)
	, lblCredits(nullptr)
	, pSettings(nullptr)
{ }

ImageTypesTabPrivate::~ImageTypesTabPrivate()
{
	// lblCredits should *not* be nullptr.
	// (Created by initCreditsLabel().)
	assert(lblCredits != nullptr);

	// pSettings should be nullptr,
	// since it's only used when saving.
	assert(pSettings == nullptr);
}

/** TImageTypesConfig functions (protected) **/

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
	const unsigned int imageTypeCount = ImageTypesConfig::imageTypeCount();
	for (unsigned int i = 0; i < imageTypeCount; i++) {
		// TODO: Decrement the column number for >IMG_INT_MEDIA?
		if (i == RomData::IMG_INT_MEDIA) {
			// No INT MEDIA boxes, so eliminate the column.
			continue;
		}

		QLabel *const lblImageType = new QLabel(U82Q(imageTypeName(i)), q);
		lblImageType->setObjectName(QLatin1String(
			fmt::format(FSTR("lblImageType{:d}"), i).c_str()));

		lblImageType->setAlignment(Qt::AlignTop|Qt::AlignHCenter);
		lblImageType->setStyleSheet(cssImageType);
		ui.gridImageTypes->addWidget(lblImageType, 0, i+1);
	}

	// Create the system name labels.
	const QString cssSysName = QLatin1String(
		"QLabel { margin-right: 0.25em; }");
	const unsigned int sysCount = ImageTypesConfig::sysCount();
	for (unsigned int sys = 0; sys < sysCount; sys++) {
		QLabel *const lblSysName = new QLabel(U82Q(sysName(sys)), q);
		lblSysName->setObjectName(QLatin1String(
			fmt::format(FSTR("lblSysName{:d}"), sys).c_str()));

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

	// TODO: Decrement the column number for >IMG_INT_MEDIA?
	if (imageType == RomData::IMG_INT_MEDIA) {
		// No INT MEDIA boxes, so eliminate the column.
		return;
	}

	SysData_t &sysData = v_sysData[sys];

	// Create the ComboBox.
	Q_Q(ImageTypesTab);
	QComboBox *const cbo = new QComboBox(q);
	const string cbo_name = fmt::format(FSTR("cbo{:0>4X}"), cbid);
	cbo->setObjectName(QLatin1String(cbo_name.c_str()));
	ui.gridImageTypes->addWidget(cbo, sys+1, imageType+1);
	sysData.cboImageType[imageType] = cbo;

	// Connect the signal handler.
	cbo->setProperty("rp-config.cbid", cbid);
	QObject::connect(cbo, SIGNAL(currentIndexChanged(int)),
			 q, SLOT(cboImageType_currentIndexChanged()));
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
	const SysData_t &sysData = v_sysData[sys];

	QComboBox *const cbo = sysData.cboImageType[imageType];
	assert(cbo != nullptr);
	if (!cbo)
		return;

	// NOTE: Need to add one more than the total number,
	// since "No" counts as an entry.
	assert(max_prio <= static_cast<int>(ImageTypesConfig::imageTypeCount()));
	const bool blockCbo = cbo->blockSignals(true);
	// tr: Don't use this image type for this particular system.
	cbo->addItem(QC_("ImageTypesTab|Values", "No"));
	for (int i = 1; i <= max_prio; i++) {
		cbo->addItem(QString::number(i));
	}
	cbo->setCurrentIndex(0);
	cbo->blockSignals(blockCbo);
}

/**
 * Finish adding the ComboBoxes.
 */
void ImageTypesTabPrivate::finishComboBoxes(void)
{
	// Nothing to do here.
	return;
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

/** TImageTypesConfig functions (public) **/

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
	const SysData_t &sysData = v_sysData[sys];

	QComboBox *const cbo = sysData.cboImageType[imageType];
	assert(cbo != nullptr);
	if (cbo) {
		const bool blockCbo = cbo->blockSignals(true);
		cbo->setCurrentIndex(prio < ImageTypesConfig::imageTypeCount() ? prio+1 : 0);
		cbo->blockSignals(blockCbo);
	}
}

/** Other ImageTypesTabPrivate functions **/

/**
 * Initialize the Credits label.
 */
void ImageTypesTabPrivate::initCreditsLabel(void)
{
	// Create the Credits label if it hasn't been created yet.
	if (!lblCredits) {
		RP_Q(ImageTypesTab);

		QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
		sizePolicy.setHorizontalStretch(0);
		sizePolicy.setVerticalStretch(0);

		lblCredits = new QLabel(q);
		sizePolicy.setHeightForWidth(lblCredits->sizePolicy().hasHeightForWidth());
		lblCredits->setSizePolicy(sizePolicy);
		lblCredits->setFocusPolicy(Qt::StrongFocus);
		lblCredits->setTextFormat(Qt::RichText);
		lblCredits->setAlignment(Qt::AlignBottom|Qt::AlignLeading|Qt::AlignLeft);
		lblCredits->setWordWrap(true);
		lblCredits->setOpenExternalLinks(true);
		lblCredits->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse);

		ui.vboxMain->addWidget(lblCredits);
	}

	// tr: External image credits.
	QString sCredits = QC_("ImageTypesTab",
		"GameCube, Wii, Wii U, Nintendo DS, and Nintendo 3DS external images\n"
		"are provided by <a href=\"https://www.gametdb.com/\">GameTDB</a>.\n"
		"amiibo images are provided by <a href=\"https://amiibo.life/\">amiibo.life</a>,"
		" the Unofficial amiibo Database.");

	// Replace "\n" with "<br/>".
	sCredits.replace(QChar(L'\n'), QLatin1String("<br/>"));
	lblCredits->setText(sCredits);
}

/** ImageTypesTab **/

ImageTypesTab::ImageTypesTab(QWidget *parent)
	: super(parent)
	, d_ptr(new ImageTypesTabPrivate(this))
{
	Q_D(ImageTypesTab);
	d->ui.setupUi(this);

	// Create the control grid.
	d->createGrid();

	// Create the credits label.
	// NOTE: Creating it here instead of in the UI file in order to work around
	// issues with tab ordering. (setTabOrder() isn't working if the control
	// right before it is the final QComboBox that was created...)
	d->initCreditsLabel();
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
		d->initCreditsLabel();
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
	if (d->loadDefaults()) {
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
	if (!d->changed) {
		// Configuration was not changed.
		return;
	}

	d->pSettings = pSettings;
	d->save();
	d->pSettings = nullptr;

	// Configuration saved.
	d->changed = false;
}

/**
 * A QComboBox index has changed.
 * Check the QComboBox's "rp-config.cbid" property for the cbid.
 */
void ImageTypesTab::cboImageType_currentIndexChanged(void)
{
	Q_D(ImageTypesTab);
	QComboBox *const cbo = qobject_cast<QComboBox*>(QObject::sender());
	assert(cbo != nullptr);
	if (!cbo)
		return;

	const unsigned int cbid = cbo->property("rp-config.cbid").toUInt();

	const int idx = cbo->currentIndex();
	const unsigned int prio = static_cast<unsigned int>(idx <= 0 ? 0xFF : idx-1);
	if (d->cboImageType_priorityValueChanged(cbid, prio)) {
		// Configuration has been changed.
		emit modified();
	}
}
