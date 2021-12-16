cmake_minimum_required(VERSION 3.20)
project(cctop)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
#set(CURSES_INCLUDE_PATH "/opt/homebrew/ncurses/include")

set(CMAKE_CXX_STANDARD 17)

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
else ()
    set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/ncurses")
endif ()

add_executable(cctop
        cctop.h
        main.cpp
        lib/Console.cpp lib/Console.h
        lib/Parser.cpp lib/Parser.h
        macos/Disk.cpp macos/Disk.h
        macos/Memory.cpp macos/Memory.h
        macos/Network.cpp macos/Network.h
        macos/Platform.cpp macos/Platform.h
        macos/Processor.cpp macos/Processor.h
        macos/VirtualMemory.cpp macos/VirtualMemory.h
        macos/ProcessList.cpp macos/ProcessList.h
        macos/Battery.cpp macos/Battery.h
        lib/Options.cpp lib/Options.h
        lib/Help.cpp lib/Help.h
        common/Docker.cpp common/Docker.h
        common/Debug.cpp common/Debug.h)

include(FindPkgConfig)
pkg_check_modules(CURL libcurl REQUIRED)

set(CURSES_NEED_NCURSES TRUE)
set(CURSES_NEED_WIDE TRUE)

#pkg_check_modules(CURSES libncursesw REQUIRED)
find_package(Curses REQUIRED)
#include(FindNcursesw)
#find_package(Ncursesw REQUIRED)
#find_library(NCURSESW NAMES "ncursesw" PATHS "/opt/homebrew/opt/ncurses/lib")
#message(FATAL_ERROR ${NCURSESW})
#find_package(Ncursesw)

#    message(FATAL_ERROR "not found ncursesw")
#if (NOT NCURSESW_FOUND)
#    message(FATAL_ERROR "not found ncursesw")
#endif ()

include_directories(
        ${CURL_INCLUDE_DIRS} ${CURSES_INCLUDE_DIRS} macos
)
target_link_libraries(
        cctop
        ${CURL_LIBRARIES}
        ${CURSES_LIBRARIES}
)

if (APPLE)
    find_library(carbon_lib Carbon) # we look for the Carbon framework and use carbon_lib as an alias for it
    find_library(iokit_lib IOKit)
    find_library(forcefeedback_lib ForceFeedback)
    find_library(cocoa_lib Cocoa)
    find_library(audiounit_lib AudioUnit)
    find_library(coreaudio_lib CoreAudio)
    find_library(opengl_lib OpenGL)
    find_library(corefoundation_lib CoreFoundation)
    set(frameworks
            ${carbon_lib}
            ${iokit_lib}
            ${forcefeedback_lib}
            ${cocoa_lib}
            ${audiounit_lib}
            ${coreaudio_lib}
            ${opengl_lib}
            ${corefoundation_lib})

    target_link_libraries(cctop ${frameworks})
endif ()

#set(CURL_LIBRARY "-lcurl")
#find_package(CURL REQUIRED)
#include_directories(${CURL_INCLUDE_DIR})
#target_link_libraries(cctop ${CURL_LIBRARIES})

#set(CURL_LIBRARY "-lcurl")
#find_package(CURL REQUIRED)

#add_executable(cctop main.cpp)
#include_directories(${CURL_INCLUDE_DIR})
#target_link_libraries(cctop ${CURL_LIBRARIES})

