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

#include "OcaListBase.h"

#include "OcaObject.h"

#include <QtCore>

// -----------------------------------------------------------------------------
// OcaListBase::ItemData

class OcaListBase::ItemData
{
  public:
    ItemData( OcaObject* obj, void* data );
    ~ItemData();

    OcaObject*  m_obj;
    void*       m_data;
};

// -----------------------------------------------------------------------------

OcaListBase::ItemData::ItemData( OcaObject* obj, void* data )
:
  m_obj( obj ),
  m_data( data )
{
}

// -----------------------------------------------------------------------------

OcaListBase::ItemData::~ItemData()
{
  m_obj = NULL;
  m_data = NULL;
}


// -----------------------------------------------------------------------------
// OcaListBase

OcaListBase::OcaListBase()
{
}

// -----------------------------------------------------------------------------

OcaListBase::~OcaListBase()
{
  clear();
}

// -----------------------------------------------------------------------------

oca_index OcaListBase::findItemIndex( const OcaObject* item ) const
{
  oca_index idx = m_index.value( item, -1 );
  if( -1 < idx ) {
    Q_ASSERT( m_items.size() > idx );
    Q_ASSERT( m_items.at(idx)->m_obj == item );
  }
  else {
    Q_ASSERT( -1 == idx );
  }
  return idx;
}

// -----------------------------------------------------------------------------

oca_index OcaListBase::removeItem( OcaObject* item )
{
  oca_index idx = findItemIndex( item );
  if( -1 < idx ) {
    ItemData* data = m_items.takeAt( idx );
    m_index.remove( item );
    if( idx < m_items.size() ) {
      updateIndex( idx );
    }
    delete data;
    data = NULL;
  }

  return idx;
}

// -----------------------------------------------------------------------------

oca_index OcaListBase::moveItem( OcaObject* item, oca_index idx )
{
  oca_index idx_old = findItemIndex( item );
  if( 0 > idx_old ) {
    return -1;
  }
  if( ( 0 > idx ) || ( m_items.size() <= idx ) ) {
    return -1;
  }

  ItemData* data = m_items.takeAt( idx_old );
  m_items.insert( idx, data );
  if( idx < idx_old ) {
    updateIndex( idx, idx_old );
  }
  else {
    updateIndex( idx_old, idx );
  }
  return idx_old;
}

// -----------------------------------------------------------------------------

void OcaListBase::clear()
{
  while( ! m_items.isEmpty() ) {
    delete m_items.takeFirst();
  }
  m_index.clear();
}

// -----------------------------------------------------------------------------

bool OcaListBase::isEmpty() const
{
  return m_items.isEmpty();
}

// -----------------------------------------------------------------------------

ulong OcaListBase::getLength() const
{
  return m_items.size();
}

// -----------------------------------------------------------------------------

OcaObject* OcaListBase::getItemInternal( oca_index idx, void** data ) const
{
  OcaObject* obj = NULL;
  if( ( -1 < idx ) && ( m_items.size() > idx ) ) {
    ItemData* t = m_items.at( idx );
    obj = t->m_obj;
    if( NULL != data ) {
      *data = t->m_data;
    }
  }
  return obj;
}

// -----------------------------------------------------------------------------

void* OcaListBase::getItemDataInternal( oca_index idx ) const
{
  void* data = NULL;
  getItemInternal( idx, &data );
  return data;
}

// -----------------------------------------------------------------------------

oca_index OcaListBase::appendItemInternal( OcaObject* obj, void* data )
{
  oca_index idx = -1;
  if( !  m_index.contains( obj ) ) {
    ItemData* itemdata = new ItemData( obj, data );
    idx = m_items.size();
    m_items.append( itemdata );
    m_index.insert( obj, idx );
  }
  return idx;
}

// -----------------------------------------------------------------------------

void OcaListBase::updateIndex( oca_index start, oca_index stop /* = -1 */ )
{
  if( -1 == stop ) {
    stop = m_items.size() - 1;
  }
  Q_ASSERT( start <= stop );
  for( oca_index idx = start; idx <= stop; idx++ ) {
    OcaObject* item = m_items.at( idx )->m_obj;
    Q_ASSERT( m_index.contains( item ) );
    m_index[ item ] = idx;
  }
}

// -----------------------------------------------------------------------------

