################################################################################
### recfsusb2n - CMake #########################################################
################################################################################

# CMake requirements
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.2 FATAL_ERROR)

# Compat
IF (COMMAND cmake_policy)
    cmake_policy(SET CMP0004 OLD)
ENDIF (COMMAND cmake_policy)

# Paths
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")
SET(FSUSB_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
SET(ARIB25_SRC_DIR "${CMAKE_SOURCE_DIR}/src/arib25")

# Check build directory
IF (${CMAKE_SOURCE_DIR} MATCHES ${CMAKE_BINARY_DIR})
	MESSAGE("Please do an out-of-tree build:")
	MESSAGE("rm CMakeCache.txt; mkdir build; cd build; cmake ..; make")
	MESSAGE(FATAL_ERROR "in-tree-build detected")
ENDIF (${CMAKE_SOURCE_DIR} MATCHES ${CMAKE_BINARY_DIR})

# Project
PROJECT(recfsusb2n)

# Options
OPTION(VERBOSE "Verbose cmake output" OFF)

# Libraries
INCLUDE(cmake-helpers)
INCLUDE(cmake-libs-general)
INCLUDE(cmake-libs-boost)

# Subdirs
ADD_SUBDIRECTORY(src)

