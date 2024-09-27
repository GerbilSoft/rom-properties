/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * ListDataModel.hpp: QAbstractListModel for RFT_LISTDATA.                 *
 *                                                                         *
 * Copyright (c) 2012-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpbase
#include "librpbase/RomFields.hpp"

// Qt includes
#include <QtCore/QAbstractListModel>
#include <QtCore/QSize>

// C++ includes
#include <set>

class ListDataModelPrivate;
class ListDataModel : public QAbstractListModel
{
	Q_OBJECT
	typedef QAbstractListModel super;

	Q_PROPERTY(uint32_t lc READ lc WRITE setLC NOTIFY lcChanged)
	Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize NOTIFY iconSizeChanged)

public:
	explicit ListDataModel(QObject *parent = nullptr);
	~ListDataModel() override;

protected:
	ListDataModelPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(ListDataModel)
private:
	Q_DISABLE_COPY(ListDataModel)

public:
	// Role for an rp_image*.
	static constexpr int RpImageRole = Qt::UserRole + 0x4049;

public:
	// Qt Model/View interface.
	int rowCount(const QModelIndex &parent = QModelIndex()) const final;
	int columnCount(const QModelIndex &parent = QModelIndex()) const final;

	QVariant data(const QModelIndex &index, int role) const final;
	Qt::ItemFlags flags(const QModelIndex &index) const final;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const final;

	/**
	 * Set the field to use in this model.
	 * Field data is *copied* into the model.
	 * @param field Field
	 */
	void setField(const LibRpBase::RomFields::Field *pField);

public:
	/**
	 * Set the language code to use in this model.
	 * @param lc Language code. (0 for default)
	 */
	void setLC(uint32_t lc);

	/**
	 * Set the language code to use in this model.
	 * @param def_lc ROM default language code.
	 * @param user_lc User-specified language code.
	 */
	void setLC(uint32_t def_lc, uint32_t user_lc);

	/**
	 * Get the language code used in this model.
	 * @return Language code. (0 for default)
	 */
	uint32_t lc(void) const;

	/**
	 * Get all supported language codes.
	 *
	 * If this is not showing RFT_LISTDATA_MULTI, an empty set
	 * will be returned.
	 *
	 * @return Supported language codes.
	 */
	std::set<uint32_t> getLCs(void) const;

	/**
	 * Set the icon size.
	 * @param iconSize Icon size.
	 */
	void setIconSize(QSize iconSize);

	/**
	 * Set the icon size.
	 * @param width Icon width.
	 * @param height Icon height.
	 */
	inline void setIconSize(int width, int height)
	{
		setIconSize(QSize(width, height));
	}

	/**
	 * Get the icon size.
	 * @return Icon size.
	 */
	QSize iconSize(void) const;

signals:
	/**
	 * Language code has changed.
	 * @param lc Language code.
	 */
	void lcChanged(uint32_t lc);

	/**
	 * Icon size has changed.
	 * @param iconSize Icon size.
	 */
	void iconSizeChanged(QSize iconSize);
};
