## Copyright 2013-2019 Anton Runov
##
## This file is part of Octaudio.
##
## Octaudio is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## Octaudio is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Octaudio.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required( VERSION 2.8 )
project( octaudio )
set( OCTAUDIO_VERSION "1.1.0" )

# Octave
find_program( OCTAVE_CONFIG_EXECUTABLE octave-config )

if( NOT OCTAVE_CONFIG_EXECUTABLE )
  message( FATAL_ERROR "octave-config not found" )
endif()

execute_process( COMMAND ${OCTAVE_CONFIG_EXECUTABLE} -p OCTINCLUDEDIR
                 OUTPUT_VARIABLE OCTAVE_OCTINCDIR   OUTPUT_STRIP_TRAILING_WHITESPACE )

execute_process( COMMAND ${OCTAVE_CONFIG_EXECUTABLE} -p INCLUDEDIR
                 OUTPUT_VARIABLE OCTAVE_INCDIR   OUTPUT_STRIP_TRAILING_WHITESPACE )

execute_process( COMMAND ${OCTAVE_CONFIG_EXECUTABLE} -p LIBDIR
                 OUTPUT_VARIABLE OCTAVE_LIBDIR   OUTPUT_STRIP_TRAILING_WHITESPACE )

execute_process( COMMAND ${OCTAVE_CONFIG_EXECUTABLE} -p OCTLIBDIR
                 OUTPUT_VARIABLE OCTAVE_OCTLIBDIR   OUTPUT_STRIP_TRAILING_WHITESPACE )

find_path( OCTAVE_INCLUDE_DIR oct.h ${OCTAVE_OCTINCDIR} NO_DEFAULT_PATH )

