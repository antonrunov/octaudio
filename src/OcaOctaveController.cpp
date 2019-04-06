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

#include "OcaOctaveController.h"
#include "OcaOctaveHost.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include <fcntl.h>
#include <unistd.h>

static const char* HISTORY_FILE_HEADER = "#octaudio command history\n";

// -----------------------------------------------------------------------------
// OcaStdoutThread

#ifdef Q_OS_WIN32

class OcaStdoutThread : public QThread
{
  public:
    OcaStdoutThread( OcaOctaveController* controller );
    void run();
    void stop();
  protected:
    OcaOctaveController* m_controller;
    volatile bool m_run;
};

// -----------------------------------------------------------------------------

OcaStdoutThread::OcaStdoutThread( OcaOctaveController* controller )
: QThread( controller ),
  m_controller( controller ),
  m_run( false )
{
}

// -----------------------------------------------------------------------------

void OcaStdoutThread::run()
{
  m_run = true;
  while( m_run ) {
    m_controller->readStdout();
    usleep( 100000 );
  }
}

// -----------------------------------------------------------------------------

void OcaStdoutThread::stop()
{
  m_run = false;
  printf( "\n" );
  fflush( stdout );
  wait();
}
#endif

// -----------------------------------------------------------------------------
// OcaOctaveController


OcaOctaveController::OcaOctaveController()
:
  m_state( e_StateStopped ),
  m_lastError( 0 ),
  m_historyFileName( ".octaudio_history" ),
  m_historyBackupFileName( ".octaudio_history_old" ),
  m_host( NULL )
{
  int fds[2] = { -1, -1 };
#ifndef Q_OS_WIN32
  pipe( fds );
  dup2( fds[1], fileno(stdout) );
  setvbuf( stdout, NULL, _IOLBF, 0 );
  close( fds[1] );
  m_pipeFd = fds[0];
  int result = fcntl( m_pipeFd,  F_SETFL, O_NONBLOCK );
  Q_ASSERT( 0 == result );
  (void) result;
  QSocketNotifier* pNotifier = new QSocketNotifier( m_pipeFd,
                                                    QSocketNotifier::Read, this );
  connect( pNotifier, SIGNAL(activated(int)), SLOT(readStdout()) );
#else
  _pipe( fds, 4096, O_BINARY );
  _dup2( fds[1], fileno(stdout) );
  setvbuf( stdout, NULL, _IOLBF, 0 );
  close( fds[1] );
  m_pipeFd = fds[0];

  m_stdoutThread = new OcaStdoutThread( this );
  m_stdoutThread->start();
#endif
}

// -----------------------------------------------------------------------------

OcaOctaveController::~OcaOctaveController()
{
  if( e_StateStopped != m_state ) {
    stopThread();
  }
#ifdef Q_OS_WIN32
  m_stdoutThread->stop();
#endif
  Q_ASSERT( -1 != m_pipeFd );
  close( m_pipeFd );
  m_pipeFd = -1;
}

// -----------------------------------------------------------------------------

void OcaOctaveController::writeCommandToHistoryFile(  const QString& command )
{
  m_historyFile.write( "\n" );
  m_historyFile.write( command.trimmed().toLocal8Bit() );
  m_historyFile.write( "\n" );
}

// -----------------------------------------------------------------------------

