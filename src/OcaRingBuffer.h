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

#ifndef OcaRingBuffer_h
#define OcaRingBuffer_h

class OcaRingBuffer
{
  public:
    OcaRingBuffer( int length );
    ~OcaRingBuffer();

  public:
    int read( float* data, int length );
    int write( float* data, int length );
    int getAvailableSpace() const { return ring( m_idxRead - m_idxWrite - 1 ); }
    int getAvailableLength() const { return ring( m_idxWrite - m_idxRead ); }
    unsigned int getReadCount() const {return m_readCount;}

  protected:
    int ring( int idx ) const;

  protected:
    int           m_length;
    float*        m_buffer;

    volatile int  m_idxRead;
    volatile int  m_idxWrite;
    unsigned int          m_readCount;
};

#endif // OcaRingBuffer_h
