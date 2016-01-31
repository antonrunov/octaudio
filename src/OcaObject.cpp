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

#include "OcaObject.h"

#include "OcaApp.h"

#include <QtCore>

int OcaObject::s_objCounter = 0;

// -----------------------------------------------------------------------------

OcaObject::OcaObject()
: QObject(),
  m_container( NULL ),
  m_nameFlag( 0 ),
  m_displayNameFlag( 0 ),
  m_rwlock( QReadWriteLock::Recursive ),
  m_active( true ),
  m_id( 0 )
{
  fprintf( stderr, "new OcaObject = %p (%d)\n", this, ++s_objCounter );
  m_id = OcaApp::getSelf()->registerObject( this );
  m_active = ( 0 != m_id );
  Q_ASSERT( m_active );
}

// -----------------------------------------------------------------------------

void OcaObject::close()
{
  {
    WLock lock( this );
    if( isClosed() ) {
      return;
    }
    m_active = false;
  }

  emit closed( this );

  disconnect( SIGNAL(dataChanged(OcaObject*, uint) ) );
  disconnect( SIGNAL(dataRangeChanged(OcaObject*, uint,double,double) ) );
  bool unregistered = OcaApp::getSelf()->unregisterObject( this );
  Q_ASSERT( unregistered );
  (void) unregistered;
  onClose();
}

// -----------------------------------------------------------------------------

OcaObject::~OcaObject()
{
  Q_ASSERT( isReleased() );
  fprintf( stderr, "delete OcaObject %s = %p (%d)\n",
           m_name.toLocal8Bit().data(), this, --s_objCounter );
}

// -----------------------------------------------------------------------------

QString OcaObject::getName() const
{
  OcaLock( this );
  return m_name;
}

// -----------------------------------------------------------------------------

QString OcaObject::getDisplayName() const
{
  OcaLock( this );
  return m_displayName;
}

// -----------------------------------------------------------------------------

QString OcaObject::getDisplayText() const
{
  OcaLock( this );
  QString s;
  if( ! m_displayName.isEmpty() ) {
    s = m_displayName;
    //s = QString( "%1 - %2" ) . arg( m_displayName ) . arg( m_name );
  }
  else {
    s = m_name;
  }

  return s;
}

// -----------------------------------------------------------------------------

bool OcaObject::setName( const QString& name )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( ( ! name.isEmpty() ) && ( m_name != name ) ) {
      m_name = name;
      flags = m_nameFlag;
    }
  }

  return emitChanged( flags );
}

// -----------------------------------------------------------------------------

bool OcaObject::setDisplayName( const QString& name )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_displayName != name ) {
      m_displayName = name;
      flags = m_displayNameFlag;
    }
  }

  return emitChanged( flags );
}

// -----------------------------------------------------------------------------

bool OcaObject::isValidObject( OcaObject* obj )
{
  return ( getObject( obj->getId() ) == obj ) && ( ! obj->isClosed() );
}

// -----------------------------------------------------------------------------

OcaObject* OcaObject::getObject( oca_ulong id )
{
  return OcaApp::getSelf()->getObject( id );
}

// -----------------------------------------------------------------------------

bool OcaObject::connectObject( const OcaObject* obj, const char* slot,
                                                     bool collect /* = true */ )
{
  if( isClosed() ) {
    return false;
  }
  OcaLock lock( obj );
  if( obj->isClosed() ) {
    return false;
  }
  if( collect && ( ! obj->setContainer( NULL, this ) ) ) {
    return false;
  }
  if( ! connect( obj, SIGNAL(closed(OcaObject*)), slot, Qt::DirectConnection ) ) {
    if( collect ) {
      obj->setContainer( this, NULL );
    }
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------

void OcaObject::disconnectObject( const OcaObject* obj, bool collected /* = true */ )
{
  disconnect( obj, NULL, this, NULL );
  Q_ASSERT( collected == ( this == obj->getContainer() ) );
  if( collected ) {
    obj->setContainer( this, NULL );
  }
}

// -----------------------------------------------------------------------------

bool OcaObject::setContainer( OcaObject* old_container, OcaObject* container ) const
{
  bool result = false;
  QMutexLocker locker( &m_containerMutex );
  if( old_container == m_container ) {
    m_container = container;
    result = true;
  }
  return result;
}

// -----------------------------------------------------------------------------

bool OcaObject::emitChanged( OcaObject* obj, uint flags )
{
  bool result = false;
  if( flags ) {
    emit dataChanged( obj, flags );
    result = true;
  }
  return result;
}

// -----------------------------------------------------------------------------