void OcaOctaveController::initHistory()
{
  m_historyFile.setFileName( m_historyFileName );
  m_historyFile.open( QIODevice::ReadWrite );
  bool error = false;
  if( ! m_historyFile.isWritable() ) {
    fprintf( stderr,
        "OcaOctaveController::initHistory - ERROR, history file not accessible\n" );
    error = true;
  }
  else if( 0 == m_historyFile.size() ) {
    m_historyFile.write( HISTORY_FILE_HEADER );
  }
  else {
    QByteArray line = m_historyFile.readLine();
    if( line == HISTORY_FILE_HEADER ) {
      while( ( ! m_historyFile.atEnd() ) && ( ! error ) ) {
        QString cmd;
        while( true ) {
          QByteArray line = m_historyFile.readLine();
          if( line == "\n" ) {
            break;
          }
          cmd.append( QString::fromLocal8Bit( line ) );
          if( m_historyFile.atEnd() ) {
            if( ! line.endsWith( '\n' ) ) {
              m_historyFile.write( "\n" );
            }
            break;
          }
          if( line.isEmpty() ) {
            fprintf( stderr,
                "OcaOctaveController::initHistory - ERROR, read failed\n" );
            error = true;
            break;
          }
        }
        cmd = cmd.trimmed();
        if( ! cmd.isEmpty() ) {
          m_commandHistory.append( cmd );
        }
      }
    }
    else {
      QFile::remove( m_historyBackupFileName );
      if( ! m_historyFile.rename( m_historyBackupFileName ) ) {
        fprintf( stderr, "OcaOctaveController::initHistory - ERROR, backup failed\n" );
        error = true;
      }
      else {
        m_historyFile.setFileName( m_historyFileName );
        m_historyFile.open( QIODevice::ReadWrite );
        Q_ASSERT( 0 == m_historyFile.size() );
        m_historyFile.write( HISTORY_FILE_HEADER );
      }
    }
  }
  m_historyFile.close();

  if( error ) {
    m_historyFile.setFileName( "" );
  }
}


// -----------------------------------------------------------------------------

bool OcaOctaveController::startThread()
{
  fprintf( stderr, "OcaOctaveController::startThread, thread = %p\n",
                                             QThread::currentThread() );
  m_host = new OcaOctaveHost();
  m_host->moveToThread( this );
  OcaOctaveHost::initialize();
  initHistory();

  start();
  return true;
}

// -----------------------------------------------------------------------------

void OcaOctaveController::stopThread()
{
  fprintf( stderr, "OcaOctaveController::stopThread\n" );
  m_host->disconnect( this );
  m_state = e_StateStopped;
  quit();
  wait();

  delete m_host;
  m_host = NULL;
  OcaOctaveHost::shutdown();
}

// -----------------------------------------------------------------------------

bool OcaOctaveController::getReadyState() const
{
  return e_StateReady == m_state;
}

// -----------------------------------------------------------------------------

int OcaOctaveController::getLastError() const
{
  return m_lastError;
}

// -----------------------------------------------------------------------------

QString OcaOctaveController::getLastErrorMessage() const
{
  return m_lastErrorString;
}

// -----------------------------------------------------------------------------

QStringList OcaOctaveController::getCompletions( const QString& hint ) const
{
  QStringList list;
  if( e_StateReady == m_state ) {
    list = m_host->getCompletions( hint );
  }
  return list;
}

// -----------------------------------------------------------------------------

QStringList OcaOctaveController::getCommandHistory() const
{
  return m_commandHistory;
}

// -----------------------------------------------------------------------------

void OcaOctaveController::runCommand( const QString& command, OcaTrackGroup* group )
{
  if( e_StateReady == m_state ) {
    m_state = e_StateWaiting;
    m_lastError = 0;
    m_lastErrorString.clear();
    QStringList list = command.split( '\n' );
    QString command_fixed;
    for( int i = 0; i < list.size(); i++ ) {
      QString s = list.at(i).trimmed();
      if( ! s.isEmpty() ) {
        if( ! command_fixed.isEmpty() ) {
          command_fixed.append( '\n' );
        }
        command_fixed.append( s );
      }
    }

    fprintf( stderr, "OcaOctaveController => WAITING\n" );
    addCommandToHistory( command_fixed );
    m_host->evalCommand( command_fixed, group );
    emit readyStateChanged( false, 0 );
  }
  else {
    emit outputReceived( "OcaOctaveController::runCommand failed: not ready", 128 );
    fprintf( stderr, "OcaOctaveController => ERROR (not ready)\n" );
  }
}

