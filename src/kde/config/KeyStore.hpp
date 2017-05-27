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

	Q_PROPERTY(int count READ count)

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
				Status_NotAKey,		// Not a key.
				Status_Incorrect,	// Key is incorrect.
				Status_OK,		// Key is OK.
			};

			QString name;		// Key name.
			QString value;		// Key value. (as QString for display purposes)
			uint8_t status;		// Key status. (See the Status enum.)
			bool allowKanji;	// Allow Kanji for UTF-16LE + BOM.
		};

	signals:
		/** Key management **/

		/**
		 * Keys are about to be added to the KeyStore.
		 * @param start First key index.
		 * @param end Last key index.
		 */
		void keysAboutToBeInserted(int start, int end);

		/**
		 * Keys have been added to the KeyStore.
		 */
		void keysInserted(void);

		/**
		 * Keys are about to be removed from the KeyStore.
		 * @param start First key index.
		 * @param end Last key index.
		 */
		void keysAboutToBeRemoved(int start, int end);

		/**
		 * Keys have been removed from the KeyStore.
		 */
		void keysRemoved(void);

		/**
		 * A key has changed.
		 * @param idx Key index.
		 */
		void keyChanged(int idx);

	public:
		/**
		 * Get the number of keys.
		 * @return Number of keys.
		 */
		int count(void) const;

		/**
		 * Is the KeyStore empty?
		 * @return True if empty; false if not.
		 */
		bool isEmpty(void) const;

		/**
		 * Get a Key object.
		 * @param idx Key index.
		 * @return Key object, or nullptr on error.
		 */
		const Key *getKey(int idx) const;
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_KEYSTORE_HPP__ */
