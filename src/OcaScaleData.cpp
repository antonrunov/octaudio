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

#include "OcaScaleData.h"

#include "math.h"

// -----------------------------------------------------------------------------

OcaScaleData::OcaScaleData()
:
  m_scale_log( 0.0 ),
  m_scale( 1.0 ),
  m_zero( 0.0 )
{
}

// -----------------------------------------------------------------------------

OcaScaleData::~OcaScaleData()
{
}

// -----------------------------------------------------------------------------

double OcaScaleData::moveScale( double step )
{
  m_scale_log += step / 8;
  m_scale = pow( 2.0, m_scale_log );
  return m_scale;
}

// -----------------------------------------------------------------------------

double OcaScaleData::moveZero( double step )
{
  setZero( m_zero - step * 0.05 * m_scale );
  return m_zero;
}

// -----------------------------------------------------------------------------

bool OcaScaleData::setScale( double scale )
{
  bool changed = false;
  if( ( 0 < scale ) && ( m_scale != scale ) ) {
    m_scale_log = log2( scale );
    m_scale = scale;
    changed = true;
  }
  return changed;
}

// -----------------------------------------------------------------------------

bool OcaScaleData::setZero( double zero )
{
  bool changed = false;
  if( m_zero != zero ) {
    m_zero = zero;
    changed = true;
  }
  return changed;
}

// -----------------------------------------------------------------------------

