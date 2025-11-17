cmake_minimum_required(VERSION 3.10)

include(CheckCXXCompilerFlag)

project(main LANGUAGES C CXX)

#--------------#
# C++ settings #
#--------------#

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_link_options(-fuse-ld=lld)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

#-----------------#
# setup sanitizer #
#-----------------#

if (MSVC)
	add_compile_options(/fsanitize=address /utf-8)
else()
	add_compile_options(-g -flto -pipe -pedantic)

	if (NOT MINGW)
		check_cxx_compiler_flag("-fsanitize=address" ASAN_AD)

		if (ASAN_AD)
			add_link_options(-fsanitize=address)
			add_compile_options(-fsanitize=address)
		endif()

		check_cxx_compiler_flag("-fsanitize=undefined" ASAN_UB)

		if (ASAN_UB)
			add_link_options(-fsanitize=undefined)
			add_compile_options(-fsanitize=undefined)
		endif()
	endif()
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
