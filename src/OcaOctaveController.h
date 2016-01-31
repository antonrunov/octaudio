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

#ifndef OcaOctaveController_h
#define OcaOctaveController_h

#include <QThread>
#include <QStringList>
#include <QFile>

class OcaTrackGroup;
class OcaOctaveHost;
class OcaStdoutThread;

class OcaOctaveController : public QThread
{
  Q_OBJECT ;

  protected:
    OcaOctaveController();
    ~OcaOctaveController();
    friend class OcaApp;

  public:
    bool startThread();
    void stopThread();

  public:
    bool    getReadyState() const;
    int     getLastError() const;
    QString getLastErrorMessage() const;

  public:
    QStringList getCompletions( const QString& hint ) const;
    QStringList getCommandHistory() const;

  public slots:
    void runCommand( const QString& command, OcaTrackGroup* group );
    void abortCurrentCommand();

  signals:
    void outputReceived( const QString& line, int error );
    void readyStateChanged( bool ready_state, int error );
    void updateUiRequestedFromOctaveThread();

  protected:
    virtual void run();

  protected slots:
    void readStdout();
    void onListenerStateChanged( int listener_state );
    void onCommandFailed(const QString& text, int error );

  protected:
    void writeCommandToHistoryFile(  const QString& command );
    void initHistory();
    void addCommandToHistory( const QString& command );

  public:
    enum EStates {
      e_StateStopped = 0,
      e_StateReady,
      e_StateWaiting,
    };

  protected:
    int   m_state;
    int   m_pipeFd;

  protected:
    QStringList m_commandHistory;
    QString     m_lastErrorString;
    int         m_lastError;
    QFile       m_historyFile;
    QString     m_historyFileName;
    QString     m_historyBackupFileName;

  protected:
    OcaOctaveHost*      m_host;
    QString             m_stdoutBuf;

#ifdef Q_OS_WIN32
  friend class OcaStdoutThread;

  protected:
    OcaStdoutThread*    m_stdoutThread;
#endif
};

#endif // OcaOctaveController_h
