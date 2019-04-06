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

#include "OcaMonitorDock.h"

#include "OcaMonitor.h"
#include "OcaMonitorScreen.h"
#include "OcaScaleControl.h"
#include "OcaObjectListener.h"
#include "OcaTrack.h"
#include "OcaDialogPropertiesSmartTrack.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaMonitorDock::OcaMonitorDock(  OcaMonitor* monitor )
: QDockWidget( "Monitor" ),
  m_listener( NULL ),
  m_monitor( monitor ),
  m_cursorControl( NULL ),
  m_yScaleControl( NULL )
{
  const uint flags =  OcaMonitor::e_FlagCursorChanged
                    | OcaMonitor::e_FlagYScaleChanged
                    | OcaMonitor::e_FlagActiveSubtrackChanged
                    | OcaMonitor::e_FlagNameChanged;

  m_listener = new OcaObjectListener( m_monitor, flags, 10, this );
  connect(  m_listener,
            SIGNAL(updateRequired(uint,QHash<QString,uint>&,QList<OcaObject*>&)),
            SLOT(onUpdateRequired(uint))  );
  m_listener->addEvent( NULL, flags );

  QWidget* w = new QWidget;
  setWidget( w );
  QVBoxLayout* layout = new QVBoxLayout( w );
  OcaMonitorScreen* s = new OcaMonitorScreen( m_monitor );
  layout->addWidget( s );
  QHBoxLayout* bottom = new QHBoxLayout();
  layout->addLayout( bottom );

  m_cursorControl = new OcaScaleControl( this );
  m_cursorControl->setValue( monitor->getCursor() );
  m_cursorControl->setTransparency( 0.2, 0.5 );

  m_yScaleControl = new OcaScaleControl( this );
  m_yScaleControl->setValue( monitor->getYScale() );
  m_yScaleControl->setTransparency( 0.2, 0.5 );

  m_monitor->connect( m_cursorControl, SIGNAL(changed(double)),
                                       SLOT(setCursor(double)) );
  m_monitor->connect( m_yScaleControl, SIGNAL(changed(double)),
                                           SLOT(setYScale(double)) );
  m_monitor->connect( m_cursorControl, SIGNAL(moved(double)),
                                       SLOT(moveCursor(double)) );
  m_monitor->connect( m_yScaleControl, SIGNAL(moved(double)),
                                           SLOT(moveYScale(double)) );

  bottom->addWidget( m_cursorControl );
  bottom->addWidget( m_yScaleControl );

  setFeatures(  QDockWidget::DockWidgetFloatable
                | QDockWidget::DockWidgetClosable
                | QDockWidget::DockWidgetMovable  );

  setContextMenuPolicy( Qt::CustomContextMenu );
  connect( this, SIGNAL(customContextMenuRequested(const QPoint&)),
                              SLOT(openContextMenu(const QPoint&))  );
}

// -----------------------------------------------------------------------------

OcaMonitorDock::~OcaMonitorDock()
{
  m_monitor = NULL;
}

// -----------------------------------------------------------------------------

void OcaMonitorDock::closeEvent( QCloseEvent* event )
{
  m_monitor->close();
}

// -----------------------------------------------------------------------------

void OcaMonitorDock::openContextMenu( const QPoint& pos )
{
  QMenu* menu = new QMenu( this );
  QAction* action = NULL;
  menu->addAction( "Delete", m_monitor, SLOT(close())  );

  action = menu->addAction( "Abs Value Mode", m_monitor, SLOT(setAbsValueMode(bool)) );
  action->setCheckable( true );
  action->setChecked( m_monitor->getAbsValueMode() );

  action = menu->addAction( "Common Scale", m_monitor, SLOT(setCommonScaleOn(bool))  );
  action->setCheckable( true );
  action->setChecked( m_monitor->isCommonScaleOn() );

  menu->addAction( "Properties...", this, SLOT(openProperties()) );
  menu->exec( mapToGlobal(pos) );
}

// -----------------------------------------------------------------------------

void OcaMonitorDock::onUpdateRequired( uint flags )
{
  if( OcaMonitor::e_FlagCursorChanged & flags ) {
    m_cursorControl->setValue( m_monitor->getCursor() );
  }
  if( OcaMonitor::e_FlagYScaleChanged & flags ) {
    m_yScaleControl->setValue( m_monitor->getYScale() );
  }
  const uint name_flags =   OcaMonitor::e_FlagActiveSubtrackChanged
                          | OcaMonitor::e_FlagNameChanged;
  if( name_flags & flags ) {
    OcaTrack* t = m_monitor->getActiveSubtrack();
    setWindowTitle( QString( "%1 <%2>" )
                    . arg( m_monitor->getDisplayText() )
                    . arg( ( NULL != t ) ? t->getDisplayText() : QString() )  );
  }
}

// -----------------------------------------------------------------------------

void OcaMonitorDock::openProperties()
{
  OcaDialogPropertiesSmartTrack* dlg = new OcaDialogPropertiesSmartTrack( m_monitor );
  dlg->show();
}

// -----------------------------------------------------------------------------

