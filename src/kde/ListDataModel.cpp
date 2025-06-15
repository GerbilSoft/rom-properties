/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * ListDataModel.cpp: QAbstractListModel for RFT_LISTDATA.                 *
 *                                                                         *
 * Copyright (c) 2012-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ListDataModel.hpp"
#include "RomDataFormat.hpp"

// Other rom-properties libraries
#include "librpbase/RomFields.hpp"
#include "librptexture/img/rp_image.hpp"
using namespace LibRpTexture;
using LibRpBase::RomFields;

// C++ STL classes
using std::array;
using std::set;
using std::string;
using std::unordered_map;
using std::vector;

/** ListDataModelPrivate **/

class ListDataModelPrivate
{
public:
	explicit ListDataModelPrivate(ListDataModel *q);

protected:
	ListDataModel *const q_ptr;
	Q_DECLARE_PUBLIC(ListDataModel)
private:
	Q_DISABLE_COPY(ListDataModelPrivate)

public:
	// Row/column count
	int columnCount;
	int rowCount;

	// Header strings
	vector<QString> headers;

	// Map of language codes to string arrays.
	// If this is RFT_LISTDATA, only one language code is present: 0
	// - Key: LC
	// - Value: Array of QStrings
	unordered_map<uint32_t, vector<QString> > map_data;

	// Flat array of QStrings.
	// Size should always be columnCount*rowCount.
	// Ordering is per-row. (row0, col0; row0, col1; row0, col2; row1, col0; etc)
	// Points to an element in map_data.
	const vector<QString> *pData;

	// Icons
	// NOTE: References to rp_image* are kept in case
	// the icon size is changed.
	vector<QPixmap> icons;
	vector<rp_image_const_ptr> icons_rp;
	QSize iconSize;

	// Qt::ItemFlags
	Qt::ItemFlags itemFlags;

	// Text alignment
	uint16_t align_headers;
	uint16_t align_data;

	// Checkboxes
	uint32_t checkboxes;
	bool hasCheckboxes;

	// Current language code
	uint32_t lc;

public:
	/**
	 * Clear all internal data.
	 */
	void clearData(void);

	/**
	 * Update the icons pixmap vector.
	 */
	void updateIconPixmaps(void);

	/**
	 * Convert a single language from RFT_LISTDATA or RFT_LISTDATA_MULTI to vector<QString>.
	 * @param list_data Single language RFT_LISTDATA data.
	 * @param pField Field
	 * @return vector<QString>.
	 */
	static vector<QString> convertListDataToVector(const RomFields::ListData_t *list_data, const RomFields::Field *pField);

public:
	/**
	 * Update the current language code.
	 * @param lc New language code.
	 */
	void updateLC(uint32_t lc);

	/**
	 * Update the current language code.
	 * @param def_lc ROM default language code.
	 * @param user_lc User-specified language code.
	 */
	void updateLC(uint32_t def_lc, uint32_t user_lc);

public:
	// Column data alignment table
	// All values are known to fit in uint8_t.
	// NOTE: Need to include AlignVCenter.
	static const array<uint8_t, 4> align_tbl;
};

// Column data alignment table
// All values are known to fit in uint8_t.
// NOTE: Need to include AlignVCenter.
const array<uint8_t, 4> ListDataModelPrivate::align_tbl = {{
	// Order: TXA_D, TXA_L, TXA_C, TXA_R
	Qt::AlignLeft | Qt::AlignVCenter,
	Qt::AlignLeft | Qt::AlignVCenter,
	Qt::AlignCenter,
	Qt::AlignRight | Qt::AlignVCenter,
}};

ListDataModelPrivate::ListDataModelPrivate(ListDataModel *q)
	: q_ptr(q)
	, columnCount(0)
	, rowCount(0)
	, pData(nullptr)
	, iconSize(QSize(32, 32))
	, itemFlags(Qt::NoItemFlags)
	, align_headers(0)
	, align_data(0)
	, checkboxes(0)
	, hasCheckboxes(false)
	, lc('en')
{
	// TODO: Better default icon size?
}

/**
 * Clear all internal data.
 */
