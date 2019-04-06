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

#include "OcaSmartTrack.h"

#include "OcaTrack.h"
#include "OcaSmartScreen.h"

#include <QtCore>

// ------------------------------------------------------------------------------------

OcaSmartTrack::OcaSmartTrack( const QString& name )
: OcaTrackBase( name ),
  m_activeSubtrack( NULL ),
  m_commonScale( true ),
  m_autoScale( false ),
  m_startTime( NAN ),
  m_duration( 0 ),
  m_addIdx( 0 ),
  m_auxTransparency( 0.4 )
{
}

// ------------------------------------------------------------------------------------

OcaSmartTrack::~OcaSmartTrack()
{
}

// ------------------------------------------------------------------------------------

uint  OcaSmartTrack::getSubtrackCount() const
{
  OcaLock lock( this );
  return m_subtracks.getLength();
}

// ------------------------------------------------------------------------------------

bool OcaSmartTrack::isAutoScaleOn() const
{
  OcaLock lock( this );
  bool ret = m_autoScale;
  if( ( ! m_commonScale ) && ( NULL != m_activeSubtrack ) ) {
    ret = m_activeSubtrack->autoScale;
  }
  return ret;
}

// ------------------------------------------------------------------------------------

OcaTrack* OcaSmartTrack::getActiveSubtrack() const
{
  OcaLock lock( this );
  OcaTrack* t = NULL;
  if( NULL != m_activeSubtrack ) {
    t = m_activeSubtrack->track;
  }
  return t;
}

// ------------------------------------------------------------------------------------

OcaTrack* OcaSmartTrack::getSubtrack( oca_index idx ) const
{
  OcaLock lock( this );
  SubtrackInfo* s = NULL;
  OcaTrack* t = m_subtracks.getItem( idx, &s );
  if( NULL != s ) {
    t = s->track;
  }
  return t;
}

// ------------------------------------------------------------------------------------

QList<OcaTrack*> OcaSmartTrack::findSubtracks( const QString& name ) const
{
  OcaLock lock( this );
  return m_subtracks.findItemsByName( name );
}

// ------------------------------------------------------------------------------------

