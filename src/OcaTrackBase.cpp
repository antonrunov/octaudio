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

#include "OcaTrackBase.h"

#include "OcaTrackGroup.h"

#include <QtCore>

// -----------------------------------------------------------------------------

OcaTrackBase::OcaTrackBase( const QString& name )
:
  m_selected( false ),
  m_absValueMode( false ),
  m_hidden( false )
{
  static int counter = 1;
  if( name.isEmpty() ) {
    setName( QString( "track %1" ) . arg( counter++, 3, 10, QChar('0') ) );
  }
  else {
    setName( name );
  }
  m_nameFlag = e_FlagNameChanged;
  m_displayNameFlag = e_FlagNameChanged;
}

// -----------------------------------------------------------------------------

OcaTrackBase::~OcaTrackBase()
{
}

// -----------------------------------------------------------------------------

void OcaTrackBase::setHidden( bool hidden )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_hidden != hidden ) {
      setSelected( false );
      m_hidden = hidden;
      flags = e_FlagHiddenChanged;
    }
  }

  emitChanged( flags );
}

// -----------------------------------------------------------------------------

bool OcaTrackBase::isHidden() const
{
  return m_hidden;
}

// -----------------------------------------------------------------------------

void OcaTrackBase::setSelected( bool selected )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( ! isHidden() ) {
      if( m_selected != selected ) {
        m_selected = selected;
        flags = e_FlagSelectedChanged;
      }
    }
  }

  emitChanged( flags );
}

// -----------------------------------------------------------------------------

void OcaTrackBase::setAbsValueMode( bool on )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_absValueMode != on ) {
      m_absValueMode = on;
      flags = e_FlagAbsValueModeChanged;
    }
  }
  emitChanged( flags );
}

// -----------------------------------------------------------------------------

