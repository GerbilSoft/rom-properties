/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * stdafx.h: Common definitions and includes.                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef __cplusplus
/** C++ **/

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <cstring>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

// C++ includes.
#include <algorithm>
#include <array>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#else /* !__cplusplus */
/** C **/

// C includes.
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#endif /* __cplusplus */

#ifdef __cplusplus
// Qt includes.
#include <QtCore/qglobal.h>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QMap>
#include <QtCore/QMetaType>
#include <QtCore/QMimeData>
#include <QtCore/QSignalMapper>
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QTranslator>
#include <QtCore/QUrl>
#include <QtCore/QVector>

#include <QtGui/QDrag>
#include <QtGui/QFont>
#include <QtGui/QFontMetrics>
#include <QtGui/QIcon>
#include <QtGui/QImage>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>
#include <QtGui/QValidator>

// Was QtGui in Qt4; now QtCore in Qt5
#include <QSortFilterProxyModel>

// Was QtGui in Qt4; now QtWidgets in Qt5
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSpacerItem>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeView>
#include <QWidget>

#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#endif /* __cplusplus */

// libi18n
#include "libi18n/i18n.h"

// librpbase common headers
#include "common.h"
#include "aligned_malloc.h"
#include "ctypex.h"
#include "dll-macros.h"

#ifdef __cplusplus
// librpbase C++ headers
#include "librpbase/RomData.hpp"
#include "librpbase/RomFields.hpp"
#include "librpbase/RomMetaData.hpp"
#include "librpbase/config/Config.hpp"
#include "librpbase/SystemRegion.hpp"
#include "librpbase/img/RpPngWriter.hpp"

// librpfile C++ headers
#include "librpfile/FileSystem.hpp"
#include "librpfile/IRpFile.hpp"
#include "librpfile/RpFile.hpp"

// librptexture C++ headers
#include "librptexture/img/rp_image.hpp"
#endif /* !__cplusplus */

// librptext C++ headers
#include "librptext/conversion.hpp"
#include "librptext/printf.hpp"

#ifdef __cplusplus
// KDE UI frontend headers
#include "RpQt.hpp"
#endif /* __cplusplus */
