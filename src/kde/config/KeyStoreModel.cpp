/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyStoreModel.cpp: QAbstractListModel for KeyStore.                     *
 *                                                                         *
 * Copyright (c) 2012-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "KeyStoreModel.hpp"
#include "KeyStoreQt.hpp"

// C++ STL classes
using std::array;

/** KeyStoreModelPrivate **/

class KeyStoreModelPrivate
{
public:
	explicit KeyStoreModelPrivate(KeyStoreModel *q);

protected:
	KeyStoreModel *const q_ptr;
	Q_DECLARE_PUBLIC(KeyStoreModel)
private:
	Q_DISABLE_COPY(KeyStoreModelPrivate)

public:
	KeyStoreQt *keyStore;

	// Style variables.
	struct style_t {
		style_t() { init_fonts(); init_icons(); }

		/**
		 * Initialize the fonts.
		 */
		void init_fonts(void);

		/**
		 * Initialize the icons.
		 */
		void init_icons(void);

		// Monospace font.
		QFont fntMonospace;
		QSize szValueHint;	// Size hint for the value column.

		// Pixmaps for Column::IsValid.
		// TODO: Hi-DPI support.
		static constexpr int pxmIsValid_size = 16;
		QPixmap pxmIsValid_unknown;
		QPixmap pxmIsValid_invalid;
		QPixmap pxmIsValid_good;
	};
	style_t style;

	// Translated column names.
	array<QString, 3> columnNames;

	/**
	 * (Re-)Translate the column names.
	 */
	void retranslateUi(void);

	/**
	 * Cached copy of keyStore->sectCount().
	 * This value is needed after the KeyStore is destroyed,
	 * so we need to cache it here, since the destroyed()
	 * slot might be run *after* the KeyStore is deleted.
	 */
	int sectCount;
};

// Windows-style LOWORD()/HIWORD()/MAKELONG() functions.
// QModelIndex internal ID:
// - LOWORD: Section.
// - HIWORD: Key index. (0xFFFF for section header)
#ifndef _WIN32
static inline constexpr uint16_t LOWORD(uint32_t dwValue) {
	return (dwValue & 0xFFFF);
}
static inline constexpr uint16_t HIWORD(uint32_t dwValue) {
	return (dwValue >> 16);
}
static inline constexpr uint32_t MAKELONG(uint16_t wLow, uint16_t wHigh) {
	return (wLow | (wHigh << 16));
}
#endif /* _WIN32 */

KeyStoreModelPrivate::KeyStoreModelPrivate(KeyStoreModel *q)
	: q_ptr(q)
	, keyStore(nullptr)
	, sectCount(0)
{
	retranslateUi();
}

/**
 * Initialize the fonts.
 */
void KeyStoreModelPrivate::style_t::init_fonts(void)
{
	// Monospace font
	fntMonospace = getSystemMonospaceFont();

	// Size hint for the monospace column.
	// NOTE: Needs an extra space, possibly due to margins...
	const QFontMetrics fm(fntMonospace);
	szValueHint = fm.size(Qt::TextSingleLine,
		QLatin1String("0123456789ABCDEF0123456789ABCDEF "));
}

/**
 * Initialize the icons.
 */
void KeyStoreModelPrivate::style_t::init_icons(void)
{
	// Initialize the Column::IsValid pixmaps.
	// TODO: Handle SP_MessageBoxQuestion on non-Windows systems,
	// which usually have an 'i' icon here (except for GNOME).
	QStyle *const style = QApplication::style();
	pxmIsValid_unknown = style->standardIcon(QStyle::SP_MessageBoxQuestion)
				.pixmap(pxmIsValid_size, pxmIsValid_size);
	pxmIsValid_invalid = style->standardIcon(QStyle::SP_MessageBoxCritical)
				.pixmap(pxmIsValid_size, pxmIsValid_size);
	pxmIsValid_good    = style->standardIcon(QStyle::SP_DialogApplyButton)
				.pixmap(pxmIsValid_size, pxmIsValid_size);
}

/**
 * (Re-)Translate the column names.
 */
void KeyStoreModelPrivate::retranslateUi(void)
{
	// Translate and cache the column names.

	// tr: Column 0: Key Name.
	columnNames[0] = QC_("KeyManagerTab", "Key Name");
	// tr: Column 1: Value.
	columnNames[1] = QC_("KeyManagerTab", "Value");
	// tr: Column 2: Verification status.
	columnNames[2] = QC_("KeyManagerTab", "Valid?");
}

