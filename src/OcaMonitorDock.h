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

#ifndef OcaMonitorDock_h
#define OcaMonitorDock_h

#include <QDockWidget>

class OcaMonitor;
class OcaScaleControl;
class OcaObjectListener;

class OcaMonitorDock : public QDockWidget {
  Q_OBJECT ;

  public:
    OcaMonitorDock(  OcaMonitor* monitor );
    ~OcaMonitorDock();

  protected:
    void closeEvent( QCloseEvent* event );

  protected:
    OcaObjectListener*  m_listener;
    OcaMonitor*         m_monitor;
    OcaScaleControl*    m_cursorControl;
    OcaScaleControl*    m_yScaleControl;

  protected slots:
    virtual void openContextMenu( const QPoint& pos );
    virtual void onUpdateRequired( uint flags );
    virtual void openProperties();
};

#endif // OcaMonitorDock_h
