# KDE UI frontend test suite
PROJECT(kde-tests LANGUAGES CXX)

###### Sources

### ListDataSortProxyModel tests

SET(ListDataSortProxyModelTest_SRCS
	ListDataSortProxyModelTest.cpp
	../ListDataModel.cpp
	../ListDataSortProxyModel.cpp
	../RomDataFormat.cpp
	../RpQImageBackend.cpp
	../RpQUrl.cpp
	../RpQt.cpp
	)
SET(ListDataSortProxyModelTest_H
	../ListDataModel.hpp
	../ListDataSortProxyModel.hpp
	../RomDataFormat.hpp
	../RpQImageBackend.hpp
	../RpQUrl.hpp
	../RpQt.hpp
	)

### RomDataFormat tests

SET(RomDataFormatTest_SRCS
	RomDataFormatTest.cpp
	../RomDataFormat.cpp
	)
SET(RomDataFormatTest_H
	../RomDataFormat.hpp
	)
	
###### Add version-specific subdirectories

# FIXME: Tests are failing on Launchpad on armhf/arm64.
IF(0)
IF(BUILD_KDE4)
	ADD_SUBDIRECTORY(kde4)
ENDIF(BUILD_KDE4)
IF(BUILD_KF5)
	ADD_SUBDIRECTORY(kf5)
ENDIF(BUILD_KF5)
IF(BUILD_KF6)
	ADD_SUBDIRECTORY(kf6)
ENDIF(BUILD_KF6)
ENDIF(0)
