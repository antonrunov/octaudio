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

#ifndef OcaPropProxySubtrack_h
#define OcaPropProxySubtrack_h

#include "OcaOctaveHost.h"

#include <QColor>

class OcaSmartTrack;
class OcaTrack;

class OcaPropProxySubtrack : public OcaPropProxy<OcaTrack>
{
  Q_OBJECT ;
  //Q_PROPERTY( bool auto_scale READ isAutoScaleOn  WRITE setAutoScaleOn );
  Q_PROPERTY( double scale READ getScale  WRITE setScale );
  Q_PROPERTY( double zero READ getZero  WRITE setZero );
  Q_PROPERTY( QColor color READ getColor  WRITE setColor );
  Q_PROPERTY( int index READ getIndex );

  public:
    OcaPropProxySubtrack( OcaSmartTrack* strack );
    ~OcaPropProxySubtrack();

  public:
    QColor  getColor() const;
    double  getScale() const;
    double  getZero() const;
    bool    isAutoScaleOn() const;
    int     getIndex() const;

    void    setColor( QColor color );
    void    setScale( double scale );
    void    setZero( double zero );
    void    setAutoScaleOn( bool on );

  protected:
    OcaSmartTrack*  m_strack;
};

#endif // OcaPropProxySubtrack_h
