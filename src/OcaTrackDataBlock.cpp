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

#include "OcaTrackDataBlock.h"
#include "OcaApp.h"

#include <QtCore>

const int OcaTrackDataBlock::s_AVG_FACTOR = 32;
volatile int OcaTrackDataBlock::s_counter = 0;

// ------------------------------------------------------------------------------------

OcaTrackDataBlock::OcaTrackDataBlock( int channels )
:
  m_channels( channels ),
  m_length( 0 )
{
  Q_ASSERT( 0 < m_channels );
  m_dataDir = OcaApp::getDataCacheDir();

  QString name = QString( "%1.bin" ) . arg( s_counter++, 6, 16, QLatin1Char('0') );
  m_files.append( m_dataDir.filePath( name ) );
}

// ------------------------------------------------------------------------------------

OcaTrackDataBlock::~OcaTrackDataBlock()
{
  for( int i = 0; i < m_files.size(); i++ ) {
    QFile::remove( m_files[i] );
  }
  m_dataDir.rmdir( m_dataDir.path() );
}

// ------------------------------------------------------------------------------------

long OcaTrackDataBlock::getAvailableDecimation( long decimation_hint )
{
  if( 2 > decimation_hint ) {
    return 1;
  }
  int k = floor( log2(decimation_hint) / log2(s_AVG_FACTOR) );
  return pow( s_AVG_FACTOR, k );
}

// ------------------------------------------------------------------------------------

qint64 OcaTrackDataBlock::getLength() const
{
  return m_length;
}

// ------------------------------------------------------------------------------------

long OcaTrackDataBlock::read( OcaDataVector* dst, qint64 ofs, long len ) const
{
  len = qMin( (qint64)len, m_length - ofs );
  if( ( 0 >= len ) || ( 0 > ofs ) ) {
    return 0;
  }
  dst->alloc( m_channels, len );
  double* dst_v = dst->data();
  long result = 0;
  QFile f( m_files[0] );
  f.open( QIODevice::ReadWrite );
  int K = m_channels * sizeof(double);
  f.seek( ofs * K );
  result = f.read( (char*)dst_v, len * K ) / K;
  f.close();
  Q_ASSERT( result == len );
  return result;
}

// ------------------------------------------------------------------------------------

int OcaTrackDataBlock::calcAvg( OcaAvgData* dst, const OcaDataVector* src, long ofs, long len )
{
  int result = 0;
  const double* v0 = src->constData() + ofs * m_channels;
  const double* v_max = src->constData() + ( ofs + len ) * m_channels;
  const double* v_max2 = src->constData() + ( src->length() - s_AVG_FACTOR ) * m_channels;


  for( int c = 0; c < m_channels; c++ ) {

    const double* v = v0++;
    int M = s_AVG_FACTOR;
    result = 0;
    OcaAvgData* d = dst++;

    while( v < v_max ) {
      double min = *v;
      double max = *v;
      double sum = *v;
      double sumsq = (*v) * (*v);
      if( v > v_max2 ) {
        M = ( v_max2 - v ) / m_channels + s_AVG_FACTOR ;
        Q_ASSERT( 0 < M );
      }
      v += m_channels;
      for( int i = 1; i < M; i++ ) {
        min = qMin( min, *v );
        max = qMax( max, *v );
        sum += *v;
        sumsq += (*v) * (*v);
        v += m_channels;
      }
      d->min = min;
      d->max = max;
      d->avg = sum / M;
      d->var = sumsq / M - pow( d->avg, 2 );
      d += m_channels;
      result++;
    }

  }

  return result;
}

// ------------------------------------------------------------------------------------

int OcaTrackDataBlock::calcAvg2( OcaAvgData* dst, const OcaAvgVector* src, long ofs, long len )
{
  int result = 0;
  const OcaAvgData* v0 = src->constData() + ofs * m_channels;
  const OcaAvgData* v_max = src->constData() + ( ofs + len ) * m_channels;
  const OcaAvgData* v_max2 = src->constData() + ( src->length() - s_AVG_FACTOR ) * m_channels;

  for( int c = 0; c < m_channels; c++ ) {

    const OcaAvgData* v = v0++;
    int M = s_AVG_FACTOR;
    result = 0;
    OcaAvgData* d = dst++;

    while( v < v_max ) {
      double min = v->min;
      double max = v->max;
      double sum = v->avg;
      double sumsq = sum*sum + v->var;
      if( v > v_max2 ) {
        M = ( v_max2 - v ) / m_channels + s_AVG_FACTOR;
        Q_ASSERT( 0 < M );
      }
      v += m_channels;
      for( int i = 1; i < M; i++ ) {
        min = qMin( min, v->min );
        max = qMax( max, v->max );
        sum += v->avg;
        sumsq += pow( v->avg, 2 ) + v->var;
        v += m_channels;
      }
      d->min = min;
      d->max = max;
      d->avg = sum / M;
      d->var = sumsq / M - pow( d->avg, 2 );
      d += m_channels;
      result++;
    }

  }

  return result;
}

