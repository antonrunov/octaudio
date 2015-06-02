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

## oca_loadwavm( filename, [basename] )

function oca_loadwavm( filename, basename )
  [ x, fs ] = wavread( filename );
  if ! exist( "basename", "var" )
    [dir, name ] = fileparts( filename );
    basename = name;
  end
  c = size(x)(2);

  if 1 == c
    [idx, id ] = oca_track_add( basename, fs );
    oca_data_set( 0, x, id );
  else
    for i = 1:c
      [idx, id{i} ] = oca_track_add( sprintf( "%s-%d", basename, i ), fs );
      oca_data_set( 0, x(:,i), id{i} );
    end
  end
  if 2 == c
    oca_track_setprop( "stereo_pan", -1, id{1} );
    oca_track_setprop( "stereo_pan", 1, id{2} );
  end
endfunction
