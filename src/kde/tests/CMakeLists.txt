# KDE UI frontend test suite
PROJECT(kde-tests LANGUAGES CXX)

###### Sources

### ListDataSortProxyModel tests

SET(ListDataSortProxyModelTest_SRCS
	ListDataSortProxyModelTest.cpp
	)

### RomDataFormat tests

SET(RomDataFormatTest_SRCS
	RomDataFormatTest.cpp
	)

### RomDataView tests

SET(RomDataViewTest_SRCS
	RomDataViewTest.cpp
	)

###### Add version-specific subdirectories

IF(BUILD_KDE4)
	ADD_SUBDIRECTORY(kde4)
ENDIF(BUILD_KDE4)
IF(BUILD_KF5)
	ADD_SUBDIRECTORY(kf5)
ENDIF(BUILD_KF5)
IF(BUILD_KF6)
	ADD_SUBDIRECTORY(kf6)
ENDIF(BUILD_KF6)
