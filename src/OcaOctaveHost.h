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

#ifndef OcaOctaveHost_h
#define OcaOctaveHost_h

#include <QObject>
#include <QHash>

#ifndef Q_OS_WIN32
#include <pthread.h>
#endif

class OcaTrackGroup;
class OcaWindowData;
class octave_scalar_map;
class OcaObject;

class OcaOctaveHost : public QObject
{
  Q_OBJECT ;

  public:
    static void initialize();
    static void shutdown();
    static OcaWindowData* getWindowData();
    static octave_scalar_map* getObjectContext( OcaObject* obj );

    static const octave_scalar_map* getInfo() { return s_instance->m_info; }

  protected:
    static OcaTrackGroup*   s_group; // TODO
    static OcaOctaveHost*   s_instance;
    static QHash<OcaObject*,octave_scalar_map*>  s_context;

    octave_scalar_map* m_info;

  public:
    OcaOctaveHost();
    ~OcaOctaveHost();

  public:
    void start();
    bool evalCommand( const QString& command, OcaTrackGroup* group );
    QStringList getCompletions( const QString& hint ) const;
    void abortCurrentCommand();

  protected slots:
    void removeObjectContext( OcaObject* obj );

  signals:
    void stateChanged( int state );
    void commandFailed( const QString& text, int error );

  protected:
    QString   m_command;
#ifndef Q_OS_WIN32
    pthread_t m_threadId;
#endif

  protected:
    void processCommand();
    virtual void customEvent( QEvent * event );

  protected:
    static void userInterruptionHanler( int sig );
};

template <typename T>
class OcaPropProxy : public QObject {
  public:
    OcaPropProxy() : m_item( NULL ) {};
    ~OcaPropProxy() { m_item = NULL; }
    void setItem( T* item ) { m_item = item; }
  protected:
    T*  m_item;
};


#endif // OcaOctaveHost_h