/** KeyStoreModel **/

KeyStoreModel::KeyStoreModel(QObject *parent)
	: super(parent)
	, d_ptr(new KeyStoreModelPrivate(this))
{
	// TODO: Handle system theme changes.
	// On Windows, listen for WM_THEMECHANGED.
	// Not sure about other systems...
}

KeyStoreModel::~KeyStoreModel()
{
	delete d_ptr;
}


int KeyStoreModel::rowCount(const QModelIndex& parent) const
{
	Q_D(const KeyStoreModel);
	if (!d->keyStore) {
		// KeyStore isn't set yet.
		return 0;
	}

	if (!parent.isValid()) {
		// Root item. Return the number of sections.
		return d->keyStore->sectCount();
	} else {
		if (parent.column() > 0) {
			// rowCount is only valid for column 0.
			return 0;
		}
		// Check the internal ID.
		const uint32_t id = static_cast<uint32_t>(parent.internalId());
		if (HIWORD(id) == 0xFFFF) {
			// Section header.
			return d->keyStore->keyCount(LOWORD(id));
		} else {
			// Key. No rows.
			return 0;
		}
	}
}

int KeyStoreModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	Q_D(const KeyStoreModel);
	if (!d->keyStore) {
		// KeyStore isn't set yet.
		return 0;
	}

	// NOTE: We have to return Column::Max for everything.
	// Otherwise, it acts a bit wonky.
	return static_cast<int>(Column::Max);
}

QModelIndex KeyStoreModel::index(int row, int column, const QModelIndex& parent) const
{
	Q_D(const KeyStoreModel);
	if (!d->keyStore || !hasIndex(row, column, parent))
		return {};

	if (!parent.isValid()) {
		// Root item.
		if (row < 0 || row >= d->keyStore->sectCount()) {
			// Invalid index.
			return {};
		}
		return createIndex(row, column, MAKELONG(row, 0xFFFF));
	} else {
		// Check the internal ID.
		const uint32_t id = static_cast<uint32_t>(parent.internalId());
		if (HIWORD(id) == 0xFFFF) {
			// Section header.
			if (row < 0 || row >= d->keyStore->keyCount(LOWORD(id))) {
				// Invalid index.
				return {};
			}
			return createIndex(row, column, MAKELONG(LOWORD(id), row));
		} else {
			// Key. Cannot create child index.
			return {};
		}
	}
}

QModelIndex KeyStoreModel::parent(const QModelIndex& index) const
{
	Q_D(const KeyStoreModel);
	if (!d->keyStore || !index.isValid())
		return {};

	// Check the internal ID.
	const uint32_t id = static_cast<uint32_t>(index.internalId());
	if (HIWORD(id) == 0xFFFF) {
		// Section header. Parent is root.
		return {};
	} else {
		// Key. Parent is a section header.
		return createIndex(LOWORD(id), 0, MAKELONG(LOWORD(id), 0xFFFF));
	}
}

QVariant KeyStoreModel::data(const QModelIndex& index, int role) const
{
	Q_D(const KeyStoreModel);
	if (!d->keyStore || !index.isValid())
		return {};

	// Check the internal ID.
	const uint32_t id = static_cast<uint32_t>(index.internalId());
	if (HIWORD(id) == 0xFFFF) {
		// Section header.
		if (index.column() != 0) {
			// Invalid column.
			return {};
		}

		switch (role) {
			case Qt::DisplayRole:
				return U82Q(d->keyStore->sectName(LOWORD(id)));
			default:
				break;
		}

		// Nothing for this role.
		return {};
	}

	// Key index.
	const KeyStoreQt::Key *key = d->keyStore->getKey(LOWORD(id), HIWORD(id));
	if (!key)
		return {};

	switch (role) {
		case Qt::DisplayRole:
			switch (static_cast<Column>(index.column())) {
				case Column::KeyName:
					return U82Q(key->name);
				case Column::Value:
					return U82Q(key->value);
				default:
					break;
			}
			break;

		case Qt::EditRole:
			switch (static_cast<Column>(index.column())) {
				case Column::Value:
					return U82Q(key->value);
				default:
					break;
			}
			break;

		case Qt::DecorationRole:
			// Images must use Qt::DecorationRole.
			// TODO: Add a QStyledItemDelegate to center-align the icon.
			switch (static_cast<Column>(index.column())) {
				case Column::IsValid:
					switch (key->status) {
						default:
						case KeyStoreQt::Key::Status::Unknown:
							// Unknown...
							return d->style.pxmIsValid_unknown;
						case KeyStoreQt::Key::Status::NotAKey:
							// The key data is not in the correct format.
							return d->style.pxmIsValid_invalid;
						case KeyStoreQt::Key::Status::Empty:
							// Empty key.
							break;
						case KeyStoreQt::Key::Status::Incorrect:
							// Key is incorrect.
							return d->style.pxmIsValid_invalid;
						case KeyStoreQt::Key::Status::OK:
							// Key is correct.
							return d->style.pxmIsValid_good;
					}

				default:
					break;
			}
			break;

		case Qt::TextAlignmentRole:
			// Text should be left-aligned horizontally, center-aligned vertically.
			// NOTE: Center-aligning the encryption key causes
			// weirdness when editing, especially since if the
			// key is short, the editor will start in the middle
			// of the column instead of the left side.
			return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);

		case Qt::FontRole:
			switch (static_cast<Column>(index.column())) {
				case Column::Value:
					// The key value should use a monospace font.
					return d->style.fntMonospace;

				default:
					break;
			}
			break;

		case Qt::SizeHintRole:
			switch (static_cast<Column>(index.column())) {
				case Column::Value:
					// Use the monospace size hint.
					return d->style.szValueHint;
				case Column::IsValid:
					// Increase row height by 4px.
					return QSize(d->style.pxmIsValid_size, (d->style.pxmIsValid_size + 4));
				default:
					break;
			}
			break;

		case AllowKanjiRole:
			return key->allowKanji;

		default:
			break;
	}

	// Default value.
	return {};
}

