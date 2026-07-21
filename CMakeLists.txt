cmake_minimum_required(VERSION 3.28)

project(obs-soundboard-censor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
set(CMAKE_PREFIX_PATH
    "${DEPS_DIR}"
    "${DEPS_DIR}/obs-deps-2025-07-11-x64"
    "${DEPS_DIR}/obs-deps-qt6-2025-02-04-x64"
    ${CMAKE_PREFIX_PATH})

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 COMPONENTS Widgets Core REQUIRED)

add_library(obs-soundboard-censor MODULE
    src/plugin-main.cpp
    src/audio-decoder.cpp
    src/censor-filter.cpp)

target_link_libraries(obs-soundboard-censor PRIVATE
    OBS::libobs
    OBS::obs-frontend-api
    Qt6::Core
    Qt6::Widgets)

set_target_properties(obs-soundboard-censor PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    PREFIX "")

target_include_directories(obs-soundboard-censor PRIVATE
    "${DEPS_DIR}/obs-deps-2025-07-11-x64/include"
    "${DEPS_DIR}/include")

if(WIN32 OR APPLE)
    include(cmake/FindLibAv.cmake)
    find_libav()
endif()

install(TARGETS obs-soundboard-censor
    RUNTIME DESTINATION obs-plugins/64bit
    LIBRARY DESTINATION obs-plugins/64bit)

install(DIRECTORY data/
    DESTINATION data/obs-plugins/obs-soundboard-censor)
