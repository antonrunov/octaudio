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

#include "OcaTrackGroup.h"

#include "OcaTrackBase.h"
#include "OcaTrack.h"
#include "OcaRingBuffer.h"
#include "OcaAudioController.h"
#include "OcaApp.h"
#include "OcaResampler.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// ------------------------------------------------------------------------------------

OcaTrackGroup::OcaTrackGroup( const QString name, double rate )
:
  m_startTime( 0.0 ),
  m_duration( 0 ),
  m_cursorPosition( 0 ),
  m_regionStart( NAN ),
  m_regionEnd( NAN ),
  m_viewPosition( 0.0 ),
  m_viewPositionEnd( 10.0 ),

  m_activeTrack( NULL ),
  m_defaultSampleRate( rate ),

  m_soloTrack( NULL ),
  m_recordingTrack1( NULL ),
  m_recordingTrack2( NULL ),
  m_autoRecordingTracks( false )
{
  static int counter = 1;
  if( name.isEmpty() ) {
    setName( QString("group %1") . arg( counter++) );
  }
  else {
    setName( name );
  }
  setName( name );
  m_nameFlag = e_FlagNameChanged;
  m_displayNameFlag = e_FlagNameChanged;
  connect( OcaApp::getAudioController(), SIGNAL(dataChanged(OcaObject*,uint)),
                                         SLOT(onAudioControllerEvent(OcaObject*,uint)),
                                                                   Qt::DirectConnection );
}

// ------------------------------------------------------------------------------------

