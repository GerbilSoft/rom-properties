# MicroTAR: Basic tar v7 library.
PROJECT(microtar LANGUAGES C)

# rom-properties: FIXME: Fix these warnings.
IF(NOT MSVC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=stringop-truncation")
ENDIF(NOT MSVC)

ADD_LIBRARY(${PROJECT_NAME} STATIC src/microtar.c)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
DO_SPLIT_DEBUG(${PROJECT_NAME})
