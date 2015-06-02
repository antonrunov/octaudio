## Copyright 2013-2015 Anton Runov
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

## oca_loadwav( filename, [track] )

function oca_loadwav( filename, track )
  [ x, fs ] = wavread( filename );
  x = mean( x, 2 );
  if exist( "track", "var" )
    id = oca_track_find( track );
    if 0 == id
      error( "invalid track" );
    end
    oca_data_clear( id );
    oca_track_setprop( "rate", fs, id );
    oca_data_set( 0, x, id );
  else
    [dir, name ] = fileparts( filename );
    [idx, id ] = oca_track_add( name, fs );
    oca_data_set( 0, x, id );
  end
endfunction
