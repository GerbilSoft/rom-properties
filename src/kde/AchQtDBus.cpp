/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * AchQtDBus.cpp: QtDBus notifications for achievements.                   *
 *                                                                         *
 * Copyright (c) 2020-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchQtDBus.hpp"

// librpbase, librptexture
#include "librpbase/Achievements.hpp"
using LibRpBase::Achievements;
using LibRpTexture::argb32_t;

// QtDBus
#include "notificationsinterface.h"

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
// for Qt::escape()
#  include <QtGui/QTextDocument>
#endif /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */

// Achievement spritesheets
#include "AchSpriteSheet.hpp"

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
	const AchSpriteSheet achSpriteSheet(iconSize);
	QVariantMap hints;

	// Get the icon.
	QImage icon = achSpriteSheet.getIcon(id).toImage();
	if (!icon.isNull()) {
		if (icon.format() != QImage::Format_ARGB32) {
			// Need to use ARGB32 format.
#if QT_VERSION >= QT_VERSION_CHECK(5,13,0)
			icon.convertTo(QImage::Format_ARGB32);
#else /* QT_VERSION < QT_VERSION_CHECK(5,13,0) */
			icon = icon.convertToFormat(QImage::Format_ARGB32);
#endif /* QT_VERSION >= QT_VERSION_CHECK(5,13,0) */
		}

		// NOTE: The R and B channels need to be swapped for XDG notifications.
		// Swap the R and B channels in place.
		// TODO: Qt 6.0 will have an in-place rgbSwap() function.
		// TODO: SSSE3-optimized version?
		argb32_t *bits = reinterpret_cast<argb32_t*>(icon.bits());
		const int strideDiff = icon.bytesPerLine() - (icon.width() * sizeof(uint32_t));
		for (unsigned int y = (unsigned int)icon.height(); y > 0; y--) {
			unsigned int x;
			for (x = (unsigned int)icon.width(); x > 1; x -= 2) {
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

		// Set up the NotifyIconStruct.
		NotifyIconStruct nis;
		nis.width = icon.width();
		nis.height = icon.height();
		nis.rowstride = icon.bytesPerLine();
		nis.has_alpha = true;
		nis.bits_per_sample = 8;	// 8 bits per *channel*.
		nis.channels = 4;
		// TODO: constBits(), sizeInBytes()
		// NOTE: byteCount() doesn't work with deprecated functions disabled.
		nis.data = QByteArray::fromRawData((const char*)icon.bits(),
			icon.bytesPerLine() * icon.height());

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
{}

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
