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
#include <unordered_map>
using std::array;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

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
		, m_lblDesc(nullptr)
		, m_widgetValue(nullptr)
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

#ifndef USE_GTK_GRID
	/**
	 * Convert row/column into a 32-bit value.
	 * - LOWORD == column/left
	 * - HIWORD == row/top
	 * NOTE: Parameter ordering matches gtk_grid_get_child_at().
	 * @param column
	 * @param row
	 * @return 32-bit value
	 */
	static constexpr inline uint32_t rowColumnToDWORD(uint16_t column, uint16_t row)
	{
		return (row << 16) | column;
	}

	/**
	 * Get all widgets from a GtkTable in a way that can be looked up easily.
	 * @param table GtkTable
	 * @return Map of widgets, with uint32_t key: LOWORD == column, HIWORD == row
	 */
	unordered_map<uint32_t, GtkWidget*> gtk_table_get_widgets(GtkTable *table);
#endif /* USE_GTK_GRID */

	/**
	 * Get the widgets from a row in RomDataView.
	 * Widgets will be returned in m_lblDesc and m_widgetValue.
	 *
	 * NOTE: Cannot return a value from this function due to
	 * how Google Test functions.
	 *
	 * @param romDataView RpRomDataView
	 * @param row Row number (starting at 0)
	 */
	void getRowWidgets(RpRomDataView *romDataView, int row = 0);

	GtkWidget *m_lblDesc;
	GtkWidget *m_widgetValue;
};

void RomDataViewTest::SetUp()
{
	m_vectorFile->resize(VECTOR_FILE_SIZE);
	m_romData = std::make_shared<RomDataTestObject>(m_vectorFile);

	// May be used by getRowWidgets().
	m_lblDesc = nullptr;
	m_widgetValue = nullptr;
}

void RomDataViewTest::TearDown()
{
	deleteRomDataView();
	m_romData.reset();
	m_vectorFile->clear();
}

#ifndef USE_GTK_GRID
/**
 * Get all widgets from a GtkTable in a way that can be looked up easily.
 * @param table GtkTable
 * @return Map of widgets, with uint32_t key: LOWORD == column, HIWORD == row
 */
unordered_map<uint32_t, GtkWidget*> RomDataViewTest::gtk_table_get_widgets(GtkTable *table)
{
	unordered_map<uint32_t, GtkWidget*> ret;
	g_return_val_if_fail(GTK_IS_TABLE(table), ret);

	GtkContainer *const container = GTK_CONTAINER(table);
	GList *const widgets = gtk_container_get_children(container);
	for (GList *p = widgets; p != nullptr; p = g_list_next(p)) {
		// NOTE: Assuming each widget occupies exactly 1 cell.
		GtkWidget *const widget = GTK_WIDGET(p->data);
		if (!widget) {
			continue;
		}

		guint left = 0, top = 0;
		gtk_container_child_get(container, widget, "left-attach", &left, "top-attach", &top, nullptr);
		ret.emplace(rowColumnToDWORD(left, top), widget);
	}

	g_list_free(widgets);
	return ret;
}
#endif /* USE_GTK_GRID */

/**
 * Get the widgets from a row in RomDataView.
 * Widgets will be returned in m_lblDesc and m_widgetValue.
 *
 * NOTE: Cannot return a value from this function due to
 * how Google Test functions.
 *
 * @param romDataView RpRomDataView
 * @param row Row number (starting at 0)
 */
