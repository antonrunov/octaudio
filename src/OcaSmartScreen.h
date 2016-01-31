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

#ifndef OcaSmartScreen_h
#define OcaSmartScreen_h

#include "octaudio.h"
#include "OcaDataScreen.h"
#include "OcaSmartTrack.h"

#include <QHash>
#include <QList>

class QLabel;
class QTimer;
class OcaScaleControl;
class OcaSmartTrack;

class OcaSmartScreen : public OcaDataScreen
{
  Q_OBJECT ;

  public:
    OcaSmartScreen(  OcaSmartTrack* trackObj, OcaTrackGroup* group );
    ~OcaSmartScreen();

  protected slots:
    void openProperties();
    void openHandleContextMenu( const QPoint& pos );

  protected:
    virtual OcaTrackBase* getTrackObject() const { return m_trackObj; }

  protected:
    QHash<OcaTrack*, QList<DataBlock*>*>  m_trackBlocks;
    OcaSmartTrack*                        m_trackObj;

  protected:
    virtual void updateTrackData( uint flags, uint group_flags );
    void         updateTracks();
    void         updateSubtrackList();
    virtual void updateData();
    virtual void updateHandleText();
    virtual void updateHandlePalette();

  protected:
    void clearHash( QHash<OcaTrack*, QList<DataBlock*>*>* hash );

  protected:
    virtual void resizeEvent( QResizeEvent* event );
};

#endif // OcaSmartScreen_h
