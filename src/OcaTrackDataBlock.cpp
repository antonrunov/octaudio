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

#include "OcaTrackDataBlock.h"

#include <QtCore>

const int OcaTrackDataBlock::s_CHUNK_SIZE = 16384;
const int OcaTrackDataBlock::s_AVG_MAX_DEPTH = 16;
const int OcaTrackDataBlock::s_AVG_FACTOR = 8;

// ------------------------------------------------------------------------------------

OcaTrackDataBlock::OcaTrackDataBlock( int channels )
:
  m_channels( channels ),
  m_length( 0 )
{
  Q_ASSERT( 0 < m_channels );
  m_avgChunks.resize( s_AVG_MAX_DEPTH );
}

// ------------------------------------------------------------------------------------

OcaTrackDataBlock::~OcaTrackDataBlock()
{
}

// ------------------------------------------------------------------------------------

long OcaTrackDataBlock::getAvailableDecimation( long decimation_hint )
{
  if( 2 > decimation_hint ) {
    return 1;
  }
  int k = floor( log2(decimation_hint) / log2(s_AVG_FACTOR) );
  return pow( s_AVG_FACTOR, qMin( k, s_AVG_MAX_DEPTH ) );
}

// ------------------------------------------------------------------------------------

long OcaTrackDataBlock::getLength() const
{
  return m_length;
}

// ------------------------------------------------------------------------------------

long OcaTrackDataBlock::read( OcaDataVector* dst, long ofs, long len ) const
{
  len = qMin( len, m_length - ofs );
  if( ( 0 >= len ) || ( 0 > ofs ) ) {
    return 0;
  }
  dst->alloc( m_channels, len );
  double* dst_v = dst->data();
  int block_idx = ofs / s_CHUNK_SIZE;
  int i0 = ofs - block_idx * s_CHUNK_SIZE;
  long result = 0;
  while( result < len ) {
    Q_ASSERT( m_chunks.size() > block_idx );
    const OcaDataVector* v = &m_chunks.at( block_idx++ );
    int l = qMin( len-result, v->length() - i0 );
    Q_ASSERT( 0 < l );
    Q_ASSERT( s_CHUNK_SIZE >= l + i0 );
    memcpy( dst_v, v->constData() + i0 * m_channels, l * m_channels * sizeof(double) );
    result += l;
    dst_v += l * m_channels;
    i0 = 0;
  }
  return result;
}

// ------------------------------------------------------------------------------------

