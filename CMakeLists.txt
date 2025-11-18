cmake_minimum_required(VERSION 3.10)

project(main LANGUAGES C CXX)

#--------------#
# C++ settings #
#--------------#

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

#-----------------#
# setup sanitizer #
#-----------------#

if (MSVC)
	add_compile_options(/Wall /sdl /permissive- /fsanitize /utf-8)
else()
	add_compile_options(-Wall -Wextra -Wpedantic -Wold-style-cast)
endif()

#-------------------#
# project configure #
#-------------------#

add_executable(main
	src/main.cpp
)

target_include_directories(main
	PUBLIC
		${CMAKE_SOURCE_DIR}/src
)

set_property(TARGET main PROPERTY
DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
