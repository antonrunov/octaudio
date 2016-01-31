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

#ifndef OcaMonitor_h
#define OcaMonitor_h

#include "octaudio.h"
#include "OcaSmartTrack.h"

class OcaTrackGroup;

class OcaMonitor : public OcaSmartTrack
{
  Q_OBJECT ;
  Q_PROPERTY( OcaTrackGroup* group READ getGroup  WRITE setGroup );
  Q_PROPERTY( double cursor READ getCursor  WRITE setCursor );
  Q_PROPERTY( double time_scale READ getYScale  WRITE setYScale );

  public:
    OcaMonitor( const QString& name, OcaTrackGroup* group );
    virtual ~OcaMonitor();

  public:
    bool            setGroup( OcaTrackGroup* group );
    OcaTrackGroup*  getGroup() const { return m_group; };
    double          getCursor() const;
    double          getYScale() const { return m_timeData.getScale(); }

  public slots:
    bool            setCursor( double cursor );
    bool            setYScale( double duration );
    void            moveCursor( double step );
    void            moveYScale( double step );

  protected:
    OcaTrackGroup*  m_group;
    OcaScaleData    m_timeData;

  protected slots:
    void onGroupChanged( OcaObject* obj, uint flags );
    void onGroupClosed( OcaObject* obj );

  protected:
    virtual void onClose();

  public:
    virtual bool isSolo() const { return false; }
    virtual void setHidden( bool ) {}
    virtual bool isAudible() const { return false; }
    virtual void setSelected( bool ) {}
};


#endif // OcaMonitor_h