// -----------------------------------------------------------------------------

void OcaOctaveController::abortCurrentCommand()
{
  if( e_StateWaiting == m_state ) {
    m_host->abortCurrentCommand();
  }
}

// -----------------------------------------------------------------------------

void OcaOctaveController::readStdout()
{
  char buf[ 1024 ];
  //fprintf( stderr, "OcaOctaveController::readStdout $$$ %d (%d)\n", result, m_pipeFd );
  int result = -1;
  do {
    result = read( m_pipeFd, buf, 1023 );
    if( 0 < result ) {
      //fprintf( stderr, "OcaOctaveController::readStdout\n" );
      //fwrite( buf, 1, result, stderr );
      //fprintf( stderr, "OcaOctaveController::readStdout - END\n" );
      buf[ result ] = 0;

      m_stdoutBuf += QString::fromLocal8Bit( buf );
      QStringList lines = m_stdoutBuf.split( '\n' );
      m_stdoutBuf = lines.takeLast();
      //Q_ASSERT( ! lines.isEmpty() );
      /*
      if( '\n' != m_stdoutBuf[ m_stdoutBuf.length() - 1 ] ) {
        m_stdoutBuf = lines.takeLast();
      }
      else {
        m_stdoutBuf.clear();
        QString s = lines.takeLast();
        Q_ASSERT( s.isEmpty() );
      }
      */
      while( ! lines.isEmpty() ) {
        QString s = lines.takeFirst();
        //fprintf( stderr, "readStdout: %s\n", s.toLocal8Bit().data() );
        emit outputReceived( s, 0 );
      }
    }
  } while( ( 1023 == result ) && ( e_StateReady == m_state ) );
}

// -----------------------------------------------------------------------------

void OcaOctaveController::onListenerStateChanged( int listener_state )
{
  if( ( e_StateReady != m_state ) && ( 1 == listener_state ) ) {
    m_state = e_StateReady;
#ifndef Q_OS_WIN32
    readStdout();
#endif
    if( 0 != m_lastError ) {
      emit outputReceived( m_lastErrorString, m_lastError );
    }
    fprintf( stderr, "OcaOctaveController => READY\n" );
    emit readyStateChanged( true, 0 );
  }
  else {
    Q_ASSERT( false );
  }
}

// -----------------------------------------------------------------------------

void OcaOctaveController::onCommandFailed(const QString& text, int error )
{
  m_lastError = error;
  if( m_lastErrorString.isEmpty() ) {
    m_lastErrorString = text;
  }
  else {
    m_lastErrorString += "\n" + text;
  }
}

// -----------------------------------------------------------------------------

void OcaOctaveController::run()
{
  fprintf( stderr, "OctaveThread::run - started %p\n", this );
  connect(
      m_host,
      SIGNAL(commandFailed(const QString&,int)),
      SLOT(onCommandFailed(const QString&,int)),
      Qt::QueuedConnection
  );
  connect(
      m_host,
      SIGNAL(stateChanged(int)),
      SLOT(onListenerStateChanged(int)),
      Qt::QueuedConnection
  );

  m_host->start();
  printf( "started\n" );
  fflush( stdout );
  exec();

  fprintf( stderr, "OctaveThread::run - finished\n" );
}

// -----------------------------------------------------------------------------

void OcaOctaveController::addCommandToHistory( const QString& command )
{
  if( m_commandHistory.isEmpty() || ( m_commandHistory.last() != command ) ) {
    m_commandHistory.append( command );
    if( ! m_historyFile.fileName().isEmpty() ) {
      m_historyFile.open( QIODevice::WriteOnly | QIODevice::Append );
      if( m_historyFile.isWritable() ) {
        writeCommandToHistoryFile( command );
      }
      m_historyFile.close();
    }
  }
}

// -----------------------------------------------------------------------------

