cmake_minimum_required(VERSION 3.30 FATAL_ERROR)
project(LanSound)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# LTO on
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include(cmake/CPM.cmake)

CPMAddPackage("gh:xiph/opus@1.5.2")

# boost is a huge project and directly downloading the 'alternate release'
# from github is much faster than recursively cloning the repo.
CPMAddPackage(
        NAME Boost
        VERSION 1.88.0
        URL https://github.com/boostorg/boost/releases/download/boost-1.88.0/boost-1.88.0-cmake.tar.xz
        URL_HASH SHA256=f48b48390380cfb94a629872346e3a81370dc498896f16019ade727ab72eb1ec
        OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_SKIP_INSTALL_RULES ON" # fixme: set `OFF` for installation
        "BUILD_SHARED_LIBS OFF" "BOOST_INCLUDE_LIBRARIES filesystem\\\;system\\\;asio\\\;cobalt"
)

include_directories(
        Opus::opus
        #        Boost::Boost
)

add_executable(LanSound)

target_link_libraries(LanSound
        Opus::opus
        Boost::asio
        Boost::cobalt
)

target_sources(
        LanSound
        PRIVATE
        src/main.cpp
)

target_sources(LanSound
    PRIVATE
    FILE_SET my_modules TYPE CXX_MODULES FILES
    src/packet.cppm
)

target_sources(
        LanSound
        PRIVATE
        FILE_SET my_modules TYPE CXX_MODULES FILES
        src/encoder.cppm
        src/queue.cppm
        src/send_packet.cppm
)
