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

#ifndef OcaTrackDataBlock_h
#define OcaTrackDataBlock_h

#include "octaudio.h"

#include <QList>
#include <QVector>

class OcaTrackDataBlock
{
  public:
    OcaTrackDataBlock();
    ~OcaTrackDataBlock();

  public:
    static long getAvailableDecimation( long decimation_hint );

  public:
    long getLength() const;
    long read( OcaDataVector* dst, long ofs, long len ) const;
    long write( const OcaDataVector* src, long ofs );
    long readAvg( OcaAvgVector* dst, long decimation, long ofs, long len ) const;

    bool split( long ofs, OcaTrackDataBlock* rem );
    long append( const OcaTrackDataBlock* block );

    bool validate() const;

  protected:
    int calcAvg( OcaAvgData* dst, const OcaDataVector* v, int ofs, int len );
    int calcAvg2( OcaAvgData* dst, const OcaAvgVector* v, int ofs, int len );
    void writeAvgChunks( long ofs, const OcaAvgVector* avg, int order );

  protected:
    long                              m_length;
    QList<OcaDataVector>              m_chunks;
    QVector< QList<OcaAvgVector> >    m_avgChunks;

  protected:
    static const int s_CHUNK_SIZE;
    static const int s_AVG_MAX_DEPTH;
    static const int s_AVG_FACTOR;

};

#endif // OcaTrackDataBlock_h
