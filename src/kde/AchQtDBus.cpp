/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * AchQtDBus.cpp: QtDBus notifications for achievements.                   *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchQtDBus.hpp"
using LibRpBase::Achievements;

// librptexture
using LibRpTexture::argb32_t;

// QtDBus
#include "notificationsinterface.h"

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
// for Qt::escape()
#include <QtGui/QTextDocument>
#endif /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */

// C++ STL classes.
using std::unordered_map;

class AchQtDBusPrivate
{
	public:
		AchQtDBusPrivate();
		~AchQtDBusPrivate();

	private:
		RP_DISABLE_COPY(AchQtDBusPrivate)

	public:
		// Static AchQtDBus instance.
		// TODO: Q_GLOBAL_STATIC() equivalent, though we
		// may need special initialization if the compiler
		// doesn't support thread-safe statics.
		static AchQtDBus instance;
		bool hasRegistered;

	public:
		/**
		 * Load the specified sprite sheet.
		 * @param iconSize Icon size. (16, 24, 32, 64)
		 * @return QImage, or null QImage on error.
		 */
		QImage loadSpriteSheet(int iconSize);

	public:
		/**
		 * Notification function. (static)
		 * @param user_data	[in] User data. (this)
		 * @param id		[in] Achievement ID.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int RP_C_API notifyFunc(intptr_t user_data, Achievements::ID id);

		/**
		 * Notification function. (non-static)
		 * @param id	[in] Achievement ID.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int notifyFunc(Achievements::ID id);

	public:
		// Sprite sheets.
		// - Key: Icon size
		// - Value: QImage
		unordered_map<int, QImage> map_imgAchSheet;
};

struct NotifyIconStruct {
	int width;
	int height;
	int rowstride;
	bool has_alpha;
	int bits_per_sample;
	int channels;
	QByteArray data;
};
Q_DECLARE_METATYPE(NotifyIconStruct);

static inline QDBusArgument &operator<<(QDBusArgument &argument, const NotifyIconStruct &nis)
{
	argument.beginStructure();
	argument << nis.width;
	argument << nis.height;
	argument << nis.rowstride;
	argument << nis.has_alpha;
	argument << nis.bits_per_sample;
	argument << nis.channels;
	argument << nis.data;
	argument.endStructure();
	return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, NotifyIconStruct &nis)
{
	argument.beginStructure();
	argument >> nis.width;
	argument >> nis.height;
	argument >> nis.rowstride;
	argument >> nis.has_alpha;
	argument >> nis.bits_per_sample;
	argument >> nis.channels;
	argument >> nis.data;
	argument.endStructure();
	return argument;
}

/** AchQtDBusPrivate **/

// Singleton instance.
// Using a static non-pointer variable in order to
// handle proper destruction when the DLL is unloaded.
AchQtDBus AchQtDBusPrivate::instance;

AchQtDBusPrivate::AchQtDBusPrivate()
	: hasRegistered(false)
{
	// NOTE: Cannot register here because the static Achievements
	// instance might not be fully initialized yet.
	qDBusRegisterMetaType<NotifyIconStruct>();
}

AchQtDBusPrivate::~AchQtDBusPrivate()
{
	if (hasRegistered) {
		Achievements *const pAch = Achievements::instance();
		pAch->clearNotifyFunction(notifyFunc, reinterpret_cast<intptr_t>(this));
	}
}

/**
 * Load the specified sprite sheet.
 * @param iconSize Icon size. (16, 24, 32, 64)
 * @return QImage, or invalid QImage on error.
 */
QImage AchQtDBusPrivate::loadSpriteSheet(int iconSize)
{
	assert(iconSize == 16 || iconSize == 24 || iconSize == 32 || iconSize == 64);

	// Check if the sprite sheet is already loaded.
	auto iter = map_imgAchSheet.find(iconSize);
	if (iter != map_imgAchSheet.end()) {
		// Sprite sheet is already loaded.
		return iter->second;
	}

	char ach_filename[32];
	snprintf(ach_filename, sizeof(ach_filename), ":/ach/ach-%dx%d.png", iconSize, iconSize);
	QImage imgAchSheet(QString::fromLatin1(ach_filename));
	if (imgAchSheet.isNull()) {
		// Invalid...
		return imgAchSheet;
	}

	// Make sure it's ARGB32.
	if (imgAchSheet.format() != QImage::Format_ARGB32) {
		imgAchSheet = imgAchSheet.convertToFormat(QImage::Format_ARGB32);
		if (imgAchSheet.isNull()) {
			// Invalid...
			return imgAchSheet;
		}
	}

	// Make sure the bitmap has the expected size.
	assert(imgAchSheet.width() == (int)(iconSize * Achievements::ACH_SPRITE_SHEET_COLS));
	assert(imgAchSheet.height() == (int)(iconSize * Achievements::ACH_SPRITE_SHEET_ROWS));
	if (imgAchSheet.width() != (int)(iconSize * Achievements::ACH_SPRITE_SHEET_COLS) ||
	    imgAchSheet.height() != (int)(iconSize * Achievements::ACH_SPRITE_SHEET_ROWS))
	{
		// Incorrect size. We can't use it.
		return QImage();
	}

	// NOTE: The R and B channels need to be swapped for XDG notifications.
	// Swap the R and B channels in place.
	// TODO: Qt 6.0 will have an in-place rgbSwap() function.
	// TODO: SSSE3-optimized version?
	argb32_t *bits = reinterpret_cast<argb32_t*>(imgAchSheet.bits());
	const int strideDiff = imgAchSheet.bytesPerLine() - (imgAchSheet.width() * sizeof(uint32_t));
	for (unsigned int y = (unsigned int)imgAchSheet.height(); y > 0; y--) {
		unsigned int x;
		for (x = (unsigned int)imgAchSheet.width(); x > 1; x -= 2) {
			// Swap the R and B channels
			std::swap(bits[0].r, bits[0].b);
			std::swap(bits[1].r, bits[1].b);
			bits += 2;
		}
		if (x == 1) {
			// Last pixel
			std::swap(bits->r, bits->b);
			bits++;
		}

		// Next line.
		bits += strideDiff;
	}

	// Sprite sheet is correct.
	map_imgAchSheet.emplace(iconSize, imgAchSheet);
	return imgAchSheet;
}

/**
 * Notification function. (static)
 * @param user_data	[in] User data. (this)
 * @param id		[in] Achievement ID.
 * @return 0 on success; negative POSIX error code on error.
 */
int RP_C_API AchQtDBusPrivate::notifyFunc(intptr_t user_data, Achievements::ID id)
{
	AchQtDBusPrivate *const pAchQtP = reinterpret_cast<AchQtDBusPrivate*>(user_data);
	return pAchQtP->notifyFunc(id);
}

/**
 * Notification function. (non-static)
 * @param id	[in] Achievement ID.
 * @return 0 on success; negative POSIX error code on error.
 */
int AchQtDBusPrivate::notifyFunc(Achievements::ID id)
{
	assert((int)id >= 0);
	assert(id < Achievements::ID::Max);
	if ((int)id < 0 || id >= Achievements::ID::Max) {
		// Invalid achievement ID.
		return -EINVAL;
	}

	// Connect to org.freedesktop.Notifications.
	org::freedesktop::Notifications iface(
		QLatin1String("org.freedesktop.Notifications"),
		QLatin1String("/org/freedesktop/Notifications"),
		QDBusConnection::sessionBus());
	if (!iface.isValid()) {
		// Invalid interface.
		return -EIO;
	}

	Achievements *const pAch = Achievements::instance();

	// Build the text.
	// TODO: Better formatting?
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QString text = QLatin1String("<u>");
	text += U82Q(pAch->getName(id)).toHtmlEscaped();
	text += QLatin1String("</u>\n");
	text += U82Q(pAch->getDescUnlocked(id)).toHtmlEscaped();
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	QString text = QLatin1String("<u>");
	text += Qt::escape(U82Q(pAch->getName(id)));
	text += QLatin1String("</u>\n");
	text += Qt::escape(U82Q(pAch->getDescUnlocked(id)));
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,0,0) */