void RomDataViewTest::getRowWidgets(RpRomDataView *romDataView, int row)
{
	// Initialize the widgets to nullptr before doing anything else.
	m_lblDesc = nullptr;
	m_widgetValue = nullptr;

	// There shouldn't be any tabs.
	// First child widget of RomDataView is the header row: GtkBox (GTK3+) or GtkHBox (GTK2)
	// Second child widget of RomDataView is the GtkGrid (GTK3+) or GtkTable (GTK2)
#if GTK_CHECK_VERSION(3, 89, 3)
	// GTK4: Use first_child/next_sibling.
	GtkWidget *const hboxHeaderRow = gtk_widget_get_first_child(GTK_WIDGET(romDataView));
	ASSERT_TRUE(GTK_IS_BOX(hboxHeaderRow));

	GtkWidget *const tableTab0 = gtk_widget_get_next_sibling(hboxHeaderRow);
	ASSERT_TRUE(GTK_IS_GRID(tableTab0));
#else /* !GTK_CHECK_VERSION(3, 89, 3) */
	// GTK2, GTK3: Need to get the entire widget list and get the second entry.
	m_widgetList = gtk_container_get_children(GTK_CONTAINER(romDataView));
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
	// FIXME: GtkGrid doesn't have an easy way to get the total number of
	// rows and columns. GtkTable does...
	ASSERT_TRUE(GTK_IS_GRID(tableTab0));
#  else /* !GTK_CHECK_VERSION(3, 0, 0) */
	ASSERT_TRUE(GTK_IS_TABLE(tableTab0));

	// Verify the number of rows and columns in GtkTable.
	guint table_rows = 0, table_columns = 0;
	gtk_table_get_size(GTK_TABLE(tableTab0), &table_rows, &table_columns);
	EXPECT_EQ(2, table_columns) << "Main table has the wrong number of columns.";
	EXPECT_EQ(1, table_rows) << "Main table has the wrong number of rows.";
#  endif /* GTK_CHECK_VERSION(3, 0, 0) */
#endif /* GTK_CHECK_VERSION(3, 89, 3) */

#ifdef USE_GTK_GRID
	// Get the widgets for the first row.
	m_lblDesc = gtk_grid_get_child_at(GTK_GRID(tableTab0), 0, row);
	ASSERT_TRUE(GTK_IS_LABEL(m_lblDesc));
	m_widgetValue = gtk_grid_get_child_at(GTK_GRID(tableTab0), 1, row);
	ASSERT_TRUE(GTK_IS_WIDGET(m_widgetValue));
#else /* !USE_GTK_GRID */
	unordered_map<uint32_t, GtkWidget*> mapWidgets = gtk_table_get_widgets(GTK_TABLE(tableTab0));
	ASSERT_FALSE(mapWidgets.empty());

	auto iter = mapWidgets.find(rowColumnToDWORD(0, row));
	ASSERT_TRUE(iter != mapWidgets.end());
	m_lblDesc = iter->second;
	ASSERT_TRUE(GTK_IS_LABEL(m_lblDesc));

	iter = mapWidgets.find(rowColumnToDWORD(1, row));
	ASSERT_TRUE(iter != mapWidgets.end());
	m_widgetValue = iter->second;
	ASSERT_TRUE(GTK_IS_WIDGET(m_widgetValue));
#endif /* USE_GTK_GRID */
}

/**
 * Test RomDataView with no RomData object.
 */
