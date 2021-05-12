/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * LanguageComboBox.hpp: Language QComboBox subclass.                      *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_KDE_LANGUAGECOMBOBOX_HPP__
#define __ROMPROPERTIES_KDE_LANGUAGECOMBOBOX_HPP__

// Qt includes
#include <QComboBox>

// C++ includes
#include <set>

class LanguageComboBox : public QComboBox
{
	Q_OBJECT

	Q_PROPERTY(uint32_t selectedLC READ selectedLC WRITE setSelectedLC NOTIFY lcChanged)

	public:
		explicit LanguageComboBox(QWidget *parent = 0);

	private:
		typedef QComboBox super;
		Q_DISABLE_COPY(LanguageComboBox)

	public:
		/** Language codes **/

		/**
		 * Set the language codes.
		 * @param set_lc Set of language codes.
		 */
		void setLCs(const std::set<uint32_t> &set_lc);

		/**
		 * Set the language codes.
		 * @param p_lc Array of language codes.
		 * @param len Number of language codes.
		 */
		ATTR_ACCESS_SIZE(read_only, 2, 3)
		void setLCs(const uint32_t *p_lc, size_t len);

		/**
		 * Get the set of language codes.
		 * @return Set of language codes.
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
		 * @return Selected language code. (0 if none)
		 */
		uint32_t selectedLC(void) const;

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
};

#endif /* __ROMPROPERTIES_KDE_ROMDATAVIEW_HPP__ */