	// Hints, including image data.
	// FIXME: Icon size. Using 32px for now.
	static const int iconSize = 32;
	const QImage imgspr = loadSpriteSheet(iconSize);
	QVariantMap hints;
	QImage subIcon;
	if (!imgspr.isNull()) {
		// Determine row and column.
		const int col = ((int)id % Achievements::ACH_SPRITE_SHEET_COLS);
		const int row = ((int)id / Achievements::ACH_SPRITE_SHEET_COLS);

		// Extract the sub-icon.
		subIcon = imgspr.copy(col*iconSize, row*iconSize, iconSize, iconSize);

		// Set up the NotifyIconStruct.
		NotifyIconStruct nis;
		nis.width = subIcon.width();
		nis.height = subIcon.height();
		nis.rowstride = subIcon.bytesPerLine();
		nis.has_alpha = true;
		nis.bits_per_sample = 8;	// 8 bits per *channel*.
		nis.channels = 4;
		// TODO: constBits(), sizeInBytes()
		// NOTE: byteCount() doesn't work with deprecated functions disabled.
		nis.data = QByteArray::fromRawData((const char*)subIcon.bits(),
			subIcon.bytesPerLine() * subIcon.height());

		// NOTE: The hint name changed in various versions of the specification.
		// We'll use the oldest version for compatibility purposes.
		// - 1.0: "icon_data"
		// - 1.1: "image_data"
		// - 1.2: "image-data"
		QVariant var;
		var.setValue(nis);
		hints.insert(QLatin1String("icon_data"), var);
	}

	const QString qs_summary = U82Q(C_("Achievements", "Achievement Unlocked"));
	iface.Notify(
		QLatin1String("rom-properties"),	// app-name [s]
		0,					// replaces_id [u]
		QString(),				// app_icon [s]
		qs_summary,				// summary [s]
		text,					// body [s]
		QStringList(),				// actions [as]
		hints,					// hints [a{sv}]
		5000);					// timeout (ms) [i]

	// NOTE: Not waiting for a response.
	return 0;
}

/** AchQtDBus **/

AchQtDBus::AchQtDBus()
	: d_ptr(new AchQtDBusPrivate())
{ }

AchQtDBus::~AchQtDBus()
{
	delete d_ptr;
}

/**
 * Get the AchQtDBus instance.
 *
 * This automatically initializes librpbase's Achievement
 * object and reloads the achievements data if it has been
 * modified.
 *
 * @return AchQtDBus instance.
 */
AchQtDBus *AchQtDBus::instance(void)
{
	AchQtDBus *const q = &AchQtDBusPrivate::instance;

	// NOTE: Cannot register in the private class constructor because
	// the Achievements instance might not be fully initialized yet.
	// Registering here instead.
	Achievements *const pAch = Achievements::instance();
	pAch->setNotifyFunction(AchQtDBusPrivate::notifyFunc, reinterpret_cast<intptr_t>(q->d_ptr));

	return q;
}
