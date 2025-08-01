/***************************************************************************
 * ROM Properties Page shell extension. (gtk/tests)                        *
 * RomDataViewTest.cpp: RomDataView tests                                  *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Google Test
#include "gtest_init.hpp"

// RomDataTestObject
#include "librpbase/RomDataTestObject.hpp"
#include "librpbase/RomFields.hpp"
using LibRpBase::RomData;
using LibRpBase::RomDataTestObject;
using LibRpBase::RomDataTestObjectPtr;
using LibRpBase::RomFields;

// VectorFile for the test object
#include "librpfile/VectorFile.hpp"
using LibRpFile::VectorFile;
using LibRpFile::VectorFilePtr;

// GTK includes
#include <gtk/gtk.h>
#include "gtk-compat.h"

// RomDataView
#include "RomDataView.hpp"

// C++ STL classes
using std::array;
using std::string;

// libfmt
#include "rp-libfmt.h"

namespace RomPropertiesGTK { namespace Tests {

class RomDataViewTest : public ::testing::Test
{
protected:
	RomDataViewTest()
		: m_vectorFile(std::make_shared<VectorFile>(VECTOR_FILE_SIZE))
		, m_romDataView(nullptr)
#if !GTK_CHECK_VERSION(3, 89, 3)
		, m_widgetList(nullptr)
#endif /* !GTK_CHECK_VERSION(3, 89, 3) */
	{ }

	~RomDataViewTest()
	{
#if !GTK_CHECK_VERSION(3, 89, 3)
		g_list_free(m_widgetList);
#endif /* !GTK_CHECK_VERSION(3, 89, 3) */
		deleteRomDataView();
	}

public:
	void SetUp() override;
	void TearDown() override;

protected:
	RomDataTestObjectPtr m_romData;

	// Dummy VectorFile with a 16 KiB buffer
	static constexpr size_t VECTOR_FILE_SIZE = 16U * 1024U;
	VectorFilePtr m_vectorFile;

	// RomDataView
	GtkWidget *m_romDataView;

#if !GTK_CHECK_VERSION(3, 89, 3)
	// Temporary widget list when enumerating widgets.
	// NOTE: Must be here so it can be freed if an assertion occurs.
	GList *m_widgetList;
#endif /* !GTK_CHECK_VERSION(3, 89, 3) */

	inline void deleteRomDataView(void)
	{
#if GTK_CHECK_VERSION(3, 98, 4)
		g_clear_object(&m_romDataView);
#else /* !GTK_CHECK_VERSION(3, 98, 4) */
		if (m_romDataView) {
			gtk_widget_destroy(m_romDataView);
			m_romDataView = nullptr;
		}
#endif /* GTK_CHECK_VERSION(3, 98, 4) */
	}
};

void RomDataViewTest::SetUp()
{
	m_vectorFile->resize(VECTOR_FILE_SIZE);
	m_romData = std::make_shared<RomDataTestObject>(m_vectorFile);
}

void RomDataViewTest::TearDown()
{
	deleteRomDataView();
	m_romData.reset();
	m_vectorFile->clear();
}

/**
 * Test RomDataView with a RomData object with an RFT_STRING field.
 */