TEST_F(RomDataViewTest, NoRomData)
{
	// Create a RomDataView.
	// TODO: Set description format type properly.
	m_romDataView = rp_rom_data_view_new_with_romData("", nullptr, RP_DFT_GNOME);
#if GTK_CHECK_VERSION(3, 98, 4)
	g_object_ref_sink(m_romDataView);
#endif /* GTK_CHECK_VERSION(3, 98, 4) */

	// NOTE: For efficiency reasons, GTK RomDataView uses g_idle_add()
	// to schedule its display update. Force it to run here.
	// FIXME: With an empty RomData object, rp_rom_data_view_is_showing_data()
	// doesn't function correctly. It needs to call rp_rom_data_view_load_rom_data().
	ASSERT_FALSE(rp_rom_data_view_is_showing_data(RP_ROM_DATA_VIEW(m_romDataView)));

	// Check the child widgets.
	// There should be a single GtkBox (GtkHBox on GTK2) for the header row.
#if GTK_CHECK_VERSION(4, 0, 0)
	GtkWidget *child = gtk_widget_get_first_child(m_romDataView);
	ASSERT_NE(nullptr, child);
	EXPECT_TRUE(GTK_IS_BOX(child));
	child = gtk_widget_get_next_sibling(child);
	ASSERT_EQ(nullptr, child);
#else /* !GTK_CHECK_VERSION(4, 0, 0) */
	GList *lstChildren = gtk_container_get_children(GTK_CONTAINER(m_romDataView));
	ASSERT_NE(nullptr, lstChildren);
	EXPECT_NE(nullptr, lstChildren->data);
	EXPECT_EQ(nullptr, g_list_next(lstChildren));
	if (lstChildren->data) {
#  if GTK_CHECK_VERSION(3, 0, 0)
		EXPECT_TRUE(GTK_IS_BOX(lstChildren->data));
#  else /* !GTK_CHECK_VERSION(3, 0, 0) */
		EXPECT_TRUE(GTK_IS_HBOX(lstChildren->data));
#  endif /* GTK_CHECK_VERSION(3, 0, 0) */
	}
	g_list_free(lstChildren);
#endif /* GTK_CHECK_VERSION(4, 0, 0) */
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
	ASSERT_TRUE(rp_rom_data_view_is_showing_data(RP_ROM_DATA_VIEW(m_romDataView)));

	// Get the widgets from the first row.
	// Widgets will be stored in m_lblDesc and m_widgetValue.
	ASSERT_NO_FATAL_FAILURE(getRowWidgets(RP_ROM_DATA_VIEW(m_romDataView)));
	ASSERT_TRUE(GTK_IS_LABEL(m_widgetValue));

	// Verify the label contents.
	// NOTE: Description label will have an added ':'.
	string stds_field_desc = s_field_desc;
	stds_field_desc += ':';

	// NOTE: Using gtk_label_get_label(), which returns mnemonics and Pango markup.
	EXPECT_STREQ(stds_field_desc.c_str(), gtk_label_get_label(GTK_LABEL(m_lblDesc))) << "Field description is incorrect.";
	EXPECT_STREQ(s_field_value, gtk_label_get_label(GTK_LABEL(m_widgetValue))) << "Field value is incorrect.";
}

/**
 * Test RomDataView with a RomData object with an RFT_BITFIELD field.
 * Non-sparse: Bitfield has 16 contiguous bits.
 */
