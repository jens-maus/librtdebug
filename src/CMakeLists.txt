#/* vim:set ts=2 nowrap: ****************************************************
#
# librtdebug - A C++ based thread-safe Runtime Debugging Library
# Copyright (C) 2003-2015 Jens Maus <mail@jens-maus.de>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# $Id: README 3 2005-01-27 15:21:21Z langner $
#
#***************************************************************************/

# check for certain functions to exist
include(CheckFunctionExists)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(GetTickCount HAVE_GETTICKCOUNT)
check_function_exists(vasprintf HAVE_VASPRINTF)

# check for certain functions to exist
include(CheckLibraryExists)
check_library_exists(pthread pthread_mutex_init "" HAVE_LIBPTHREAD)

# configure files where variables should be replaced.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)

# specify include directories
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# consider all *.ccp files found to be actual code files
file(GLOB_RECURSE CODE_FILES *.cpp)

# specify that we are creating a shared and static library at the
# same time
add_library(${CMAKE_PROJECT_NAME}.a  STATIC ${CODE_FILES})
add_library(${CMAKE_PROJECT_NAME}.so SHARED ${CODE_FILES})
set_target_properties(${CMAKE_PROJECT_NAME}.a PROPERTIES OUTPUT_NAME rtdebug 
                                                         VERSION ${PROJECT_VERSION} 
                                                         SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(${CMAKE_PROJECT_NAME}.so PROPERTIES OUTPUT_NAME rtdebug
                                                          VERSION ${PROJECT_VERSION} 
                                                          SOVERSION ${PROJECT_VERSION_MAJOR})

# define link libraries dependencies
#target_link_libraries(${CMAKE_PROJECT_NAME}.a Qt5::Core)
#target_link_libraries(${CMAKE_PROJECT_NAME}.so Qt5::Core)

# definition of install targets
install(TARGETS ${CMAKE_PROJECT_NAME}.a
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        COMPONENT library
)
install(TARGETS ${CMAKE_PROJECT_NAME}.so
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        COMPONENT library
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/rtdebug.h
              ${CMAKE_CURRENT_SOURCE_DIR}/CRTDebug.h
              DESTINATION include/rtdebug)
