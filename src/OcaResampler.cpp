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

#include "OcaResampler.h"

#include <QtCore>

// -----------------------------------------------------------------------------

static void float_to_double( const OcaFloatVector* src, OcaDataVector* dst, int channels )
{
  dst->alloc( channels, src->length() );
  const float* p_src = src->constData();
  double* p_dst = dst->data();
  double* max_dst = p_dst + dst->length() * dst->channels();
  if( dst->channels() == src->channels() ) {
    while( p_dst < max_dst ) {
      *p_dst++ = *p_src++;
    }
  }
  else if( dst->channels() > src->channels() ) {
    memset( p_dst, 0, sizeof(double) * dst->channels() * dst->length() );
    int d = dst->channels() - src->channels();
    while( p_dst < max_dst ) {
      double* max_tmp = p_dst + src->channels();
      while( p_dst < max_tmp ) {
        *p_dst++ = *p_src++;
      }
      p_dst += d;
    }
  }
  else {
    Q_ASSERT( false );
  }
}

// -----------------------------------------------------------------------------

static void double_to_float( const OcaDataVector* src,  OcaFloatVector* dst,
                                                                    int ofs, int skip)
{
  dst->alloc( src->channels(), src->length() + ofs - skip );
  const double* p_src = src->constData() + skip * src->channels();
  float* p_dst = dst->data();
  float* max_dst = p_dst + dst->length() * dst->channels();
  if( 0 < ofs ) {
    memset( p_dst, 0, sizeof(float) * ofs * dst->channels() );
    p_dst += ofs * dst->channels();
  }
  while( p_dst < max_dst ) {
    *p_dst++ = *p_src++;
  }
}

// -----------------------------------------------------------------------------

OcaResampler::OcaResampler()
:
  m_posSrc( NAN ),
  m_posDst( NAN ),
  m_resampler( NULL ),
  m_channels( 0 )
{
}

// -----------------------------------------------------------------------------

bool OcaResampler::init( int channels )
{
  if( NULL != m_resampler ) {
    m_resampler = src_delete( m_resampler );
    Q_ASSERT( NULL == m_resampler );
  }
  int error = 0;
  // SRC_SINC_BEST_QUALITY
  m_resampler = src_new( SRC_SINC_MEDIUM_QUALITY, channels, &error );
  if( 0 == error ) {
    m_channels = channels;
  }
  else {
    m_channels = 0;
  }

  return( 0 == error );
}

// -----------------------------------------------------------------------------

OcaResampler::~OcaResampler()
{
  if( NULL != m_resampler ) {
    m_resampler = src_delete( m_resampler );
    Q_ASSERT( NULL == m_resampler );
  }
}

// -----------------------------------------------------------------------------

int OcaResampler::resample( const OcaFloatVector* in, OcaFloatVector* out, double ratio, int ofs )
{
  Q_ASSERT( m_channels == in->channels() );
  Q_ASSERT( m_channels == out->channels() );
  SRC_DATA data;
  data.data_in = const_cast<float*>( in->constData() );
  data.input_frames = in->length();

  data.data_out = out->data() + ofs * m_channels;
  data.output_frames = out->length() - ofs;
  Q_ASSERT( 0 <= data.output_frames );
  if( 0 < ofs ) {
    memset( out->data(), 0, sizeof(float)*ofs*m_channels );
  }

  Q_ASSERT( 0 < data.output_frames );

  data.src_ratio = ratio;
  data.end_of_input = 0;

  int err = src_process( m_resampler, &data );
  if( 0 != err ) {
    fprintf( stderr, "OcaResampler::resample - conversion failed, error = %s\n", src_strerror( err ) );
    out->clear();
    data.input_frames_used = 0;
  }
  else {
    out->setLength( data.output_frames_gen + ofs );
  }
  return data.input_frames_used;
}

// -----------------------------------------------------------------------------

void  OcaTrackWriter::write( const OcaFloatVector* src, double t, double rate )
{
  if( m_channels != src->channels() ) {
    init( src->channels() );
  }
  OcaFloatVector resampled;
  if( m_track->getSampleRate() != rate ) {
    if( t != m_posSrc ) {
      src_reset( m_resampler );
      m_posDst = t;
    }
    double ratio =  m_track->getSampleRate() / rate;
    resampled.alloc( m_channels, src->length() * ratio + 1 );
    int src_frames = resample( src, &resampled, ratio, 0 );
    Q_ASSERT( src_frames == src->length() );
    m_posSrc = t + src_frames / rate;
    src = &resampled;
  }
  else {
    m_posDst = t;
  }
  if( ! src->isEmpty() ) {
    OcaDataVector src_d;
    float_to_double( src, &src_d, m_track->getChannels() );
    m_posDst = m_track->setData( &src_d, m_posDst );
  }
}

// -----------------------------------------------------------------------------

void  OcaTrackReader::read( OcaFloatVector* dst, double t, long len, double rate )
{
  double dt = 0;
  bool needs_resample = false;
  bool needs_reset = false;
  if( m_track->getSampleRate() != rate ) {
    needs_resample = true;
    if( m_track->getChannels() != m_channels ) {
      if( ! init( m_track->getChannels() ) ) {
        return;
      }
      m_posSrc = t;
      m_posDst = NAN;
    }
    else if( t != m_posDst ) {
      m_posSrc = t;
      needs_reset = true;
    }
    dt = t + 0.1 - m_posSrc;
  }
  else {
    m_posSrc = t;
  }

  OcaBlockListData data;
  m_track->getData( &data, m_posSrc, len / rate + dt );
  if( ! data.isEmpty() ) {
    const OcaDataVector* block = data.getBlock( 0 );
    double t0 = data.getTime(0);
    int ofs = ceil( ( t0 - m_posSrc - Oca_TIME_TOLERANCE ) * rate );
    if( ofs >= len ) {
      Q_ASSERT( needs_resample );
      dst->clear();
    }
    else if( needs_resample ) {
      int skip = 0;
      if( 0 > ofs ) {
        skip = 1;
        t0 += 1 / m_track->getSampleRate();
        ofs = ceil( ( t0 - m_posSrc - Oca_TIME_TOLERANCE ) * rate );
        Q_ASSERT( 0 <= ofs );
        needs_reset = true;
      }
      OcaFloatVector dst_tmp;
      double_to_float( block, &dst_tmp, 0, skip );
      if( needs_reset || ( 0 != ofs ) ) {
        src_reset( m_resampler );
      }
      dst->alloc( m_channels, len );
      double ratio = rate / m_track->getSampleRate();
      int src_frames = resample( &dst_tmp, dst, ratio, ofs );
      m_posDst = t + dst->length() / rate;
      m_posSrc = t0 + src_frames / m_track->getSampleRate();
    }
    else {
      Q_ASSERT( 0 <= ofs );
      double_to_float( block, dst, ofs, 0 );
    }
  }
  else {
    dst->clear();
  }
}

// -----------------------------------------------------------------------------

