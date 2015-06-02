/*
   Copyright 2013-2015 Anton Runov

   This file is part of Octaudio.

   Octaudio is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Octaudio is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Octaudio.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef octaudio_h
#define octaudio_h

#include <QVarLengthArray>

typedef unsigned long   oca_ulong;
typedef long            oca_index;

struct OcaAvgData {
  double min;
  double max;
  double avg;
  double var;
};

typedef QVarLengthArray<double,0>     OcaDataVector;
typedef QVarLengthArray<OcaAvgData,0> OcaAvgVector;
typedef QVarLengthArray<float,0>      OcaFloatVector;

#endif // octaudio_h
