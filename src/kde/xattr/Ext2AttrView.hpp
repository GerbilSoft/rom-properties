/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * Ext2AttrView.hpp: Ext2 file system attribute viewer widget.             *
 *                                                                         *
 * Copyright (c) 2022-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Qt includes
#include <QWidget>

// XAttrReader::ZAlgorithm
#include "librpfile/xattr/XAttrReader.hpp"

class Ext2AttrViewPrivate;
class Ext2AttrView : public QWidget
{
Q_OBJECT

Q_PROPERTY(int flags READ flags WRITE setFlags RESET clearFlags)
// TODO: Proper enum registration.
//Q_PROPERTY(XAttrReader::ZAlgorithm zAlgorithm READ zAlgorithm WRITE setZAlgorithm RESET clearZAlgorithm)
Q_PROPERTY(int zLevel READ zLevel WRITE setZLevel RESET clearZLevel)

public:
	explicit Ext2AttrView(QWidget *parent = nullptr);

private:
	typedef QWidget super;
	Ext2AttrViewPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(Ext2AttrView)
	Q_DISABLE_COPY(Ext2AttrView)

protected:
	// State change event. (Used for switching the UI language at runtime.)
	void changeEvent(QEvent *event) final;

	/**
	 * Event filter for top-level windows.
	 * @param object QObject
	 * @param event Event
	 * @return True to filter the event; false to pass it through.
	 */
	bool eventFilter(QObject *object, QEvent *event) final;

public:
	/**
	 * Get the current Ext2 attributes.
	 * @return Ext2 attributes
	 */
	int flags(void) const;

	/**
	 * Set the current Ext2 attributes.
	 * @param flags Ext2 attributes
	 */
	void setFlags(int flags);

	/**
	 * Clear the current Ext2 attributes.
	 */
	void clearFlags(void);

	/**
	 * Get the current compression algorithm.
	 * @return Compression algorithm
	 */
	LibRpFile::XAttrReader::ZAlgorithm zAlgorithm(void) const;

	/**
	 * Set the current compression algorithm.
	 * @param zAlgorithm Compression algorithm
	 */
	void setZAlgorithm(LibRpFile::XAttrReader::ZAlgorithm zAlgorithm);

	/**
	 * Clear the current compression algorithm.
	 */
	void clearZAlgorithm(void);

	/**
	 * Get the current compression level.
	 * @return Compression level (0 for not specified)
	 */
	int zLevel(void) const;

	/**
	* Set the current compression level.
	 * @param zLevel Compression level (0 for not specified)
	 */
	void setZLevel(int zLevel);

	/**
	 * Clear the current compression level.
	 */
	void clearZLevel(void);

	/**
	 * Set the current compression algorithm and level.
	 * @param zAlgorithm Compression algorithm
	 * @param zLevel Compression level (0 for not specified)
	 */
	void setZAlgorithmAndZLevel(LibRpFile::XAttrReader::ZAlgorithm zAlgorithm, int zLevel);

	/**
	 * Clear the current compression algorithm and level.
	 */
	void clearZAlgorithmAndZLevel(void);

protected slots:
	/**
	 * Disable user modifications of checkboxes.
	 */
	void checkBox_clicked_slot(bool checked);
};
