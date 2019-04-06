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

#include "OcaTrack.h"
#include "OcaTrackDataBlock.h"

#include <QtCore>

// ------------------------------------------------------------------------------------

OcaTrack::OcaTrack( const QString& name, double sr )
: OcaTrackBase( name ),
  m_sampleRate( sr ),
  m_startTime( NAN ),
  m_duration( 0 ),
  m_muted( false ),
  m_readonly( false ),
  m_audible( 8000 <= sr ),
  m_gain( 1.0 ),
  m_stereoPan( 0.0 ),
  m_channels( 1 )
{
}

// ------------------------------------------------------------------------------------

OcaTrack::~OcaTrack()
{
  QMap<double,OcaTrackDataBlock*>::const_iterator it= m_blocks.begin();
  for( ; it != m_blocks.end(); it++ ) {
    delete it.value();
  }
}

// ------------------------------------------------------------------------------------

void OcaTrack::moveScale( double step )
{
  uint flags = 0;
  if( 0 != step ) {
    WLock lock( this );
    m_scaleData.moveScale( step );
    flags = e_FlagScaleChanged;
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrack::moveZero( double step )
{
  uint flags = 0;
  if( 0 != step ) {
    WLock lock( this );
    m_scaleData.moveZero( step );
    flags = e_FlagZeroChanged;
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrack::setMute( bool on )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( ( isAudible() ) && ( on != m_muted ) ) {
      m_muted = on;
      flags = e_FlagMuteChanged;
    }
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrack::setScale( double scale )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_scaleData.setScale( scale ) ) {
      flags = e_FlagScaleChanged;
    }
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrack::setZero( double zero )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_scaleData.setZero( zero ) ) {
      flags = e_FlagZeroChanged;
    }
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrack::setReadonly( bool readonly )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_readonly != readonly ) {
      m_readonly = readonly;
      flags = e_FlagReadonlyChanged;
    }
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrack::setAudible( bool audible )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_audible != audible ) {
      m_audible = audible;
      flags = e_FlagAudibleChanged;
    }
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool OcaTrack::setChannels( int channels )
{
  uint flags = 0;
  if( 0 < channels ) {
    WLock lock( this );
    if( ( m_channels != channels ) && m_blocks.isEmpty() ) {
      m_channels = channels;
      flags = e_FlagChannelsChanged;
    }
  }
  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrack::setGain( double gain )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_gain != gain ) {
      m_gain = gain;
      flags = e_FlagGainChanged;
    }
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrack::setStereoPan( double pan )
{
  uint flags = 0;
  {
    WLock lock( this );
    pan = qBound( -1.0, pan, 1.0 );
    if( m_stereoPan != pan ) {
      m_stereoPan = pan;
      flags = e_FlagStereoPanChanged;
    }
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool OcaTrack::setStartTime( double t )
{
  uint flags = 0;
  {
    WLock lock( this );
    double dt = t - m_startTime;
    if( 0 != dt ) {
      QMap<double,OcaTrackDataBlock*> new_blocks;
      QMap<double,OcaTrackDataBlock*>::const_iterator it= m_blocks.begin();
      for( ; it != m_blocks.end(); it++ ) {
        new_blocks.insert( it.key() + dt, it.value() );
      }
      m_blocks = new_blocks;
      m_startTime += dt;
      flags = e_FlagTrackDataChanged | e_FlagDurationChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool OcaTrack::setSampleRate( double rate )
{
  if( ( rate == m_sampleRate ) || ( 0 >= rate ) ) {
    return ( rate == m_sampleRate );
  }
  {
    WLock lock( this );
    if( ! m_blocks.empty() ) {
      double k = m_sampleRate / rate;

      QMap<double,OcaTrackDataBlock*> new_blocks;
      double new_end_time = m_startTime;

      QMap<double,OcaTrackDataBlock*>::const_iterator it= m_blocks.begin();
      for( ; it != m_blocks.end(); it++ ) {
        double t_old = it.key();
        OcaTrackDataBlock* block = it.value();
        double t_new = ( t_old - m_startTime ) * k + m_startTime;
        new_end_time = t_new + block->getLength() / rate;
        new_blocks.insert( t_new, block );
      }
      m_blocks = new_blocks;
      m_duration = new_end_time - m_startTime;
    }
    m_sampleRate = rate;
  }
  return emitChanged( e_FlagSampleRateChanged | e_FlagTrackDataChanged | e_FlagDurationChanged );
}

// ------------------------------------------------------------------------------------

class OcaTrack::DstWrapper
{
  public:
    DstWrapper( OcaBlockListData* dst );
    DstWrapper( OcaBlockListAvg* dst, long decimation_hint );
    DstWrapper( OcaBlockListInfo* dst );
    ~DstWrapper();

  public:
    void addBlock( OcaTrackDataBlock* block, const Range& r, double t );
    long getDecimation() const { return m_decimation; }

  protected:
    long m_decimation;
    OcaBlockListData* m_data;
    OcaBlockListAvg*  m_avg;
    OcaBlockListInfo* m_info;
};

// ------------------------------------------------------------------------------------

OcaTrack::DstWrapper::DstWrapper( OcaBlockListData* dst )
:
  m_decimation( 1 ),
  m_data( dst ),
  m_avg( NULL ),
  m_info( NULL )
{
}

// ------------------------------------------------------------------------------------

OcaTrack::DstWrapper::DstWrapper( OcaBlockListAvg* dst, long decimation_hint )
:
  m_decimation( 1 ),
  m_data( NULL ),
  m_avg( dst ),
  m_info( NULL )
{
  m_decimation = OcaTrackDataBlock::getAvailableDecimation( decimation_hint );
}

// ------------------------------------------------------------------------------------

OcaTrack::DstWrapper::DstWrapper( OcaBlockListInfo* dst )
:
  m_decimation( 1 ),
  m_data( NULL ),
  m_avg( NULL ),
  m_info( dst )
{
}

// ------------------------------------------------------------------------------------

OcaTrack::DstWrapper::~DstWrapper()
{
}

// ------------------------------------------------------------------------------------

void OcaTrack::DstWrapper::addBlock( OcaTrackDataBlock* block, const Range& r, double t )
{
    if( r.start < r.end ) {
      if( NULL != m_data ) {
        OcaDataVector* out_block = new OcaDataVector;
        long len = block->read( out_block, r.start, r.end - r.start );
        if( 0 < len ) {
          m_data->appendBlock( t, out_block );
        }
        else {
          delete out_block;
          out_block = NULL;
        }
      }
      else if ( NULL != m_avg ) {
        OcaAvgVector* out_avg = new OcaAvgVector;
        long len = block->readAvg( out_avg,  m_decimation, r.start,
                                 ( r.end - r.start - 1 ) / m_decimation + 1 );
        if( 0 < len ) {
          m_avg->appendBlock( t, out_avg );
        }
        else {
          delete out_avg;
          out_avg = NULL;
        }
      }
      else {
        Q_ASSERT( NULL != m_info );
        m_info->append( QPair<double,qint64>( t, r.end - r.start ) );
      }
    }
}

// ------------------------------------------------------------------------------------

void OcaTrack::getDataBlocksInfo( OcaBlockListInfo* info, double t0, double duration ) const
{
  info->clear();
  DstWrapper wrapper( info );
  getDataInternal( &wrapper, t0, duration );
}

// ------------------------------------------------------------------------------------

void OcaTrack::getData( OcaBlockListData* dst, double t0, double duration ) const
{
  dst->clear();
  DstWrapper wrapper( dst );
  getDataInternal( &wrapper, t0, duration );
}

// ------------------------------------------------------------------------------------

long OcaTrack::getAvgData( OcaBlockListAvg* dst, double t0,
                                 double duration, long decimation_hint        ) const
{
  dst->clear();
  DstWrapper wrapper( dst, decimation_hint );
  getDataInternal( &wrapper, t0, duration );
  return wrapper.getDecimation();
}

// ------------------------------------------------------------------------------------

void OcaTrack::getDataInternal( DstWrapper* dst, double t0, double duration ) const
{
  OcaLock lock( this );

  QMap<double,OcaTrackDataBlock*>::const_iterator it0 = findBlock( t0, false );

  for( QMap<double,OcaTrackDataBlock*>::const_iterator it = it0; it != m_blocks.end(); it++ ) {
    OcaTrackDataBlock* block = it.value();
    double start_time = it.key();
    Range r = getRange( block, t0 - start_time, duration );
    if( ! r.isValid() ) {
      break;
    }
    Q_ASSERT( block->getChannels() == m_channels );
    dst->addBlock( block, r, start_time + r.start / m_sampleRate );
  }

}

// ------------------------------------------------------------------------------------

double OcaTrack::setData( const OcaDataVector* src, double t0, double duration /* = 0 */ )
{
  double t_next = NAN;
  uint flags = 0;

  if( ( ! m_readonly ) && ( ! src->isEmpty() ) && std::isfinite( t0 ) ) {
    WLock lock( this );

    if( src->channels() != m_channels ) {
      // error
    }
    else {

      bool fill = false;
      double t00 = t0;
      if( 0.0 >= duration ) {
        duration = src->length() / m_sampleRate;
      }
      else {
        fill = true;
      }

      QMap<double,OcaTrackDataBlock*>::const_iterator it0 = findBlock( t0, true );
      OcaTrackDataBlock* block_dst = NULL;
      qint64 idx0 = 0;

      if( m_blocks.constEnd() != it0 ) {
        QMap<double,OcaTrackDataBlock*>::iterator it = m_blocks.find( it0.key() );

        Range r = getRange( it.value(), t0 - it.key(), 0 );
        idx0 = r.start;
        if( r.isValid() ) {
          block_dst = it.value();
          t0 = it.key() + r.start / m_sampleRate;
          it++;
        }

        while( it != m_blocks.end() ) {
          OcaTrackDataBlock* block = it.value();
          double start = it.key();
          r = getRange( block, t0 - start, duration );
          if( ! r.isValid() ) {
            break;
          }
          Q_ASSERT( 0 == r.start );
          OcaTrackDataBlock* tmp = NULL;
          if( block->getLength() > r.end ) {
            Q_ASSERT( block->getChannels() == m_channels );
            tmp = new OcaTrackDataBlock( m_channels );
            if( ! block->split( r.end, tmp ) ) {
              Q_ASSERT( false );
            }
          }
          delete block;
          block = NULL;
          it = m_blocks.erase( it );
          if( NULL != tmp ) {
            m_blocks.insert( start + r.end / m_sampleRate, tmp );
            break;
          }
        }
      }

      if( NULL == block_dst ) {
        Q_ASSERT( 0 == idx0 );
        block_dst = new OcaTrackDataBlock( m_channels );
        m_blocks.insert( t0, block_dst );
      }
      else {
        Q_ASSERT( block_dst->getChannels() == m_channels );
      }

      qint64 len = 0;
      if( fill ) {
        OcaDataVector* src_fill = NULL;
        qint64 rem = floor( t0 + duration * m_sampleRate - t00 + Oca_TIME_TOLERANCE );
        long len_pat = src->length();
        long k = ( qMin( rem, 0x10000ll ) - 1 ) /len_pat  + 1;
        if( 1 < k ) {
          src_fill = new OcaDataVector( m_channels, k * len_pat );
          double* p = src_fill->data();
          for( int i = 0; i < k; i++ ) {
            memcpy( p, src->constData(), len_pat * m_channels * sizeof(double) );
            p += len_pat * m_channels;
          }
          src = src_fill;
          len_pat = src->length();
        }
        while( 0 < rem ) {
          long l = block_dst->write( src, idx0, qMin( rem, (qint64)len_pat ) );
          len += l;
          rem -= l;
          idx0 += l;
        }

        if( NULL != src_fill ) {
          delete src_fill;
          src_fill = NULL;
          src = NULL;
        }
      }
      else {
        len = block_dst->write( src, idx0 );
      }

      t_next = t0 + len / m_sampleRate;
      flags = ( e_FlagTrackDataChanged | updateDuration() );
    }
  }

  emitChanged( flags );
  return t_next;
}

// ------------------------------------------------------------------------------------

uint OcaTrack::updateDuration()
{
  if( m_blocks.isEmpty() ) {
    m_startTime = NAN;
    m_duration = 0;
  }
  else {
    m_startTime = m_blocks.begin().key();
    QMap<double,OcaTrackDataBlock*>::const_iterator it = --m_blocks.end();
    m_duration = it.key() - m_startTime + it.value()->getLength() / m_sampleRate;
  }

  return e_FlagDurationChanged;
}

// ------------------------------------------------------------------------------------

void OcaTrack::deleteData( double t0, double duration )
{
  cutData( NULL, t0, duration );
}

// ------------------------------------------------------------------------------------

void OcaTrack::cutData( OcaBlockListData* dst, double t0, double duration )
{
  if( NULL != dst ) {
    dst->clear();
  }
  uint flags = 0;

  {
    WLock lock( this );
    QMap<double,OcaTrackDataBlock*>::const_iterator it0 = findBlock( t0, false );
    if( m_blocks.constEnd() != it0 ) {
      QMap<double,OcaTrackDataBlock*>::iterator it = m_blocks.find( it0.key() );
      while( it != m_blocks.end() ) {
        OcaTrackDataBlock* block = it.value();
        Q_ASSERT( block->getChannels() == m_channels );
        double start_time = it.key();
        Range r = getRange( block, t0 - start_time, duration );
        if( ! r.isValid() ) {
          break;
        }
        if( NULL != dst ) {
          DstWrapper d( dst );
          d.addBlock( block, r, start_time + r.start / m_sampleRate );
        }
        bool done = false;
        if( block->getLength() > r.end ) {
          OcaTrackDataBlock* tmp = new OcaTrackDataBlock( m_channels );
          if( ! block->split( r.end, tmp ) ) {
            Q_ASSERT( false );
          }
          m_blocks.insert( start_time + r.end / m_sampleRate, tmp );
          done = true;
        }
        if( 0 < r.start ) {
          if( ! block->split( r.start, NULL ) ) {
            Q_ASSERT( false );
          }
          it++;
        }
        else {
          delete block;
          block = NULL;
          it = m_blocks.erase( it );
        }
        if( done ) {
          break;
        }
      }
    }
    flags = ( e_FlagTrackDataChanged | updateDuration() );
  }

  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

double OcaTrack::splitBlock( double t0 )
{
  double t_ret = NAN;
  uint flags = 0;
  {
    WLock lock( this );
    QMap<double,OcaTrackDataBlock*>::const_iterator it0 = findBlock( t0, false );
    if( it0 != m_blocks.end() ) {
      QMap<double,OcaTrackDataBlock*>::iterator it = m_blocks.find( it0.key() );
      OcaTrackDataBlock* block = it.value();
      Q_ASSERT( block->getChannels() == m_channels );
      double start_time = it.key();
      Range r = getRange( block, t0 - start_time, 0 );
      if( r.isValid() ) {
        if( 0 == r.start ) {
          t_ret = start_time;
        }
        else {
          OcaTrackDataBlock* tmp = new OcaTrackDataBlock( m_channels );
          if( ! block->split( r.start, tmp ) ) {
            Q_ASSERT( false );
          }
          t_ret = start_time + r.start / m_sampleRate;
          m_blocks.insert( t_ret, tmp );
          flags = e_FlagTrackDataChanged;
        }
      }
    }
  }

  emitChanged( flags );
  return t_ret;
}

// ------------------------------------------------------------------------------------

int OcaTrack::joinBlocks( double t0, double duration )
{
  int ret = 0;
  uint flags = 0;
  {
    WLock lock( this );
    QMap<double,OcaTrackDataBlock*>::const_iterator it0 = findBlock( t0, false );
    if( it0 != m_blocks.end() ) {
      QMap<double,OcaTrackDataBlock*>::iterator it = m_blocks.find( it0.key() );
      ret = 1;
      OcaTrackDataBlock* block = it.value();
      Q_ASSERT( block->getChannels() == m_channels );
      it++;
      while( it != m_blocks.end() ) {
        OcaTrackDataBlock* tmp = it.value();
        Q_ASSERT( tmp->getChannels() == m_channels );
        double start_time = it.key();
        Range r = getRange( tmp, t0 - start_time, duration );
        if( ( ! r.isValid() ) ) {
          break;
        }
        it = m_blocks.erase( it );
        block->append( tmp );
        delete tmp;
        tmp = NULL;
        flags = e_FlagTrackDataChanged;
        ret++;
      }
    }

    if( 0 != flags ) {
      flags |= updateDuration();
    }
  }

  emitChanged( flags );
  return ret;
}

// ------------------------------------------------------------------------------------

double OcaTrack::moveBlocks( double dt, double t0, double duration )
{
  double dt_actual = NAN;
  uint flags = 0;
  if( 0 != dt ) {
    WLock lock( this );
    QMap<double,OcaTrackDataBlock*>::iterator it = m_blocks.find( findBlock( t0, false ).key() );
    QMap<double,OcaTrackDataBlock*>::iterator it_end = it;
    for( ; it_end != m_blocks.end(); it_end++ ) {
      OcaTrackDataBlock* block = it_end.value();
      double start_time = it_end.key();
      Range r = getRange( block, t0 - start_time, duration );
      if( ! r.isValid() ) {
        break;
      }
    }
    if( it != it_end ) {
      if( 0 < dt ) {
        if( it_end == m_blocks.end() ) {
          dt_actual = dt;
        }
        else {
          QMap<double,OcaTrackDataBlock*>::iterator tmp = it_end - 1;
          dt_actual = qMin( dt, it_end.key() - ( tmp.key() + tmp.value()->getLength() / m_sampleRate ) );
        }
        Q_ASSERT( 0 <= dt_actual );
      }
      else {
        if( it == m_blocks.begin() ) {
          dt_actual = dt;
        }
        else {
          QMap<double,OcaTrackDataBlock*>::iterator tmp = it - 1;
          dt_actual = - qMin( -dt, it.key() - ( tmp.key() + tmp.value()->getLength() / m_sampleRate ) );
          Q_ASSERT( 0 >= dt_actual );
        }
      }
      if( 0 != dt_actual ) {
        QMap<double,OcaTrackDataBlock*> tmp_blocks;
        while( it != it_end ) {
          OcaTrackDataBlock* block = it.value();
          double start_time = it.key();
          it = m_blocks.erase( it );
          tmp_blocks.insert( start_time + dt_actual, block );
        }
        for( it = tmp_blocks.begin() ; it != tmp_blocks.end(); it++ ) {
          m_blocks.insert( it.key(), it.value() );
        }
        flags = e_FlagTrackDataChanged;
      }
    }

    if( 0 != flags ) {
      flags |= updateDuration();
    }
  }

  emitChanged( flags );
  return dt_actual;
}

// ------------------------------------------------------------------------------------

QMap<double,OcaTrackDataBlock*>::const_iterator OcaTrack::findBlock( double t0,
                                                                            bool bottom_allowed  ) const
{
  QMap<double,OcaTrackDataBlock*>::const_iterator next = m_blocks.lowerBound( t0 - Oca_TIME_TOLERANCE );
  if( std::isfinite( t0 ) && ( m_blocks.begin() != next ) ) {
    next--;
    qint64 len = next.value()->getLength();
    if( bottom_allowed ) {
      len++;
    }
    if( len <= ( t0 - next.key() + Oca_TIME_TOLERANCE ) * m_sampleRate ) {
      next++;
    }
  }
  return next;
}

// ------------------------------------------------------------------------------------

OcaTrack::Range OcaTrack::getRange( const OcaTrackDataBlock* block,
                                                     double dt, double duration ) const
{
  Range r;
  r.start = 0;
  dt += Oca_TIME_TOLERANCE;
  if( 0 < dt ) {
    r.start = floor( dt * m_sampleRate );
  }
  r.end = block->getLength();
  if( INFINITY > duration ) {
    r.end = qMax( -1.0, floor( ( dt + duration ) * m_sampleRate ) );
    if( ( r.start == r.end ) && ( 0 < dt ) ) {
      r.end++;
    }
    r.end = qMin( block->getLength(), r.end );
  }
  return r;
}

// ------------------------------------------------------------------------------------

bool OcaTrack::validateBlocks() const
{
  // TODO
  return true;
}

// ------------------------------------------------------------------------------------

