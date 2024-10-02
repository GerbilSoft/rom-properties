/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * ListDataSortProxyModel.hpp: QSortFilterProxyModel for RFT_LISTDATA.     *
 *                                                                         *
 * Copyright (c) 2012-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C includes (C++ namespace)
#include <cstdint>

// Qt includes
#include <QSortFilterProxyModel>

class ListDataSortProxyModelPrivate;
class ListDataSortProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
	typedef QSortFilterProxyModel super;

	Q_PROPERTY(uint32_t sortingMethods READ sortingMethods WRITE setSortingMethods NOTIFY sortingMethodsChanged)

public:
	explicit ListDataSortProxyModel(QObject *parent = nullptr)
		: super(parent)
		, m_sortingMethods(0)
	{}
	~ListDataSortProxyModel() override = default;

private:
	Q_DISABLE_COPY(ListDataSortProxyModel)

private:
	/**
	 * Parse a QString as a number.
	 * @param str		[in] QString
	 * @param pbAllNumeric	[out,opt] Set to true if the entire QString is numeric.
	 * @return Number
	 */
	static qlonglong parseQString(const QString &str, bool *pbAllNumeric);

	/**
	 * Numeric comparison function.
	 * @param strA
	 * @param strB
	 * @return True if strA < strB
	 */
	static bool doNumericCompare(const QString &strA, const QString &strB);

public:
	/**
	 * Comparison function.
	 * @param source_left
	 * @param source_right
	 * @return True if source_left < source_right
	 */
	bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const final;

public:
	/**
	 * Set the sorting methods.
	 * @param sortingMethods Sorting methods.
	 */
	void setSortingMethods(uint16_t sortingMethods)
	{
		if (m_sortingMethods == sortingMethods)
			return;
		m_sortingMethods = sortingMethods;
		emit sortingMethodsChanged(sortingMethods);
	}

	/**
	 * Get the sorting methods.
	 * @return Sorting methods.
	 */
	uint16_t sortingMethods(void) const
	{
		return m_sortingMethods;
	}

signals:
	/**
	 * Sorting methods have changed.
	 * @param sortingMethods Sorting methods.
	 */
	void sortingMethodsChanged(uint16_t sortingMethods);

private:
	uint16_t m_sortingMethods;
};