void OcaSmartTrack::setCommonScaleOn( bool on )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_commonScale != on ) {
      m_commonScale = on;
      flags = e_FlagControlModeChanged | e_FlagScaleChanged | e_FlagZeroChanged;
    }
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool OcaSmartTrack::setActiveSubtrack( OcaTrack* t )
{
  uint flags = 0;
  {
    WLock lock( this );
    SubtrackInfo* s = m_subtracks.findItemData( t );
    if( ( NULL != s ) && ( m_activeSubtrack != s ) ) {
      m_activeSubtrack = s;
      flags |= e_FlagActiveSubtrackChanged;
      if( ! m_commonScale ) {
        flags |= ( e_FlagScaleChanged | e_FlagZeroChanged );
      }
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

oca_index OcaSmartTrack::addSubtrack( OcaTrack* track )
{
  if( NULL == track ) {
    return -1;
  }
  uint flags = 0;
  uint flags_subtrack = 0;
  oca_index idx = -1;
  {
    WLock lock( this );
    if( NULL != m_subtracks.findItemData( track ) ) {
      return -1;
    }
    OcaLock lock2( track );
    SubtrackInfo* s = new SubtrackInfo;
    s->track = track;
    s->autoScale = false;
    s->color = QColor::fromHsvF( m_addIdx / 256.0 , 1, 0.8 );
    m_addIdx = ( m_addIdx + 73 ) % 256;

    connectObject( track, SLOT(onSubtrackClosed(OcaObject*)), false );
    connect( track, SIGNAL(dataChanged(OcaObject*,uint)),
                    SLOT(onSubtrackChanged(OcaObject*,uint)), Qt::DirectConnection );

    idx = m_subtracks.appendItem( track, s );
    flags_subtrack = e_FlagSubtrackAdded;

    if( NULL == m_activeSubtrack ) {
      m_activeSubtrack = s;
      flags |= e_FlagActiveSubtrackChanged;
    }

    flags |= updateDuration();
  }


  emitChanged( track, flags_subtrack );
  emitChanged( flags );
  return idx;
}

// ------------------------------------------------------------------------------------

oca_index OcaSmartTrack::removeSubtrack( OcaTrack* t )
{
  uint flags = 0;
  uint flags_subtrack = 0;
  oca_index idx = -1;
  {
    WLock lock( this );
    bool active_removed =
            ( ( NULL != m_activeSubtrack ) && ( m_activeSubtrack->track == t ) );
    idx = m_subtracks.findItemIndex( t );
    if( -1 < idx ) {
      delete m_subtracks.getItemData( idx );
      m_subtracks.removeItem( t );
      disconnectObject( t, false );
      if( active_removed ) {
        if( m_subtracks.isEmpty() ) {
          m_activeSubtrack = NULL;
        }
        else {
          m_activeSubtrack = m_subtracks.getItemData( 0 );
        }
        flags |= e_FlagActiveSubtrackChanged;
      }

      flags_subtrack = e_FlagSubtrackRemoved;
      flags |= updateDuration();
    }
  }

  emitChanged( t, flags_subtrack );
  emitChanged( flags );
  return idx;
}

// ------------------------------------------------------------------------------------

oca_index OcaSmartTrack::moveSubtrack( OcaTrack* t, oca_index idx )
{
  uint flags = 0;
  {
    WLock lock( this );
    idx = m_subtracks.moveItem( t, idx );
    if( -1 < idx ) {
      flags = e_FlagSubtrackMoved;
    }
  }

  emitChanged( flags );
  return idx;
}

// ------------------------------------------------------------------------------------

bool OcaSmartTrack::setAutoScaleOn( bool on )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_commonScale ) {
      if( m_autoScale != on ) {
        m_autoScale = on;
        flags = e_FlagAutoscaleChanged;
      }
    }
    else if( NULL != m_activeSubtrack ) {
      if( m_activeSubtrack->autoScale != on ) {
        m_activeSubtrack->autoScale = on;
        flags = e_FlagAutoscaleChanged;
      }
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool OcaSmartTrack::setAuxTransparency( double value )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( ( value != m_auxTransparency ) && ( 0.0 <= value ) && ( 1.0 >= value ) ) {
      m_auxTransparency = value;
      flags = e_FlagSubtrackColorsChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

QColor OcaSmartTrack::getSubtrackColor( OcaTrack* t ) const
{
  QColor c;
  {
    OcaLock lock( this );
    SubtrackInfo* s = m_subtracks.findItemData( t );
    if( NULL != s ) {
      c = s->color;
    }
  }
  return c;
}

// ------------------------------------------------------------------------------------

double OcaSmartTrack::getSubtrackScale( OcaTrack* t ) const
{
  double scale = NAN;
  OcaLock lock( this );
  SubtrackInfo* s = m_subtracks.findItemData( t );
  if( NULL != s ) {
    scale = s->scaleData.getScale();
  }
  return scale;
}

// ------------------------------------------------------------------------------------

double OcaSmartTrack::getSubtrackZero( OcaTrack* t ) const
{
  double zero = NAN;
  OcaLock lock( this );
  SubtrackInfo* s = m_subtracks.findItemData( t );
  if( NULL != s ) {
    zero = s->scaleData.getZero();
  }
  return zero;
}

// ------------------------------------------------------------------------------------

bool OcaSmartTrack::isSubtrackAutoScaleOn( OcaTrack* t ) const
{
  bool on = false;
  OcaLock lock( this );
  SubtrackInfo* s = m_subtracks.findItemData( t );
  if( NULL != s ) {
    on = s->autoScale;
  }
  return on;
}

// ------------------------------------------------------------------------------------

bool OcaSmartTrack::setSubtrackColor( OcaTrack* t, QColor color )
{
  uint flags = 0;
  {
    WLock lock( this );
    SubtrackInfo* s = m_subtracks.findItemData( t );
    if( NULL != s ) {
      s->color = color;
      flags = e_FlagSubtrackColorsChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool OcaSmartTrack::setSubtrackScale( OcaTrack* t, double scale )
{
  uint flags = 0;
  {
    WLock lock( this );
    SubtrackInfo* s = m_subtracks.findItemData( t );
    if( NULL != s ) {
      if( s->scaleData.setScale( scale ) ) {
        flags = e_FlagScaleChanged;
      }
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool OcaSmartTrack::setSubtrackZero( OcaTrack* t, double zero )
{
  uint flags = 0;
  {
    WLock lock( this );
    SubtrackInfo* s = m_subtracks.findItemData( t );
    if( NULL != s ) {
      if( s->scaleData.setZero( zero ) ) {
        flags = e_FlagZeroChanged;
      }
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool OcaSmartTrack::moveSubtrackScale( OcaTrack* t, double step )
{
  uint flags = 0;
  if( 0 != step ) {
    WLock lock( this );
    SubtrackInfo* s = m_subtracks.findItemData( t );
    if( NULL != s ) {
      if( s->scaleData.moveScale( step ) ) {
        flags = e_FlagScaleChanged;
      }
    }
  }
  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool OcaSmartTrack::moveSubtrackZero( OcaTrack* t, double step )
{
  uint flags = 0;
  if( 0 != step ) {
    WLock lock( this );
    SubtrackInfo* s = m_subtracks.findItemData( t );
    if( NULL != s ) {
      if( s->scaleData.moveZero( step ) ) {
        flags = e_FlagZeroChanged;
      }
    }
  }
  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool OcaSmartTrack::setSubtrackAutoScaleOn( OcaTrack* t, bool on )
{
  uint flags = 0;
  {
    WLock lock( this );
    SubtrackInfo* s = m_subtracks.findItemData( t );
    if( ( NULL != s ) && ( s->autoScale != on ) ) {
      s->autoScale = on;
      flags = e_FlagAutoscaleChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

oca_index OcaSmartTrack::findSubtrack( OcaTrack* t ) const
{
  OcaLock lock( this );
  return m_subtracks.findItemIndex( t );
}

// ------------------------------------------------------------------------------------

const OcaTrack* OcaSmartTrack::getCurrentTrack() const
{
  OcaLock lock( this );
  OcaTrack* t = NULL;
  if( NULL != m_activeSubtrack ) {
    t = m_activeSubtrack->track;
  }
  return t;
}

// ------------------------------------------------------------------------------------

bool OcaSmartTrack::isAudible() const
{
  OcaLock lock( this );
  bool ret = false;
  if( NULL != m_activeSubtrack ) {
    ret = m_activeSubtrack->track->isAudible();
  }
  return ret;
}

// ------------------------------------------------------------------------------------

const OcaScaleData* OcaSmartTrack::getScaleData() const
{
  const OcaScaleData* d = &m_scaleData;
  if( ( ! m_commonScale ) && ( NULL != m_activeSubtrack ) ) {
    d = &m_activeSubtrack->scaleData;
  }
  return d;
}

// ------------------------------------------------------------------------------------

OcaScaleData* OcaSmartTrack::getScaleData()
{
  OcaScaleData* d = &m_scaleData;
  if( ( ! m_commonScale ) && ( NULL != m_activeSubtrack ) ) {
    d = &m_activeSubtrack->scaleData;
  }
  return d;
}

// ------------------------------------------------------------------------------------

uint OcaSmartTrack::updateDuration()
{
  double t0 = INFINITY;
  double t1 = -INFINITY;
  uint flags = 0;
  {
    for( int i = 0; i < (int)m_subtracks.getLength(); i++ ) {
      OcaTrack* w = m_subtracks.getItem( i );
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

void OcaSmartTrack::onSubtrackClosed( OcaObject* obj )
{
  OcaTrack* t = qobject_cast<OcaTrack*>( obj );
  Q_ASSERT( NULL != t );
  removeSubtrack( t );
}

// ------------------------------------------------------------------------------------

void OcaSmartTrack::onSubtrackChanged( OcaObject* obj, uint flags )
{
  const uint flags_metadata =   OcaTrack::e_FlagTrackDataChanged
                              | OcaTrack::e_FlagNameChanged
                              | OcaTrack::e_FlagAudibleChanged;

  const uint flags_all =  flags_metadata | OcaTrack::e_FlagDurationChanged;

  if( 0 == ( flags_all & flags ) ) {
    return;
  }

  uint out_flags = ( flags_metadata & flags );
  OcaTrack* t = qobject_cast<OcaTrack*>( obj );
  Q_ASSERT( NULL != t );
  (void) t;
  if( OcaTrack::e_FlagDurationChanged & flags ) {
    OcaLock lock( this );
    out_flags |= updateDuration();
  }

  emitChanged( out_flags );
}

// ------------------------------------------------------------------------------------

void OcaSmartTrack::onClose()
{
  WLock lock(this);
  for( uint i = 0; i < m_subtracks.getLength(); i++ ) {
    delete m_subtracks.getItemData( i );
    disconnectObject( m_subtracks.getItem( i ), false );
  }
  m_subtracks.clear();
  m_activeSubtrack = NULL;
}

// ------------------------------------------------------------------------------------

double OcaSmartTrack::getZero() const
{
  return getScaleData()->getZero();
}

// ------------------------------------------------------------------------------------

double OcaSmartTrack::getScale() const
{
  return getScaleData()->getScale();
}

// ------------------------------------------------------------------------------------

void OcaSmartTrack::setScale( double scale )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( getScaleData()->setScale( scale ) ) {
      flags = e_FlagScaleChanged;
    }
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaSmartTrack::setZero( double zero )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( getScaleData()->setZero( zero ) ) {
      flags = e_FlagZeroChanged;
    }
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaSmartTrack::moveScale( double step )
{
  uint flags = 0;
  if( 0 != step ) {
    WLock lock( this );
    getScaleData()->moveScale( step );
    flags = e_FlagScaleChanged;
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

void OcaSmartTrack::moveZero( double step )
{
  uint flags = 0;
  if( 0 != step ) {
    WLock lock( this );
    getScaleData()->moveZero( step );
    flags = e_FlagZeroChanged;
  }
  emitChanged( flags );
}

// ------------------------------------------------------------------------------------

