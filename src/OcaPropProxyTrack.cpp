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

#include "OcaPropProxyTrack.h"

#include "OcaTrackBase.h"
#include "OcaTrackGroup.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaPropProxyTrack::OcaPropProxyTrack( OcaTrackGroup* group )
:
  m_group( group )
{
}

// -----------------------------------------------------------------------------

OcaPropProxyTrack::~OcaPropProxyTrack()
{
  m_group = NULL;
}

// -----------------------------------------------------------------------------

int OcaPropProxyTrack::getIndex() const
{
  return m_group->getTrackIndex( m_item ) + 1;
}

// -----------------------------------------------------------------------------

int OcaPropProxyTrack::getHeight() const
{
  return m_group->getTrackHeight( m_item );
}

// -----------------------------------------------------------------------------

void OcaPropProxyTrack::setHeight( int height )
{
  m_group->setTrackHeight( m_item, height );
}

// -----------------------------------------------------------------------------

