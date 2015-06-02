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

#include "OcaApp.h"

#include "OcaInstance.h"
#include "OcaMainWindow.h"
#include "OcaWindowData.h"
#include "OcaOctaveController.h"
#include "OcaAudioController.h"
#include "OcaObjectListener.h"

#include <QtCore>

// -----------------------------------------------------------------------------

OcaApp::OcaApp( int& argc, char** argv, bool gui_enabled )
: QApplication( argc, argv, gui_enabled ),
  m_ocaInstance( NULL ),
  m_listener( NULL ),
  m_octaveController( NULL ),
  m_audioController( NULL ),
  m_mainWindow( NULL ),
  m_gcTimer( NULL ),
  m_nextId( 1 )
{
  setApplicationName( "octaudio" );
  m_gcTimer = new QTimer( this );
  m_gcTimer->setInterval( 100 );
  m_gcTimer->setSingleShot( true );
  connect( m_gcTimer, SIGNAL(timeout()), SLOT(deleteQueuedObjects()) );
}

// -----------------------------------------------------------------------------

OcaApp::~OcaApp()
{
  if( NULL != m_ocaInstance ) {
    Q_ASSERT( NULL != m_audioController );
    Q_ASSERT( NULL != m_octaveController );

    m_octaveController->abortCurrentCommand();
    m_octaveController->stopThread();

    m_ocaInstance->close();
    m_ocaInstance = NULL;

    m_audioController->close();
    m_audioController = NULL;

    delete m_listener;
    m_listener = NULL;

    deleteQueuedObjects();

    delete m_octaveController;
    m_octaveController = NULL;
  }
  Q_ASSERT( m_listRemoved.isEmpty() );
  fprintf( stderr, "OcaApp::~OcaApp - %d objects\n", m_objects.size() );
}

// -----------------------------------------------------------------------------

int OcaApp::run()
{
  m_octaveController = new OcaOctaveController();
  connect( m_octaveController, SIGNAL(readyStateChanged(bool,int)), SLOT(onControllerReady(bool)) );

  m_audioController = new OcaAudioController();

  m_ocaInstance = new OcaInstance();

  m_listener = new OcaObjectListener( m_ocaInstance, OcaInstance::e_FlagALL, 10, this );
  connect(  m_listener,
            SIGNAL(updateRequired(uint,QHash<QString,uint>&,QList<OcaObject*>&)),
            SLOT(onUpdateRequired(uint))                                            );
  connect( m_listener, SIGNAL(closed()), SLOT(onInstanceClosed()) );
  OcaWindowData* window_data = new OcaWindowData();
  window_data->setName( applicationName() );
  m_ocaInstance->setWindow( window_data );
  m_octaveController->startThread();
  return exec();
}

// -----------------------------------------------------------------------------

void OcaApp::onUpdateRequired( uint flags )
{
  if( OcaInstance::e_FlagWindowAdded & flags ) {
    Q_ASSERT( NULL == m_mainWindow );
    m_mainWindow = new OcaMainWindow( m_ocaInstance->getWindowData() );
    m_mainWindow->show();
    m_mainWindow->raise();
  }
  if( OcaInstance::e_FlagWindowRemoved & flags ) {
    Q_ASSERT( NULL == m_ocaInstance->getWindowData() );
    if( NULL != m_mainWindow ) {
      delete m_mainWindow;
      m_mainWindow = NULL;
    }
  }
  if( ( OcaInstance::e_FlagObjectRemoved & flags )
                              && m_octaveController->getReadyState() ) {
    deleteQueuedObjects();
  }
}

// -----------------------------------------------------------------------------

void OcaApp::onInstanceClosed()
{
}

// -----------------------------------------------------------------------------

void OcaApp::onControllerReady( bool ready )
{
  if( ready ) {
    deleteQueuedObjects();
  }
  else {
    m_gcTimer->stop();
  }
}

// -----------------------------------------------------------------------------

oca_ulong OcaApp::registerObject( OcaObject* obj )
{
  oca_ulong result = 0;
  QMutexLocker locker( &m_mutex );
  if( 0 == m_nextId ) {
    Q_ASSERT( false );
    return 0;
  }
  if( ( 0 == obj->getId() ) && ( ! obj->isClosed() ) ) {
    obj->moveToThread( thread() );
    result = m_nextId++;
    m_objects[ result ] = obj;
  }
  return result;
}

// -----------------------------------------------------------------------------

bool OcaApp::unregisterObject( OcaObject* obj )
{
  QMutexLocker locker( &m_mutex );
  bool result = m_objects.contains( obj->getId() );
  if( result ) {
    Q_ASSERT( m_objects.value( obj->getId() ) == obj );
    m_objects.remove( obj->getId() );
    m_listRemoved.append( obj );
    Q_ASSERT( NULL != m_listener );
    m_listener->addEvent( NULL, OcaInstance::e_FlagObjectRemoved );
  }
  return result;
}

// -----------------------------------------------------------------------------

OcaObject* OcaApp::getObject( oca_ulong id ) const
{
  OcaObject* obj = NULL;
  QMutexLocker locker( &m_mutex );
  if( m_objects.contains( id ) ) {
    obj = m_objects.value( id );
  }
  return obj;
}

// -----------------------------------------------------------------------------

int OcaApp::deleteQueuedObjects()
{
  int result = 0;
  QList<OcaObject*> list_released;
  bool empty = false;
  {
    QMutexLocker locker( &m_mutex );
    for( int i = m_listRemoved.size()-1; i >=0; i-- ) {
      OcaObject* obj = m_listRemoved.at( i );
      if( obj->isReleased() ) {
        m_listRemoved.removeAt( i );
        list_released.append( obj );
        obj = NULL;
        result++;
      }
    }
    empty = m_listRemoved.isEmpty();
  }
  while( ! list_released.isEmpty() ) {
    delete list_released.takeFirst();
  }
  if( empty ) {
    m_gcTimer->stop();
  }
  else {
    m_gcTimer->start();
  }
  return result;
}

// -----------------------------------------------------------------------------

