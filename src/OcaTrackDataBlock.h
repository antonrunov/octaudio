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

#ifndef OcaTrackDataBlock_h
#define OcaTrackDataBlock_h

#include "OcaDataVector.h"

#include <QList>
#include <QStringList>
#include <QDir>

class OcaTrackDataBlock
{
  public:
    OcaTrackDataBlock( int channels );
    ~OcaTrackDataBlock();

  public:
    static long getAvailableDecimation( long decimation_hint );

  public:
    qint64 getLength() const;
    int  getChannels() const { return m_channels; }
    long read( OcaDataVector* dst, qint64 ofs, long len ) const;
    long write( const OcaDataVector* src, qint64 ofs, long len_max = 0 );
    long readAvg( OcaAvgVector* dst, long decimation, qint64 ofs, long len ) const;

    bool split( qint64 ofs, OcaTrackDataBlock* rem );
    qint64 append( const OcaTrackDataBlock* block );

    bool validate() const;

  protected:
    int calcAvg( OcaAvgData* dst, const OcaDataVector* v, long ofs, long len );
    int calcAvg2( OcaAvgData* dst, const OcaAvgVector* v, long ofs, long len );
    void writeAvgChunks( qint64 ofs, const OcaAvgVector* avg, int order, bool truncate );

  protected:
    int              m_channels;
    qint64           m_length;
    QStringList      m_files;
    QDir             m_dataDir;

  protected:
    static const int s_AVG_FACTOR;
    static volatile int s_counter;

};

#endif // OcaTrackDataBlock_h
