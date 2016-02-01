## Copyright 2013-2016 Anton Runov
## Copyright 2005-2013 Michael Zeising
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

## oca_savewav( filename, track=0 )

function oca_savewav( filename, track=0 )
  track = oca_track_find( track );
  if 0 == track
    error( "invalid track" );
  end
  fs = oca_track_getprop( "rate", track );
  blocks = oca_data_listblocks( "all", track );
  if isempty( blocks )
    error( "empty track" );
  end
  t0 = blocks(1,1);
  dt = blocks(2,1);
  ch = oca_track_getprop( "channels", track );

  # the rest of the code is based on the old wavwrite implementation
  fmt = "int16";
  ck_size = round( dt * fs * ch ) * 2;

  BYTEORDER = "ieee-le";
  if (! ischar (filename))
    error ("oca_savewav: expecting FILENAME to be a character string");
  endif

  ## open file for writing binary
  [fid, msg] = fopen (filename, "wb");
  if (fid < 0)
    error ("oca_savewav: %s", msg);
  endif
  cleanup = onCleanup( @() fclose(fid) );

  ## write RIFF/WAVE header
  c = 0;
  c += fwrite (fid, "RIFF", "uchar");

  ## file size - 8
  c += fwrite (fid, ck_size + 36, "uint32", 0, BYTEORDER);
  c += fwrite (fid, "WAVEfmt ", "uchar");

  ## size of fmt chunk
  c += fwrite (fid, 16, "uint32", 0, BYTEORDER);

  ## sample format code (PCM)
  c += fwrite (fid, 1, "uint16", 0, BYTEORDER);

  ## channels
  c += fwrite (fid, ch, "uint16", 0, BYTEORDER);

  ## sample rate
  c += fwrite (fid, fs, "uint32", 0, BYTEORDER);

  ## bytes per second
  byteps = fs * ch * 2;
  c += fwrite (fid, byteps, "uint32", 0, BYTEORDER);

  ## block align
  c += fwrite (fid, ch*2, "uint16", 0, BYTEORDER);

  c += fwrite (fid, 16, "uint16", 0, BYTEORDER);
  c += fwrite (fid, "data", "uchar");
  c += fwrite (fid, ck_size, "uint32", 0, BYTEORDER);

  if (c < 25)
    error ("oca_savewav: writing to file failed");
  endif

  ## write data
  t_end = t0 + dt;
  while t0 < t_end
    [x,t0] = oca_data_get( [t0,10], track );
    if isempty(x)
      break;
    end
    assert( size(x,1) == ch );
    len = size(x,2);
    x = round( vec(x) * 32768 );
    fwrite( fid, x, fmt, 0, BYTEORDER);
    t0 += len / fs;
  end

endfunction
