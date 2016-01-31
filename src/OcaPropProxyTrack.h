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

#ifndef OcaPropProxyTrack_h
#define OcaPropProxyTrack_h

#include "OcaOctaveHost.h"

class OcaTrackBase;
class OcaTrackGroup;

class OcaPropProxyTrack : public OcaPropProxy<OcaTrackBase>
{
  Q_OBJECT ;
  Q_PROPERTY( int index READ getIndex );
  Q_PROPERTY( int height READ getHeight WRITE setHeight );

  public:
    OcaPropProxyTrack( OcaTrackGroup* group );
    ~OcaPropProxyTrack();

  public:
    int     getIndex() const;
    int     getHeight() const;
    void    setHeight( int height );

  protected:
    OcaTrackGroup*  m_group;
};

#endif // OcaPropProxySubtrack_h

