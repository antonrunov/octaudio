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

#include "OcaMonitor.h"

#include "OcaTrack.h"
#include "OcaMainWindow.h"
#include "OcaMonitorScreen.h"

#include <QtCore>

// -----------------------------------------------------------------------------

OcaMonitor::OcaMonitor( const QString& name, OcaTrackGroup* group )
: OcaSmartTrack( name ),
  m_group( group )
{
  if( NULL != m_group ) {
    connectObject( m_group, SLOT(onGroupClosed(OcaObject*)), false );
    connect( m_group, SIGNAL(dataChanged(OcaObject*,uint)),
                      SLOT(onGroupChanged(OcaObject*,uint)), Qt::DirectConnection );
  }
  m_timeData.setZero( 0.0 );
  m_timeData.setScale( 0.1 );
}

// -----------------------------------------------------------------------------

OcaMonitor::~OcaMonitor()
{
}

// -----------------------------------------------------------------------------

bool OcaMonitor::setGroup( OcaTrackGroup* group )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_group != group ) {
      if( NULL != m_group ) {
        m_timeData.setZero( m_group->getCursorPosition() );
        disconnectObject( m_group, false );
      }
      m_group = group;
      flags = e_FlagGroupChanged;

      if( NULL != m_group ) {
        connectObject( m_group, SLOT(onGroupClosed(OcaObject*)), false );
        connect( m_group, SIGNAL(dataChanged(OcaObject*,uint)),
                          SLOT(onGroupChanged(OcaObject*,uint)), Qt::DirectConnection );
        flags |= e_FlagCursorChanged;
      }
    }
  }
  return emitChanged( flags );
}

// -----------------------------------------------------------------------------

bool OcaMonitor::setCursor( double cursor )
{
  uint flags = 0;
  OcaTrackGroup* group = m_group;
  if( NULL == group ) {
    WLock lock( this );
    if( m_timeData.getZero() != cursor ) {
      m_timeData.setZero( cursor );
      flags = e_FlagCursorChanged;
    }
  }
  else {
    group->setCursorPosition( cursor );
    return true;
  }
  return emitChanged( flags );
}

// -----------------------------------------------------------------------------

double OcaMonitor::getCursor() const
{
  double result = 0;
  OcaLock lock( this );
  if( NULL == m_group ) {
    result = m_timeData.getZero();
  }
  else {
    result = m_group->getCursorPosition();
  }
  return result;
}

// -----------------------------------------------------------------------------

bool OcaMonitor::setYScale( double duration )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_timeData.getScale() != duration ) {
      m_timeData.setScale( duration );
      flags = e_FlagYScaleChanged;
    }
  }
  return emitChanged( flags );
}

// -----------------------------------------------------------------------------

void OcaMonitor::moveCursor( double step )
{
  {
    WLock lock(this);
    if( NULL == m_group ) {
      m_timeData.moveZero( -step );
    }
    else {
      // TODO
      m_timeData.setZero(  m_group->getCursorPosition() );
      m_timeData.moveZero( -step );
      m_group->setCursorPosition( m_timeData.getZero() );
    }
  }
  emitChanged( e_FlagCursorChanged );
}

// -----------------------------------------------------------------------------

void OcaMonitor::moveYScale( double step )
{
  {
    WLock lock(this);
    m_timeData.moveScale( step );
  }
  emitChanged( e_FlagYScaleChanged );
}

// -----------------------------------------------------------------------------

void OcaMonitor::onGroupChanged( OcaObject* obj, uint flags )
{
  if( isClosed() ) {
    return;
  }
  if( OcaTrackGroup::e_FlagCursorChanged & flags ) {
    OcaTrackGroup* group = qobject_cast<OcaTrackGroup*>( obj );
    Q_ASSERT( NULL != group );
    (void) group;
    emitChanged( e_FlagCursorChanged );
  }
}

// -----------------------------------------------------------------------------

void OcaMonitor::onGroupClosed( OcaObject* obj )
{
  OcaTrackGroup* group = qobject_cast<OcaTrackGroup*>( obj );
  Q_ASSERT( NULL != group );
  //Q_ASSERT( m_group == group );
  (void) group;
  setGroup( NULL );
}

// -----------------------------------------------------------------------------

void OcaMonitor::onClose()
{
  setGroup( NULL );
}

// -----------------------------------------------------------------------------

