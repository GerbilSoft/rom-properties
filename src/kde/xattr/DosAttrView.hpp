/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * DosAttrView.hpp: MS-DOS file system attribute viewer widget.            *
 *                                                                         *
 * Copyright (c) 2022-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Qt includes
#include <QWidget>

class DosAttrViewPrivate;
class DosAttrView : public QWidget
{
Q_OBJECT

Q_PROPERTY(unsigned int attrs READ attrs WRITE setAttrs RESET clearAttrs)
Q_PROPERTY(bool canWriteAttrs READ canWriteAttrs WRITE setCanWriteAttrs)

public:
	explicit DosAttrView(QWidget *parent = nullptr);

private:
	typedef QWidget super;
	DosAttrViewPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(DosAttrView)
	Q_DISABLE_COPY(DosAttrView)

public:
	/**
	 * Get the current MS-DOS attributes.
	 * @return MS-DOS attributes
	 */
	unsigned int attrs(void) const;

	/**
	 * Set the current MS-DOS attributes.
	 * @param attrs MS-DOS attributes
	 */
	void setAttrs(unsigned int attrs);

	/**
	 * Clear the current MS-DOS attributes.
	 */
	void clearAttrs(void);

	/**
	 * Set if MS-DOS attributes can be written.
	 * This controls if the RHAS checkboxes can be edited by the user.
	 * @param widget DosAttrView
	 * @param can_write True if they can; false if they can't.
	 */
	void setCanWriteAttrs(bool canWriteAttrs);

	/**
	 * Can MS-DOS attributes be written?
	 * This controls if the RHAS checkboxes can be edited by the user.
	 * @param widget DosAttrView
	 * @return True if they can; false if they can't.
	 */
	bool canWriteAttrs(void) const;

protected slots:
	/**
	 * Allow writable bitfield checkboxes to be toggled.
	 * @param checked New check state
	 */
	void checkBox_writable_clicked_slot(bool checked);

	/**
	 * Prevent bitfield checkboxes from being toggled.
	 * @param checked New check state
	 */
	void checkBox_noToggle_clicked_slot(bool checked);

signals:
	/**
	 * An attribute was modified by the user.
	 * @param old_attrs Old attributes
	 * @param new_attrs New attributes
	 */
	void modified(uint32_t old_attrs, uint32_t new_attrs);
};
