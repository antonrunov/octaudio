/*
   Copyright 2018-2019 Anton Runov

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

#pragma once

#include <QDockWidget>

class OcaObjectListener;
class QScrollBar;
class Oca3DPlot;
class QLabel;

namespace QtDataVisualization {
  class Q3DSurface;
  class QSurface3DSeries;
}

using namespace QtDataVisualization;

class Oca3DPlotDock : public QDockWidget {
  Q_OBJECT ;

  public:
    Oca3DPlotDock(Oca3DPlot* plot);
    ~Oca3DPlotDock();

  protected:
    void closeEvent( QCloseEvent* event );
    bool eventFilter( QObject* obj, QEvent* ev );

  protected:
    OcaObjectListener*  m_listener;
    Oca3DPlot*          m_plot;

    Q3DSurface*       m_surface;
    QSurface3DSeries* m_seriesData;
    QScrollBar*       m_xScrollBar;
    QScrollBar*       m_yScrollBar;
    double*           m_data;
    QLabel*           m_info;

  protected:
    void updateData();
    void handlePositionChange(const QPoint &position);

  protected slots:
    virtual void onUpdateRequired( uint flags );
    void updateXAxis();
    void updateYAxis();
    virtual void openContextMenu( const QPoint& pos );
    virtual void openProperties();

};
