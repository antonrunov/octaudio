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

#include "OcaObjectListener.h"

#include "OcaObject.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------
// OcaObjectListener

OcaObjectListener::OcaObjectListener(   OcaObject* objMain,
                                        uint mask,
                                        int timeout_ms,
                                        QObject* parent /* = NULL */   )
: QObject( parent ),
  m_closed( false ),
  m_mainTracker( NULL )
{
  m_timer = new QTimer( this );
  m_timer->setSingleShot( true );
  m_timer->setInterval( timeout_ms );
  connect( m_timer, SIGNAL(timeout()), SLOT(onTimeout()) );

  m_mainTracker = new OcaObjectTracker( this, objMain, mask );

  connect(  m_mainTracker,
            SIGNAL(changed(OcaObjectTracker*)),
            SLOT(onChanged(OcaObjectTracker*)),
            Qt::QueuedConnection );

  connect(  m_mainTracker,
            SIGNAL(closed(OcaObjectTracker*)),
            SLOT(onClosed(OcaObjectTracker*)),
            Qt::QueuedConnection                );

  m_mainTracker->checkObject();

}

// -----------------------------------------------------------------------------

OcaObjectListener::~OcaObjectListener()
{
}

// -----------------------------------------------------------------------------

uint OcaObjectListener::getObjectMask( OcaObject* obj ) const
{
  uint mask = 0;
  if( m_trackers.contains( obj ) ) {
    mask = m_trackers.value( obj )->getMask();
  }
  return mask;
}

// -----------------------------------------------------------------------------

uint OcaObjectListener::setObjectMask( OcaObject* obj, uint mask )
{
  uint old = 0;
  if( m_trackers.contains( obj ) ) {
    old = m_trackers.value( obj )->setMask( mask );
  }
  return old;
}

// -----------------------------------------------------------------------------

bool OcaObjectListener::addObject( OcaObject* obj, uint mask )
{
  if( m_trackers.contains( obj ) || m_mainTracker->getObject() == obj ) {
    Q_ASSERT( false );
    return false;
  }
  OcaObjectTracker* tracker = new OcaObjectTracker( this, obj, mask );
  m_trackers.insert( obj, tracker );

  connect(  tracker,
            SIGNAL(changed(OcaObjectTracker*)),
            SLOT(onChanged(OcaObjectTracker*)),
            Qt::QueuedConnection );

  connect(  tracker,
            SIGNAL(closed(OcaObjectTracker*)),
            SLOT(onClosed(OcaObjectTracker*)),
            Qt::QueuedConnection                );

  tracker->checkObject();

  return true;
}

// -----------------------------------------------------------------------------

bool OcaObjectListener::removeObject( OcaObject* obj )
{
  if( ! m_trackers.contains( obj ) ) {
    return false;
  }
  OcaObjectTracker* tracker = m_trackers.take( obj );
  Q_ASSERT( NULL != tracker );

  if( ! tracker->isClear() ) {
    int n = m_triggeredTrackers.remove( obj );
    Q_ASSERT( 1 == n ); // FAILED!
    (void) n;
  }

  delete tracker;
  tracker = NULL;
  return false;
}

// -----------------------------------------------------------------------------

bool OcaObjectListener::addEvent( OcaObject* obj, uint flags )
{
  bool result = true;
  if( ( NULL == obj ) || ( m_mainTracker->getObject() == obj ) ) {
    m_mainTracker->onDataChanged( obj, flags );
  }
  else if( m_trackers.contains( obj ) ) {
    m_trackers.value( obj )->onDataChanged( obj, flags );
  }
  else {
    result = false;
  }
  return result;
}

// -----------------------------------------------------------------------------

void OcaObjectListener::setTimeout( int timeout_ms )
{
  m_timer->setInterval( timeout_ms );
}

// -----------------------------------------------------------------------------

uint OcaObjectListener::getFlags( OcaObject* obj /* = NULL */ ) const
{
  uint flags = 0;
  if( ( NULL == obj ) || ( m_mainTracker->getObject() == obj ) ) {
    flags = m_mainTracker->getFlags();
  }
  else if( m_triggeredTrackers.contains( obj ) ) {
    flags = m_triggeredTrackers.value( obj )->getFlags();
  }
  return flags;
}

// -----------------------------------------------------------------------------

uint OcaObjectListener::getRange( double* range_min, double* range_max,
                                            OcaObject* obj /* = NULL */ ) const
{
  uint flags = 0;
  *range_min = NAN;
  *range_max = NAN;

  if( ( NULL == obj ) || ( m_mainTracker->getObject() == obj ) ) {
    flags = m_mainTracker->getRange( range_min, range_max );
  }
  else if( m_triggeredTrackers.contains( obj ) ) {
    flags = m_triggeredTrackers.value( obj )->getRange( range_min, range_max );
  }
  return flags;
}

// -----------------------------------------------------------------------------

void OcaObjectListener::onChanged( OcaObjectTracker* tracker )
{
  if( NULL != tracker->getObject() ) {
    if( m_mainTracker == tracker ) {
    }
    else if( m_trackers.contains( tracker->getObject() ) ) {
      m_triggeredTrackers.insert( tracker->getObject(), tracker );
    }
    /*
    else {
      Q_ASSERT( false );
    }
    */
    if( ! m_timer->isActive() ) {
      m_timer->start();
    }
  }
}

// -----------------------------------------------------------------------------

