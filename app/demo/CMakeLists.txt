# ----------------------------------------------------------------------------
#
# hertz wangh@rock-chips.com
#
# SPDX-License-Identifier: Apache-2.0
#
# ----------------------------------------------------------------------------

# vi: set noexpandtab syntax=cmake:

message(STATUS "cmake version ${CMAKE_VERSION}")
if(NOT CMAKE_VERSION VERSION_LESS "2.8.12.20131121")
	cmake_policy(SET CMP0025 OLD) # report Apple's Clang as just Clang
	cmake_policy(SET CMP0042 OLD) # do not require MACOSX_RPATH
endif()

# Search packages for host system instead of packages for target system
# in case of cross compilation these macro should be defined by toolchain file
if(NOT COMMAND find_host_package)
	macro(find_host_package)
		find_package(${ARGN})
	endmacro()
endif()
if(NOT COMMAND find_host_program)
	macro(find_host_program)
		find_program(${ARGN})
	endmacro()
endif()

project (rk_demo)

cmake_minimum_required (VERSION 3.1.0 FATAL_ERROR)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCXXCompilerFlag)

# ----------------------------------------------------------------------------
# set property to classify library kinds
# ----------------------------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")
# ----------------------------------------------------------------------------
# enable test in this project
# ----------------------------------------------------------------------------
enable_testing()

# ----------------------------------------------------------------------------
# Compiler detection
# ----------------------------------------------------------------------------
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CLANG true)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(GCC true)
endif()

if(CLANG)
	# treat clang roughly like gcc
	set(GCC true)
	add_definitions(-Wall -Wextra -Wshadow -ffast-math)
elseif(CMAKE_COMPILER_IS_GNUCXX)
	add_definitions(-Wall -Wextra -Wshadow -ffast-math)
	check_cxx_compiler_flag(-Wno-narrowing GCC_HAS_NO_NARROWING)
	check_cxx_compiler_flag(-mstackrealign GCC_HAS_STACK_REALIGN)
	if (GCC_HAS_STACK_REALIGN)
		add_definitions(-mstackrealign)
	endif()
	execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
endif()

if($(CMAKE_BUILD_TYPE) MATCHES "Release")
	set(CMAKE_C_VISIBILITY_PRESET hidden)
	set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()

# add PIC flag
add_definitions(-fPIC)

# ----------------------------------------------------------------------------
# Create git version information
# ----------------------------------------------------------------------------
if(NOT DEFINED DOT_GIT_PARENT_DIR)
set(DOT_GIT_PARENT_DIR "${PROJECT_SOURCE_DIR}")
endif()

if(EXISTS "${DOT_GIT_PARENT_DIR}/.git")
	find_host_package(Git)
	if(GIT_FOUND)
		# get author
		execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%an
			WORKING_DIRECTORY ${DOT_GIT_PARENT_DIR}
			OUTPUT_VARIABLE EXEC_OUT
			ERROR_VARIABLE EXEC_ERROR
			RESULT_VARIABLE EXEC_RET
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE)

		if (NOT EXEC_RET)
			message(STATUS "author: ${EXEC_OUT}")
			set(VERSION_LAST_AUTHOR ${EXEC_OUT})
		endif()

		# get date
		execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%ad
			WORKING_DIRECTORY ${DOT_GIT_PARENT_DIR}
			OUTPUT_VARIABLE EXEC_OUT
			ERROR_VARIABLE EXEC_ERROR
			RESULT_VARIABLE EXEC_RET
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE)

		if (NOT EXEC_RET)
			message(STATUS "date  : ${EXEC_OUT}")
			set(VERSION_LAST_DATE ${EXEC_OUT})
		endif()

		# get version hash
		execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%H
			WORKING_DIRECTORY ${DOT_GIT_PARENT_DIR}
			OUTPUT_VARIABLE EXEC_OUT
			ERROR_VARIABLE EXEC_ERROR
			RESULT_VARIABLE EXEC_RET
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE)

		if (NOT EXEC_RET)
			set(VERSION_REVISION ${EXEC_OUT})
		endif()

		# get one line version information
		execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:"%h author: %an %s"
			WORKING_DIRECTORY ${DOT_GIT_PARENT_DIR}
			OUTPUT_VARIABLE EXEC_OUT
			ERROR_VARIABLE EXEC_ERROR
			RESULT_VARIABLE EXEC_RET
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE)

		if (NOT EXEC_RET)
			message(STATUS "git version: ${EXEC_OUT}")
			set(VERSION_ONE_LINE ${EXEC_OUT})
			string(REPLACE "\"" "" VERSION_ONE_LINE ${VERSION_ONE_LINE})
		endif()

		set(VERSION_VER_NUM -1)
	else(GIT_FOUND)
		set(VERSION_ONE_LINE "Missing VCS info")
		set(VERSION_LAST_AUTHOR "Unknown")
	endif()

	# add git hooks
	if (EXISTS "${PROJECT_SOURCE_DIR}/tools/hooks/")
		set(GIT_HOOK_SRC "${PROJECT_SOURCE_DIR}/tools/hooks/")
		if(EXISTS "${DOT_GIT_PARENT_DIR}/.git/hooks")
			set(GIT_HOOK_DST "${DOT_GIT_PARENT_DIR}/.git/hooks/")
			file(COPY ${GIT_HOOK_SRC} DESTINATION ${GIT_HOOK_DST})
			message(STATUS "Install git hooks done")
		endif(EXISTS "${DOT_GIT_PARENT_DIR}/.git/hooks")
	endif(EXISTS "${PROJECT_SOURCE_DIR}/tools/hooks/")

else(EXISTS "${DOT_GIT_PARENT_DIR}/.git")
	set(VERSION_ONE_LINE "Without VCS info")
	set(VERSION_LAST_AUTHOR "Unknown")
endif(EXISTS "${DOT_GIT_PARENT_DIR}/.git")

configure_file(
	"${PROJECT_SOURCE_DIR}/util/cmake/version.in"
	"${PROJECT_SOURCE_DIR}/util/include/version.h"
)

include_directories(util/include)

# ----------------------------------------------------------------------------
# Build options
# ----------------------------------------------------------------------------
find_package(PkgConfig)
INCLUDE(GNUInstallDirs)
# pkg_search_module(PTHREAD pthread)

# ----------------------------------------------------------------------------
# Set Warning as Error
# ----------------------------------------------------------------------------
option(WARNINGS_AS_ERRORS "Stop compiles on first warning" OFF)
if(WARNINGS_AS_ERRORS)
	if(GCC)
		add_definitions(-Werror)
	elseif(MSVC)
		add_definitions(/WX)
	endif()
endif(WARNINGS_AS_ERRORS)

# ----------------------------------------------------------------------------
#  Start module definition
# ----------------------------------------------------------------------------

option(MODULE_RK_IMAGE_PROCESS_DEMO "compile: image processing demo " OFF)
if( MODULE_RK_IMAGE_PROCESS_DEMO )
include_directories(image_process)
add_subdirectory(image_process)
endif()

