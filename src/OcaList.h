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

#ifndef OcaList_h
#define OcaList_h

#include "OcaListBase.h"

template <typename Item, typename Data> class OcaList : public OcaListBase
{
  public:
    Item*       getItem( oca_index idx, Data** data = NULL ) const { return (Item*) getItemInternal( idx, (void**)data ); }
    Data*       getItemData( oca_index idx ) const { return (Data*) getItemDataInternal( idx ); }
    Item*       findItem( const OcaObject* obj ) const { return getItem( findItemIndex( obj ) ); }
    Data*       findItemData( const OcaObject* obj ) const { return (Data*) getItemDataInternal( findItemIndex( obj ) ); }
    oca_index   appendItem( Item* item, Data* data = NULL ) { return appendItemInternal( (OcaObject*)item, data ); }
    oca_index   removeItem( Item* item ) { return OcaListBase::removeItem( (OcaObject*)item ); }
    oca_index   moveItem( Item* item, oca_index idx ) { return OcaListBase::moveItem( (OcaObject*)item, idx ); }

    QList<Item*> findItemsByName( const QString& name ) const
    {
      QList<Item*> result;
      for( oca_index idx = 0; idx < m_items.size(); idx++ ) {
        Item* obj= getItem( idx );
        if( obj->getName() == name ) {
          result.append( obj );
        }
      }
      return result;
    }
};

#endif // OcaList_h