OcaTrackGroup::~OcaTrackGroup()
{
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::setCursorPosition( double t )
{
  uint flags = 0;
  if( m_cursorPosition != t ) {
    WLock lock( this );
    m_cursorPosition = t;
    flags = e_FlagCursorChanged;
  }

  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

// should be called from WLock
uint OcaTrackGroup::setViewInternal( double start, double end )
{
  uint flags = 0;
  {
    if( ( m_viewPosition != start ) || ( m_viewPositionEnd != end ) ) {
      m_viewPosition = start;
      m_viewPositionEnd = end;
      flags = e_FlagViewPositionChanged;
    }
  }
  return flags;
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::setView( double start, double end )
{
  uint flags = 0;
  {
    WLock lock( this );
    flags = setViewInternal( start, end );
  }

  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::setViewPosition( double start )
{
  uint flags = 0;
  {
    WLock lock( this );
    double delta = start - m_viewPosition;
    flags = setViewInternal( m_viewPosition + delta, m_viewPositionEnd + delta );
  }

  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::setViewEnd( double end )
{
  uint flags = 0;
  {
    WLock lock( this );
    double delta = end - m_viewPositionEnd;
    flags = setViewInternal( m_viewPosition + delta, m_viewPositionEnd + delta );
  }

  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::setViewCenter( double center )
{
  uint flags = 0;
  {
    WLock lock( this );
    double delta = center - getViewCenterPosition();
    flags = setViewInternal( m_viewPosition + delta, m_viewPositionEnd + delta );
  }

  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::setViewDuration( double duration )
{
  uint flags = 0;
  if( 0 < duration ) {
    WLock lock( this );
    flags = setViewInternal( m_viewPosition, m_viewPosition + duration );
  }

  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::setRegion( double start, double end )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( ( m_regionStart != start ) || ( m_regionEnd != end ) ) {
      m_regionStart = start;
      m_regionEnd = end;
      flags = e_FlagRegionChanged;
    }
  }

  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

double OcaTrackGroup::getRegionDuration() const
{
  OcaLock lock( this );
  double dur = m_regionEnd - m_regionStart;
  return std::isfinite(dur) ? dur : 0;
}

// ------------------------------------------------------------------------------------

double OcaTrackGroup::readPlaybackData( double t, double t_max, OcaRingBuffer* rbuff,
                                                                double rate, bool duplex )
{
  long len_read = 0;
  int length = rbuff->getAvailableSpace() / 2;
  if( std::isfinite( t_max ) ) {
    length = qMin( length, qRound( ( t_max - t ) * rate ) );
  }
  if( 0 < length ) {
    OcaLock lock( this );
    OcaFloatVector buffer( 2, length );
    memset( buffer.data(), 0, sizeof(float)*length * 2 );

    bool solo = false;
    for( uint i = 0; ( i < m_tracks.getLength() ) && ( ! solo ); i++ ) {
      const OcaTrack* w = NULL;
      if( NULL != m_soloTrack ) {
        w = m_soloTrack->getCurrentTrack();
        solo = true;
      }
      else {
        w = qobject_cast<OcaTrack*>( m_tracks.getItem( i ) );
      }
      if( ( NULL == w ) || ( w->isHidden() ) || ( ! w->isAudible() ) ) {
        continue;
      }
      if( duplex && ( ( w == m_recordingTrack1 ) || (  w == m_recordingTrack2 ) ) ) {
        continue;
      }
      if( w->isMuted() && ( ! solo ) ) {
        continue;
      }
      OcaLock lock(w);
      OcaTrackReader* reader = m_readers.value( w );
      if( NULL == reader ) {
        reader = new OcaTrackReader( w );
        m_readers.insert( w, reader );
      }
      Q_ASSERT( NULL != reader );
      OcaFloatVector data;
      reader->read( &data, t, length, rate );

      if( 0 < data.length() ) {
        const float* p_src = data.constData();
        float* p_dst = buffer.data();
        float* max_dst = p_dst + data.length() * 2;

        double gainLeft = w->getGain();
        double gainRight = gainLeft;
        double pan = w->getStereoPan();
        if( 0 > pan ) {
          gainRight *= qMax( 0.0, 1 + pan );
        }
        else {
          gainLeft *= qMax( 0.0, 1 - pan );
        }

        if( 2 > data.channels() ) {
          while( p_dst < max_dst ) {
            (*p_dst++) += (*p_src) * gainLeft;
            (*p_dst++) += (*p_src++) * gainRight;
          }
        }
        else {
          int dc = data.channels() - 2;
          while( p_dst < max_dst ) {
            (*p_dst++) += (*p_src++) * gainLeft;
            (*p_dst++) += (*p_src++) * gainRight;
            p_src += dc;
          }
        }
        len_read = qMax( len_read,  data.length() );
      }
    }
    if( 0 < len_read ) {
      int tmp = rbuff->write( buffer.data(), len_read * 2 );
      Q_ASSERT( tmp == len_read * 2 );
      (void) tmp;
      t += len_read / rate;
    }
  }

  return t;
}

// ------------------------------------------------------------------------------------

double OcaTrackGroup::writeRecordingData( double t, double t_max, OcaRingBuffer* rbuff,
                                                                  double rate, bool first )
{
  OcaLock lock( this );
  OcaTrack* track1 = m_recordingTrack1;
  OcaTrack* track2 = m_recordingTrack2;
  lock.unlock();

  if( ( NULL == track1 ) && ( NULL == track2 ) ) {
    if( first ) {
      track1 = new OcaTrack( NULL, getDefaultSampleRate() );
      track1->setChannels( 2 );
      addTrack( track1 );
      track2 = track1;
      {
        WLock lock( this );
        m_recordingTrack1 = track1;
        m_recordingTrack2 = track2;
        m_autoRecordingTracks = true;
      }
      emitChanged( e_FlagRecordingTracksChanged );
    }
    else {
      t = NAN;
    }
  }

  if( std::isfinite(t) ) {
    int multi_mode = 0;
    if( track1 == track2 ) {
      multi_mode = ( 1 == track1->getChannels() ) ? 1 : 2;
      track2 = NULL;
    }
    if( first ) {
      if( NULL != track1 ) {
        track1->deleteData( -INFINITY, INFINITY );
      }
      if( NULL != track2 ) {
        track2->deleteData( -INFINITY, INFINITY );
      }
    }
    int length = rbuff->getAvailableLength() / 2;
    if( std::isfinite( t_max ) ) {
      int tmp = qRound( ( t_max - t ) * rate );
      length = qMin( length, tmp );
      if( 0 >= tmp ) {
        t = NAN;
      }
    }
    if( 0 < length ) {
      OcaFloatVector buffer( 2, length );
      int tmp = rbuff->read( buffer.data(), length * 2 );
      Q_ASSERT( tmp == length * 2 );
      (void) tmp;

      OcaFloatVector block1( ( 2 == multi_mode ) ? 2 : 1, length );
      OcaFloatVector block2( 1, length );

      const float* p_src = buffer.constData();
      float* p_dst1 = block1.data();
      float* p_dst2 = block2.data();
      const float* max_src = p_src + length * 2;
      while( p_src < max_src ) {
        if( 1 == multi_mode ) {
          float tmp   = *p_src++;
          tmp        += *p_src++;
          (*p_dst1++) = tmp * 0.5;
        }
        else if( 2 == multi_mode ) {
          (*p_dst1++) = (*p_src++);
          (*p_dst1++) = (*p_src++);
        }
        else {
          Q_ASSERT( 0 == multi_mode );
          (*p_dst1++) = (*p_src++);
          (*p_dst2++) = (*p_src++);
        }
      }

      if( NULL != track1 ) {
        OcaTrackWriter* writer = m_writers.value( track1 );
        if( NULL == writer ) {
          writer = new OcaTrackWriter( track1 );
          m_writers.insert( track1, writer );
        }
        writer->write( &block1, t, rate );
      }

      if( NULL != track2 ) {
        OcaTrackWriter* writer = m_writers.value( track2 );
        if( NULL == writer ) {
          writer = new OcaTrackWriter( track2 );
          m_writers.insert( track2, writer );
        }
        writer->write( &block2, t, rate );
      }
      t += length / rate;
    }
  }

  return t;
}

// ------------------------------------------------------------------------------------

oca_index OcaTrackGroup::addTrack( OcaTrackBase* track )
{
  oca_index idx = -1;
  uint flags = 0;
  uint flags_active = 0;
  {
    WLock lock( this );
    idx = m_tracks.appendItem( track, new TrackData() );
    if( -1 != idx ) {
      if( ! connectObject( track, SLOT(onTrackClosed(OcaObject*)) ) ) {
        m_tracks.removeItem( track );
      }
      else {
        connect( track, SIGNAL(dataChanged(OcaObject*,uint)),
                 SLOT(onTrackChanged(OcaObject*,uint)), Qt::DirectConnection );
        flags = e_FlagTrackAdded;
        if( NULL == m_activeTrack ) {
          m_activeTrack = track;
          flags_active = e_FlagActiveTrackChanged;
        }

        OcaTrack* ts = qobject_cast<OcaTrack*>( track );
        if( ( NULL != ts ) && ts->isAudible() && ( ! ts->isMuted() ) ) {
          flags_active |= e_FlagAudioTracksChanged;
          if( 0 == idx ) {
            if( ts->getSampleRate() != m_defaultSampleRate ) {
              m_defaultSampleRate = ts->getSampleRate();
              flags_active |= e_FlagDefaultSampleRateChanged;
            }
          }
        }
        flags_active |= updateDuration();
      }
    }
  }

  emitChanged( track, flags );
  emitChanged( flags_active );
  return idx;
}

// ------------------------------------------------------------------------------------

oca_index OcaTrackGroup::removeTrack( OcaTrackBase* track )
{
  oca_index idx = -1;
  uint flags = 0;
  uint flags_active = 0;
  {
    WLock lock( this );
    if( m_activeTrack == track ) {
      flags_active |= updateActiveTrack( true );
    }
    disconnectObject( track );
    idx = m_tracks.findItemIndex( track );
    if( -1 != idx ) {
      delete m_tracks.getItemData( idx );
      m_tracks.removeItem( track );

      flags = e_FlagTrackRemoved;
      if( track->isHidden() ) {
        flags_active |= e_FlagHiddenTracksChanged;
      }
      if( track->isSelected() ) {
        flags_active |= e_FlagSelectedTracksChanged;
      }
      OcaTrack* ts = qobject_cast<OcaTrack*>( track );
      if( ( NULL != ts ) && ts->isAudible() && ( ! ts->isMuted() ) ) {
        flags_active |= e_FlagAudioTracksChanged | e_FlagSoloModeChanged;
      }
      if( m_soloTrack == track ) {
        m_soloTrack = NULL;
        flags_active |= e_FlagSoloTrackChanged;
      }
      if( m_recordingTrack1 == ts ) {
        m_recordingTrack1 = NULL;
        flags_active |= e_FlagRecordingTracksChanged;
      }
      if( m_recordingTrack2 == ts ) {
        m_recordingTrack2 = NULL;
        flags_active |= e_FlagRecordingTracksChanged;
      }
      flags_active |= updateDuration();
      Q_ASSERT( track != m_activeTrack );
    }
    else {
      Q_ASSERT( 0 == flags_active );
    }
  }

  emitChanged( track, flags );
  emitChanged( flags_active );
  return idx;
}

// ------------------------------------------------------------------------------------

oca_index OcaTrackGroup::moveTrack( OcaTrackBase* track, oca_index idx )
{
  oca_index idx_old = -1;
  uint flags = 0;
  {
    WLock lock( this );
    idx_old = m_tracks.moveItem( track, idx );
    if( -1 != idx_old ) {
      flags = e_FlagTrackMoved;
    }
  }

  emitChanged( track, flags );
  return idx_old;
}

// ------------------------------------------------------------------------------------

oca_index OcaTrackGroup::getTrackIndex( OcaTrackBase* track ) const
{
  OcaLock lock( this );
  return m_tracks.findItemIndex( track );
}

// ------------------------------------------------------------------------------------

OcaTrackBase* OcaTrackGroup::getTrackAt( oca_index idx ) const
{
  OcaLock lock( this );
  return m_tracks.getItem( idx );
}

// ------------------------------------------------------------------------------------

uint OcaTrackGroup::getTrackCount() const
{
  OcaLock lock( this );
  return m_tracks.getLength();
}

// ------------------------------------------------------------------------------------

QList<OcaTrackBase*> OcaTrackGroup::findTracks( QString name ) const
{
  OcaLock lock( this );
  return m_tracks.findItemsByName( name );
}

// ------------------------------------------------------------------------------------

const QList<OcaTrackBase*>  OcaTrackGroup::getHiddenList() const
{
  QList<OcaTrackBase*> tracks;
  OcaLock lock( this );
  for( uint i = 0; i < m_tracks.getLength(); i++ ) {
    OcaTrackBase* track = m_tracks.getItem( i );
    Q_ASSERT( NULL != track );
    if( track->isHidden() ) {
      tracks.append( track );
    }
  }
  return tracks;
}

// ------------------------------------------------------------------------------------

QList<const OcaTrack*>  OcaTrackGroup::getAudioTracks() const
{
  QList<const OcaTrack*> tracks;
  OcaLock lock( this );
  if( NULL != m_soloTrack ) {
    tracks.append( m_soloTrack->getCurrentTrack() );
  }
  else {
    for( uint i = 0; i < m_tracks.getLength(); i++ ) {
      OcaTrack* w = qobject_cast<OcaTrack*>( m_tracks.getItem( i ) );
      if( ( NULL != w ) && ( ! w->isHidden() ) && ( w->isAudible() ) ) {
        tracks.append( w );
      }
    }
  }
  return tracks;
}

// ------------------------------------------------------------------------------------

oca_index OcaTrackGroup::getNextVisibleTrackInternal( OcaTrackBase* track, int direction ) const
{
  oca_index idx = m_tracks.findItemIndex( track );
  oca_index idx0 = idx;
  oca_index result = -1;
  int d = ( 0 == direction ) ? 1 : direction;
  if( -1 != idx ) {
    while( true ) {
      idx += d;
      if( ( 0 > idx ) || ( (int)m_tracks.getLength() <= idx ) ) {
        if( ( 0 == direction ) && ( 1 == d ) ) {
          d = -1;
          idx = idx0;
        }
        else {
          break;
        }
      }
      else if( ! m_tracks.getItem( idx )->isHidden() ) {
        result = idx;
        break;
      }
    }
  }
  return result;
}

// ------------------------------------------------------------------------------------

oca_index OcaTrackGroup::getNextVisibleTrack( OcaTrackBase* track, int direction /* = 0 */ ) const
{
  OcaLock lock( this );
  return getNextVisibleTrackInternal( track, direction );
}


// ------------------------------------------------------------------------------------

int OcaTrackGroup::getTrackHeight( OcaTrackBase* track ) const
{
  OcaLock Lock( this );
  int result = -1;
  TrackData* d = m_tracks.findItemData( track );
  if( NULL != d ) {
    result = d->height;
  }
  return result;
}


// ------------------------------------------------------------------------------------

bool OcaTrackGroup::setTrackHeight( OcaTrackBase* track, int height )
{
  uint flags = 0;
  {
    WLock lock( this );
    TrackData* d = m_tracks.findItemData( track );
    if( ( NULL != d ) && ( height != d->height ) ) {
      d->height = height;
      flags = e_FlagTrackMoved;
    }
  }

  emitChanged( flags );
  return ( 0 != flags );

}

// ------------------------------------------------------------------------------------

bool OcaTrackGroup::setActiveTrack( OcaTrackBase* track )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_activeTrack != track ) {
      oca_index idx = m_tracks.findItemIndex( track );
      if( ( -1 != idx ) && ( ! track->isHidden() ) ) {
        m_activeTrack = track;
        flags = e_FlagActiveTrackChanged;
      }
    }
  }

  emitChanged( flags );
  return ( 0 != flags );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::setSoloTrack( OcaTrackBase* track )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_soloTrack != track ) {
      if( NULL == track ) {
        flags = e_FlagSoloTrackChanged | e_FlagSoloModeChanged;
        m_soloTrack = NULL;
      }
      else {
        oca_index idx = m_tracks.findItemIndex( track );
        if( ( -1 != idx ) && ( ! track->isHidden() ) ) {
          flags = e_FlagSoloTrackChanged;
          if( NULL == m_soloTrack ) {
            flags |= e_FlagSoloModeChanged;
          }
          m_soloTrack = track;
        }
      }
    }
  }

  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::setRecTrack1( OcaTrack* track )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_recordingTrack1 != track ) {
      if( NULL == track ) {
        flags = e_FlagRecordingTracksChanged;
        m_recordingTrack1 = NULL;
      }
      else {
        oca_index idx = m_tracks.findItemIndex( track );
        if( ( -1 != idx ) && ( ! track->isHidden() ) ) {
          flags = e_FlagRecordingTracksChanged;
          m_recordingTrack1 = track;
          m_autoRecordingTracks = false;
        }
      }
    }
  }

  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::setRecTrack2( OcaTrack* track )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_recordingTrack2 != track ) {
      if( NULL == track ) {
        flags = e_FlagRecordingTracksChanged;
        m_recordingTrack2 = NULL;
      }
      else {
        oca_index idx = m_tracks.findItemIndex( track );
        if( ( -1 != idx ) && ( ! track->isHidden() ) ) {
          flags = e_FlagRecordingTracksChanged;
          m_recordingTrack2 = track;
          m_autoRecordingTracks = false;
        }
      }
    }
  }

  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool OcaTrackGroup::isRecordingTrack( OcaTrack* track ) const
{
  return ( NULL != track )
            && ( ( m_recordingTrack1 == track ) || ( m_recordingTrack2 == track ) );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::setDefaultSampleRate( double sr )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( sr != m_defaultSampleRate ) {
      m_defaultSampleRate = sr;
      flags = e_FlagDefaultSampleRateChanged;
    }
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

uint OcaTrackGroup::updateDuration()
{
  double t0 = INFINITY;
  double t1 = -INFINITY;
  uint flags = 0;
  {
    for( uint i = 0; i < m_tracks.getLength(); i++ ) {
      OcaTrackBase* w = m_tracks.getItem( i );
      if( 0 < w->getDuration() ) {
        t0 = qMin( t0, w->getStartTime() );
        t1 = qMax( t1, w->getEndTime() );
      }
    }
  }
  if( std::isfinite( t1 - t0 ) ) {
    t1 -= t0;
  }
  else {
    t0 = m_startTime;
    t1 = 0;
  }
  if( ( t0 != m_startTime ) || ( t1 != m_duration ) ) {
    m_startTime = t0;
    m_duration = t1;
    flags = e_FlagDurationChanged;
  }
  return flags;
}

// ------------------------------------------------------------------------------------

uint OcaTrackGroup::updateActiveTrack( bool force /* = false */ )
{
  uint result = 0;
  if( 0 < m_tracks.getLength() ) {
    OcaTrackBase* ref_track = m_activeTrack;
    if( NULL == ref_track ) {
      ref_track = m_tracks.getItem( 0 );
      force = false;
    }
    if ( ref_track->isHidden() || force ) {
      int idx = getNextVisibleTrackInternal( ref_track, 1 );
      if( -1 == idx ) {
        idx = getNextVisibleTrackInternal( ref_track, -1 );
      }
      m_activeTrack = m_tracks.getItem( idx );
      result |= e_FlagActiveTrackChanged;
    }
    else if( NULL == m_activeTrack ) {
      m_activeTrack = ref_track;
      result |= e_FlagActiveTrackChanged;
    }
  }
  return result;
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::onTrackClosed( OcaObject* obj )
{
  if( isClosed() ) {
    return;
  }
  OcaTrackBase* t = qobject_cast<OcaTrackBase*>( obj );
  Q_ASSERT( NULL != t );
  removeTrack( t );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::onTrackChanged( OcaObject* obj, uint flags )
{
  if( isClosed() ) {
    return;
  }
  uint result = 0;
  const uint mask_audio =    OcaTrack::e_FlagMuteChanged
                            | OcaTrack::e_FlagAudibleChanged;
  const uint mask_all =   OcaTrackBase::e_FlagDurationChanged
                        | OcaTrackBase::e_FlagHiddenChanged
                        | OcaTrackBase::e_FlagSelectedChanged
                        | mask_audio;

  if( mask_all & flags ) {
    WLock lock( this );
    OcaTrackBase* t = m_tracks.findItem( obj );
    Q_ASSERT( NULL != t );
    OcaTrack* ts = qobject_cast<OcaTrack*>( t );
    if( OcaTrackBase::e_FlagDurationChanged & flags ) {
      result |= updateDuration();
    }
    if( OcaTrackBase::e_FlagHiddenChanged & flags ) {
      result |= e_FlagHiddenTracksChanged;
      result |= updateActiveTrack();
      if( ( t->isSelected() ) && ( t->isHidden() ) ) {
        Q_ASSERT( false );
      }
      if( ( NULL != ts ) && ts->isAudible() && ( ! ts->isMuted() ) ) {
        result |= e_FlagAudioTracksChanged;
      }
    }
    if( OcaTrackBase::e_FlagSelectedChanged & flags ) {
      result |= e_FlagSelectedTracksChanged;
    }
    if( NULL != ts ) {
      if( mask_audio & flags ) {
        result |= e_FlagAudioTracksChanged;
      }
    }
  }

  emitChanged( result );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::onAudioControllerEvent(OcaObject* obj, uint flags )
{
  if( isClosed() ) {
    return;
  }
  if( OcaAudioController::e_FlagStateChanged & flags ) {
    bool clear_rec_tracks = false;
    if( OcaAudioController::e_StateStopped == OcaApp::getAudioController()->getStateRecording() ) {
      WLock lock( this );
      if( m_autoRecordingTracks ) {
        m_autoRecordingTracks = false;
        clear_rec_tracks = true;
      }
      QHashIterator<const OcaTrack*,OcaTrackWriter*> it( m_writers );
      while( it.hasNext() ) {
        delete it.next().value();
      }
      m_writers.clear();

    }
    if( clear_rec_tracks ) {
      setRecTrack1( NULL );
      setRecTrack2( NULL );
    }
  }
  if( OcaAudioController::e_StateStopped == OcaApp::getAudioController()->getState() ) {
    WLock lock( this );
    QHashIterator<const OcaTrack*,OcaTrackReader*> it( m_readers );
    while( it.hasNext() ) {
      delete it.next().value();
    }
    m_readers.clear();
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroup::onClose()
{
  QList<OcaObject*> list;
  {
    WLock lock(this);
    for( oca_index idx = m_tracks.getLength() - 1; idx >=0; idx-- ) {
      delete m_tracks.getItemData( idx );
      list.append( m_tracks.getItem( idx ) );
    }
    m_tracks.clear();
    m_activeTrack = NULL;
    m_soloTrack = NULL;
    m_recordingTrack1 = NULL;
    m_recordingTrack2 = NULL;
  }

  for( int i = 0; i < list.size(); i++ ) {
    disconnectObject( list.at(i) );
    list.at(i)->close();
  }
}

// ------------------------------------------------------------------------------------

