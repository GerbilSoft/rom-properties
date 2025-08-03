/***************************************************************************
 * ROM Properties Page shell extension. (gtk/tests)                        *
 * SortFuncsTest_gtk3.cpp: sort_funcs.c test (GTK2/GTK3)                   *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest_init.hpp"

// GTK
#include "gtk-compat.h"

// sort_funcs.h
// TODO: Test the GTK4 sorting functions.
#include "gtk3/sort_funcs.h"

// for ARRAY_SIZE()
#include "common.h"

// C includes (C++ namespace)
#include <cstdio>

// C++ STL classes
using std::array;
using std::string;

// libfmt
#include "rp-libfmt.h"

// Test data
#include "SortFuncsTest_data.h"

namespace RomPropertiesGTK { namespace Tests {

class SortFuncsTest_gtk3 : public ::testing::Test
{
	protected:
		SortFuncsTest_gtk3()
			: listStore(nullptr)
			, sortProxy(nullptr)
		{ }

		~SortFuncsTest_gtk3() override
		{
			g_clear_object(&sortProxy);
			g_clear_object(&listStore);
		}

	public:
		void SetUp() override;
		void TearDown() override;

	protected:
		GtkListStore *listStore;	// List data
		GtkTreeModel *sortProxy;	// Sort proxy
};

void SortFuncsTest_gtk3::SetUp()
{
	// Create the GtkListStore and sort proxy tree models.
	listStore = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	sortProxy = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(listStore));

	// Add the "randomized" list data.
	// NOTE: Outer vector is rows, not columns!
	// TODO: Add from the sorted data, then do a random sort?
	for (const auto *const p : list_data_randomized) {
		GtkTreeIter treeIter;
		gtk_list_store_append(listStore, &treeIter);
		gtk_list_store_set(listStore, &treeIter, 0, p[0], 1, p[1], 2, p[2], 3, p[3], -1);
	}

	// Sorting order (function pointers)
	static constexpr array<GtkTreeIterCompareFunc, 4> sort_funcs = {{
		// Column 0: Greek alphabet, standard sort
		rp_sort_RFT_LISTDATA_standard,
		// Column 1: Greek alphabet, case-insensitive sort
		rp_sort_RFT_LISTDATA_nocase,
		// Column 2: Numbers, standard sort
		rp_sort_RFT_LISTDATA_standard,
		// Column 3: Numbers, numeric sort
		rp_sort_RFT_LISTDATA_numeric,
	}};

	// Set the column sort functions.
	for (gint col = 0; col < 4; col++) {
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sortProxy),
			col, sort_funcs[col], GINT_TO_POINTER(col), nullptr);
	}
}

void SortFuncsTest_gtk3::TearDown()
{
	g_clear_object(&sortProxy);
	g_clear_object(&listStore);
}

/**
 * Test sorting each column in ascending order.
 */
TEST_F(SortFuncsTest_gtk3, ascendingSort)
{
	static constexpr int columnCount = 4;
	const int rowCount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(listStore), nullptr);
	ASSERT_GT(rowCount, 0) << "No rows available???";

	for (int col = 0; col < columnCount; col++) {
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortProxy), col, GTK_SORT_ASCENDING);
		int row = 0;

		GtkTreeIter iter;
		gboolean ok = gtk_tree_model_get_iter_first(sortProxy, &iter);
		ASSERT_TRUE(ok);
		do {
			gchar *str = nullptr;
			gtk_tree_model_get(sortProxy, &iter, col, &str, -1);
			EXPECT_STREQ(sorted_strings_asc[col][row], str) << "sorting column " << col << ", checking row " << row;
			g_free(str);

			// Next row.
			row++;
			ok = gtk_tree_model_iter_next(sortProxy, &iter);
		} while (ok);

		ASSERT_EQ(rowCount, row) << "Row count does not match the number of rows received";
	}
}

/**
 * Test sorting each column in descending order.
 */
TEST_F(SortFuncsTest_gtk3, descendingSort)
{
	static constexpr int columnCount = 4;
	const int rowCount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(listStore), nullptr);
	ASSERT_GT(rowCount, 0) << "No rows available???";

	for (int col = 0; col < columnCount; col++) {
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortProxy), col, GTK_SORT_DESCENDING);
		int row = 0;

		GtkTreeIter iter;
		gboolean ok = gtk_tree_model_get_iter_first(sortProxy, &iter);
		ASSERT_TRUE(ok);
		do {
			gchar *str = nullptr;
			gtk_tree_model_get(sortProxy, &iter, col, &str, -1);
			const int drow = ARRAY_SIZE(sorted_strings_asc[row]) - row - 1;
			EXPECT_STREQ(sorted_strings_asc[col][drow], str) << "sorting column " << col << ", checking row " << row;
			g_free(str);

			// Next row.
			row++;
			ok = gtk_tree_model_iter_next(sortProxy, &iter);
		} while (ok);

		ASSERT_EQ(rowCount, row) << "Row count does not match the number of rows received";
	}
}

} }

#ifdef HAVE_SECCOMP
const unsigned int rp_gtest_syscall_set = 0;
#endif /* HAVE_SECCOMP */

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fmt::print(stderr, FSTR("GTK{:d} UI frontend test suite: SortFuncs tests.\n\n"), GTK_MAJOR_VERSION);
	fflush(nullptr);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
