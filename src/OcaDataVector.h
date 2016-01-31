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

#ifndef OcaDataVector_h
#define OcaDataVector_h

#include <QtGlobal>

template <typename Type> class OcaBareArray {
  public:
    OcaBareArray() : m_channels(0), m_alloc(0), m_length(0), m_data(NULL) {}
    OcaBareArray( int channels, long len ) : m_alloc(0), m_data(NULL) { alloc( channels, len, len ); }
    ~OcaBareArray() { clear(); }

  public:
    long length() const { return m_length; }
    int  channels() const { return m_channels; }
    Type* data() { return m_data; }
    const Type* constData() const { return m_data; }
    bool isEmpty() const { return ( 0 == m_length ); }
    void alloc( int channels, long len ) { alloc( channels, len, len ); }
    void alloc( int channels, long len, long alloc_len );
    void clear();
    void setLength( long len ) { Q_ASSERT( len * m_channels <= m_alloc ); m_length = len; }

  protected:
    int     m_channels;
    long    m_alloc;
    long    m_length;
    Type*   m_data;
};

template <typename Type>
inline void OcaBareArray<Type>::alloc( int channels, long len, long alloc_len )
{
  Q_ASSERT( len <= alloc_len );
  Q_ASSERT( 0 < channels );
  long size = alloc_len * channels;
  if( m_alloc < size ) {
    if( NULL != m_data ) {
      Q_ASSERT( false );
      delete [] m_data;
    }
    m_data = new Type[ size ];
    m_alloc = size;
  }
  m_channels = channels;
  m_length = len;
}

template <typename Type>
inline void OcaBareArray<Type>::clear()
{
  if( NULL != m_data ) {
    m_alloc = 0;
    m_length = 0;
    m_channels = 0;
    delete [] m_data;
    m_data = NULL;
  }
}

struct OcaAvgData {
  double min;
  double max;
  double avg;
  double var;
};

typedef OcaBareArray<double>     OcaDataVector;
typedef OcaBareArray<OcaAvgData> OcaAvgVector;
typedef OcaBareArray<float>      OcaFloatVector;

#endif // OcaDataVector_h
