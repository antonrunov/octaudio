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

#ifndef OcaListBase_h
#define OcaListBase_h

#include "octaudio.h"
#include "OcaObject.h"

#include <QList>
#include <QHash>

class OcaListBase
{
  public:
    OcaListBase();
    ~OcaListBase();

  public:
    oca_index   findItemIndex( const OcaObject* item ) const;
    oca_index   removeItem( OcaObject* item );
    oca_index   moveItem( OcaObject* item, oca_index idx );
    void        clear();
    bool        isEmpty() const;
    ulong       getLength() const;

  protected:
    OcaObject*  getItemInternal( oca_index idx, void** data ) const;
    void*       getItemDataInternal( oca_index idx ) const;
    oca_index   appendItemInternal( OcaObject*, void* data );
    void        updateIndex( oca_index start, oca_index stop = -1 );

  protected:
    class ItemData;
    QList<ItemData*>                    m_items;
    QHash<const OcaObject*,oca_index>   m_index;
};

#endif // OcaListBase_h