TEST_F(RomDataViewTest, RFT_BITFIELD_non_sparse)
{
	// Add an RFT_BITFIELD field.
	static const char s_field_desc[] = "RFT_BITFIELD 0";

	static const array<const char*, 16> bitfield_names = {{
		"bit 0", "bit 1", "bit 2", "bit 3",
		"bit 4", "bit 5", "bit 6", "bit 7",
		"bit 8", "bit 9", "bit 10", "bit 11",
		"bit 12", "bit 13", "bit 14", "bit 15",
	}};
	static constexpr uint32_t bitfield_value = 0xAA55;

	vector<string> *const v_bitfield_names = RomFields::strArrayToVector(bitfield_names);

	RomFields *const fields = m_romData->getWritableFields();
	fields->addField_bitfield(s_field_desc, v_bitfield_names, 4, bitfield_value);

	/** Verify the GTK widgets. **/

	// Create a RomDataView.
	// TODO: Set description format type properly.
	m_romDataView = rp_rom_data_view_new_with_romData("", m_romData, RP_DFT_GNOME);
#if GTK_CHECK_VERSION(3, 98, 4)
	g_object_ref_sink(m_romDataView);
#endif /* GTK_CHECK_VERSION(3, 98, 4) */

	// NOTE: For efficiency reasons, GTK RomDataView uses g_idle_add()
	// to schedule its display update. Force it to run here.
	ASSERT_TRUE(rp_rom_data_view_is_showing_data(RP_ROM_DATA_VIEW(m_romDataView)));

	// Get the widgets from the first row.
	// Widgets will be stored in m_lblDesc and m_widgetValue.
	ASSERT_NO_FATAL_FAILURE(getRowWidgets(RP_ROM_DATA_VIEW(m_romDataView)));
	GtkWidget *const gridBitfield = m_widgetValue;
#ifdef USE_GTK_GRID
	ASSERT_TRUE(GTK_IS_GRID(gridBitfield));
#else /* !USE_GTK_GRID */
	ASSERT_TRUE(GTK_IS_TABLE(gridBitfield));
#endif /* USE_GTK_GRID */

	// Verify the label contents.
	// NOTE: Description label will have an added ':'.
	string stds_field_desc = s_field_desc;
	stds_field_desc += ':';

	// NOTE: Using gtk_label_get_label(), which returns mnemonics and Pango markup.
	EXPECT_STREQ(stds_field_desc.c_str(), gtk_label_get_label(GTK_LABEL(m_lblDesc))) << "Field description is incorrect.";

	// Grid should be 4x4, since we specified 4 items per column,
	// and we have 16 items.
	guint rowCount = 0, columnCount = 0;
#ifdef USE_GTK_GRID
	// FIXME: GtkGrid doesn't have an easy way to get the total number of
	// rows and columns. GtkTable does...

	// Assuming we have the correct number.
	rowCount = 4;
	columnCount = 4;
#else /* !USE_GTK_GRID */
	gtk_table_get_size(GTK_TABLE(gridBitfield), &rowCount, &columnCount);
	EXPECT_EQ(4, columnCount) << "Bitfield table has the wrong number of columns.";
	EXPECT_EQ(4, rowCount) << "Bitfield table has the wrong number of rows.";

	// Get the widgets.
	unordered_map<uint32_t, GtkWidget*> mapWidgets = gtk_table_get_widgets(GTK_TABLE(gridBitfield));
	ASSERT_FALSE(mapWidgets.empty());
#endif /* USE_GTK_GRID */

	// Go through each item.
	unsigned int bit = 0;
	unsigned int row = 0, col = 0;
	for (; row < rowCount && bit < bitfield_names.size(); bit++) {
		const char *const name = bitfield_names[bit];
		EXPECT_NE(nullptr, name);
		if (!name) {
			// nullptr description.
			// Continue without incrementing row or col.
			continue;
		}

#ifdef USE_GTK_GRID
		GtkWidget *const checkBox = gtk_grid_get_child_at(GTK_GRID(gridBitfield), col, row);
#else /* !USE_GTK_GRID */
		auto iter = mapWidgets.find(rowColumnToDWORD(col, row));
		EXPECT_TRUE(iter != mapWidgets.end());
		if (iter == mapWidgets.end()) {
			continue;
		}
		GtkWidget *const checkBox = iter->second;
#endif /* USE_GTK_GRID */
		EXPECT_TRUE(GTK_IS_CHECK_BUTTON(checkBox));
		if (!GTK_IS_CHECK_BUTTON(checkBox)) {
			continue;
		}

		// NOTE: gtk_check_button_get_label() wasn't added until GTK4.
		// We'll have to get the label property manually.
		gchar *label = nullptr;
		g_object_get(checkBox, "label", &label, nullptr);

		// Verify the checkbox's label.
		EXPECT_STREQ(name, label) << "GtkCheckButton " << bit << " label is incorrect.";
		g_free(label);

		// Verify the checkbox's value.
		EXPECT_EQ(!!(bitfield_value & (1U << bit)), gtk_check_button_get_active(GTK_CHECK_BUTTON(checkBox))) << "GtkCheckButton " << bit << " value is incorrect.";

		// Next column.
		col++;
		if (col >= columnCount) {
			col = 0;
			row++;
		}
	}

	// Make sure we've processed all of the bits.
	EXPECT_EQ(bit, bitfield_names.size()) << "Incorrect number of bits processed.";
}

/**
 * Test RomDataView with a RomData object with an RFT_BITFIELD field.
 * Sparse: Bitfield has non-contiguous bits.
 */