int OcaTrackDataBlock::calcAvg( OcaAvgData* dst, const OcaDataVector* src, int ofs, int len )
{
  int idx0 = ofs / s_AVG_FACTOR;
  int result = 0;
  const double* v0 = src->constData() + idx0 * s_AVG_FACTOR * m_channels;
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

int OcaTrackDataBlock::calcAvg2( OcaAvgData* dst, const OcaAvgVector* src, int ofs, int len )
{
  int idx0 = ofs / s_AVG_FACTOR;
  int result = 0;
  const OcaAvgData* v0 = src->constData() + idx0 * s_AVG_FACTOR * m_channels;
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

long OcaTrackDataBlock::write( const OcaDataVector* src, long ofs, long len_max /* = 0 */ )
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
  int block_idx = ofs / s_CHUNK_SIZE;
  int block_idx_end = ( ofs + len - 1 ) / s_CHUNK_SIZE;
  m_chunks.reserve( block_idx_end + 1 );
  for( int i = m_chunks.size(); i <= block_idx_end; i++ ) {
    m_chunks.append( OcaDataVector() );
    m_chunks[i].alloc( m_channels, 0, s_CHUNK_SIZE );
  }
  int avg_ofs = ofs / s_AVG_FACTOR;
  int avg_len = (ofs + len - 1 ) / s_AVG_FACTOR + 1 - avg_ofs;
  OcaAvgVector avg( m_channels, avg_len );
  int avg_idx = 0;

  int i0 = ofs - block_idx * s_CHUNK_SIZE;
  long result = 0;
  const double* src_v = src->constData();
  while( result < len ) {
    Q_ASSERT( m_chunks.size() > block_idx );
    OcaDataVector* v = &m_chunks[ block_idx ];
    int l = qMin( (int)(len-result), s_CHUNK_SIZE - i0 );
    Q_ASSERT( 0 < l );
    Q_ASSERT( i0 + l <= s_CHUNK_SIZE );
    if( v->length() < i0 + l ) {
      v->setLength( i0 + l );
    }
    memcpy( v->data() + i0 * m_channels, src_v, l * sizeof(double) * m_channels );
    avg_idx += calcAvg( avg.data() + avg_idx * m_channels, v, i0, l );
    Q_ASSERT( avg_idx <= avg.length() );
    result += l;
    src_v += l * m_channels;
    i0 = 0;
    block_idx++;
  }
  m_length = qMax( m_length, ofs + result );
  writeAvgChunks( avg_ofs, &avg, 0, false );
  return result;
}

// ------------------------------------------------------------------------------------

void OcaTrackDataBlock::writeAvgChunks( long ofs, const OcaAvgVector* avg, int order, bool truncate )
{
  QList<OcaAvgVector>& dst = m_avgChunks[ order ];
  int block_idx = ofs / s_CHUNK_SIZE;
  int idx0 = ofs - block_idx * s_CHUNK_SIZE;
  int len = avg->length();

  int block_idx_end = ( ofs + len - 1 ) / s_CHUNK_SIZE;
  dst.reserve( block_idx_end + 1 );
  for( int i = dst.size(); i <= block_idx_end; i++ ) {
    dst.append( OcaAvgVector() );
    dst[i].alloc( m_channels, 0, s_CHUNK_SIZE );
  }

  int avg_ofs = ofs / s_AVG_FACTOR;
  int avg_len = (ofs + len - 1 ) / s_AVG_FACTOR + 1 - avg_ofs;
  OcaAvgVector avg2( m_channels, avg_len );
  int avg_idx = 0;

  order++;
  const OcaAvgData* v = avg->constData();
  int last_size = 0;
  while( 0 < len ) {
    OcaAvgVector* dst_v = &dst[block_idx];
    int l = qMin( len, s_CHUNK_SIZE - idx0 );
    if( dst_v->length() < idx0 + l ) {
      dst_v->setLength( idx0 + l );
    }
    last_size = idx0 + l;
    memcpy( dst_v->data() + idx0 * m_channels, v, l * sizeof(OcaAvgData) * m_channels );
    if( order < s_AVG_MAX_DEPTH ) {
      avg_idx += calcAvg2( avg2.data() + avg_idx * m_channels, dst_v, idx0, l );
      Q_ASSERT( avg_idx <= avg2.length() );
    }

    len -= l;
    v += l * m_channels;
    block_idx++;
    idx0 = 0;
  }


  if( truncate ) {
    while( dst.size() > block_idx_end + 1 ) {
      dst.removeLast();
    }
    if( 0 < last_size ) {
      dst[block_idx_end].setLength( last_size );
    }
  }

  if( order < s_AVG_MAX_DEPTH ) {
    writeAvgChunks( avg_ofs, &avg2, order, truncate );
  }
}

// ------------------------------------------------------------------------------------

long OcaTrackDataBlock::readAvg( OcaAvgVector* dst, long decimation,
                                                    long ofs, long len ) const
{
  int k = floor( log2(decimation) / log2(s_AVG_FACTOR) );
  if( ( s_AVG_MAX_DEPTH < k ) ||
      ( decimation != pow( s_AVG_FACTOR, qMin( k, s_AVG_MAX_DEPTH ) ) ) ) {
    return 0;
  }
  long result = 0;

  len = qMin( len, m_length - ofs );
  if( 0 < k ) {
    QList<OcaAvgVector> chunks = m_avgChunks.at( k - 1 );

    int idx0 = qRound( ofs / decimation );
    int idx1 = qRound( ( ofs + len - 1 ) / decimation );
    len = idx1 - idx0 + 1;
    dst->alloc( m_channels, len );
    OcaAvgData* dst_v = dst->data();
    int block_idx = idx0 / s_CHUNK_SIZE;
    int i0 = idx0 - block_idx * s_CHUNK_SIZE;
    while( result < len ) {
      const OcaAvgVector* v = &chunks.at( block_idx++ );
      int l = qMin( len-result, v->length() - i0 );
      Q_ASSERT( 0 < l );
      memcpy( dst_v, v->constData() + i0 * m_channels, l * sizeof(OcaAvgData) * m_channels );
      result += l;
      dst_v += l * m_channels;
      i0 = 0;
    }
  }
  else {
    dst->alloc( m_channels, len );
    OcaAvgData* dst_v = dst->data();
    int block_idx = ofs / s_CHUNK_SIZE;
    int i0 = ofs - block_idx * s_CHUNK_SIZE;
    while( result < len ) {
      const OcaDataVector* v = &m_chunks.at( block_idx++ );
      int l = qMin( len-result, v->length() - i0 );
      Q_ASSERT( 0 < l );
      const double* src_v = v->constData() + i0 * m_channels;
      for( int i=0; i < l * m_channels; i++ ) {
        double tmp = *(src_v++);
        dst_v->min = tmp;
        dst_v->max = tmp;
        dst_v->avg = tmp;
        dst_v->var = 0;
        dst_v++;
      }
      result += l;
      i0 = 0;
    }
  }
  return result;
}

// ------------------------------------------------------------------------------------

bool OcaTrackDataBlock::split( long ofs, OcaTrackDataBlock* rem )
{
  if( ( ofs >= m_length ) || ( 0 >= ofs ) ) {
    return false;
  }
  int block_idx = ofs / s_CHUNK_SIZE;
  int idx0 = ofs - block_idx * s_CHUNK_SIZE;
  Q_ASSERT( m_chunks.size() > block_idx );
  Q_ASSERT( idx0 < s_CHUNK_SIZE );

  if( NULL != rem ) {
    int idx_new = block_idx;
    int rem_ofs = 0;
    if( 0 < idx0 ) {
      const OcaDataVector* v = &m_chunks.at( idx_new++ );
      OcaDataVector tmp( m_channels, v->length() - idx0 );
      memcpy( tmp.data(), v->constData()+idx0*m_channels, tmp.length()*m_channels*sizeof(double) );
      rem_ofs = rem->write( &tmp, 0 );
    }
    while( m_chunks.size() > idx_new ) {
      rem_ofs += rem->write( &m_chunks.at(idx_new++), rem_ofs );
    }
  }

  if( 0 < idx0 ) {
    m_chunks[ block_idx ].setLength( idx0 );
    block_idx++;
  }
  while( m_chunks.size() > block_idx ) {
    m_chunks.removeLast();
  }

  block_idx = m_chunks.size() - 1;
  OcaDataVector* v = &m_chunks[ block_idx ];
  Q_ASSERT( block_idx * s_CHUNK_SIZE + v->length() == ofs );
  m_length = ofs;

  int avg_ofs = ofs / s_AVG_FACTOR;
  int i0 = m_length - block_idx * s_CHUNK_SIZE;
  OcaAvgVector avg( m_channels, 1 );
  calcAvg( avg.data(), v, i0-1, 1 );
  writeAvgChunks( avg_ofs, &avg, 0, true );

  return true;
}

// ------------------------------------------------------------------------------------

long OcaTrackDataBlock::append( const OcaTrackDataBlock* block )
{
  const long BS = s_CHUNK_SIZE * 128;
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

