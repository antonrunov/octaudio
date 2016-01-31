## Copyright 2013-2016 Anton Runov
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

if( USE_TIMESTAMP AND NOT "${CMAKE_VERSION}" VERSION_LESS "2.8.11" )
  string( TIMESTAMP BUILD_TIMESTAMP "\"%Y-%m-%d %H:%M:%S\"" )
else()
  set( BUILD_TIMESTAMP "0" )
endif()
message( "timestamp: ${BUILD_TIMESTAMP}" )

if( USE_GIT )
  execute_process( COMMAND ${GIT_EXECUTABLE} show -s --pretty=format:%H WORKING_DIRECTORY ${SRC_TREE} OUTPUT_VARIABLE src_revision )
  execute_process( COMMAND ${GIT_EXECUTABLE} status --porcelain -uno WORKING_DIRECTORY ${SRC_TREE} OUTPUT_VARIABLE src_status )
  #message( "rev: ${src_revision}" )
  #message( "status: ${src_status}" )
  if( "${src_revision}" STREQUAL "" )
    set( BUILD_REVISION "0" )
  elseif( "${src_status}" STREQUAL "" )
    set( BUILD_REVISION "\"${src_revision}\"" )
  else()
    set( BUILD_REVISION "\"M\"" )
  endif()
else()
  set( BUILD_REVISION "0" )
endif()

message( "git revision: ${BUILD_REVISION}" )

configure_file( ${SRC_TREE}/cmake/octaudio_buildinfo.cpp.in octaudio_buildinfo.cpp )
