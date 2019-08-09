/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * FileFormat.cpp: Texture file format base class.                         *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "FileFormat.hpp"
#include "FileFormat_p.hpp"

// librpbase
#include "librpbase/file/IRpFile.hpp"
#include "librpbase/threads/Atomics.h"
using LibRpBase::IRpFile;

// C includes. (C++ namespace)
#include <cassert>

namespace LibRpTexture {

/** FileFormatPrivate **/

/**
 * Initialize a FileFormatPrivate storage class.
 *
 * @param q FileFormat class.
 * @param file Texture file.
 */
FileFormatPrivate::FileFormatPrivate(FileFormat *q, IRpFile *file)
	: q_ptr(q)
	, ref_cnt(1)
	, isValid(false)
	, file(nullptr)
{
	if (file) {
		// Reference the file.
		this->file = file->ref();
	}
}

FileFormatPrivate::~FileFormatPrivate()
{
	// Unreference the file.
	if (this->file) {
		this->file->unref();
	}
}

/** FileFormat **/

FileFormat::FileFormat(FileFormatPrivate *d)
	: d_ptr(d)
{ }

FileFormat::~FileFormat()
{
	delete d_ptr;
}

/**
 * Take a reference to this FileFormat* object.
 * @return this
 */
FileFormat *FileFormat::ref(void)
{
	RP_D(FileFormat);
	ATOMIC_INC_FETCH(&d->ref_cnt);
	return this;
}

/**
 * Unreference this FileFormat* object.
 * If ref_cnt reaches 0, the FileFormat* object is deleted.
 */
void FileFormat::unref(void)
{
	RP_D(FileFormat);
	assert(d->ref_cnt > 0);
	if (ATOMIC_DEC_FETCH(&d->ref_cnt) <= 0) {
		// All references removed.
		delete this;
	}
}

/**
 * Is this ROM valid?
 * @return True if it is; false if it isn't.
 */
bool FileFormat::isValid(void) const
{
	RP_D(const FileFormat);
	return d->isValid;
}

/**
 * Is the file open?
 * @return True if the file is open; false if it isn't.
 */
bool FileFormat::isOpen(void) const
{
	RP_D(const FileFormat);
	return (d->file != nullptr);
}

/**
 * Close the opened file.
 */
void FileFormat::close(void)
{
	// Unreference the file.
	RP_D(FileFormat);
	if (d->file) {
		d->file->unref();
		d->file = nullptr;
	}
}

}
