/***************************************************************************
 * ROM Properties Page shell extension. (kde/tests)                        *
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

// for Q2U8()
#include "RpQt.hpp"
// for RP_KDE_UPPER
#include "RpQtNS.hpp"

// RomDataView
#include "RomDataView.hpp"

// Qt includes
#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

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
	RomDataTestObjectPtr m_romData;

	// Dummy VectorFile with a 16 KiB buffer
	static constexpr size_t VECTOR_FILE_SIZE = 16U * 1024U;
	VectorFilePtr m_vectorFile;

	// RomDataView
	unique_ptr<RomDataView> m_romDataView;
};

void RomDataViewTest::SetUp()
{
	m_vectorFile->resize(VECTOR_FILE_SIZE);
	m_romData = std::make_shared<RomDataTestObject>(m_vectorFile);
}

void RomDataViewTest::TearDown()
{
	m_romDataView.reset();
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

	/** Verify the Qt widgets. **/

	// Create a RomDataView.
	m_romDataView.reset(new RomDataView(m_romData));

	// There shouldn't be any tabs.
	// Get the first VBox and form layout.
	QVBoxLayout *const vboxTab0 = findDirectChild<QVBoxLayout*>(m_romDataView.get(), QLatin1String("vboxTab0"));
	ASSERT_NE(nullptr, vboxTab0);
	QFormLayout *const formTab0 = findDirectChild<QFormLayout*>(vboxTab0, QLatin1String("formTab0"));
	ASSERT_NE(nullptr, formTab0);

	// Get the layout items for the first row.
	QLayoutItem *const itemDesc = formTab0->itemAt(0, QFormLayout::LabelRole);
	ASSERT_NE(nullptr, itemDesc);
	QLayoutItem *const itemValue = formTab0->itemAt(0, QFormLayout::FieldRole);
	ASSERT_NE(nullptr, itemValue);

	// Both the description and value widgets should be QLabel.
	QLabel *const lblDesc = qobject_cast<QLabel*>(itemDesc->widget());
	ASSERT_NE(nullptr, lblDesc);
	QLabel *const lblValue = qobject_cast<QLabel*>(itemValue->widget());
	ASSERT_NE(nullptr, lblValue);

	// Verify the label contents.
	// NOTE: Description label will have an added ':'.
	QString qs_field_desc = QLatin1String(s_field_desc);
	qs_field_desc += QChar(L':');

	EXPECT_EQ(qs_field_desc, lblDesc->text()) << "Field description is incorrect.";
	EXPECT_EQ(QLatin1String(s_field_value), lblValue->text()) << "Field value is incorrect.";
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

	/** Verify the Qt widgets. **/

	// Create a RomDataView.
	m_romDataView.reset(new RomDataView(m_romData));

	// There shouldn't be any tabs.
	// Get the first VBox and form layout.
	QVBoxLayout *const vboxTab0 = findDirectChild<QVBoxLayout*>(m_romDataView.get(), QLatin1String("vboxTab0"));
	ASSERT_NE(nullptr, vboxTab0);
	QFormLayout *const formTab0 = findDirectChild<QFormLayout*>(vboxTab0, QLatin1String("formTab0"));
	ASSERT_NE(nullptr, formTab0);

	// Get the layout items for the first row.
	QLayoutItem *const itemDesc = formTab0->itemAt(0, QFormLayout::LabelRole);
	ASSERT_NE(nullptr, itemDesc);
	QLayoutItem *const itemValue = formTab0->itemAt(0, QFormLayout::FieldRole);
	ASSERT_NE(nullptr, itemValue);

	// Label should be QLabel.
	QLabel *const lblDesc = qobject_cast<QLabel*>(itemDesc->widget());
	ASSERT_NE(nullptr, lblDesc);

	// Verify the label contents.
	// NOTE: Description label will have an added ':'.
	QString qs_field_desc = QLatin1String(s_field_desc);
	qs_field_desc += QChar(L':');
	EXPECT_EQ(qs_field_desc, lblDesc->text()) << "Field description is incorrect.";

	// The value item will be a QGridLayout.
	QGridLayout *const gridLayout = qobject_cast<QGridLayout*>(itemValue->layout());
	ASSERT_NE(nullptr, gridLayout);

	// Grid should be 4x4, since we specified 4 items per column,
	// and we have 16 items.
	EXPECT_EQ(4, gridLayout->columnCount());
	EXPECT_EQ(4, gridLayout->rowCount());

	// Go through each item.
	unsigned int bit = 0;
	const int rowCount = gridLayout->rowCount();
	const int columnCount = gridLayout->columnCount();
	for (int row = 0; row < rowCount && bit < bitfield_names.size(); row++) {
		for (int col = 0; col < columnCount && bit < bitfield_names.size(); col++, bit++) {
			QLayoutItem *const layoutItem = gridLayout->itemAtPosition(row, col);
			EXPECT_NE(nullptr, layoutItem);
			if (!layoutItem) {
				continue;
			}

			// Get the QCheckBox.
			QCheckBox *const checkBox = qobject_cast<QCheckBox*>(layoutItem->widget());
			EXPECT_NE(nullptr, checkBox);
			if (!checkBox) {
				continue;
			}

			// Verify the checkbox's label.
			EXPECT_EQ(QLatin1String(bitfield_names[bit]), checkBox->text()) << "QCheckBox " << bit << " label is incorrect.";

			// Verify the checkbox's value.
			EXPECT_EQ(!!(bitfield_value & (1U << bit)), checkBox->isChecked()) << "QCheckBox " << bit << " value is incorrect.";
		}
	}

	// Make sure we've processed all of the bits.
	EXPECT_EQ(bit, bitfield_names.size()) << "Incorrect number of bits processed.";
}

} }

#ifdef HAVE_SECCOMP
const unsigned int rp_gtest_syscall_set = RP_GTEST_SYSCALL_SET_QT;
#endif /* HAVE_SECCOMP */

/**
 * Test suite main function.
 */
extern "C" int gtest_main(int argc, TCHAR *argv[])
{
	fputs("KDE (" RP_KDE_UPPER ") UI frontend test suite: RomDataView tests.\n\n", stderr);
	fflush(nullptr);

	// Force usage of the "minimal" QPA backend for Qt5+.
	setenv("XDG_SESSION_TYPE", "", 1);
	setenv("QT_QPA_PLATFORM", "minimal", 1);

	// Need to construct a QApplication before using QtWidgets.
	QApplication app(argc, argv);

	// coverity[fun_call_w_exception]: uncaught exceptions cause nonzero exit anyway, so don't warn.
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
