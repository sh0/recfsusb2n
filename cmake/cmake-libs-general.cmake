################################################################################
### pkg-config #################################################################
################################################################################

# Messages
MESSAGE(STATUS "################################################")
MESSAGE(STATUS "Checking for pkg-config")

# Find
FIND_PACKAGE(PkgConfig)

# Check
IF (NOT PKG_CONFIG_FOUND)
    MESSAGE(FATAL_ERROR "Could not find pkg-config!")
ENDIF (NOT PKG_CONFIG_FOUND)
MESSAGE(STATUS "Found pkg-config")

# Debug
IF (VERBOSE)
    MESSAGE(STATUS "Path: ${PKG_CONFIG_EXECUTABLE}")
ENDIF (VERBOSE)

################################################################################
### pthreads ###################################################################
################################################################################

# Messages
MESSAGE(STATUS "################################################")
MESSAGE(STATUS "Checking for pthreads")

# Find
FIND_PACKAGE(Threads)
PRINT_LIBRARY_INFO("pthreads" PTHREAD_FOUND "${PTHREAD_INCLUDE_DIR}" "${PTHREAD_LIBRARIES}" FATAL_ERROR)

#MESSAGE(STATUS "################################################")

