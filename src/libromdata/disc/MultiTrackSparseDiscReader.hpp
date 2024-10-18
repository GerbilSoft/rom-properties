/*******************************************************************************
 * ROM Properties Page shell extension. (libromdata)                           *
 * MultiTrackSparseDiscReader.hpp: Multi-track sparse image reader interface.  *
 *                                                                             *
 * Copyright (c) 2016-2024 by David Korth.                                     *
 * SPDX-License-Identifier: GPL-2.0-or-later                                   *
 *******************************************************************************/

#pragma once

#include "librpbase/disc/SparseDiscReader.hpp"
#include "IsoPartition.hpp"

// for ISOPtr
#include "../Media/ISO.hpp"

namespace LibRomData {

class NOVTABLE MultiTrackSparseDiscReader : public LibRpBase::SparseDiscReader
{
protected:
	explicit MultiTrackSparseDiscReader(LibRpBase::SparseDiscReaderPrivate *d, const LibRpFile::IRpFilePtr &file)
		: super(d, file)
	{}

private:
	typedef SparseDiscReader super;
	RP_DISABLE_COPY(MultiTrackSparseDiscReader)

public:
	/** MultiTrackSparseDiscReader functions **/

	/**
	 * Get the track count.
	 * @return Track count
	 */
	virtual int trackCount(void) const = 0;

	/**
	 * Get the starting LBA of the specified track number.
	 * @param trackNumber Track number (1-based)
	 * @return Starting LBA, or -1 if the track number is invalid
	 */
	virtual int startingLBA(int trackNumber) const = 0;

	/**
	 * Open a track using IsoPartition.
	 * @param trackNumber Track number (1-based)
	 * @return IsoPartition, or nullptr on error
	 */
	virtual IsoPartitionPtr openIsoPartition(int trackNumber) = 0;

	/**
	 * Create an ISO RomData object for a given track number.
	 * @param trackNumber Track number (1-based)
	 * @return ISO object, or nullptr on error
	 */
	virtual ISOPtr openIsoRomData(int trackNumber) = 0;
};

typedef std::shared_ptr<MultiTrackSparseDiscReader> MultiTrackSparseDiscReaderPtr;

} // namespace LibRomData
