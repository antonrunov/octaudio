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

#ifndef OcaTrackScreen_h
#define OcaTrackScreen_h

#include "octaudio.h"
#include "OcaDataScreen.h"
#include "OcaTrack.h"

#include <QMutex>

class OcaTrackGroup;
class QLabel;
class QTimer;

class OcaTrackScreen : public OcaDataScreen
{
  Q_OBJECT ;

  public:
    OcaTrackScreen(  OcaTrack* trackObj, OcaTrackGroup* group );
    ~OcaTrackScreen();

  protected:
    virtual void updateTrackData(  uint flags, uint group_flags );
    virtual void updateHandlePalette();
    virtual void updateHandleText();
    void updateScale();
    void updateData();

  protected:
    virtual OcaTrackBase* getTrackObject() const { return m_trackObj; }

  protected slots:
    void setRec1( bool on );
    void setRec2( bool on );
    void openProperties();

  protected slots:
    virtual void openHandleContextMenu( const QPoint& pos );

  protected:
    virtual void resizeEvent( QResizeEvent* event );

  protected:
    OcaTrack*     m_trackObj;

};

#endif // OcaTrackScreen_h