bool KeyStoreModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	Q_D(KeyStoreModel);
	if (!d->keyStore || !index.isValid())
		return false;

	// Check the internal ID.
	const uint32_t id = static_cast<uint32_t>(index.internalId());
	if (HIWORD(id) == 0xFFFF) {
		// Section header. Not editable.
		return false;
	}

	// Key index.

	// Only Column::Value can be edited, and only text.
	if (index.column() != static_cast<int>(Column::Value) || role != Qt::EditRole)
		return false;

	// Edit the value.
	// KeyStoreQt::setKey() will emit a signal if the value changes,
	// which will cause KeyStoreModel to emit dataChanged().
	d->keyStore->setKey(LOWORD(id), HIWORD(id), Q2U8(value.toString()));
	return true;
}

Qt::ItemFlags KeyStoreModel::flags(const QModelIndex &index) const
{
	Q_D(const KeyStoreModel);
	if (!d->keyStore || !index.isValid())
		return {};

	// Check the internal ID.
	const uint32_t id = (uint32_t)index.internalId();
	if (HIWORD(id) == 0xFFFF) {
		// Section header.
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	// Key index.
	switch (static_cast<Column>(index.column())) {
		case Column::Value:
			// Value can be edited.
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
		default:
			// Standard flags.
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}
}

QVariant KeyStoreModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(orientation);

	switch (role) {
		case Qt::DisplayRole: {
			RP_D(const KeyStoreModel);
			if (section >= 0 && section < static_cast<int>(d->columnNames.size())) {
				return d->columnNames[section];
			}
			break;
		}

		case Qt::TextAlignmentRole:
			// Center-align the text.
			return Qt::AlignHCenter;

		default:
			break;
	}

	// Default value.
	return {};
}

/**
 * Set the KeyStore to use in this model.
 * @param keyStore KeyStore.
 */
void KeyStoreModel::setKeyStore(KeyStoreQt *keyStore)
{
	Q_D(KeyStoreModel);
	if (d->keyStore == keyStore) {
		// No point in setting it to the same thing...
		return;
	}

	// If we have a KeyStore already, disconnect its signals.
	if (d->keyStore) {
		// Notify the view that we're about to remove all rows.
		const int sectCount = d->keyStore->sectCount();
		if (sectCount > 0) {
			beginRemoveRows(QModelIndex(), 0, sectCount-1);
		}

		// Disconnect the KeyStore's signals.
		disconnect(d->keyStore, SIGNAL(destroyed(QObject*)),
			   this, SLOT(keyStore_destroyed_slot(QObject*)));
		disconnect(d->keyStore, SIGNAL(keyChanged(int,int)),
			   this, SLOT(keyStore_keyChanged_slot(int,int)));
		disconnect(d->keyStore, SIGNAL(allKeysChanged()),
			   this, SLOT(keyStore_allKeysChanged_slot()));

		d->keyStore = nullptr;

		// Done removing rows.
		d->sectCount = 0;
		if (sectCount > 0) {
			endRemoveRows();
		}
	}

	if (keyStore) {
		// Notify the view that we're about to add rows.
		const int sectCount = keyStore->sectCount();
		if (sectCount > 0) {
			beginInsertRows(QModelIndex(), 0, sectCount-1);
		}

		// Set the KeyStore.
		d->keyStore = keyStore;
		// NOTE: sectCount must be set here.
		d->sectCount = sectCount;

		// Connect the KeyStore's signals.
		connect(d->keyStore, SIGNAL(destroyed(QObject*)),
			this, SLOT(keyStore_destroyed_slot(QObject*)));
		connect(d->keyStore, SIGNAL(keyChanged(int,int)),
			this, SLOT(keyStore_keyChanged_slot(int,int)));
		connect(d->keyStore, SIGNAL(allKeysChanged()),
			this, SLOT(keyStore_allKeysChanged_slot()));

		// Done adding rows.
		if (sectCount > 0) {
			endInsertRows();
		}
	}

	// KeyStore has been changed.
	emit keyStoreChanged();
}

