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

#ifndef OcaMonitorScreen_h
#define OcaMonitorScreen_h

#include "octaudio.h"
#include "OcaDataScreen.h"
#include "OcaSmartScreen.h"

#include <QMutex>

class QLabel;
class QTimer;
class OcaScaleControl;
class OcaMonitor;

class OcaMonitorScreen : public OcaSmartScreen
{
  Q_OBJECT ;

  public:
    OcaMonitorScreen(  OcaMonitor* monitorObj );
    ~OcaMonitorScreen();

  protected:
    virtual uint updateViewport( uint group_flags );
    virtual void updateHandleText() {}
    virtual void updateHandlePalette() {}

  protected:
    virtual void wheelEvent( QWheelEvent* event );
    virtual void resizeEvent( QResizeEvent* event );
    virtual void dropEvent( QDropEvent* event );
};

#endif // OcaMonitorScreen_h
