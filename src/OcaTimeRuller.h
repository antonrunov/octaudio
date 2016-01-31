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

#ifndef OcaTimeRuller_h
#define OcaTimeRuller_h

#include <QWidget>

class OcaTrackGroup;
class OcaObjectListener;

class OcaTimeRuller : public QWidget
{
  Q_OBJECT ;

  public:
    OcaTimeRuller( OcaTrackGroup* group );
    ~OcaTimeRuller();

  public:
    void setZeroOffset( int offset ) { m_zeroOffset = offset; }
    void setBasePosition( double t );

  protected:
    void setViewport( OcaTrackGroup* group );

  protected:
    int   mapFromView( OcaTrackGroup* group, double t ) const;

  protected:
    OcaTrackGroup*      m_group;
    OcaObjectListener*  m_listener;

  protected slots:
    virtual void onUpdateRequired( uint flags );

  protected:
    int    m_zeroOffset;
    double m_timeScale;
    double m_offset;
    double m_timeMs;
    double m_step;
    double m_stepMs;
    int    m_labelPrecision;
    int    m_subTicks;
    double m_cursorPosition;
    double m_selectionLeft;
    double m_selectionRight;
    double m_basePosition;
    double m_basePositionTime;

  protected:
    virtual void paintEvent( QPaintEvent* e );

};

#endif // OcaTimeRuller_h
