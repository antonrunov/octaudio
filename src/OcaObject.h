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

# ifndef OcaObject_h
# define OcaObject_h

#include "octaudio.h"

#include <QObject>
#include <QReadWriteLock>
#include <QMutex>
#include <QMetaType>

class OcaObject : public QObject
{
  Q_OBJECT ;
  Q_PROPERTY( QString name READ getName WRITE setName );
  Q_PROPERTY( QString display_name READ getDisplayName WRITE setDisplayName );
  Q_PROPERTY( QString display_text READ getDisplayText );

  public:
    OcaObject();
    virtual ~OcaObject();

  public slots:
    void close();

  public:
    oca_ulong getId() const { return m_id; }
    bool isClosed() const { return ! m_active; }
    bool isReleased() const { return( 0 == receivers( SIGNAL(closed(OcaObject*)) ) ); }
    QString getName() const;
    QString getDisplayName() const;
    QString getDisplayText() const;
    bool setName( const QString& name );
    bool setDisplayName( const QString& name );

  public:
    static bool       isValidObject( OcaObject* obj );
    static OcaObject* getObject( oca_ulong id );

  protected:
    bool connectObject( const OcaObject* obj, const char* slot, bool collect = true );
    void disconnectObject( const OcaObject* obj, bool collected = true );
    virtual void onClose() {}

  public:
    OcaObject* getContainer() const { return m_container; }

  private:
    mutable OcaObject*  m_container;
    mutable QMutex      m_containerMutex;

    bool setContainer( OcaObject* old_container, OcaObject* container ) const;

  signals:
    void closed( OcaObject* obj );
    void dataChanged( OcaObject* obj, uint flags );
    void dataRangeChanged( OcaObject* obj, uint flags, double rangeMin, double rangeMax );

  protected:
    bool emitChanged( uint flags ) { return emitChanged( this, flags ); }
    bool emitChanged( OcaObject* obj, uint flags );

  protected:
    QString m_name;
    QString m_displayName;
    uint    m_nameFlag;
    uint    m_displayNameFlag;

  private:
    mutable QReadWriteLock  m_rwlock;

  protected:
    class WLock : private QWriteLocker
    {
      public:
        WLock( OcaObject* obj ) : QWriteLocker( &obj->m_rwlock ) {}
        ~WLock() {}
        void lock () { QWriteLocker::relock(); }
        void unlock () { QWriteLocker::unlock(); }
    };

  private:
    bool      m_active;
    oca_ulong m_id;

  private:
    static int  s_objCounter;

  friend class OcaLock;
};

class OcaLock : private QReadLocker
{
  public:
    OcaLock( const OcaObject* obj ) : QReadLocker( &obj->m_rwlock ) {}
    ~OcaLock() {}
    void lock () { QReadLocker::relock(); }
    void unlock () { QReadLocker::unlock(); }
};

# endif // OcaObject_h
