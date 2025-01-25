/***************************************************************************
 * ROM Properties Page shell extension. (kde/tests)                        *
 * ListDataSortProxyModelTest.cpp: ListDataSortProxyModel test.            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// RpQImageBackend
#include "RpQt.hpp"
#include "RpQImageBackend.hpp"
using LibRpTexture::rp_image;

// librpbase
#include "RomFields.hpp"
using LibRpBase::RomFields;

// ListDataModel
#include "ListDataModel.hpp"
#include "ListDataSortProxyModel.hpp"

// C includes (C++ namespace)
#include <cstdio>

// C++ STL classes
using std::string;
using std::vector;

namespace LibRomData { namespace Tests {

class ListDataSortProxyModelTest : public ::testing::Test
{
	protected:
		ListDataSortProxyModelTest()
			: field(nullptr)
			, listDataModel(nullptr)
			, proxyModel(nullptr)
		{
			// Register RpQImageBackend.
			rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);
		}

		~ListDataSortProxyModelTest() override
		{
			delete proxyModel;
			delete listDataModel;
			delete field;
		}

	public:
		void SetUp() override;
		void TearDown() override;

	protected:
		RomFields:: Field *field;
		ListDataModel *listDataModel;
		ListDataSortProxyModel *proxyModel;

	public:
		static const char *const sorted_strings_asc[4][25];
};

const char *const ListDataSortProxyModelTest::sorted_strings_asc[4][25] = {
	// Column 0: Greek alphabet, standard sort
	{NULL,
	 "Alpha", "Epsilon", "Eta", "Gamma",
	 "Iota", "Lambda", "Nu", "Omicron",
	 "Phi", "Psi", "Rho", "Tau",
	 "bEta", "cHi", "dElta", "kAppa",
	 "mU", "oMega", "pI", "sIgma",
	 "tHeta", "uPsilon", "xI", "zEta"},
	// Column 1: Greek alphabet, case-insensitive sort
	{NULL,
	 "Alpha", "bEta", "cHi", "dElta",
	 "Epsilon", "Eta", "Gamma", "Iota",
	 "kAppa", "Lambda", "mU", "Nu",
	 "oMega", "Omicron", "Phi", "pI",
	 "Psi", "Rho", "sIgma", "Tau",
	 "tHeta", "uPsilon", "xI", "zEta"},
	// Column 2: Numbers, standard sort
	{NULL,
	 "1", "10", "11", "12",
	 "13", "14", "15", "16",
	 "17", "18", "19", "2",
	 "20", "21", "22", "23",
	 "24", "3", "4", "5",
	 "6", "7", "8", "9"},
	// Column 3: Numbers, numeric sort
	{NULL,
	 "1", "2", "3", "4",
	 "5", "6", "7", "8",
	 "9", "10", "11", "12",
	 "13", "14", "15", "16",
	 "17", "18", "19", "20",
	 "21", "22", "23", "24"}
};

void ListDataSortProxyModelTest::SetUp()
{
	// Create a fake RFT_LISTDATA field.
	field = new RomFields::Field("LDSPMT", RomFields::RFT_LISTDATA, 0, 0);
	auto &listDataDesc = field->desc.list_data;
	listDataDesc.names = new vector<string>({"Col0", "Col1", "Col2", "Col3"});
	listDataDesc.rows_visible = 0;
	listDataDesc.col_attrs.align_headers = 0;
	listDataDesc.col_attrs.align_data = 0;
	listDataDesc.col_attrs.sizing = 0;
	listDataDesc.col_attrs.sorting = AFLD_ALIGN4(RomFields::COLSORT_STANDARD, RomFields::COLSORT_NOCASE, RomFields::COLSORT_STANDARD, RomFields::COLSORT_NUMERIC);
	listDataDesc.col_attrs.sort_col = -1;
	listDataDesc.col_attrs.sort_dir = RomFields::COLSORTORDER_ASCENDING;
	listDataDesc.col_attrs.is_timestamp = 0;
	listDataDesc.col_attrs.dtflags = static_cast<RomFields::DateTimeFlags>(0);

	// Add the actual list_data data.
	// - Column 0 and 1: Strings. 0 is standard sort, 1 is case-insensitive.
	// - Column 2 and 3: Numbers. 2 is standard sort, 3 is numeric sort.
	auto &listDataData = field->data.list_data;
	memset(&listDataData.mxd, 0, sizeof(listDataData.mxd));
	// NOTE: Outer vector is rows, not columns!
	// TODO: Add from the sorted data, then do a random sort?
	// NOTE: Using empty strings instead of nullptr due to std::string requirements.
	listDataData.data.single = new RomFields::ListData_t({
		{"pI", "tHeta", "2", "7"},
		{"cHi", "Iota", "15", "1"},
		{"uPsilon", "Alpha", "1", "22"},
		{"Psi", "mU", "14", "15"},
		{"xI", "Nu", "20", "16"},
		{"Gamma", "Phi", "", "12"},
		{"Epsilon", "Rho", "11", "23"},
		{"zEta", "pI", "5", "8"},
		{"Lambda", "Eta", "8", "5"},
		{"Nu", "bEta", "18", "19"},
		{"Iota", "Tau", "10", "13"},
		{"Eta", "", "13", "20"},
		{"kAppa", "Psi", "23", "9"},
		{"Omicron", "Gamma", "4", "18"},
		{"tHeta", "sIgma", "7", "4"},
		{"", "zEta", "3", "21"},
		{"sIgma", "Omicron", "21", "14"},
		{"mU", "oMega", "6", "24"},
		{"bEta", "Epsilon", "24", "11"},
		{"oMega", "cHi", "16", "6"},
		{"Tau", "xI", "19", "17"},
		{"Alpha", "uPsilon", "22", ""},
		{"Phi", "dElta", "12", "10"},
		{"Rho", "kAppa", "9", "3"},
		{"dElta", "Lambda", "17", "2"}
	});

	// Create a ListDataModel.
	listDataModel = new ListDataModel();
	listDataModel->setField(field);

	// Create a ListDataSortProxyModel.
	proxyModel = new ListDataSortProxyModel();
	proxyModel->setSortingMethods(listDataDesc.col_attrs.sorting);
	proxyModel->setSourceModel(listDataModel);
}

void ListDataSortProxyModelTest::TearDown()
{
	delete proxyModel;
	proxyModel = nullptr;

	delete listDataModel;
	listDataModel = nullptr;

	delete field;
	field = nullptr;
}

/**
 * Test sorting each column in ascending order.
 */