// ------------------------------------------------------------------------------------

long OcaTrackDataBlock::write( const OcaDataVector* src, qint64 ofs, long len_max /* = 0 */ )
{
  if( ( ofs > m_length ) || ( 0 > ofs ) ) {
    return 0;
  }
  if( src->channels() != m_channels ) {
    return 0;
  }
  long len = src->length();
  if( 0 < len_max ) {
    len = qMin( len, len_max );
  }
  qint64 avg_ofs = ofs / s_AVG_FACTOR;
  long avg_len = (ofs + len - 1 ) / s_AVG_FACTOR + 1 - avg_ofs;
  OcaAvgVector avg( m_channels, avg_len );
  long avg_idx = 0;

  long result = 0;
  const double* src_v = src->constData();

  QFile f( m_files[0] );
  f.open( QIODevice::ReadWrite );
  int K = m_channels * sizeof(double);
  int i0 = ofs % s_AVG_FACTOR;
  f.seek( ( ofs - i0 ) * K );
  OcaDataVector tmp( m_channels, s_AVG_FACTOR );
  if( 0 != i0 ) {
    f.read( (char*)tmp.data(), i0 * K );
  }
  result = f.write( (char*)src_v, len * K ) / K;
  Q_ASSERT( result == len );
  f.close();

  if( 0 != i0 ) {
    int tmp_len = qMin( i0 + len, (long)s_AVG_FACTOR );
    memcpy( tmp.data() + i0 * m_channels, src_v, (tmp_len - i0) * K );
    tmp.setLength( tmp_len );
    avg_idx = calcAvg( avg.data(), &tmp, 0, tmp_len );
    i0 = s_AVG_FACTOR - i0;
  }
  if( avg_idx < avg_len ) {
    avg_idx += calcAvg( avg.data() + avg_idx * m_channels, src, i0, result - i0 );
  }
  Q_ASSERT( avg_idx == avg.length() );

  m_length = qMax( m_length, ofs + result );
  writeAvgChunks( avg_ofs, &avg, 1, false );
  return result;
}

// ------------------------------------------------------------------------------------

void OcaTrackDataBlock::writeAvgChunks( qint64 ofs, const OcaAvgVector* avg, int order, bool truncate )
{
  long len = avg->length();
  if( m_files.size() <= order ) {
    QString name = QString( "%1.bin" ) . arg( s_counter++, 6, 16, QLatin1Char('0') );
    Q_ASSERT( m_files.size() == order );
    m_files.append( m_dataDir.filePath( name ) );
  }

  qint64 avg_ofs = ofs / s_AVG_FACTOR;
  long avg_len = (ofs + len - 1 ) / s_AVG_FACTOR + 1 - avg_ofs;
  OcaAvgVector avg2( m_channels, avg_len );
  long avg_idx = 0;

  const OcaAvgData* v = avg->constData();

  QFile f( m_files[order] );
  f.open( QIODevice::ReadWrite );
  int K = m_channels * sizeof(OcaAvgData);
  int i0 = ofs % s_AVG_FACTOR;
  f.seek( ( ofs - i0 ) * K );
  OcaAvgVector tmp( m_channels, s_AVG_FACTOR );
  if( 0 != i0 ) {
    f.read( (char*)tmp.data(), i0 * K );
  }
  f.write( (char*)v, len * K );
  f.close();

  order++;

  if( 1 < ofs + len ) {
    if( 0 != i0 ) {
      int tmp_len = qMin( i0 + len, (long)s_AVG_FACTOR );
      memcpy( tmp.data() + i0 * m_channels, v, (tmp_len - i0) * K );
      tmp.setLength( tmp_len );
      avg_idx = calcAvg2( avg2.data(), &tmp, 0, tmp_len );
      i0 = s_AVG_FACTOR - i0;
    }
    if( avg_idx < avg_len ) {
      avg_idx += calcAvg2( avg2.data() + avg_idx * m_channels, avg, i0, len - i0 );
    }

    if( truncate ) {
      f.resize( (ofs + len) * m_channels * sizeof(OcaAvgData) );
    }

    writeAvgChunks( avg_ofs, &avg2, order, truncate );
  }
  else if( truncate ) {
    f.resize( m_channels * sizeof(OcaAvgData) );
    while( m_files.size() > order ) {
      QFile::remove( m_files.takeLast() );
    }
  }
}

