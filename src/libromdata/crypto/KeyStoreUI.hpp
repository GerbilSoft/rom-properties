/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * KeyStoreUI.hpp: Key store UI base class.                                *
 *                                                                         *
 * Copyright (c) 2012-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes.
#include <stdint.h>

// C++ includes.
#include <string>

namespace LibRpFile {
	class IRpFile;
}

namespace LibRomData {

class KeyStoreUIPrivate;
class RP_LIBROMDATA_PUBLIC NOVTABLE KeyStoreUI
{
public:
	KeyStoreUI();
	virtual ~KeyStoreUI();

private:
	KeyStoreUIPrivate *const d_ptr;
	friend class KeyStoreUIPrivate;
	RP_DISABLE_COPY(KeyStoreUI)

public:
	/** Key struct **/
	struct Key {
		enum class Status : uint8_t {
			Empty = 0,	// Key is empty
			Unknown,	// Key status is unknown
			NotAKey,	// Not a key
			Incorrect,	// Key is incorrect
			OK,		// Key is OK
		};

		std::string name;	// Key name [ASCII]
		std::string value;	// Key value [ASCII, for display purposes]
		Status status;		// Key status (See the Status enum)
		bool modified;		// True if the key has been modified since last reset() or allKeysSaved().
		bool allowKanji;	// Allow kanji for UTF-16LE + BOM.
	};

public:
	/**
	 * (Re-)Load the keys from keys.conf.
	 */
	void reset(void);

public:
	/**
	 * Convert a sectIdx/keyIdx pair to a flat key index.
	 * @param sectIdx	[in] Section index.
	 * @param keyIdx	[in] Key index.
	 * @return Flat key index, or -1 if invalid.
	 */
	int sectKeyToIdx(int sectIdx, int keyIdx) const;

	/**
	 * Convert a flat key index to sectIdx/keyIdx.
	 * @param idx		[in] Flat key index.
	 * @param pSectIdx	[out] Section index.
	 * @param pKeyIdx	[out] Key index.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int idxToSectKey(int idx, int *pSectIdx, int *pKeyIdx) const;

protected: /*signals:*/
	/** Key management signals. **/
	// These must be reimplemented by subclasses
	// and implemented as notification signals.

	/**
	 * A key has changed.
	 * @param sectIdx Section index.
	 * @param keyIdx Key index.
	 */
	virtual void keyChanged_int(int sectIdx, int keyIdx) = 0;

	/**
	 * A key has changed.
	 * @param idx Flat key index.
	 */
	virtual void keyChanged_int(int idx) = 0;

	/**
	 * All keys have changed.
	 */
	virtual void allKeysChanged_int(void) = 0;

public:
	/**
	 * Get the number of sections. (top-level)
	 * @return Number of sections.
	 */
	int sectCount(void) const;

	/**
	 * Get a section name.
	 * @param sectIdx Section index.
	 * @return Section name, or nullptr on error.
	 */
	const char *sectName(int sectIdx) const;

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
	 * @param sectIdx Section index
	 * @param keyIdx Key index
	 * @param value New value [NULL-terminated UTF-8 string]
	 * @return 0 on success; non-zero on error.
	 */
	int setKey(int sectIdx, int keyIdx, const char *value);

	/**
	 * Set a key's value.
	 * If successful, and the new value is different,
	 * keyChanged() will be emitted.
	 *
	 * @param idx Flat key index
	 * @param value New value [NULL-terminated UTF-8 string]
	 * @return 0 on success; non-zero on error.
	 */
	int setKey(int idx, const char *value);

public /*slots*/:
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

protected /*signals*/:
	/**
	 * KeyStore has been changed by the user.
	 * This must be reimplemented by subclasses
	 * and implemented as a notification signal.
	 */
	virtual void modified_int(void) = 0;

public:
	enum class ImportStatus : uint8_t {
		InvalidParams = 0,	// Invalid parameters. (Should not happen!)
		UnknownKeyID,		// Unknown key ID. (Should not happen!)
		OpenError,		// Could not open the file. (TODO: More info?)
		ReadError,		// Could not read the file. (TODO: More info?)
		InvalidFile,		// File is not the correct type.
		NoKeysImported,		// No keys were imported.
		KeysImported,		// Keys were imported.
	};

	/**
	 * Return data for the import functions.
	 */
	struct ImportReturn {
		ImportStatus status;		/* ImportStatus */
		uint8_t error_code;		// POSIX error code. (0 for success or unknown)
		uint8_t keysExist;		// Keys not imported because they're already in the file.
		uint8_t keysInvalid;		// Keys not imported because they didn't verify.
		uint8_t keysNotUsed;		// Keys not imported because they aren't used by rom-properties.
		uint8_t keysCantDecrypt;	// Keys not imported because they're encrypted and the key isn't available.
		uint8_t keysImportedVerify;	// Keys imported and verified.
		uint8_t keysImportedNoVerify;	// Keys imported but unverified.
	};

	/**
	 * Import file ID
	 */
	enum class ImportFileID {
		WiiKeysBin = 0,
		WiiUOtpBin,
		N3DSboot9bin,
		N3DSaeskeydb,
	};

	/**
	 * Import keys from a binary file.
	 * @param fileID	[in] Type of file
	 * @param file		[in] Opened file
	 * @return ImportReturn
	 */
	ImportReturn importKeysFromBin(ImportFileID fileID, LibRpFile::IRpFile *file);

	/**
	 * Import keys from a binary file.
	 * @param fileID	[in] Type of file
	 * @param filename	[in] Filename (UTF-8)
	 * @return ImportReturn
	 */
	ImportReturn importKeysFromBin(ImportFileID fileID, const char *filename);

#ifdef _WIN32
	/**
	 * Import keys from a binary file.
	 * @param fileID	[in] Type of file
	 * @param filenameW	[in] Filename (UTF-16)
	 * @return ImportReturn
	 */
	ImportReturn importKeysFromBin(ImportFileID fileID, const wchar_t *filenameW);
#endif /* _WIN32 */
};

}
