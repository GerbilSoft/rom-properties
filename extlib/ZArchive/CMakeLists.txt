cmake_minimum_required (VERSION 3.15)

project("ZArchive"
    VERSION "0.1.3"
    DESCRIPTION "Library for creating and reading zstd-compressed file archives"
    HOMEPAGE_URL "https://github.com/Exzap/ZArchive"
)

if (WIN32)
    option(BUILD_STATIC_TOOL "Build the standalone executable statically" ON)
elseif(UNIX)
    option(BUILD_STATIC_TOOL "Build the standalone executable statically" OFF)
    if (BUILD_STATIC_TOOL)
        set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
    endif()
endif()

if (BUILD_STATIC_TOOL)
    message(STATUS "Building standalone executable statically")
    set(VCPKG_LIBRARY_LINKAGE "static" CACHE STRING "Vcpkg target triplet")
    set(STATIC_TOOL_FLAG "-static")
else()
    message(STATUS "Building standalone executable dynamically")
    set(VCPKG_LIBRARY_LINKAGE "dynamic" CACHE STRING "Vcpkg target triplet")
    set(STATIC_TOOL_FLAG "")
endif()


set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

set(SOURCE_FILES_LIB
    src/zarchivewriter.cpp
    src/zarchivereader.cpp
    src/sha_256.c
)

# rom-properties: Disable some compiler warnings for now.
IF(NOT MSVC)
    ADD_DEFINITIONS(-Wno-error=reorder)
ENDIF(NOT MSVC)

# build static library
add_library (zarchive ${SOURCE_FILES_LIB})
add_library (ZArchive::zarchive ALIAS zarchive)
target_compile_features(zarchive PUBLIC cxx_std_20)
set_target_properties(zarchive PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

target_include_directories(zarchive
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

# rom-properties: Use our own zstd library here.
#find_package(zstd MODULE REQUIRED) # MODULE because zstd::zstd is not defined upstream
#target_link_libraries(zarchive PRIVATE zstd::zstd ${STATIC_TOOL_FLAG})
target_link_libraries(zarchive PRIVATE ${ZSTD_LIBRARY})

# standalone executable
add_executable (zarchiveTool src/main.cpp)
set_property(TARGET zarchiveTool PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_target_properties(zarchiveTool PROPERTIES OUTPUT_NAME "zarchive")
target_link_libraries(zarchiveTool PRIVATE zarchive ${STATIC_TOOL_FLAG})

# install
install(DIRECTORY include/zarchive/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/zarchive" FILES_MATCHING PATTERN "zarchive*.h")
install(TARGETS zarchive)
install(TARGETS zarchiveTool)

# pkg-config
include(JoinPaths) # can be replaced by cmake_path(APPEND) in CMake 3.20
join_paths(PKGCONFIG_INCLUDEDIR "\${prefix}" "${CMAKE_INSTALL_INCLUDEDIR}")
join_paths(PKGCONFIG_LIBDIR "\${prefix}" "${CMAKE_INSTALL_LIBDIR}")

configure_file("zarchive.pc.in" "zarchive.pc" @ONLY)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/zarchive.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
