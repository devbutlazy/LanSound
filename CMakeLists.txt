cmake_minimum_required(VERSION 3.14)
project(LanSound)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPUS REQUIRED opus)

find_package(Boost REQUIRED COMPONENTS filesystem system)

include_directories(
    ${OPUS_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

add_executable(LanSound
    encodeTest.cpp
    # Other cpp files if needed
)

target_link_libraries(LanSound
    ${OPUS_LIBRARIES}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)

message(STATUS "Opus include dirs: ${OPUS_INCLUDE_DIRS}")
message(STATUS "Opus libraries: ${OPUS_LIBRARIES}")
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