find_path( OCTAVE_HDF5_INCLUDE_DIR hdf5.h ${OCTAVE_INCDIR} ${OCTAVE_INCDIR}/hdf5/*  NO_DEFAULT_PATH )

find_library( OCTAVE_LIBRARY octave HINTS ${OCTAVE_LIBDIR} ${OCTAVE_OCTLIBDIR} NO_DEFAULT_PATH )
find_library( OCTAVE_INTERP_LIBRARY octinterp HINTS ${OCTAVE_LIBDIR} ${OCTAVE_OCTLIBDIR} NO_DEFAULT_PATH )
set( OCTAVE_LIBRARIES ${OCTAVE_LIBRARY} ${OCTAVE_INTERP_LIBRARY} )

find_library( OCTAVE_CRUFT_LIBRARY cruft HINTS ${OCTAVE_LIBDIR} ${OCTAVE_OCTLIBDIR} NO_DEFAULT_PATH )
if( OCTAVE_CRUFT_LIBRARY )
  list( APPEND OCTAVE_LIBRARIES ${OCTAVE_CRUFT_LIBRARY} )
else()
  mark_as_advanced( OCTAVE_CRUFT_LIBRARY )
endif()

# Qt
find_package( Qt5 REQUIRED Core Widgets Gui Network )
find_package( Qt5 QUIET OPTIONAL_COMPONENTS DataVisualization)
add_definitions(${QT_DEFINITIONS})

# QtDataVisualisation
if(TARGET Qt5::DataVisualization )
  list( APPEND EXTRA_LIBS Qt5::DataVisualization )
  add_definitions( -DOCA_BUILD_3DPLOT )
else()
  message( STATUS "Qt5::DataVisualization not found" )
endif()

# libsamplerate
find_path( SAMPLERATE_INCLUDES samplerate.h )
find_library( SAMPLERATE_LIBRARY samplerate )
list( APPEND EXTRA_LIBS ${SAMPLERATE_LIBRARY} )

# portaudio
find_path( PORTAUDIO_INCLUDE_DIR portaudio.h "/usr/local/include" )
find_library( PORTAUDIO_LIBRARY portaudio "/usr/local/lib" )

# libpthread
find_library( PTHREAD_LIBRARY pthread )
if( PTHREAD_LIBRARY )
  list( APPEND EXTRA_LIBS ${PTHREAD_LIBRARY} )
endif()
mark_as_advanced( PTHREAD_LIBRARY )


# Options
if( "${CMAKE_BUILD_TYPE}" STREQUAL "" )
  set( CMAKE_BUILD_TYPE Release CACHE STRING "build type" FORCE )
endif()

if( NOT "${CMAKE_VERSION}" VERSION_LESS "2.8.11" )
  option( OCTAUDIO_USE_TIMESTAMP "insert build timestamp into octaudio version info" OFF )
else()
  set( OCTAUDIO_USE_TIMESTAMP OFF )
endif()

find_package( Git QUIET )
if( GIT_FOUND )
  option( OCTAUDIO_USE_GIT_REVISION "insert git revision into octaudio version info" OFF )
else()
  set( OCTAUDIO_USE_GIT_REVISION OFF )
endif()

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )

mark_as_advanced( CLEAR CMAKE_VERBOSE_MAKEFILE )

find_program( MARKED_EXECUTABLE marked )
find_program( SED_EXECUTABLE sed )
if( MARKED_EXECUTABLE AND SED_EXECUTABLE )
  option( OCTAUDIO_BUILD_HTMLDOC "build html documentation" ON )
else()
  set( OCTAUDIO_BUILD_HTMLDOC OFF )
endif()
mark_as_advanced( MARKED_EXECUTABLE SED_EXECUTABLE )

# Platform specific config
set( DOC_PREFIX "doc/octaudio/" )
set( SHARE_PREFIX "" )

if( APPLE )
  set( GUI_TYPE MACOSX_BUNDLE )
  find_library( CARBON_LIBRARY Carbon )
  find_library( APP_SERVICES_LIBRARY ApplicationServices )
  find_library( APP_KIT_LIBRARY AppKit )
  find_library( AUDIO_TOOLBOX_LIBRARY AudioToolbox )
  find_library( AUDIO_UNIT_LIBRARY AudioUnit )
  find_library( COREAUDIO_LIBRARY CoreAudio )

  list( APPEND  EXTRA_LIBS
                ${CARBON_LIBRARY}
                ${APP_KIT_LIBRARY}
                ${APP_SERVICES_LIBRARY}
                ${COREAUDIO_LIBRARY}
                ${AUDIO_TOOLBOX_LIBRARY}
                ${AUDIO_UNIT_LIBRARY}    )

  mark_as_advanced( CARBON_LIBRARY APP_SERVICES_LIBRARY APP_KIT_LIBRARY
                    AUDIO_TOOLBOX_LIBRARY  AUDIO_UNIT_LIBRARY COREAUDIO_LIBRARY )
  mark_as_advanced( CLEAR CMAKE_OSX_ARCHITECTURES )
  if( "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "" )
    set( CMAKE_OSX_ARCHITECTURES x86_64 CACHE STRING "traget arch" FORCE )
  endif()
  if( "${CMAKE_OSX_ARCHITECTURES}" MATCHES "i386" )
    set( OCTAUDIO_BUILD_TYPE "mac32" )
  else()
    set( OCTAUDIO_BUILD_TYPE "macosx" )
  endif()

  set( SHARE_PREFIX "octaudio.app/Contents/" )
  set( DOC_PREFIX "doc/" )

elseif( WIN32 )
  list( APPEND  EXTRA_LIBS setupapi winmm )
  if( "${CMAKE_CXX_COMPILER}" MATCHES "mingw" )
    set( OCTAUDIO_BUILD_TYPE "mingw32" )
  elseif( CYGWIN )
    # not tested yet
    set( OCTAUDIO_BUILD_TYPE "cygwin" )
  else()
    # not tested yet
    set( OCTAUDIO_BUILD_TYPE "win32" )
  endif()
  set( DOC_PREFIX "doc/" )

elseif( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
  find_library( ALSA_LIBRARY asound )
  if( ALSA_LIBRARY )
    list( APPEND  EXTRA_LIBS ${ALSA_LIBRARY} )
  endif()
  mark_as_advanced( ALSA_LIBRARY )

  if( "${CMAKE_LIBRARY_ARCHITECTURE}" MATCHES "x86_64" )
    set( OCTAUDIO_BUILD_TYPE "lnx64" )
  else()
    set( OCTAUDIO_BUILD_TYPE "lnx32" )
  endif()
  set( DOC_PREFIX "share/doc/octaudio/" )

else()
  set( OCTAUDIO_BUILD_TYPE "${CMAKE_SYSTEM_NAME}-${CMAKE_LIBRARY_ARCHITECTURE}" )
endif()


if( NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release" )
  set( OCTAUDIO_BUILD_TYPE "${OCTAUDIO_BUILD_TYPE}-${CMAKE_BUILD_TYPE}" )
endif()

string( TOLOWER ${OCTAUDIO_BUILD_TYPE} OCTAUDIO_BUILD_TYPE )


##
configure_file( cmake/octaudio_configinfo.h.in octaudio_configinfo.h )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${OCTAVE_INCLUDE_DIR}/.. ${OCTAVE_INCLUDE_DIR} ${OCTAVE_HDF5_INCLUDE_DIR}  )
include_directories( ${PORTAUDIO_INCLUDE_DIR} ${SAMPLERATE_INCLUDES} )

set( octaudio_SRC
  src/OcaDialogAbout.cpp
  src/OcaDialogPreferences.cpp
  src/OcaResampler.cpp
  src/OcaTrackDataBlock.cpp
  src/OcaDialogPropertiesSmartTrack.cpp
  src/OcaRingBuffer.cpp
  src/OcaPropProxyTrack.cpp
  src/OcaPropProxySubtrack.cpp
  src/OcaDialogPropertiesTrack.cpp
  src/OcaValidatorDouble.cpp
  src/OcaDialogPropertiesGroup.cpp
  src/OcaDialogPropertiesBase.cpp
  src/OcaTrackGroup.cpp
  src/OcaMonitorDock.cpp
  src/OcaApp.cpp
  src/OcaObjectListener.cpp
  src/OcaObject.cpp
  src/OcaListBase.cpp
  src/OcaWindowData.cpp
  src/OcaMonitorScreen.cpp
  src/OcaMonitor.cpp
  src/OcaScaleData.cpp
  src/OcaSmartScreen.cpp
  src/OcaTrackScreen.cpp
  src/OcaAudioController.cpp
  src/OcaTimeRuller.cpp
  src/OcaTrackGroupView.cpp
  src/OcaOctaveHost.cpp
  src/OcaOctaveController.cpp
  src/OcaPopupList.cpp
  src/OcaCommandEditor.cpp
  src/OcaConsoleLog.cpp
  src/OcaConsole.cpp
  src/OcaMainWindow.cpp
  src/OcaDataScreen.cpp
  src/OcaSmartTrack.cpp
  src/OcaTrack.cpp
  src/OcaTrackBase.cpp
  src/OcaScaleControl.cpp
  src/OcaInstance.cpp
  src/octaudio_main.cpp
  )

set( octaudio_HEADERS_MOC
  src/OcaDialogAbout.h
  src/OcaDialogPreferences.h
  src/OcaDialogPropertiesSmartTrack.h
  src/OcaPropProxyTrack.h
  src/OcaPropProxySubtrack.h
  src/OcaDialogPropertiesTrack.h
  src/OcaValidatorDouble.h
  src/OcaDialogPropertiesGroup.h
  src/OcaDialogPropertiesBase.h
  src/OcaMonitorDock.h
  src/OcaTrackGroup.h
  src/OcaObjectListener.h
  src/OcaInstance.h
  src/OcaObject.h
  src/OcaWindowData.h
  src/OcaApp.h
  src/OcaMonitorScreen.h
  src/OcaMonitor.h
  src/OcaSmartScreen.h
  src/OcaTrackScreen.h
  src/OcaAudioController.h
  src/OcaTimeRuller.h
  src/OcaTrackGroupView.h
  src/OcaOctaveHost.h
  src/OcaOctaveController.h
  src/OcaPopupList.h
  src/OcaCommandEditor.h
  src/OcaConsoleLog.h
  src/OcaConsole.h
  src/OcaMainWindow.h
  src/OcaDataScreen.h
  src/OcaSmartTrack.h
  src/OcaTrack.h
  src/OcaTrackBase.h
  src/OcaScaleControl.h
  )

if(TARGET Qt5::DataVisualization )
  list( APPEND octaudio_SRC
    src/OcaDialogProperties3DPlot.cpp
    src/Oca3DPlot.cpp
    src/Oca3DPlotDock.cpp
  )

  list( APPEND octaudio_HEADERS_MOC
    src/OcaDialogProperties3DPlot.h
    src/Oca3DPlot.h
    src/Oca3DPlotDock.h
  )
endif()

QT5_WRAP_CPP( octaudio_MOC ${octaudio_HEADERS_MOC} )

add_custom_command( OUTPUT octaudio_buildinfo.cpp
  COMMAND ${CMAKE_COMMAND}
  -D SRC_TREE=${CMAKE_CURRENT_SOURCE_DIR}
  -D USE_TIMESTAMP=${OCTAUDIO_USE_TIMESTAMP}
  -D USE_GIT=${OCTAUDIO_USE_GIT_REVISION}
  -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
  -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/buildinfo.cmake
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/buildinfo.cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake/octaudio_buildinfo.cpp.in  )

add_library( buildinfo OBJECT EXCLUDE_FROM_ALL octaudio_buildinfo.cpp )
list( APPEND octaudio_SRC $<TARGET_OBJECTS:buildinfo> )

add_executable( octaudio ${GUI_TYPE} ${octaudio_SRC} ${octaudio_MOC} )
target_link_libraries( octaudio ${OCTAVE_LIBRARIES} ${PORTAUDIO_LIBRARY} ${EXTRA_LIBS} Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Network)
set_target_properties( octaudio PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME Octaudio )

if( OCTAUDIO_USE_TIMESTAMP OR OCTAUDIO_USE_GIT_REVISION )
  add_custom_command( TARGET octaudio PRE_LINK COMMAND ${CMAKE_COMMAND} -E remove -f octaudio_buildinfo.cpp COMMAND ${CMAKE_MAKE_PROGRAM} buildinfo VERBATIM )
endif()

#if( APPLE )
#  if( "${CMAKE_BUILD_TYPE}" STREQUAL "Release" )
#    add_custom_command( TARGET octaudio POST_BUILD COMMAND macdeployqt octaudio.app -no-plugins )
#  endif()
#endif()

if( OCTAUDIO_BUILD_HTMLDOC )
  foreach( fpath "overview" "commands" "tour" "../README" )
    get_filename_component( fname ${fpath} NAME )
    add_custom_command( OUTPUT "htmldoc/${fname}.html"
      COMMAND cmake -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/htmldoc"
      COMMAND ${SED_EXECUTABLE} -e "'s/\\.md/.html/'" -e "'s!(doc/!(!'"
              "${CMAKE_CURRENT_SOURCE_DIR}/doc/${fpath}.md"
              | ${MARKED_EXECUTABLE} --gfm -o "${CMAKE_CURRENT_BINARY_DIR}/htmldoc/${fname}.html"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/doc/${fpath}.md" )
    list( APPEND htmldoc_files "${CMAKE_CURRENT_BINARY_DIR}/htmldoc/${fname}.html" )
  endforeach()
  add_custom_target( htmldoc ALL DEPENDS ${htmldoc_files} )
endif()

install( TARGETS "octaudio" RUNTIME DESTINATION "bin" BUNDLE DESTINATION "." )
install( DIRECTORY "scripts/utils" "scripts/core" DESTINATION "${SHARE_PREFIX}share/octaudio/m" FILES_MATCHING PATTERN "*.m" )
install( FILES "scripts/startup/octaudiorc" DESTINATION "${SHARE_PREFIX}share/octaudio/m/startup" )
install( FILES "doc/build.txt" "doc/shortcuts.txt" "COPYING" DESTINATION "${DOC_PREFIX}" )
if( OCTAUDIO_BUILD_HTMLDOC )
install( FILES ${htmldoc_files} DESTINATION "${DOC_PREFIX}" )
endif()

if( WIN32 )
  install( PROGRAMS "setup/octaudio.bat" DESTINATION "." )
  #install( FILES "${QT_BINARY_DIR}/QtCored4.dll" "${QT_BINARY_DIR}/QtGuid4.dll" DESTINATION "bin" CONFIGURATIONS "Debug" )
  #install( FILES "${QT_BINARY_DIR}/QtCore4.dll" "${QT_BINARY_DIR}/QtGui4.dll" DESTINATION "bin" CONFIGURATIONS "Release" )
  if( EXISTS "${QT_BINARY_DIR}/libsamplerate-0.dll" )
    install( FILES "${QT_BINARY_DIR}/libsamplerate-0.dll" DESTINATION "bin" )
  endif()
endif()

