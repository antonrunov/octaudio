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

#ifndef OcaBlockList_h
#define OcaBlockList_h

#include "octaudio.h"

#include <QList>
#include <QVarLengthArray>

template<typename Data> class OcaBlockList
{
  public:
    ~OcaBlockList() {
      clear();
    }
  public:
    void appendBlock( double t, Data* block ) {
      Item d;
      d.t = t;
      d.block = block;
      m_blocks.append(d);
    }
    void clear() {
      for( int i = 0; i < m_blocks.size(); i++ ) {
        delete m_blocks.at(i).block;
      }
      m_blocks.clear();
    }
    bool    isEmpty() const { return m_blocks.isEmpty(); }
    int     getSize() const { return m_blocks.size(); }
    double  getTime( int idx ) const { return m_blocks.at(idx).t; }
    Data*   getBlock( int idx ) const { return m_blocks.at(idx).block; }

  protected:
    struct Item {
      double  t;
      Data*   block;
    };

    QList<Item>   m_blocks;
};

#endif // OcaBlockList_h
