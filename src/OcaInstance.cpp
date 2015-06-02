/*
   Copyright 2013-2015 Anton Runov

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

#include "OcaInstance.h"

#include "OcaWindowData.h"

#include <QtCore>

// -----------------------------------------------------------------------------

OcaInstance::OcaInstance()
:
  m_mainWindowData( NULL )
{
}

// -----------------------------------------------------------------------------

void OcaInstance::setWindow( OcaWindowData* window_data )
{
  uint flags = 0;
  {
    WLock lock( this );

    Q_ASSERT( NULL == m_mainWindowData );
    if( ! connectObject( window_data, SLOT(onWindowClosed()) ) ) {
      Q_ASSERT( false );
    }
    else {
      m_mainWindowData = window_data;
      flags |= e_FlagWindowAdded;
    }
  }

  emitChanged( m_mainWindowData, flags );
}

// -----------------------------------------------------------------------------

OcaInstance::~OcaInstance()
{
  Q_ASSERT( NULL == m_mainWindowData );
}

// -----------------------------------------------------------------------------

void OcaInstance::onWindowClosed()
{
  {
    WLock lock( this );
    Q_ASSERT( NULL != m_mainWindowData );
    disconnectObject( m_mainWindowData );
    m_mainWindowData = NULL;
  }
  emitChanged( m_mainWindowData, e_FlagWindowRemoved );
}

// -----------------------------------------------------------------------------

