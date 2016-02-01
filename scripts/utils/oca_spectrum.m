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

## oca_spectrum( w=[], track=0, limit=-120, linfreq=false )

function oca_spectrum( w=[], track=0, limit=-120, linfreq=false )
  sr = oca_track_getprop( "rate", track );
  [ sp, freq ] = pwelch( mean( oca_data_get( "region", track ), 1 ), w, [], [], sr );
  if linfreq
    plot( freq, max( limit, 10*log10( sp ) ) );
  else
    semilogx( freq, max( limit, 10*log10( sp ) ) );
  end
endfunction
