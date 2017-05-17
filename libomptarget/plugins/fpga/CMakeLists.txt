##===----------------------------------------------------------------------===##
#
#                     The LLVM Compiler Infrastructure
#
# This file is dual licensed under the MIT and the University of Illinois Open
# Source Licenses. See LICENSE.txt for details.
#
##===----------------------------------------------------------------------===##
#
# Build a plugin for a FPGA machine if available.
#
##===----------------------------------------------------------------------===##
libomptarget_say("Building FPGA offloading plugin.")

# Define the suffix for the runtime messaging dumps.
add_definitions(-DTARGET_NAME=FPGA)

if(LIBOMPTARGET_CMAKE_BUILD_TYPE MATCHES debug)
  add_definitions(-DFPGA_ERROR_REPORT)
endif()

add_library(omptarget.rtl.fpga SHARED src/rtl.cpp)

# Install plugin under the lib destination folder.
install(TARGETS omptarget.rtl.fpga LIBRARY DESTINATION lib${LIBOMPTARGET_LIBDIR_SUFFIX})

target_link_libraries(omptarget.rtl.fpga
  "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/../exports")

# Report to the parent scope that we are building a plugin for FPGA.
set(LIBOMPTARGET_SYSTEM_TARGETS "${LIBOMPTARGET_SYSTEM_TARGETS} unkown-unkown-fpga" PARENT_SCOPE)
