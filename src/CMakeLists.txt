################################################################################
### recfsusb2n - Sources #######################################################
################################################################################

# Includes
SET(FSUSB_CFLAGS_INT "-Wall -march=native -O2 -g -DB25 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
SET(FSUSB_CFLAGS_EXT "${BOOST_CFLAGS}")

# Libraries
SET(FSUSB_LINK_INT "-lpthread")
SET(FSUSB_LINK_EXT "${BOOST_LDFLAGS}")

# Sources
FILE(GLOB FSUSB_SRC ${FSUSB_SRC_DIR}/*.cpp)
FILE(GLOB ARIB25_SRC ${ARIB25_SRC_DIR}/*.c)

# Compile
ADD_EXECUTABLE(recfsusb2n ${FSUSB_SRC} ${ARIB25_SRC})
ADD_TARGET_CFLAGS(recfsusb2n "${FSUSB_CFLAGS_INT} ${FSUSB_CFLAGS_EXT}")
TARGET_LINK_LIBRARIES(recfsusb2n ${FSUSB_LINK_INT} ${FSUSB_LINK_EXT})

# Install
INSTALL(TARGETS recfsusb2n DESTINATION bin)