/**
 * Get the KeyStore in use by this model.
 * @return KeyStore.
 */
KeyStoreQt *KeyStoreModel::keyStore(void) const
{
	Q_D(const KeyStoreModel);
	return d->keyStore;
}

/** Private slots **/

/**
 * KeyStore object was destroyed.
 * @param obj QObject that was destroyed.
 */
void KeyStoreModel::keyStore_destroyed_slot(QObject *obj)
{
	Q_D(KeyStoreModel);
	if (obj != d->keyStore)
		return;

	// Our KeyStore was destroyed.
	// NOTE: It's still valid while this function is running.
	// QAbstractItemModel segfaults if we NULL it out before
	// calling beginRemoveRows(). (QAbstractListModel didn't.)
	const int old_sectCount = d->sectCount;
	if (old_sectCount > 0) {
		beginRemoveRows(QModelIndex(), 0, old_sectCount-1);
	}
	d->keyStore = nullptr;
	d->sectCount = 0;
	if (old_sectCount > 0) {
		endRemoveRows();
	}

	emit keyStoreChanged();
}

/**
 * A key in the KeyStore has changed.
 * @param sectIdx Section index.
 * @param keyIdx Key index.
 */
void KeyStoreModel::keyStore_keyChanged_slot(int sectIdx, int keyIdx)
{
	const uint32_t parent_id = MAKELONG(sectIdx, 0xFFFF);
	const QModelIndex qmi_left = createIndex(keyIdx, 0, parent_id);
	const QModelIndex qmi_right = createIndex(keyIdx, static_cast<int>(Column::Max) - 1, parent_id);
	emit dataChanged(qmi_left, qmi_right);
}

/**
 * All keys in the KeyStore have changed.
 */
void KeyStoreModel::keyStore_allKeysChanged_slot(void)
{
	Q_D(KeyStoreModel);
	if (d->sectCount <= 0)
		return;

	// TODO: Enumerate all child keys too?
	const QModelIndex qmi_left = createIndex(0, 0);
	const QModelIndex qmi_right = createIndex(d->sectCount - 1, static_cast<int>(Column::Max) - 1);
	emit dataChanged(qmi_left, qmi_right);
}

/** Public slots **/

/**
 * System language has changed.
 *
 * Call this from the parent widget's changeEvent() function
 * on QEvent::FontChange.
 */
void KeyStoreModel::eventLanguageChange(void)
{
	Q_D(KeyStoreModel);
	d->retranslateUi();
	emit headerDataChanged(Qt::Horizontal, 0, static_cast<int>(d->columnNames.size()-1));
	// FIXME: Re-translate section names?
}

/**
 * System font has changed.
 *
 * Call this from the parent widget's changeEvent() function
 * on QEvent::FontChange.
 */
void KeyStoreModel::eventFontChange(void)
{
	// Reinitialize the fonts.
	Q_D(KeyStoreModel);
	emit layoutAboutToBeChanged();
	d->style.init_fonts();
	emit layoutChanged();
}

/**
 * System color scheme has changed.
 * Icons may need to be re-cached.
 *
 * Call this from the parent widget's changeEvent() function
 * on QEvent::PaletteChange.
 */
void KeyStoreModel::eventPaletteChange(void)
{
	// Reinitialize the icons.
	Q_D(KeyStoreModel);
	emit layoutAboutToBeChanged();
	d->style.init_icons();
	emit layoutChanged();
}
