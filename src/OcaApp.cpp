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

#include "OcaApp.h"

#include "OcaInstance.h"
#include "OcaMainWindow.h"
#include "OcaWindowData.h"
#include "OcaOctaveController.h"
#include "OcaAudioController.h"
#include "OcaObjectListener.h"

#include <QtCore>
#include <QtNetwork>

static const char* SESSION_FILE_HEADER = "#Octaudio session file v.0. Don't edit!\n";

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
    m_sessionFile.remove();
  }
  Q_ASSERT( m_listRemoved.isEmpty() );
  fprintf( stderr, "OcaApp::~OcaApp - %d objects\n", m_objects.size() );
}

// -----------------------------------------------------------------------------

bool OcaApp::removeDirRecursively( const QString& path )
{
  QDir dir( path );
  bool result = false;
  if( dir.exists() ) {
    dir.setFilter( QDir::AllEntries | QDir::NoDotAndDotDot );
    QDirIterator it( dir );
    while (it.hasNext()) {
      it.next();
      if( it.fileInfo().isDir() ) {
        removeDirRecursively( it.filePath() );
      }
      else {
        //fprintf( stderr, "OcaApp::removeDirRecursively: remove file: %s\n", it.fileName().toLocal8Bit().data() );
        dir.remove( it.fileName() );
      }
    }
    //fprintf( stderr, "OcaApp::removeDirRecursively: %s\n", path.toLocal8Bit().data() );
    result = dir.rmdir( path );
  }
  return result;
}

// -----------------------------------------------------------------------------

int OcaApp::run()
{
  m_octaveController = new OcaOctaveController();
  connect( m_octaveController, SIGNAL(readyStateChanged(bool,int)), SLOT(onControllerReady(bool)) );

  m_audioController = new OcaAudioController();

  m_ocaInstance = new OcaInstance();

  QString s = QDir::homePath() + "/.config/octaudio/sessions";
  QDirIterator it( s, QDir::Files );
  while (it.hasNext()) {
    it.next();
    QString id = it.fileName();
    QLocalSocket client;
    client.connectToServer( id );
    if( ! client.waitForConnected(100) ) {
      QFile f( it.filePath() );
      if( f.open( QIODevice::ReadOnly ) ) {
        QByteArray line = f.readLine();
        if( line.isEmpty() ) {
          bool r2 = f.remove();
          fprintf( stderr, "removing empty stale session %s: %s\n",
                                            id.toLocal8Bit().data(),
                                          ( r2 ? "DONE" : "FAILED" )   );
        }
        else if( line != SESSION_FILE_HEADER ) {
          fprintf( stderr, "skiping stale session of unknown format %s\n", id.toLocal8Bit().data() );
        }
        else {
          fprintf( stderr, "removing stale session %s\n", id.toLocal8Bit().data() );
          while( ! f.atEnd() ) {
            QString cache_dir = f.readLine();
            cache_dir = cache_dir.trimmed();
            if( ! cache_dir.isEmpty() ) {
              bool r1 = removeDirRecursively( cache_dir );
              fprintf( stderr, "  %s: %s\n", r1 ? "OK" : "FAILED", cache_dir.toLocal8Bit().data() );
            }
          }
          bool r2 = f.remove();
          fprintf( stderr, "  %s\n", r2 ? "DONE" : "FAILED" );
        }
      }
      QLocalServer::removeServer( id );
    }
  }
  QDir::current().mkpath( s );
  m_sessionId = QUuid::createUuid().toString();
  QLocalServer::removeServer( m_sessionId );
  QLocalServer* server = new QLocalServer( this );
  if( ! server->listen( m_sessionId ) ) {
    fprintf( stderr, "failed to init session\n" );
    return 1;
  }
  s += "/" + m_sessionId;
  m_sessionFile.setFileName( s );
  m_sessionFile.open( QIODevice::WriteOnly );
  m_sessionFile.write( SESSION_FILE_HEADER );
  m_sessionFile.flush();

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

QDir OcaApp::checkDataCacheDir()
{
  QString data_path = QString( "%1/data.%2" )
      . arg( m_ocaInstance->getDataCacheBase() )
      . arg( m_sessionId );

  QDir dir = QDir::current();
  dir.mkpath( data_path );
  dir.cd( data_path );

  QMutexLocker locker( &m_mutex );
  if( m_dataCacheDir != dir ) {
    m_dataCacheDir = dir;
    m_sessionFile.write( QString("%1\n").arg(dir.canonicalPath()).toLocal8Bit() );
    m_sessionFile.flush();
  }

  return dir;
}

// -----------------------------------------------------------------------------