TEST_F(RomDataViewTest, RFT_BITFIELD_sparse)
{
	// Add an RFT_BITFIELD field.
	static const char s_field_desc[] = "RFT_BITFIELD 0";

	// NOTE: 5 bits are missing.
	// This results in only 3 rows.
	static const array<const char*, 16> bitfield_names = {{
		"bit 0", "bit 1", nullptr, "bit 3",
		nullptr, "bit 5", "bit 6", nullptr,
		"bit 8", nullptr, "bit 10", "bit 11",
		"bit 12", nullptr, "bit 14", "bit 15",
	}};
	static constexpr uint32_t bitfield_value = 0xAA55;

	vector<string> *const v_bitfield_names = RomFields::strArrayToVector(bitfield_names);

	RomFields *const fields = m_romData->getWritableFields();
	fields->addField_bitfield(s_field_desc, v_bitfield_names, 4, bitfield_value);

	/** Verify the GTK widgets. **/

	// Create a RomDataView.
	// TODO: Set description format type properly.
	m_romDataView = rp_rom_data_view_new_with_romData("", m_romData, RP_DFT_GNOME);
#if GTK_CHECK_VERSION(3, 98, 4)
	g_object_ref_sink(m_romDataView);
#endif /* GTK_CHECK_VERSION(3, 98, 4) */

	// NOTE: For efficiency reasons, GTK RomDataView uses g_idle_add()
	// to schedule its display update. Force it to run here.
	ASSERT_TRUE(rp_rom_data_view_is_showing_data(RP_ROM_DATA_VIEW(m_romDataView)));

	// Get the widgets from the first row.
	// Widgets will be stored in m_lblDesc and m_widgetValue.
	ASSERT_NO_FATAL_FAILURE(getRowWidgets(RP_ROM_DATA_VIEW(m_romDataView)));
	GtkWidget *const gridBitfield = m_widgetValue;
#ifdef USE_GTK_GRID
	ASSERT_TRUE(GTK_IS_GRID(gridBitfield));
#else /* !USE_GTK_GRID */
	ASSERT_TRUE(GTK_IS_TABLE(gridBitfield));
#endif /* USE_GTK_GRID */

	// Verify the label contents.
	// NOTE: Description label will have an added ':'.
	string stds_field_desc = s_field_desc;
	stds_field_desc += ':';

	// NOTE: Using gtk_label_get_label(), which returns mnemonics and Pango markup.
	EXPECT_STREQ(stds_field_desc.c_str(), gtk_label_get_label(GTK_LABEL(m_lblDesc))) << "Field description is incorrect.";

	// Grid should be 4x4, since we specified 4 items per column,
	// and we have 16 items.
	guint rowCount = 0, columnCount = 0;
#ifdef USE_GTK_GRID
	// FIXME: GtkGrid doesn't have an easy way to get the total number of
	// rows and columns. GtkTable does...

	// Assuming we have the correct number.
	rowCount = 3;
	columnCount = 4;
#else /* !USE_GTK_GRID */
	gtk_table_get_size(GTK_TABLE(gridBitfield), &rowCount, &columnCount);
	EXPECT_EQ(4, columnCount) << "Bitfield table has the wrong number of columns.";
	// FIXME: GtkTable is initialized with the *maximum* number of rows...
	EXPECT_EQ(/*3*/ 4, rowCount) << "Bitfield table has the wrong number of rows.";

	// Get the widgets.
	unordered_map<uint32_t, GtkWidget*> mapWidgets = gtk_table_get_widgets(GTK_TABLE(gridBitfield));
	ASSERT_FALSE(mapWidgets.empty());
#endif /* USE_GTK_GRID */

	// Go through each item.
	unsigned int bit = 0;
	unsigned int row = 0, col = 0;
	for (; row < rowCount && bit < bitfield_names.size(); bit++) {
		const char *const name = bitfield_names[bit];
		if (!name) {
			// nullptr description.
			// Continue without incrementing row or col.
			continue;
		}

#ifdef USE_GTK_GRID
		GtkWidget *const checkBox = gtk_grid_get_child_at(GTK_GRID(gridBitfield), col, row);
#else /* !USE_GTK_GRID */
		auto iter = mapWidgets.find(rowColumnToDWORD(col, row));
		EXPECT_TRUE(iter != mapWidgets.end());
		if (iter == mapWidgets.end()) {
			continue;
		}
		GtkWidget *const checkBox = iter->second;
#endif /* USE_GTK_GRID */
		EXPECT_TRUE(GTK_IS_CHECK_BUTTON(checkBox));
		if (!GTK_IS_CHECK_BUTTON(checkBox)) {
			continue;
		}

		// NOTE: gtk_check_button_get_label() wasn't added until GTK4.
		// We'll have to get the label property manually.
		gchar *label = nullptr;
		g_object_get(checkBox, "label", &label, nullptr);

		// Verify the checkbox's label.
		EXPECT_STREQ(name, label) << "GtkCheckButton " << bit << " label is incorrect.";
		g_free(label);

		// Verify the checkbox's value.
		EXPECT_EQ(!!(bitfield_value & (1U << bit)), gtk_check_button_get_active(GTK_CHECK_BUTTON(checkBox))) << "GtkCheckButton " << bit << " value is incorrect.";

		// Next column.
		col++;
		if (col >= columnCount) {
			col = 0;
			row++;
		}
	}

	// Verify that the remaining grid cells are empty.
	for (; row < rowCount; row++) {
		for (; col < columnCount; col++) {
#ifdef USE_GTK_GRID
			GtkWidget *const checkBox = gtk_grid_get_child_at(GTK_GRID(gridBitfield), col, row);
			EXPECT_EQ(nullptr, checkBox);
#else /* !USE_GTK_GRID */
			auto iter = mapWidgets.find(rowColumnToDWORD(col, row));
			EXPECT_TRUE(iter == mapWidgets.end());
#endif /* USE_GTK_GRID */
		}

		// Next row.
		col = 0;
	}

	// Make sure we've processed all of the bits.
	EXPECT_EQ(bit, bitfield_names.size()) << "Incorrect number of bits processed.";
}

