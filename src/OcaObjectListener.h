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

#ifndef OcaObjectListener_h
#define OcaObjectListener_h

#include <QObject>
#include <QMutex>
#include <QList>
#include <QHash>

class QTimer;
class OcaObject;
class OcaObjectTracker;

class OcaObjectListener : public QObject
{
  Q_OBJECT ;

  public:
    OcaObjectListener( OcaObject* objMain, uint mask, int timeout_ms,
                                                      QObject* parent = NULL );
    ~OcaObjectListener();

  public:
    uint getObjectMask( OcaObject* obj ) const;
    uint setObjectMask( OcaObject* obj, uint mask );
    bool addObject( OcaObject* obj, uint mask );
    bool removeObject( OcaObject* obj );
    bool addEvent( OcaObject* obj, uint flag );

  public:
    void setTimeout( int timeout_ms );
    bool isClosed() const { return m_closed; }
    uint getFlags( OcaObject* obj = NULL ) const;
    uint getRange( double* range_min, double* range_max, OcaObject* obj = NULL ) const;

  signals:
    void closed();
    void updateRequired( uint flags, QHash<QString,uint>& cum_flags,
                                          QList<OcaObject*>& objects  );

  protected slots:
    void onChanged( OcaObjectTracker* tracker );
    void onClosed( OcaObjectTracker* tracker );
    void onTimeout();

  protected:
    QTimer*           m_timer;
    mutable QMutex    m_mutex;
    bool              m_closed;

    OcaObjectTracker*                     m_mainTracker;
    QHash<OcaObject*,OcaObjectTracker*>   m_trackers;
    QHash<OcaObject*,OcaObjectTracker*>   m_triggeredTrackers;

  friend class OcaObjectTracker;
};


class OcaObjectTracker : public QObject
{
  Q_OBJECT;

  public:
    OcaObjectTracker( OcaObjectListener* parent,
                                 OcaObject* obj,
                                      uint mask   );
    ~OcaObjectTracker();

  signals:
    void changed( OcaObjectTracker* tracker );
    void closed( OcaObjectTracker* tracker );

  public:
    uint        reset();
    bool        isClear() const { return m_clear; }
    uint        getFlags() const { return m_flagsOld; }
    uint        getRange( double* range_min, double* range_max ) const;
    OcaObject*  getObject() const { return m_object; }
    uint        getMask() const { return m_mask; }
    uint        setMask( uint mask ) { uint old = m_mask; m_mask = mask; return old; }
    void        checkObject();

  public slots:
    void onDataChanged( OcaObject* obj, uint flags );

  protected slots:
    void onClosed( OcaObject* obj );
    void onDataRangeChanged( OcaObject* obj, uint flags, double range_min, double range_max );

  protected:
    OcaObject*  m_object;
    QMutex*     m_mutex;
    uint        m_mask;
    bool        m_clear;

    uint        m_flagsOld;
    uint        m_rangeFlagsOld;
    double      m_rangeMinOld;
    double      m_rangeMaxOld;

    uint        m_flagsCurrent;
    uint        m_rangeFlagsCurrent;
    double      m_rangeMinCurrent;
    double      m_rangeMaxCurrent;
};

#endif // OcaObjectListener_h
