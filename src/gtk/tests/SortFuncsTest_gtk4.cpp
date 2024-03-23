/***************************************************************************
 * ROM Properties Page shell extension. (gtk/tests)                        *
 * SortFuncsTest_gtk4.cpp: sort_funcs.c test (GTK4)                        *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest/gtest.h"
#include "tcharx.h"

// GTK
#include "gtk-compat.h"

// sort_funcs.h
#include "ListDataItem.h"
#include "gtk4/sort_funcs.h"

// for ARRAY_SIZE()
#include "common.h"

// C includes (C++ namespace)
#include <cstdio>

// C++ STL classes
using std::string;

// Test data
#include "SortFuncsTest_data.h"

namespace LibRomData { namespace Tests {

class ListDataSortProxyModelTest : public ::testing::Test
{
	protected:
		ListDataSortProxyModelTest()
			: listStore(nullptr)
			, sortProxy(nullptr)
		{
			memset(sorter, 0, sizeof(sorter));
		}

		~ListDataSortProxyModelTest() override
		{
			// GtkSortListModel takes ownership of listStore,
			// so listStore will get unref()'d when sortProxy does.
			g_clear_object(&sortProxy);

			for (GtkCustomSorter *&p : sorter) {
				g_clear_object(&p);
			}
		}

	public:
		void SetUp() override;
		void TearDown() override;

	protected:
		GListStore *listStore;		// List data
		GtkSortListModel *sortProxy;	// Sort proxy
		GtkCustomSorter *sorter[4];
};

void ListDataSortProxyModelTest::SetUp()
{
	// Create the GListStore and sort proxy tree models.
	// ListDataItem will be used so we can effectively test sort_funcs.c.
	// NOTE: GtkSortListModel takes ownership of listStore,
	// so listStore will get unref()'d when sortProxy does.
	listStore = g_list_store_new(RP_TYPE_LIST_DATA_ITEM);
	sortProxy = gtk_sort_list_model_new(G_LIST_MODEL(listStore), nullptr);

	// Add the "randomized" list data.
	// NOTE: Outer vector is rows, not columns!
	// TODO: Add from the sorted data, then do a random sort?
	for (auto *p : list_data_randomized) {
		RpListDataItem *const item = rp_list_data_item_new(4, RP_LIST_DATA_ITEM_COL0_TYPE_TEXT);
		rp_list_data_item_set_column_text(item, 0, p[0]);
		rp_list_data_item_set_column_text(item, 1, p[1]);
		rp_list_data_item_set_column_text(item, 2, p[2]);
		rp_list_data_item_set_column_text(item, 3, p[3]);
		g_list_store_append(listStore, item);
		g_object_unref(item);
	}

	// Sorting order (function pointers)
	static const GCompareDataFunc sort_funcs[4] = {
		// Column 0: Greek alphabet, standard sort
		rp_sort_RFT_LISTDATA_standard,
		// Column 1: Greek alphabet, case-insensitive sort
		rp_sort_RFT_LISTDATA_nocase,
		// Column 2: Numbers, standard sort
		rp_sort_RFT_LISTDATA_standard,
		// Column 3: Numbers, numeric sort
		rp_sort_RFT_LISTDATA_numeric,
	};

	// Set the column sort functions.
	for (gint col = 0; col < 4; col++) {
		sorter[col] = gtk_custom_sorter_new(sort_funcs[col], GINT_TO_POINTER(col), nullptr);
	}
}

void ListDataSortProxyModelTest::TearDown()
{
	// GtkSortListModel takes ownership of listStore,
	// so listStore will get unref()'d when sortProxy does.
	g_clear_object(&sortProxy);

	for (GtkCustomSorter *&p : sorter) {
		g_clear_object(&p);
	}
}

/**
 * Test sorting each column in ascending order.
 */
TEST_F(ListDataSortProxyModelTest, ascendingSort)
{
	static const int columnCount = 4;
	const int rowCount = g_list_model_get_n_items(G_LIST_MODEL(listStore));
	ASSERT_GT(rowCount, 0) << "No rows available???";
	ASSERT_EQ(rowCount, g_list_model_get_n_items(G_LIST_MODEL(sortProxy))) << "Sort proxy row count doesn't match the original model!";

	for (int col = 0; col < columnCount; col++) {
		// FIXME: Ascending vs. descending?
		gtk_sort_list_model_set_sorter(GTK_SORT_LIST_MODEL(sortProxy), GTK_SORTER(sorter[col]));

		int row;
		for (row = 0; row < rowCount; row++) {
			RpListDataItem *const item = RP_LIST_DATA_ITEM(g_list_model_get_item(G_LIST_MODEL(sortProxy), row));
			EXPECT_NE(item, nullptr) << "Unexpected NULL item received";
			if (!item)
				continue;

			// Get the string from this column.
			const char *str = rp_list_data_item_get_column_text(item, col);
			EXPECT_NE(str, nullptr) << "Unexpected NULL string pointer";
			if (str) {
				EXPECT_NE(str[0], '\0') << "Unexpected empty string";

				const int drow = ARRAY_SIZE(sorted_strings_asc[row]) - row - 1;
				EXPECT_STREQ(sorted_strings_asc[col][drow], str) << "sorting column " << col << ", checking row " << row;
			}
		};

		ASSERT_EQ(rowCount, row) << "Row count does not match the number of rows received";
	}
}

/**
 * Test sorting each column in descending order.
 */
TEST_F(ListDataSortProxyModelTest, descendingSort)
{
	static const int columnCount = 4;
	const int rowCount = g_list_model_get_n_items(G_LIST_MODEL(listStore));
	ASSERT_GT(rowCount, 0) << "No rows available???";
	ASSERT_EQ(rowCount, g_list_model_get_n_items(G_LIST_MODEL(sortProxy))) << "Sort proxy row count doesn't match the original model!";

	for (int col = 0; col < columnCount; col++) {
		// FIXME: Ascending vs. descending?
		gtk_sort_list_model_set_sorter(GTK_SORT_LIST_MODEL(sortProxy), GTK_SORTER(sorter[col]));

		int row;
		for (row = 0; row < rowCount; row++) {
			RpListDataItem *const item = RP_LIST_DATA_ITEM(g_list_model_get_item(G_LIST_MODEL(sortProxy), row));
			EXPECT_NE(item, nullptr) << "Unexpected NULL item received";
			if (!item)
				continue;

			// Get the string from this column.
			const char *str = rp_list_data_item_get_column_text(item, col);
			EXPECT_NE(str, nullptr) << "Unexpected NULL string pointer";
			if (str) {
				EXPECT_NE(str[0], '\0') << "Unexpected empty string";
				EXPECT_STREQ(sorted_strings_asc[col][row], str) << "sorting column " << col << ", checking row " << row;
			}
		};

		ASSERT_EQ(rowCount, row) << "Row count does not match the number of rows received";
	}
}

} }

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fprintf(stderr, "GTK%d UI frontend test suite: ListDataSortProxyModel tests.\n\n", GTK_MAJOR_VERSION);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
