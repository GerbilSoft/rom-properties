/***************************************************************************
 * ROM Properties Page shell extension. (kde/tests)                        *
 * RomDataViewTest.cpp: RomDataView tests                                  *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// RomDataTestObject
#include "librpbase/RomDataTestObject.hpp"
#include "librpbase/RomFields.hpp"
using LibRpBase::RomData;
using LibRpBase::RomDataTestObject;
using LibRpBase::RomFields;

// VectorFile for the test object
#include "librpfile/VectorFile.hpp"
using LibRpFile::VectorFile;
using LibRpFile::VectorFilePtr;

// for Q2U8()
#include "RpQt.hpp"
// for RP_KDE_UPPER
#include "RpQtNS.hpp"

// C++ STL classes
using std::array;
using std::unique_ptr;

namespace RomPropertiesKDE { namespace Tests {

class RomDataViewTest : public ::testing::Test
{
protected:
	RomDataViewTest()
		: m_vectorFile(std::make_shared<VectorFile>(VECTOR_FILE_SIZE))
	{ }

public:
	void SetUp() override;
	void TearDown() override;

protected:
	unique_ptr<RomDataTestObject> m_romData;

	// Dummy VectorFile with a 16 KiB buffer
	static constexpr size_t VECTOR_FILE_SIZE = 16U * 1024U;
	VectorFilePtr m_vectorFile;
};

void RomDataViewTest::SetUp()
{
	m_vectorFile->resize(VECTOR_FILE_SIZE);
	m_romData.reset(new RomDataTestObject(m_vectorFile));
}

void RomDataViewTest::TearDown()
{
	m_romData.reset();
	m_vectorFile->clear();
}

/**
 * Test RomDataView with a RomData object with an RFT_STRING field.
 */
TEST_F(RomDataViewTest, RFT_STRING)
{
	// TODO
}

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("KDE (" RP_KDE_UPPER ") UI frontend test suite: RomDataView tests.\n\n", stderr);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
