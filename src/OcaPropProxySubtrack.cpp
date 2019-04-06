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

#include "OcaPropProxySubtrack.h"

#include "OcaTrack.h"
#include "OcaSmartTrack.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaPropProxySubtrack::OcaPropProxySubtrack( OcaSmartTrack* strack )
:
  m_strack( strack )
{
}

// -----------------------------------------------------------------------------

OcaPropProxySubtrack::~OcaPropProxySubtrack()
{
  m_strack = NULL;
}

// -----------------------------------------------------------------------------

QColor OcaPropProxySubtrack::getColor() const
{
  return m_strack->getSubtrackColor( m_item );
}

// -----------------------------------------------------------------------------

double OcaPropProxySubtrack::getScale() const
{
  return m_strack->getSubtrackScale( m_item );
}

// -----------------------------------------------------------------------------

double OcaPropProxySubtrack::getZero() const
{
  return m_strack->getSubtrackZero( m_item );
}

// -----------------------------------------------------------------------------

bool OcaPropProxySubtrack::isAutoScaleOn() const
{
  return m_strack->isSubtrackAutoScaleOn( m_item );
}

// -----------------------------------------------------------------------------

int OcaPropProxySubtrack::getIndex() const
{
  return m_strack->findSubtrack( m_item );
}

// -----------------------------------------------------------------------------

void OcaPropProxySubtrack::setColor( QColor color )
{
  m_strack->setSubtrackColor( m_item, color );
}

// -----------------------------------------------------------------------------

void OcaPropProxySubtrack::setScale( double scale )
{
  m_strack->setSubtrackScale( m_item, scale );
}

// -----------------------------------------------------------------------------

void OcaPropProxySubtrack::setZero( double zero )
{
  m_strack->setSubtrackZero( m_item, zero );
}

// -----------------------------------------------------------------------------

void OcaPropProxySubtrack::setAutoScaleOn( bool on )
{
  m_strack->setSubtrackAutoScaleOn( m_item, on );
}

// -----------------------------------------------------------------------------