/**
 * Test RomDataView with a RomData object with an RFT_DATETIME field.
 */
TEST_F(RomDataViewTest, RFT_DATETIME)
{
	// Add an RFT_DATETIME field.
	static const char s_field_desc[] = "RFT_STRING 0";
	static constexpr time_t time_value = 722574855;
	static const char s_field_value[] = "11/24/92 03:14:15";

	RomFields *const fields = m_romData->getWritableFields();
	fields->addField_dateTime(s_field_desc, time_value,
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME |
		RomFields::RFT_DATETIME_IS_UTC);

	/** Verify the GTK widgets. **/

	// Create a RomDataView.
	// TODO: Set description format type properly.
	m_romDataView = rp_rom_data_view_new_with_romData("", m_romData, RP_DFT_GNOME);
#if GTK_CHECK_VERSION(3, 98, 4)
	g_object_ref_sink(m_romDataView);
#endif /* GTK_CHECK_VERSION(3, 98, 4) */

	// NOTE: For efficiency reasons, GTK RomDataView uses g_idle_add()
	// to schedule its display update. Force it to run here.
	ASSERT_TRUE(rp_rom_data_view_is_showing_data(RP_ROM_DATA_VIEW(m_romDataView)));

	// Get the widgets from the first row.
	// Widgets will be stored in m_lblDesc and m_widgetValue.
	ASSERT_NO_FATAL_FAILURE(getRowWidgets(RP_ROM_DATA_VIEW(m_romDataView)));
	ASSERT_TRUE(GTK_IS_LABEL(m_widgetValue));

	// Verify the label contents.
	// NOTE: Description label will have an added ':'.
	string stds_field_desc = s_field_desc;
	stds_field_desc += ':';

	// NOTE: Using gtk_label_get_label(), which returns mnemonics and Pango markup.
	EXPECT_STREQ(stds_field_desc.c_str(), gtk_label_get_label(GTK_LABEL(m_lblDesc))) << "Field description is incorrect.";
	EXPECT_STREQ(s_field_value, gtk_label_get_label(GTK_LABEL(m_widgetValue))) << "Field value is incorrect.";
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