TEST_F(ListDataSortProxyModelTest, ascendingSort)
{
	const int columnCount = proxyModel->columnCount();
	const int rowCount = proxyModel->rowCount();
	ASSERT_GT(columnCount, 0) << "No columns available???";
	ASSERT_GT(rowCount, 0) << "No rows available???";

	for (int col = 0; col < columnCount; col++) {
		proxyModel->sort(col, Qt::AscendingOrder);

		for (int row = 0; row < rowCount; row++) {
			QModelIndex index = proxyModel->index(row, col);
			EXPECT_TRUE(index.isValid());

			QVariant data = proxyModel->data(index);
			EXPECT_TRUE(data.canConvert<QString>());

			QString str = data.toString();
			// NOTE: Allowing an empty QString to match a NULL pointer in sorted_strings_asc[].
			if (str.isEmpty() && !sorted_strings_asc[col][row]) {
				continue;
			}

			EXPECT_STREQ(sorted_strings_asc[col][row], str.toUtf8().constData()) << "sorting column " << col << ", checking row " << row;
		}
	}
}

/**
 * Test sorting each column in descending order.
 */
TEST_F(ListDataSortProxyModelTest, descendingSort)
{
	const int columnCount = proxyModel->columnCount();
	const int rowCount = proxyModel->rowCount();
	ASSERT_GT(columnCount, 0) << "No columns available???";
	ASSERT_GT(rowCount, 0) << "No rows available???";

	for (int col = 0; col < columnCount; col++) {
		proxyModel->sort(col, Qt::DescendingOrder);

		for (int row = 0; row < rowCount; row++) {
			QModelIndex index = proxyModel->index(row, col);
			EXPECT_TRUE(index.isValid());

			QVariant data = proxyModel->data(index);
			EXPECT_TRUE(data.canConvert<QString>());

			QString str = data.toString();
			const int drow = ARRAY_SIZE(sorted_strings_asc[row]) - row - 1;

			// NOTE: Allowing an empty QString to match a NULL pointer in sorted_strings_asc[].
			if (str.isEmpty() && !sorted_strings_asc[col][drow]) {
				continue;
			}

			EXPECT_STREQ(sorted_strings_asc[col][drow], str.toUtf8().constData()) << "sorting column " << col << ", checking row " << row;
		}
	}
}

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("KDE (" RP_KDE_UPPER ") UI frontend test suite: ListDataSortProxyModel tests.\n\n", stderr);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
