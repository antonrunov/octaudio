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

#ifndef OcaApp_h
#define OcaApp_h

#include "octaudio.h"

#include <QApplication>
#include <QMutex>
#include <QHash>
#include <QList>
#include <QDir>

class OcaInstance;
class OcaOctaveController;
class OcaAudioController;
class OcaObjectListener;
class OcaMainWindow;
class OcaObject;
class QTimer;

class OcaApp : public QApplication
{
  Q_OBJECT ;

  public:
    OcaApp( int& argc, char** argv, bool gui_enabled );
    virtual ~OcaApp();
    int  run();
    OcaObject* getObject( oca_ulong id ) const;

  public:
    static OcaInstance* getOcaInstance() { return getSelf()->m_ocaInstance; }
    static OcaOctaveController* getOctaveController() { return getSelf()->m_octaveController; }
    static OcaAudioController*  getAudioController() { return getSelf()->m_audioController; }
    static QDir getDataCacheDir() { return getSelf()->checkDataCacheDir(); }

  protected:
    static OcaApp* getSelf() { return qobject_cast<OcaApp*>( qApp ); }
    static bool removeDirRecursively( const QString& path );

  protected:
    OcaInstance* m_ocaInstance;

  protected slots:
    void onUpdateRequired( uint flags );
    void onInstanceClosed();
    void onControllerReady( bool ready );
    int  deleteQueuedObjects();

  protected:
    oca_ulong registerObject( OcaObject* obj );
    bool      unregisterObject( OcaObject* obj );

  protected:
    QDir checkDataCacheDir();

  protected:
    OcaObjectListener*    m_listener;

    OcaOctaveController*  m_octaveController;
    OcaAudioController*   m_audioController;
    OcaMainWindow*        m_mainWindow;

    QList<OcaObject*>             m_listRemoved;
    QHash<oca_ulong,OcaObject*>   m_objects;
    mutable QMutex                m_mutex;
    QTimer*                       m_gcTimer;

    QFile   m_sessionFile;
    QDir    m_dataCacheDir;
    QString m_sessionId;

  private:
    oca_ulong   m_nextId;

  friend class OcaObject;
};

#endif  // OcaApp_h