void ListDataModelPrivate::clearData(void)
{
	headers.clear();
	map_data.clear();
	pData = nullptr;
	itemFlags = Qt::NoItemFlags;
	checkboxes = 0;
	hasCheckboxes = false;
	align_headers = 0;
	align_data = 0;

	// Clear icons.
	icons.clear();
	icons_rp.clear();
}

/**
 * Update the icons pixmap vector.
 */
void ListDataModelPrivate::updateIconPixmaps(void)
{
	icons.clear();
	icons.reserve(icons_rp.size());

	for (const auto &img : icons_rp) {
		if (!img) {
			icons.emplace_back();
			continue;
		}

		QPixmap pixmap = QPixmap::fromImage(rpToQImage(img));

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
		// NOTE: Assuming square pixmaps.
		if (img->width() > iconSize.width()) {
			// Instead of scaling icons down, set a device pixel ratio.
			// This allows for higher-resolution display on high-DPI screens.
			pixmap.setDevicePixelRatio(static_cast<qreal>(img->width()) / static_cast<qreal>(iconSize.width()));
		} else {
			// Scale up using integer scaling, then set a device pixel ratio.
			int w = img->width();
			int h = img->height();
			if (w <= 0) {
				continue;
			}
			while (w < iconSize.width()) {
				w += img->width();
				h += img->height();
			}
			pixmap = pixmap.scaled(w, h, Qt::KeepAspectRatio, Qt::FastTransformation);
			
			pixmap.setDevicePixelRatio(static_cast<qreal>(w) / static_cast<qreal>(iconSize.width()));
		}
#else /* QT_VERSION < QT_VERSION_CHECK(5, 0, 0) */
		// Do we need to resize the icon?
		if (img->width() != iconSize.width() ||
		    img->height() != iconSize.height())
		{
			// Resize is needed.
			pixmap = pixmap.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) */

		icons.push_back(std::move(pixmap));
	}
}

/**
 * Convert a single language from RFT_LISTDATA or RFT_LISTDATA_MULTI to vector<QString>.
 * @param list_data Single language RFT_LISTDATA data.
 * @param pField Field
 * @return vector<QString>.
 */
vector<QString> ListDataModelPrivate::convertListDataToVector(const RomFields::ListData_t *list_data, const RomFields::Field *pField)
{
	vector<QString> data;
	if (!list_data || list_data->empty()) {
		// No data...
		return data;
	}

	const auto &listDataDesc = pField->desc.list_data;
	const unsigned int flags = pField->flags;
	const bool hasCheckboxes = !!(flags & RomFields::RFT_LISTDATA_CHECKBOXES);

	const size_t columnCount = list_data->at(0).size();
	const size_t rowCount = list_data->size();

	data.reserve(columnCount * rowCount);
	for (const vector<string> &data_row : *list_data) {
		if (hasCheckboxes && data_row.empty()) {
			// Skip this row.
			continue;
		}

		// Add item text.
		assert(data_row.size() == columnCount);
		int cols = static_cast<int>(columnCount);
		unsigned int is_timestamp = listDataDesc.col_attrs.is_timestamp;
		for (const string &u8_str : data_row) {
			if (unlikely(is_timestamp & 1)) {
				// Timestamp column. Format the timestamp.
				RomFields::TimeString_t time_string;
				memcpy(time_string.str, u8_str.data(), 8);

				QString str = formatDateTime(time_string.time,
					listDataDesc.col_attrs.dtflags);
				if (likely(!str.isEmpty())) {
					data.push_back(std::move(str));
				} else {
					data.push_back(QC_("RomData", "Unknown"));
				}
			} else {
				data.push_back(U82Q(u8_str));
			}

			// Next column
			is_timestamp >>= 1;
			cols--;
			if (cols <= 0) {
				break;
			}
		}

		// If there's fewer columns in the data row than we have allocated,
		// add blank QStrings.
		for (size_t i = columnCount; i < data_row.size(); i++) {
			data.emplace_back();
		}
	}

	return data;
}

/**
 * Update the current language code.
 * @param lc New language code.
 */
