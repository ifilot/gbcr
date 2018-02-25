#****************************************************************************
#                                                                           #
#    INORGANIC MATERIALS CHEMISTRY CONFIDENTIAL                             #
#                                                                           #
#    Copyright 2018 Inorganic Materials Chemistry                           #
#                   Eindhoven University of Technology (TU/e)               #
#                                                                           #
#    All Rights Reserved.                                                   #
#                                                                           #
#    NOTICE:  All information contained herein is, and remains              #
#    the property of Inorganic Materials Chemistry (TU/e) and its suppliers,#
#    if any.  The intellectual and technical concepts contained             #
#    herein are proprietary to Inorganic Materials Chemistry (TU/e)         #
#    and its suppliers and may be covered by U.S. and Foreign Patents,      #
#    patents in process, and are protected by trade secret or copyright law.#
#    Dissemination of this information or reproduction of this Materials    #
#    is strictly forbidden unless prior written permission is obtained      #
#    from Inorganic Materials Chemistry (TU/e).                             #
#                                                                           #
#    Authors: Ivo Filot         <i.a.w.filot@tue.nl>                        #
#             Michel van Etten  <m.p.c.v.etten@tue.nl>                      #
#             Emiel Hensen      <e.j.m.hensen@tue.nl>                       #
#                                                                           #
#****************************************************************************/

# set minimum cmake requirements
cmake_minimum_required(VERSION 2.8)
project(gbcr)

# add custom directory to look for .cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules )

# Enable release build
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# set Boost
if(APPLE)
    SET(BOOST_INCLUDEDIR "/opt/local/include")
    SET(BOOST_LIBRARYDIR "/opt/local/lib")
ELSE()
    SET(BOOST_INCLUDEDIR "/usr/include")
    SET(BOOST_LIBRARYDIR "/usr/lib/x86_64-linux-gnu")
ENDIF()
SET (Boost_NO_SYSTEM_PATHS ON)
SET (Boost_USE_MULTITHREADED ON)
SET (Boost_USE_STATIC_LIBS ON)
SET (Boost_USE_STATIC_RUNTIME OFF)
SET (BOOST_ALL_DYN_LINK OFF)

find_package(Boost COMPONENTS system REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TCLAP tclap)

# Set include folders
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${Boost_INCLUDE_DIRS}
                    ${TCLAP_INCLUDE_DIRS})

# use C++14
add_definitions(-std=c++14)

# Add sources
file(GLOB SOURCES "*.cpp")

# Set executable
add_executable(gbcr ${SOURCES})

# Link libraries
target_link_libraries(gbcr ${Boost_LIBRARIES})

###
# Installing
##
install (TARGETS gbcr DESTINATION bin)
