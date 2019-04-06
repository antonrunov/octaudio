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

#include "OcaMonitorScreen.h"

#include "OcaMonitor.h"
#include "OcaTrackGroup.h"
#include "OcaObjectListener.h"
#include "OcaTrack.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaMonitorScreen::OcaMonitorScreen(  OcaMonitor* monitorObj )
: OcaSmartScreen( monitorObj, NULL )
{
  m_timeScale = 0.01;
  m_label->hide();
}

// -----------------------------------------------------------------------------

OcaMonitorScreen::~OcaMonitorScreen()
{
  fprintf( stderr, "OcaMonitorScreen::~OcaMonitorScreen\n" );
  m_trackObj = NULL;
}

// -----------------------------------------------------------------------------

uint OcaMonitorScreen::updateViewport( uint group_flags )
{
  OcaMonitor* monitor = qobject_cast<OcaMonitor*>( m_trackObj );
  uint add_flags = 0;

  add_flags = OcaSmartTrack::e_FlagTrackDataChanged;
  m_timeScale = monitor->getYScale() / width();
  m_viewPosition = monitor->getCursor() - monitor->getYScale() / 2;
  m_cursorPosition = width()/2;

  return add_flags;
}

// -----------------------------------------------------------------------------

void OcaMonitorScreen::wheelEvent( QWheelEvent* event )
{
  bool processed = true;

  switch( event->orientation() + event->modifiers() ) {

    case  Qt::Vertical:
      {
        OcaMonitor* monitor = qobject_cast<OcaMonitor*>( m_trackObj );
        double d = 0 < event->delta() ? -1 : 1;
        monitor->moveCursor( d );
      }
      break;
    case  Qt::Vertical + Qt::ControlModifier :
      {
        OcaMonitor* monitor = qobject_cast<OcaMonitor*>( m_trackObj );
        double d = 0 < event->delta() ? -1 : 1;
        monitor->moveYScale( d );
      }
      break;
    default:
      processed = false;
  }
  if( ! processed ) {
    OcaSmartScreen::wheelEvent( event );
  }
}

// -----------------------------------------------------------------------------

void OcaMonitorScreen::resizeEvent( QResizeEvent* event )
{
  if( event->oldSize().width() != width() ) {
    //updateData();
    // TODO
    m_listener->addEvent( NULL,
          OcaMonitor::e_FlagTrackDataChanged | OcaMonitor::e_FlagYScaleChanged );
  }
  OcaDataScreen::resizeEvent( event );
}

// -----------------------------------------------------------------------------

void OcaMonitorScreen::dropEvent( QDropEvent* event )
{
  OcaObject* obj = OcaObject::getObject(
      (oca_ulong)event->mimeData()->data("application/x-octaudio-id").toULongLong() );
  if( NULL != obj ) {
    OcaTrack* t = qobject_cast<OcaTrack*>( obj );
    if( NULL != t ) {
      m_trackObj->addSubtrack( t );
    }
  }
}

// -----------------------------------------------------------------------------

