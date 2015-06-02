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

# [ val, ret ] = oca_track_getcontext( field, [id], [group_id] )
# [ val, ret ] = oca_track_getcontext( { field, default_value }, [id], [group_id] )

function [ val, ret ] = oca_track_getcontext( field, id=0, group_id=0 )
  if ischar( field )
    [ val, ret ] = oca_context_get( oca_track_find( id, group_id ), field );
  elseif iscell( field )
    [ val, ret ] = oca_context_get( oca_track_find( id, group_id ), field{} );
  end
endfunction
