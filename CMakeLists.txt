cmake_minimum_required(VERSION 3.28)

project(obs-soundboard-censor VERSION 1.0.0)

set(PROJECT_AUTHOR "Alex Vanino")
set(PROJECT_URL "https://github.com/TheRealAlexV/obs-soundboard-censor")

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
    src/censor-filter.cpp
    src/soundboard-dock.cpp)

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

if(WIN32)
    set(RELEASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/release")
    set(PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/package")

    add_custom_target(stage-release ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RELEASE_DIR}/obs-plugins/64bit"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RELEASE_DIR}/data/obs-plugins/obs-soundboard-censor/locale"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:obs-soundboard-censor>"
            "${RELEASE_DIR}/obs-plugins/64bit/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/data/locale/en-US.ini"
            "${RELEASE_DIR}/data/obs-plugins/obs-soundboard-censor/locale/"
        DEPENDS obs-soundboard-censor
        COMMENT "Staging release files")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/installer/installer-Windows.iss.in"
        "${CMAKE_CURRENT_BINARY_DIR}/installer/installer-Windows.iss"
        @ONLY)

    find_program(ISCC iscc
        PATHS "C:/InnoSetup6"
        REQUIRED)

    add_custom_target(installer
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_DIR}"
        COMMAND "${ISCC}" /O"${PACKAGE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/installer/installer-Windows.iss"
        DEPENDS stage-release
        COMMENT "Building Windows installer")

    add_custom_target(package ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E tar cfv
            "${PACKAGE_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}-Windows-x64.zip"
            --format=zip
            "${RELEASE_DIR}/"
        DEPENDS stage-release
        COMMENT "Creating ZIP package")
endif()
