function(find_libav)
  if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(
      FFMPEG
      REQUIRED
      IMPORTED_TARGET
      libavformat
      libavcodec
      libavutil
      libswresample)
    if(FFMPEG_FOUND)
      target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE PkgConfig::FFMPEG)
    else()
      message(FATAL_ERROR "FFMPEG not found!")
    endif()
    return()
  endif()

  if(NOT buildspec)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/buildspec.json" buildspec)
  endif()
  string(JSON version GET ${buildspec} dependencies prebuilt version)

  if(MSVC)
    set(arch ${CMAKE_GENERATOR_PLATFORM})
  elseif(APPLE)
    set(arch universal)
  endif()
  set(deps_root "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-${version}-${arch}")

  if(EXISTS "${deps_root}")
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${deps_root}/include")
    target_link_libraries(
      ${CMAKE_PROJECT_NAME}
      PRIVATE "${deps_root}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avcodec${CMAKE_STATIC_LIBRARY_SUFFIX}"
              "${deps_root}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avformat${CMAKE_STATIC_LIBRARY_SUFFIX}"
              "${deps_root}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}avutil${CMAKE_STATIC_LIBRARY_SUFFIX}"
              "${deps_root}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}swresample${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(MSVC)
      file(GLOB_RECURSE av_dlls LIST_DIRECTORIES false "${deps_root}/bin/av*.dll")
      file(GLOB_RECURSE sw_dlls LIST_DIRECTORIES false "${deps_root}/bin/sw*.dll")
      install(FILES ${av_dlls};${sw_dlls} DESTINATION "obs-plugins/64bit")
    endif()
    return()
  endif()

  find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
  find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
  find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
  find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)

  find_library(AVCODEC_LIBRARY avcodec)
  find_library(AVFORMAT_LIBRARY avformat)
  find_library(AVUTIL_LIBRARY avutil)
  find_library(SWRESAMPLE_LIBRARY swresample)

  if(AVCODEC_LIBRARY AND AVFORMAT_LIBRARY AND AVUTIL_LIBRARY AND SWRESAMPLE_LIBRARY)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
      ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR}
      ${AVUTIL_INCLUDE_DIR} ${SWRESAMPLE_INCLUDE_DIR})
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
      ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY}
      ${AVUTIL_LIBRARY} ${SWRESAMPLE_LIBRARY})
  else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
      pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavformat libavcodec libavutil libswresample)
      target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE PkgConfig::FFMPEG)
    else()
      message(FATAL_ERROR "FFmpeg libraries not found")
    endif()
  endif()
endfunction(find_libav)
