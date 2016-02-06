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

## oca_loadwav( filename, [track], [mono] )

function oca_loadwav( filename, track=[], mono=false )
  fs = nthargout( 2, @wavread, filename, 0 );
  [len, ch] = wavread( filename, 'size' );
  if mono
    ch = 1;
  end

  if ! isempty(track)
    id = oca_track_find( track );
    if 0 == id
      error( "invalid track" );
    end
    oca_data_clear( id );
    oca_track_setprop( "rate", fs, id );
  else
    [dir, name ] = fileparts( filename );
    id = oca_track_add( name, fs );
  end
  oca_track_setprop( "channels", ch, id );

  t = 0;
  res = 0;
  block_sz = fs*10;

  while res < len
    x = wavread( filename, res + [1,block_sz] )';
    if mono
      x = mean( x, 1 );
    end
    assert( ! isempty(x) );
    assert( size( x, 1 ) == ch );
    t = oca_data_set( t, x, id );
    res += size( x, 2 );
  end

endfunction
