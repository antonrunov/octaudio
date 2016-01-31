/*
   Copyright 2013-2016 Anton Runov

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

#ifndef OcaResampler_h
#define OcaResampler_h

#include "OcaTrack.h"

#include <samplerate.h>

// -----------------------------------------------------------------------------

class OcaResampler
{
  public:
    OcaResampler();
    ~OcaResampler();

  public:
    bool init( int channels );
    int resample( const OcaFloatVector* in, OcaFloatVector* out, double ratio, int ofs );

  protected:
    double      m_posSrc;
    double      m_posDst;
    SRC_STATE*  m_resampler;
    int         m_channels;
};

// -----------------------------------------------------------------------------

class OcaTrackWriter : public OcaResampler
{
  public:
    OcaTrackWriter( OcaTrack* track ) : m_track( track ) {}

  public:
    void  write( const OcaFloatVector* src, double t, double rate );

  protected:
    OcaTrack*   m_track;
};

// -----------------------------------------------------------------------------

class OcaTrackReader : public OcaResampler
{
  public:
    OcaTrackReader( const OcaTrack* track ) : m_track( track ) {}

  public:
    void  read( OcaFloatVector* dst, double t, long len, double rate );

  protected:
    const OcaTrack*   m_track;
};

#endif // OcaResampler_h
