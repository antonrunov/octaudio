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

#include "OcaRingBuffer.h"

#include <QtCore>

// -----------------------------------------------------------------------------

OcaRingBuffer::OcaRingBuffer( int length )
:
  m_length( length + 1 ),
  m_idxRead( 0 ),
  m_idxWrite( 0 )
{
  m_buffer = new float[ m_length ];
}

// -----------------------------------------------------------------------------

OcaRingBuffer::~OcaRingBuffer()
{
  if( NULL != m_buffer ) {
    delete [] m_buffer;
    m_buffer = NULL;
  }
}

// -----------------------------------------------------------------------------

int OcaRingBuffer::read( float* data, int length )
{
  int available_length = getAvailableLength();
  int remainder = 0;
  if( length > available_length ) {
    remainder = length - available_length;
    length = available_length;
  }
  Q_ASSERT( length < m_length );

  if( 0 < length ) {
    int next_idx = ring( m_idxRead + length );
    if( next_idx > m_idxRead ) {
      memcpy( data, m_buffer + m_idxRead, length * sizeof(float) );
    }
    else {
      int tmp_length = m_length - m_idxRead;
      memcpy( data, m_buffer + m_idxRead, tmp_length * sizeof(float) );
      memcpy( data + tmp_length, m_buffer, (length - tmp_length) * sizeof(float) );
    }
    m_idxRead = next_idx;
  }
  if( 0 < remainder ) {
    memset( data + length, 0, remainder * sizeof(float) );
  }
  ++m_readCount;
  return length;
}

// -----------------------------------------------------------------------------

int OcaRingBuffer::write( float* data, int length )
{
  int available_space = getAvailableSpace();
  if( length > available_space ) {
    length = available_space;
  }

  if( 0 < length ) {
    int next_idx = ring( m_idxWrite + length );
    if( next_idx > m_idxWrite ) {
      memcpy( m_buffer + m_idxWrite, data, length * sizeof(float) );
    }
    else {
      int tmp_length = m_length - m_idxWrite;
      memcpy( m_buffer + m_idxWrite, data, tmp_length * sizeof(float) );
      memcpy( m_buffer, data + tmp_length, (length - tmp_length) * sizeof(float) );
    }
    m_idxWrite = next_idx;
  }
  return length;
}

// -----------------------------------------------------------------------------

int OcaRingBuffer::ring( int idx ) const
{
  int n = idx % m_length;
  if( 0 > n ) {
    n += m_length;
  }
  return n;
}

// -----------------------------------------------------------------------------

