/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * LanguageComboBox.hpp: Language QComboBox subclass.                      *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// for ATTR_ACCESS_SIZE()
#include "common.h"

// Qt includes
#include <QComboBox>

// C++ includes
#include <set>

class LanguageComboBox : public QComboBox
{
Q_OBJECT

Q_PROPERTY(uint32_t selectedLC READ selectedLC WRITE setSelectedLC NOTIFY lcChanged)
Q_PROPERTY(bool forcePAL READ isForcePAL WRITE setForcePAL)

public:
	explicit LanguageComboBox(QWidget *parent = nullptr);

private:
	typedef QComboBox super;
	Q_DISABLE_COPY(LanguageComboBox)

protected:
	/**
	 * Update all icons.
	 */
	void updateIcons(void);

public:
	/** Language codes **/

	/**
	 * Set the language codes.
	 * @param set_lc Set of language codes
	 */
	ATTR_ACCESS(read_only, 2)
	void setLCs(const std::set<uint32_t> &set_lc);

	/**
	 * Set the language codes.
	 * @param p_lc Array of language codes (NULL-terminated)
	 */
	ATTR_ACCESS(read_only, 2)
	void setLCs(const uint32_t *p_lc);

	/**
	 * Set the language codes.
	 * @param p_lc Array of language codes
	 * @param len Number of language codes
	 */
	ATTR_ACCESS_SIZE(read_only, 2, 3)
	void setLCs(const uint32_t *p_lc, size_t len);

	/**
	 * Get the set of language codes.
	 * @return Set of language codes
	 */
	std::set<uint32_t> lcs(void) const;

	/**
	 * Clear the language codes.
	 */
	void clearLCs();

	/**
	 * Set the selected language code.
	 *
	 * NOTE: This function will return true if the LC was found,
	 * even if it was already selected.
	 *
	 * @param lc Language code. (0 to unselect)
	 * @return True if set; false if LC was not found.
	 */
	bool setSelectedLC(uint32_t lc);

	/**
	 * Get the selected language code.
	 * @return Selected language code (0 if none)
	 */
	uint32_t selectedLC(void) const;

	/**
	 * Set the Force PAL setting.
	 * @param forcePAL Force PAL setting
	 */
	void setForcePAL(bool forcePAL);

	/**
	 * Get the Force PAL setting.
	 * @return Force PAL setting
	 */
	inline bool isForcePAL(void) const
	{
		return m_forcePAL;
	}

signals:
	/**
	 * Selected language code has been changed.
	 * @param lc New language code. (0 if none)
	 */
	void lcChanged(uint32_t lc);

private slots:
	/**
	 * Emit lcChanged() on currentIndexChanged().
	 * @param index Current index.
	 */
	void this_currentIndexChanged_slot(int index);

protected:
	bool m_forcePAL;
};