void ListDataModelPrivate::updateLC(uint32_t lc)
{
	if (this->lc == lc) {
		// Same LC.
		return;
	} else if (map_data.size() <= 1) {
		// Only one language is present.
		return;
	}

	// Search for the new pData.
	Q_Q(ListDataModel);
	auto iter = map_data.find(lc);
	if (iter == map_data.end()) {
		// Not found. Don't do anything.
		return;
	}

	// Check if the pData is different.
	if (pData == &(iter->second)) {
		// Same pData.
		return;
	}

	// New pData.
	pData = &(iter->second);
	const QModelIndex indexFirst = q->createIndex(0, 0);
	const QModelIndex indexLast = q->createIndex(rowCount-1, columnCount-1);
	emit q->dataChanged(indexFirst, indexLast);

	// Language code has changed.
	this->lc = lc;
	emit q->lcChanged(lc);
}

/**
 * Update the current language code.
 * @param def_lc ROM default language code.
 * @param user_lc User-specified language code.
 */
void ListDataModelPrivate::updateLC(uint32_t def_lc, uint32_t user_lc)
{
	if (map_data.size() <= 1) {
		// Only one language is present.
		return;
	}

	Q_Q(ListDataModel);
	const std::vector<QString> *pData = nullptr;
	uint32_t lc = 0;

	if (user_lc != 0) {
		// Search for the new pData with the user LC first.
		auto iter = map_data.find(user_lc);
		if (iter != map_data.end()) {
			// Found it!
			pData = &(iter->second);
			lc = user_lc;
		}
	}

	if (!pData && def_lc != user_lc) {
		// Search for the ROM default LC.
		auto iter = map_data.find(def_lc);
		if (iter != map_data.end()) {
			// Found it!
			pData = &(iter->second);
			lc = def_lc;
		}
	}

	if (!pData) {
		// Not found.
		return;
	}

	// Check if the pData is different.
	if (pData == this->pData) {
		// Same pData.
		return;
	}

	// New pData.
	this->pData = pData;
	const QModelIndex indexFirst = q->createIndex(0, 0);
	const QModelIndex indexLast = q->createIndex(rowCount-1, columnCount-1);
	emit q->dataChanged(indexFirst, indexLast);

	// Language code has changed.
	this->lc = lc;
	emit q->lcChanged(lc);
}

/** ListDataModel **/

ListDataModel::ListDataModel(QObject *parent)
	: super(parent)
	, d_ptr(new ListDataModelPrivate(this))
{}

ListDataModel::~ListDataModel()
{
	delete d_ptr;
}

int ListDataModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	Q_D(const ListDataModel);
	return d->rowCount;
}

int ListDataModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	Q_D(const ListDataModel);
	return d->columnCount;
}

QVariant ListDataModel::data(const QModelIndex &index, int role) const
{
	Q_D(const ListDataModel);
	if (!index.isValid() || !d->pData)
		return {};
	const int row = index.row();
	const int column = index.column();
	if (row < 0 || row >= d->rowCount || column < 0 || column >= d->columnCount)
		return {};

	switch (role) {
		case Qt::DisplayRole:
			return d->pData->at((row * d->columnCount) + column);

		case Qt::TextAlignmentRole:
			// Qt::Alignment
			return d->align_tbl[((d->align_data >> (column * RomFields::TXA_BITS)) & RomFields::TXA_MASK)];

		case Qt::CheckStateRole:
			if (column != 0 || !d->hasCheckboxes)
				break;
			return (d->checkboxes & (1U << row)) ? Qt::Checked : Qt::Unchecked;

		case Qt::DecorationRole:
			if (column != 0 || d->icons.empty())
				break;
			if (row <= static_cast<int>(d->icons.size()))
				return d->icons[row];
			break;

		case RpImageRole:
			if (column != 0 || d->icons.empty())
				break;
			if (row <= static_cast<int>(d->icons.size())) {
				// NOTE: We can't put an std::shared_ptr<> in QVariant.
				// Pass a pointer to the std::shared_ptr<> instead.
				if (d->icons_rp[row]) {
					return QVariant::fromValue((void*)&d->icons_rp[row]);
				}
			}
			break;

		default:
			break;
	}

	// Default value.
	return {};
}

Qt::ItemFlags ListDataModel::flags(const QModelIndex &index) const
{
	Q_D(const ListDataModel);
	if (!index.isValid() || !d->pData)
		return Qt::NoItemFlags;

	const int row = index.row();
	const int column = index.column();
	if (row < 0 || row >= d->rowCount || column < 0 || column >= d->columnCount)
		return Qt::NoItemFlags;

	return d->itemFlags;
}