TEST_F(RomDataViewTest, RFT_STRING)
{
	// Add an RFT_STRING field.
	static const char s_field_desc[] = "RFT_STRING 0";
	static const char s_field_value[] = "Test string! omgwtflolbbq";

	RomFields *const fields = m_romData->getWritableFields();
	fields->addField_string(s_field_desc, s_field_value);

	/** Verify the GTK widgets. **/

	// Create a RomDataView.
	// TODO: Set description format type properly.
	m_romDataView = rp_rom_data_view_new_with_romData("", m_romData, RP_DFT_GNOME);
#if GTK_CHECK_VERSION(3, 98, 4)
	g_object_ref_sink(m_romDataView);
#endif /* GTK_CHECK_VERSION(3, 98, 4) */

	// NOTE: For efficiency reasons, GTK RomDataView uses g_idle_add()
	// to schedule its display update. Force it to run here.
	ASSERT_EQ(true, rp_rom_data_view_is_showing_data(RP_ROM_DATA_VIEW(m_romDataView)));

	// There shouldn't be any tabs.
	// First child widget of RomDataView is the header row: GtkBox (GTK3+) or GtkHBox (GTK2)
	// Second child widget of RomDataView is the GtkGrid (GTK3+) or GtkTable (GTK2)
#if GTK_CHECK_VERSION(3, 89, 3)
	// GTK4: Use first_child/next_sibling.
	GtkWidget *const hboxHeaderRow = gtk_widget_get_first_child(m_romDataView);
	ASSERT_TRUE(GTK_IS_BOX(hboxHeaderRow));

	GtkWidget *const tableTab0 = gtk_widget_get_next_sibling(hboxHeaderRow);
	ASSERT_TRUE(GTK_IS_GRID(tableTab0));
#else /* !GTK_CHECK_VERSION(3, 89, 3) */
	// GTK2, GTK3: Need to get the entire widget list and get the second entry.
	m_widgetList = gtk_container_get_children(GTK_CONTAINER(m_romDataView));
	ASSERT_NE(nullptr, m_widgetList);

	GList *widgetIter = g_list_first(m_widgetList);
	ASSERT_NE(nullptr, widgetIter);
	GtkWidget *const hboxHeaderRow = GTK_WIDGET(widgetIter->data);
#  if GTK_CHECK_VERSION(3, 0, 0)
	ASSERT_TRUE(GTK_IS_BOX(hboxHeaderRow));
#  else /* !GTK_CHECK_VERSION(3, 0, 0) */
	ASSERT_TRUE(GTK_IS_HBOX(hboxHeaderRow));
#  endif /* GTK_CHECK_VERSION(3, 0, 0) */

	widgetIter = g_list_next(m_widgetList);
	ASSERT_NE(nullptr, widgetIter);
	GtkWidget *const tableTab0 = GTK_WIDGET(widgetIter->data);
#  if GTK_CHECK_VERSION(3, 0, 0)
	ASSERT_TRUE(GTK_IS_GRID(tableTab0));
#  else /* !GTK_CHECK_VERSION(3, 0, 0) */
	ASSERT_TRUE(GTK_IS_TABLE(tableTab0));
#  endif /* GTK_CHECK_VERSION(3, 0, 0) */
#endif /* GTK_CHECK_VERSION(3, 89, 3) */

	// FIXME: GtkGrid doesn't have an easy way to get the total number of
	// rows and columns. GtkTable does...

#if GTK_CHECK_VERSION(3, 0, 0)
	// Get the widgets for the first row.
	GtkWidget *const lblDesc = gtk_grid_get_child_at(GTK_GRID(tableTab0), 0, 0);
	ASSERT_TRUE(GTK_IS_LABEL(lblDesc));
	GtkWidget *const lblValue = gtk_grid_get_child_at(GTK_GRID(tableTab0), 1, 0);
	ASSERT_TRUE(GTK_IS_LABEL(lblValue));
#else /* !GTK_CHECK_VERSION(3, 0, 0) */
	// TODO: GtkTable version.
	GtkWidget *const lblDesc = nullptr;
	GtkWidget *const lblValue = nullptr;
	ASSERT_TRUE(!"not implemented for GTK2 yet...");
#endif /* GTK_CHECK_VERSION(3, 0, 0) */

	// Verify the label contents.
	// NOTE: Description label will have an added ':'.
	string stds_field_desc = s_field_desc;
	stds_field_desc += ':';

	// NOTE: Using gtk_label_get_label(), which returns mnemonics and Pango markup.
	EXPECT_STREQ(stds_field_desc.c_str(), gtk_label_get_label(GTK_LABEL(lblDesc))) << "Field description is incorrect.";
	EXPECT_STREQ(s_field_value, gtk_label_get_label(GTK_LABEL(lblValue))) << "Field value is incorrect.";
}

} }

#ifdef HAVE_SECCOMP
const unsigned int rp_gtest_syscall_set = RP_GTEST_SYSCALL_SET_GTK;
#endif /* HAVE_SECCOMP */

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fmt::print(stderr, FSTR("GTK{:d} UI frontend test suite: RomDataView tests.\n\n"), GTK_MAJOR_VERSION);
	fflush(nullptr);

#if !GLIB_CHECK_VERSION(2, 35, 1)
        // g_type_init() is automatic as of glib-2.35.1
        // and is marked deprecated.
        g_type_init();
#endif /* !GLIB_CHECK_VERSION(2, 35, 1) */
#if !GLIB_CHECK_VERSION(2, 32, 0)
        // g_thread_init() is automatic as of glib-2.32.0
        // and is marked deprecated.
        if (!g_thread_supported()) {
                g_thread_init(NULL);
        }
#endif /* !GLIB_CHECK_VERSION(2, 32, 0) */

	// gtk_init()'s parameters were dropped in GTK 3.89.4.
	// NOTE: GTK+ 3.24.19 was an incorrect tag from the GTK4 dev branch,
	// so it also lacks parameters in gtk_init(). Release files from
	// GTK+ 3.24.19 are missing from the official servers...
#if GTK_CHECK_VERSION(3, 89, 4)
	gtk_init();
#else /* !GTK_CHECK_VERSION(3, 89, 4) */
	gtk_init(nullptr, nullptr);
#endif /* GTK_CHECK_VERSION(3, 89, 4) */
	// TODO: Add the GTK version to the program name?
	g_set_prgname("com.gerbilsoft.rom-properties.RomDataViewTest_gtk");

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
