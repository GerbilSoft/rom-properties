/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyStore.hpp: Key store object for Qt.                                  *
 *                                                                         *
 * Copyright (c) 2012-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_CONFIG_KEYSTORE_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_KEYSTORE_HPP__

#include <QtCore/QObject>

class KeyStorePrivate;
class KeyStore : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int totalKeyCount READ totalKeyCount)
	// NOTE: modified() isn't a modifier, since it's only emitted
	// if d->changed becomes true.
	Q_PROPERTY(bool changed READ hasChanged)

	public:
		KeyStore(QObject *parent = 0);
		virtual ~KeyStore();

	private:
		typedef QObject super;
		KeyStorePrivate *const d_ptr;
		Q_DECLARE_PRIVATE(KeyStore)
		Q_DISABLE_COPY(KeyStore)

	public:
		/** Key struct. **/
		struct Key {
			enum Status {
				Status_Empty = 0,	// Key is empty.
				Status_Unknown,		// Key status is unknown.
				Status_NotAKey,		// Not a key.
				Status_Incorrect,	// Key is incorrect.
				Status_OK,		// Key is OK.
			};

			QString name;		// Key name.
			QString value;		// Key value. (as QString for display purposes)
			uint8_t status;		// Key status. (See the Status enum.)
			bool modified;		// True if the key has been modified since last reset() or allKeysSaved().
			bool allowKanji;	// Allow kanji for UTF-16LE + BOM.
		};

	public:
		/**
		 * (Re-)Load the keys from keys.conf.
		 */
		void reset(void);

	signals:
		/** Key management **/

		/**
		 * A key has changed.
		 * @param sectIdx Section index.
		 * @param keyIdx Key index.
		 */
		void keyChanged(int sectIdx, int keyIdx);

		/**
		 * A key has changed.
		 * @param idx Flat key index.
		 */
		void keyChanged(int idx);

		/**
		 * All keys have changed.
		 */
		void allKeysChanged(void);

	public:
		/**
		 * Get the number of sections. (top-level)
		 * @return Number of sections.
		 */
		int sectCount(void) const;

		/**
		 * Get a section name.
		 * @param sectIdx Section index.
		 * @return Section name, or empty string on error.
		 */
		QString sectName(int sectIdx) const;

		/**
		 * Get the number of keys in a given section.
		 * @param sectIdx Section index.
		 * @return Number of keys in the section, or -1 on error.
		 */
		int keyCount(int sectIdx) const;

		/**
		 * Get the total number of keys.
		 * @return Total number of keys.
		 */
		int totalKeyCount(void) const;

		/**
		 * Is the KeyStore empty?
		 * @return True if empty; false if not.
		 */
		bool isEmpty(void) const;

		/**
		 * Get a Key object.
		 * @param sectIdx Section index.
		 * @param keyIdx Key index.
		 * @return Key object, or nullptr on error.
		 */
		const Key *getKey(int sectIdx, int keyIdx) const;

		/**
		 * Get a Key object using a linear key count.
		 * TODO: Remove this once we switch to a Tree model.
		 * @param idx Key index.
		 * @return Key object, or nullptr on error.
		 */
		const Key *getKey(int idx) const;

		/**
		 * Set a key's value.
		 * If successful, and the new value is different,
		 * keyChanged() will be emitted.
		 *
		 * @param sectIdx Section index.
		 * @param keyIdx Key index.
		 * @param value New value.
		 * @return 0 on success; non-zero on error.
		 */
		int setKey(int sectIdx, int keyIdx, const QString &value);

		/**
		 * Set a key's value.
		 * If successful, and the new value is different,
		 * keyChanged() will be emitted.
		 *
		 * @param idx Flat key index.
		 * @param value New value.
		 * @return 0 on success; non-zero on error.
		 */
		int setKey(int idx, const QString &value);

	public slots:
		/**
		 * Mark all keys as saved.
		 * This clears the "modified" field.
		 *
		 * NOTE: We aren't providing a save() function,
		 * since that's OS-dependent. This function should
		 * be called by the OS-specific save code.
		 */
		void allKeysSaved(void);

	public:
		/**
		 * Has KeyStore been changed by the user?
		 * @return True if it has; false if it hasn't.
		 */
		bool hasChanged(void) const;

	signals:
		/**
		 * KeyStore has been changed by the user.
		 */
		void modified(void);

	public:
		enum ImportStatus {
			Import_InvalidParams = 0,	// Invalid parameters. (Should not happen!)
			Import_OpenError,	// Could not open the file. (TODO: More info?)
			Import_ReadError,	// Could not read the file. (TODO: More info?)
			Import_InvalidFile,	// File is not the correct type.
			Import_NoKeysImported,	// No keys were imported.
			Import_KeysImported,	// Keys were imported.
		};

		/**
		 * Return data for the import functions.
		 */
		struct ImportReturn {
			uint8_t status;			/* ImportStatus */
			uint8_t keysExist;		// Keys not imported because they're already in the file.
			uint8_t keysInvalid;		// Keys not imported because they didn't verify.
			uint8_t keysNotUsed;		// Keys not imported because they aren't used by rom-properties.
			uint8_t keysCantDecrypt;	// Keys not imported because they're encrypted and the key isn't available.
			uint8_t keysImportedVerify;	// Keys imported and verified.
			uint8_t keysImportedNoVerify;	// Keys imported but unverified.
		};

		/**
		 * Import a Wii keys.bin file.
		 * @param filename keys.bin filename.
		 * @return Key import status.
		 */
		ImportReturn importWiiKeysBin(const QString &filename);

		/**
		 * Import a 3DS boot9.bin file.
		 * @param filename boot9.bin filename.
		 * @return Key import status.
		 */
		ImportReturn import3DSboot9bin(const QString &filename);

		/**
		 * Import a 3DS aeskeydb.bin file.
		 * @param filename aeskeydb.bin filename.
		 * @return Key import status.
		 */
		ImportReturn import3DSaeskeydb(const QString &filename);
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_KEYSTORE_HPP__ */