QVariant ListDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(orientation);

	Q_D(const ListDataModel);
	if (section < 0 || section >= d->columnCount)
		return {};

	switch (role) {
		case Qt::DisplayRole:
			// NOTE: d->headers[] might be empty if the RFT_LISTDATA
			// field doesn't have column names.
			if (section >= static_cast<int>(d->headers.size()))
				break;
			return d->headers[section];

		case Qt::TextAlignmentRole:
			// Qt::Alignment
			return d->align_tbl[((d->align_headers >> (section * RomFields::TXA_BITS)) & RomFields::TXA_MASK)];
	}

	// Default value.
	return {};
}

/**
 * Set the field to use in this model.
 * Field data is *copied* into the model.
 * @param field Field
 */
void ListDataModel::setField(const RomFields::Field *pField)
{
	Q_D(ListDataModel);

	// Remove data if it's already set.
	if (d->rowCount > 0) {
		// Notify the view that we're about to remove all rows and columns.
		if (d->rowCount > 0) {
			beginRemoveRows(QModelIndex(), 0, (d->rowCount - 1));
			d->rowCount = 0;
			endRemoveRows();
		}

		if (d->columnCount > 0) {
			beginRemoveColumns(QModelIndex(), 0, (d->columnCount - 1));
			d->columnCount = 0;
			endRemoveColumns();
		}

		d->clearData();
	}

	if (!pField) {
		// NULL field. Nothing to do here.
		return;
	}

	assert(pField->type == RomFields::RFT_LISTDATA);
	if (pField->type != RomFields::RFT_LISTDATA) {
		// Not an RFT_LISTDATA field.
		return;
	}

	const auto &listDataDesc = pField->desc.list_data;
	const unsigned int flags = pField->flags;

	// Validate flags.
	// Cannot have both checkboxes and icons.
	const bool hasCheckboxes = !!(flags & RomFields::RFT_LISTDATA_CHECKBOXES);
	const bool hasIcons = !!(flags & RomFields::RFT_LISTDATA_ICONS);
	assert(!(hasCheckboxes && hasIcons));
	if (hasCheckboxes && hasIcons) {
		// Both are set. This shouldn't happen...
		return;
	}

	// Single language ListData_t.
	// For RFT_LISTDATA_MULTI, this is only used for row and column count.
	const RomFields::ListData_t *list_data;
	const bool isMulti = !!(flags & RomFields::RFT_LISTDATA_MULTI);
	if (isMulti) {
		// Multiple languages.
		const auto *const multi = pField->data.list_data.data.multi;
		assert(multi != nullptr);
		assert(!multi->empty());
		if (!multi || multi->empty()) {
			// No data...
			return;
		}

		list_data = &multi->cbegin()->second;
	} else {
		// Single language.
		list_data = pField->data.list_data.data.single;
	}

	assert(list_data != nullptr);
	assert(!list_data->empty());
	if (!list_data || list_data->empty()) {
		// No data...
		return;
	}

	if (hasIcons) {
		assert(pField->data.list_data.mxd.icons != nullptr);
		if (!pField->data.list_data.mxd.icons) {
			// No icons vector...
			return;
		}
	}

	// Copy alignment values.
	d->align_headers = listDataDesc.col_attrs.align_headers;
	d->align_data = listDataDesc.col_attrs.align_data;

	// Set up the columns.
	// NOTE: listDataDesc.names can be nullptr,
	// which means we don't have any column headers.
	int columnCount;
	if (listDataDesc.names) {
		columnCount = static_cast<int>(listDataDesc.names->size());
		d->headers.clear();
		d->headers.reserve(columnCount);

		for (const string &u8_str : *(listDataDesc.names)) {
			d->headers.push_back(U82Q(u8_str));
		}
	} else {
		// No column headers.
		// Use the first row for the column count.
		d->headers.clear();
		columnCount = static_cast<int>(list_data->at(0).size());
	}
	beginInsertColumns(QModelIndex(), 0, (columnCount - 1));
	d->columnCount = columnCount;
	endInsertColumns();

	// Checkboxes
	if (hasCheckboxes) {
		d->checkboxes = pField->data.list_data.mxd.checkboxes;
		d->hasCheckboxes = true;
	}

	// Set Qt::ItemFlags
	if (hasIcons) {
		d->itemFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
	} else {
		d->itemFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	// Add the row data.
	int rowCount;
	if (isMulti) {
		// RFT_LISTDATA_MULTI: Multiple languages.
		const auto *const multi = pField->data.list_data.data.multi;
		// NOTE: Assuming all languages have the same number of rows.
		rowCount = static_cast<int>(multi->cbegin()->second.size());

		for (const auto &pdm : *multi) {
			assert(static_cast<int>(pdm.second.size()) == rowCount);
			auto pair = d->map_data.emplace(pdm.first,
				d->convertListDataToVector(&(pdm.second), pField));
			if (!d->pData && pdm.first == d->lc) {
				d->pData = &(pair.first->second);
			}
		}

		if (!d->pData) {
			// Specified language code was not found.
			// Use the first language code by default.
			d->pData = &(d->map_data.cbegin()->second);
		}
	} else {
		// RFT_LISTDATA: Single language.
		rowCount = static_cast<int>(list_data->size());
		auto pair = d->map_data.emplace(0,
			d->convertListDataToVector(list_data, pField));
		d->pData = &(pair.first->second);
	}

	if (hasIcons) {
		// Icons.
		// NOTE: Icons are the same for all languages.
		// Also, we can assume all rows are present, since
		// icons and checkboxes are mutually exclusive.

		// NOTE 2: Since we're using shared_ptr<> now, we can just
		// copy the entire vector over without manually iterating.
		d->icons_rp = *(pField->data.list_data.mxd.icons);

		// Update the icons pixmap vector.
		d->updateIconPixmaps();
	}

	if (d->pData) {
		beginInsertRows(QModelIndex(), 0, (rowCount - 1));

		// NOTE: We may have skipped empty rows if checkboxes are enabled.
		if (likely(!hasCheckboxes)) {
			d->rowCount = rowCount;
		} else {
			d->rowCount = static_cast<int>(d->pData->size() / d->columnCount);
		}

		endInsertRows();
	}
}

/** Properties **/

/**
 * Set the language code to use in this model.
 * @param lc Language code. (0 for default)
 */
void ListDataModel::setLC(uint32_t lc)
{
	Q_D(ListDataModel);
	d->updateLC(lc);
}

/**
 * Set the language code to use in this model.
 * @param def_lc ROM default language code.
 * @param user_lc User-specified language code.
 */
void ListDataModel::setLC(uint32_t def_lc, uint32_t user_lc)
{
	Q_D(ListDataModel);
	d->updateLC(def_lc, user_lc);
}

/**
 * Get the language code used in this model.
 * @return Language code. (0 for default)
 */
uint32_t ListDataModel::lc(void) const
{
	Q_D(const ListDataModel);
	return d->lc;
}

/**
 * Get all supported language codes.
 *
 * If this is not showing RFT_LISTDATA_MULTI, an empty set
 * will be returned.
 *
 * @return Supported language codes.
 */
set<uint32_t> ListDataModel::getLCs(void) const
{
	Q_D(const ListDataModel);
	set<uint32_t> set_lc;

	if (d->map_data.empty()) {
		return set_lc;
	} else if (d->map_data.size() == 1) {
		// Check if the map has a single language with lc == 0.
		if (d->map_data.cbegin()->first == 0) {
			// lc == 0.
			return set_lc;
		}
	}

	for (const auto &pmd : d->map_data) {
		set_lc.insert(pmd.first);
	}
	return set_lc;
}

/**
 * Set the icon size.
 * @param iconSize Icon size.
 */
void ListDataModel::setIconSize(QSize iconSize)
{
	Q_D(ListDataModel);
	if (d->iconSize == iconSize) {
		// Same icon size.
		return;
	}

	d->iconSize = iconSize;
	if (!d->icons_rp.empty()) {
		d->updateIconPixmaps();
		const QModelIndex indexFirst = createIndex(0, 0);
		const QModelIndex indexLast = createIndex(d->rowCount-1, 0);
		emit dataChanged(indexFirst, indexLast);
	}

	emit iconSizeChanged(d->iconSize);
}

/**
 * Get the icon size.
 * @return Icon size.
 */
QSize ListDataModel::iconSize(void) const
{
	Q_D(const ListDataModel);
	return d->iconSize;
}
