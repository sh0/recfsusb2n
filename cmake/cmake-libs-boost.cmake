################################################################################
### Boost ######################################################################
################################################################################

IF (NOT BOOST_CHECKED)

    # Messages
    MESSAGE(STATUS "################################################")
    MESSAGE(STATUS "Checking for Boost")

    # Find
    SET(Boost_USE_MULTITHREADED ON)
    FIND_PACKAGE(Boost 1.40 COMPONENTS thread filesystem)
    SET(BOOST_CFLAGS "-I${Boost_INCLUDE_DIR}")
    SET(BOOST_LDFLAGS "${Boost_LIBRARIES}")
    PRINT_LIBRARY_INFO("Boost" Boost_FOUND "${BOOST_CFLAGS}" "${BOOST_LDFLAGS}" FATAL_ERROR)

    # Set as checked
    SET(BOOST_CHECKED 1)

ENDIF (NOT BOOST_CHECKED)

