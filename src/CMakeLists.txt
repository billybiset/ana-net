# $Id$

cmake_minimum_required(VERSION 2.6)

project( ana )

if ( CMAKE_BUILD_TYPE STREQUAL Debug )
    message ( STATUS "Adding profiling flags." )
    add_definitions ( -pg -ggdb3)
    set(CMAKE_EXE_LINKER_FLAGS_${build_type} "${CMAKE_EXE_LINKER_FLAGS_${build_type}} -pg")
endif ()

add_subdirectory( asio )

option( apps "Compile sample applications as well" OFF )

if ( apps )
    message ( STATUS "Compiling sample applications as well." )
    add_subdirectory( apps )
endif ( apps )

option( tests "Compile test applications as well" OFF )

if ( tests )
    message ( STATUS "Compiling test applications as well." )
    add_subdirectory( test )
endif ( tests )
