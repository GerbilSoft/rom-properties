/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * KeyStoreModel.hpp: QAbstractListModel for KeyStore.                     *
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

#ifndef __ROMPROPERTIES_KDE_CONFIG_KEYSTOREMODEL_HPP__
#define __ROMPROPERTIES_KDE_CONFIG_KEYSTOREMODEL_HPP__

// Qt includes.
#include <QtCore/QAbstractListModel>

class KeyStore;

class KeyStoreModelPrivate;
class KeyStoreModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(KeyStore* keyStore READ keyStore WRITE setKeyStore NOTIFY keyStoreChanged)

	public:
		explicit KeyStoreModel(QObject *parent = 0);
		virtual ~KeyStoreModel();

	private:
		typedef QAbstractListModel super;
		KeyStoreModelPrivate *const d_ptr;
		Q_DECLARE_PRIVATE(KeyStoreModel)
		Q_DISABLE_COPY(KeyStoreModel)

	public:
		enum Column {
			COL_KEY_NAME,	// Key name
			COL_VALUE,	// Value
			COL_ISVALID,	// Valid?

			COL_MAX
		};

		// Qt Model/View interface.
		virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override final;
		virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override final;

		virtual QVariant data(const QModelIndex& index, int role) const override final;
		virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override final;

	public:
		/**
		 * Set the KeyStore to use in this model.
		 * @param keyStore KeyStore.
		 */
		void setKeyStore(KeyStore *keyStore);

		/**
		 * Get the KeyStore in use by this model.
		 * @return KeyStore.
		 */
		KeyStore *keyStore(void) const;

	signals:
		/**
		 * The KeyStore has been changed.
		 */
		void keyStoreChanged(void);

	private slots:
		/**
		 * KeyStore object was destroyed.
		 * @param obj QObject that was destroyed.
		 */
		void keyStore_destroyed_slot(QObject *obj = 0);

		/**
		 * A key in the KeyStore has changed.
		 * @param keyIdx Flat key index.
		 */
		void keyStore_keyChanged_slot(int idx);

		/**
		 * All keys in the KeyStore have changed.
		 */
		void keyStore_allKeysChanged_slot(void);

		/**
		 * The system theme has changed.
		 */
		void themeChanged_slot(void);
};

#endif /* __ROMPROPERTIES_KDE_CONFIG_KEYSTOREMODEL_HPP__ */