// ------------------------------------------------------------------------------------

long OcaTrackDataBlock::readAvg( OcaAvgVector* dst, long decimation,
                                                    qint64 ofs, long len ) const
{
  int k = floor( log2(decimation) / log2(s_AVG_FACTOR) );
  if( ( 0 == m_length ) || ( decimation != pow( s_AVG_FACTOR, k ) ) ) {
    return 0;
  }
  k = qMin( k, m_files.size() - 1 );
  long result = 0;

  len = qMin( (qint64)len, ( m_length - ofs + decimation - 1 ) / decimation );
  if( 0 < k ) {
    qint64 idx0 = ofs / decimation;
    dst->alloc( m_channels, len );
    OcaAvgData* dst_v = dst->data();

    QFile f( m_files[k] );
    f.open( QIODevice::ReadWrite );
    int K = m_channels * sizeof(OcaAvgData);
    f.seek( idx0 * K );
    result = f.read( (char*)dst_v, len * K ) / K;
    //Q_ASSERT( result == len );
    f.close();
  }
  else {
    OcaDataVector d( m_channels, len );

    QFile f( m_files[0] );
    f.open( QIODevice::ReadWrite );
    int K = m_channels * sizeof(double);
    f.seek( ofs * K );
    result = f.read( (char*)d.data(), len * K ) / K;
    Q_ASSERT( result == len );
    f.close();

    dst->alloc( m_channels, len );
    OcaAvgData* dst_v = dst->data();
    const double* src_v = d.constData();
    OcaAvgData* dst_max = dst_v + result * m_channels;
    while( dst_v < dst_max ) {
      double tmp = *(src_v++);
      dst_v->min = tmp;
      dst_v->max = tmp;
      dst_v->avg = tmp;
      dst_v->var = 0;
      dst_v++;
    }
  }
  return result;
}

// ------------------------------------------------------------------------------------

bool OcaTrackDataBlock::split( qint64 ofs, OcaTrackDataBlock* rem )
{
  if( ( ofs >= m_length ) || ( 0 >= ofs ) ) {
    return false;
  }

  if( NULL != rem ) {
    const long BS = 4096 * 128;
    OcaDataVector buffer;

    long len = m_length - ofs;
    long pos = ofs;
    long pos2 = 0;
    while( 0 < len ) {
      long n = read( &buffer, pos, qMin( len, BS ) );
      len -= n;
      pos += n;
      long n2 = rem->write( &buffer, pos2 );
      Q_ASSERT( n == n2 );
      pos2 += n2;
    }
  }

  QFile f( m_files[0] );
  int K = m_channels * sizeof(double);
  f.resize( K * ofs );
  m_length = ofs;

  qint64 avg_ofs = ofs / s_AVG_FACTOR;
  int i0 = m_length % s_AVG_FACTOR;
  OcaDataVector tmp( m_channels, s_AVG_FACTOR - i0 );
  read( &tmp, m_length - s_AVG_FACTOR + i0, s_AVG_FACTOR - i0 );
  OcaAvgVector avg( m_channels, 1 );
  calcAvg( avg.data(), &tmp, 0, 1 );
  writeAvgChunks( avg_ofs, &avg, 1, true );

  return true;
}

// ------------------------------------------------------------------------------------

qint64 OcaTrackDataBlock::append( const OcaTrackDataBlock* block )
{
  const long BS = 4096 * 128;
  OcaDataVector buffer;

  long len = block->getLength();
  long ofs = 0;
  while( 0 < len ) {
    long n = block->read( &buffer, ofs, qMin( len, BS ) );
    len -= n;
    ofs += n;
    write( &buffer, m_length );
  }
  Q_ASSERT( 0 == len );
  return m_length;
}

// ------------------------------------------------------------------------------------

bool OcaTrackDataBlock::validate() const
{
  return true;
}

// ------------------------------------------------------------------------------------