void OcaObjectListener::onClosed( OcaObjectTracker* tracker )
{
  if( m_closed ) {
    return;
  }
  if( m_mainTracker == tracker ) {
    m_closed = true;
    m_timer->stop();
    QHashIterator<OcaObject*,OcaObjectTracker*> it(m_trackers);
    while( it.hasNext() ) {
      it.next();
      delete it.value();
    }
    m_trackers.clear();
    m_mainTracker->disconnect( this );
  }
  else if( m_trackers.contains( tracker->getObject() ) ) {
    if( ! tracker->isClear() ) {
      m_triggeredTrackers.remove( tracker->getObject()  );
      //int n = m_triggeredTrackers.remove( tracker->getObject()  );
      //Q_ASSERT( 1 == n );
    }
    m_trackers.remove( tracker->getObject() );
    delete tracker;
    tracker = NULL;
  }
  else {
    Q_ASSERT( false );
  }

  if( m_closed ) {
    emit closed();
  }
}

// -----------------------------------------------------------------------------

void OcaObjectListener::onTimeout()
{
  Q_ASSERT( ! m_closed );
  uint flags = 0;
  bool changed = false;
  QList<OcaObject*> objects;
  QHash<QString,uint> cum_flags;
  {
    QMutexLocker locker( &m_mutex );

    flags = m_mainTracker->reset();
    changed = ( 0 != flags );


    QHashIterator<OcaObject*,OcaObjectTracker*> it(m_triggeredTrackers);
    while( it.hasNext() ) {
      it.next();
      OcaObjectTracker* tracker = it.value();
      OcaObject* obj = tracker->getObject();
      Q_ASSERT( it.key() == obj );
      Q_ASSERT( m_trackers.contains( obj ) );
      cum_flags[ obj->metaObject()->className() ] |= tracker->reset();
      objects.append( obj );
      changed = true;
    }
  }

  if( changed ) {
    emit updateRequired( flags, cum_flags, objects );
  }

  m_triggeredTrackers.clear();
}


// -----------------------------------------------------------------------------
// OcaObjectTracker

OcaObjectTracker::OcaObjectTracker( OcaObjectListener* parent,
                                               OcaObject* obj,
                                                    uint mask   )
: QObject( parent ),
  m_object( obj ),
  m_mutex( &parent->m_mutex ),
  m_mask( mask ),
  m_clear( true ),
  m_flagsOld( 0 ),
  m_rangeFlagsOld( 0 ),
  m_rangeMinOld( NAN ),
  m_rangeMaxOld( NAN ),
  m_flagsCurrent( 0 ),
  m_rangeFlagsCurrent( 0 ),
  m_rangeMinCurrent( NAN ),
  m_rangeMaxCurrent( NAN )
{
  connect( m_object, SIGNAL(closed(OcaObject*)),
                     SLOT(onClosed(OcaObject*)), Qt::DirectConnection );
  connect( m_object, SIGNAL(dataChanged(OcaObject*,uint)),
                     SLOT(onDataChanged(OcaObject*,uint)), Qt::DirectConnection );
  connect(  m_object,
            SIGNAL(dataRangeChanged(OcaObject*,uint,double,double)),
            SLOT(onDataRangeChanged(OcaObject*,uint,double,double)),
                                                Qt::DirectConnection  );
}

// -----------------------------------------------------------------------------

OcaObjectTracker::~OcaObjectTracker()
{
  m_object->disconnect( this );
  m_object = NULL;
}

// -----------------------------------------------------------------------------

uint OcaObjectTracker::reset()
{
  m_flagsOld      = m_flagsCurrent;
  m_rangeFlagsOld = m_rangeFlagsCurrent;
  m_rangeMinOld   = m_rangeMinCurrent;
  m_rangeMaxOld   = m_rangeMaxCurrent;

  m_flagsCurrent      = 0;
  m_rangeFlagsCurrent = 0;
  m_rangeMinCurrent   = NAN;
  m_rangeMaxCurrent   = NAN;

  m_clear = true;

  return m_flagsOld;
}

// -----------------------------------------------------------------------------

uint OcaObjectTracker::getRange( double* range_min, double* range_max ) const
{
  *range_min = m_rangeMinOld;
  *range_max = m_rangeMaxOld;
  return m_rangeFlagsOld;
}

// -----------------------------------------------------------------------------

void OcaObjectTracker::checkObject()
{
  if( NULL != m_object ) {
    if( m_object->isClosed() ) {
      emit closed( this );
    }
  }
}

// -----------------------------------------------------------------------------

void OcaObjectTracker::onClosed( OcaObject* )
{
  emit closed( this );
}

// -----------------------------------------------------------------------------

void OcaObjectTracker::onDataChanged( OcaObject*, uint flags )
{
  flags &= m_mask;
  bool emit_required = false;
  if( flags ) {
    QMutexLocker locker( m_mutex );
    m_flagsCurrent |= flags;
    if( m_clear ) {
      emit_required = true;
      m_clear = false;
    }
  }

  if( emit_required ) {
    emit changed( this );
  }
}

// -----------------------------------------------------------------------------

void OcaObjectTracker::onDataRangeChanged( OcaObject*, uint flags,
                                                       double range_min,
                                                       double range_max )
{
  flags &= m_mask;
  bool emit_required = false;
  if( flags ) {
    QMutexLocker locker( m_mutex );
    m_rangeFlagsCurrent |= flags;
    if( std::isnan( m_rangeMinCurrent ) ) {
      m_rangeMinCurrent = range_min;
    }
    else {
      m_rangeMinCurrent = qMin( m_rangeMinCurrent, range_min );
    }
    if( std::isnan( m_rangeMaxCurrent ) ) {
      m_rangeMaxCurrent = range_max;
    }
    else {
      m_rangeMaxCurrent = qMax( m_rangeMaxCurrent, range_max );
    }
    if( m_clear ) {
      emit_required = true;
      m_clear = false;
    }
  }

  if( emit_required ) {
    emit changed( this );
  }
}

// -----------------------------------------------------------------------------

