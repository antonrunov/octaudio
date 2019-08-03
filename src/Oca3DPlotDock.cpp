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

#include "Oca3DPlotDock.h"
#include "Oca3DPlot.h"
#include "OcaDialogProperties3DPlot.h"

#include "OcaObjectListener.h"
#include "OcaTrack.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtDataVisualization>

// -----------------------------------------------------------------------------

Oca3DPlotDock::Oca3DPlotDock(Oca3DPlot* plot)
:
  m_listener( NULL ),
  m_plot( plot )
{
  m_listener = new OcaObjectListener( m_plot, Oca3DPlot::e_FlagALL, 10, this );
  connect(  m_listener,
            SIGNAL(updateRequired(uint,QHash<QString,uint>&,QList<OcaObject*>&)),
            SLOT(onUpdateRequired(uint))  );
  m_listener->addEvent( NULL, Oca3DPlot::e_FlagALL );

  QWidget* view = new QWidget();
  QGridLayout* view_layout = new QGridLayout(view);
  m_surface = new Q3DSurface;
  QWidget* container = QWidget::createWindowContainer(m_surface);
  m_surface->setFlags( m_surface->flags() | Qt::WindowDoesNotAcceptFocus);
  container->setMinimumSize(QSize(200,50));
  view_layout->addWidget(container, 1, 1);

  m_xScrollBar = new QScrollBar(Qt::Vertical);
  m_xScrollBar->setPageStep(100);
  m_xScrollBar->setSingleStep(1);
  m_xScrollBar->installEventFilter(this);
  view_layout->addWidget(m_xScrollBar, 1, 2);
  connect(m_xScrollBar, SIGNAL(valueChanged(int)), SLOT(updateXAxis()) );

  m_yScrollBar = new QScrollBar(Qt::Horizontal);
  m_yScrollBar->setPageStep(100);
  m_yScrollBar->setSingleStep(1);
  m_yScrollBar->installEventFilter(this);
  view_layout->addWidget(m_yScrollBar, 2, 1);
  connect(m_yScrollBar, SIGNAL(valueChanged(int)), SLOT(updateYAxis()) );
  m_info = new QLabel;
  m_info->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
  view_layout->addWidget(m_info, 3, 1);

  setWidget(view);

  m_surface->axisX()->setLabelAutoRotation(40);
  m_surface->axisZ()->setLabelAutoRotation(40);
  m_surface->axisY()->setLabelAutoRotation(90);

  m_seriesData = NULL;

  m_seriesData = new QSurface3DSeries;
  m_seriesData->setDrawMode(QSurface3DSeries::DrawSurface);
  //m_seriesData->setFlatShadingEnabled(false);

  m_surface->addSeries(m_seriesData);
  m_surface->activeTheme()->setLabelBackgroundEnabled(false);
  m_surface->activeTheme()->setLabelBorderEnabled(false);
  m_surface->activeTheme()->setBackgroundEnabled(false);
  m_surface->activeTheme()->setLightStrength(3.0);
  m_surface->activeTheme()->setAmbientLightStrength(0.6);
  //m_surface->activeTheme()->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
  //m_surface->setShadowQuality(QAbstract3DGraph::ShadowQualitySoftMedium);
  //m_surface->setShadowQuality(QAbstract3DGraph::ShadowQualitySoftHigh);
  m_surface->setShadowQuality(QAbstract3DGraph::ShadowQualityNone);

  connect(m_seriesData, &QSurface3DSeries::selectedPointChanged, this,
                        &Oca3DPlotDock::handlePositionChange);
  setContextMenuPolicy( Qt::CustomContextMenu );
  connect( this, &Oca3DPlotDock::customContextMenuRequested, this, &Oca3DPlotDock::openContextMenu);
}

// -----------------------------------------------------------------------------

Oca3DPlotDock::~Oca3DPlotDock()
{
  fprintf(stderr, "Oca3DPlotDock::~Oca3DPlotDock\n");
}

// -----------------------------------------------------------------------------

void Oca3DPlotDock::closeEvent( QCloseEvent* event )
{
  m_plot->close();
}

// -----------------------------------------------------------------------------

void Oca3DPlotDock::onUpdateRequired( uint flags )
{
  //fprintf(stderr, "data changed 0x%x\n", flags);

  const uint data_flags = Oca3DPlot::e_FlagDataChanged
                        | Oca3DPlot::e_FlagTextureChanged
                        | Oca3DPlot::e_FlagXDataChanged
                        | Oca3DPlot::e_FlagYDataChanged
                        | Oca3DPlot::e_FlagZScaleChanged;

  const uint xview_flags = Oca3DPlot::e_FlagXViewChanged | data_flags;
  const uint yview_flags = Oca3DPlot::e_FlagYViewChanged | data_flags;
  const uint zscale_flags = Oca3DPlot::e_FlagZScaleChanged;
  const uint pos_flags = Oca3DPlot::e_FlagSelectedPosChanged;
  const uint ratio_flags = Oca3DPlot::e_FlagAspectRatioChanged;
  const uint name_flags = Oca3DPlot::e_FlagNameChanged;

  if (data_flags & flags) {
    updateData();
  }

  if (xview_flags & flags) {
    float L = m_plot->getXStep();
    m_xScrollBar->blockSignals(true);
    float d = qRound( m_plot->getXScale()/L );
    d = qMax(2.0f, d);
    m_xScrollBar->setPageStep(d);
    d *= L/2;

    float v0 = qRound( (m_plot->getXPos() - m_plot->getXOrigin())/L );
    v0 = qBound(m_xScrollBar->minimum(), (int)v0, m_xScrollBar->maximum());
    m_xScrollBar->setValue(v0);
    v0 = v0*L + m_plot->getXOrigin();

    m_surface->axisX()->setRange(v0-d,v0+d);
    m_xScrollBar->blockSignals(false);
  }

  if (yview_flags & flags) {
    float L = m_plot->getYStep();
    m_yScrollBar->blockSignals(true);
    float d = qRound( m_plot->getYScale()/L );
    d = qMax(2.0f, d);
    m_yScrollBar->setPageStep(d);
    d *= L/2;

    float v0 = qRound( (m_plot->getYPos() - m_plot->getYOrigin())/L );
    v0 = qBound(m_yScrollBar->minimum(), (int)v0, m_yScrollBar->maximum());
    m_yScrollBar->setValue(v0);
    v0 = v0*L + m_plot->getYOrigin();
    m_surface->axisZ()->setRange(v0-d,v0+d);
    m_yScrollBar->blockSignals(false);
  }

  if (zscale_flags & flags) {
    float z_min = m_plot->getZMin();
    float z_scale = m_plot->getZScale();
    m_surface->axisY()->setRange(z_min,z_min+z_scale);
  }

  if (ratio_flags & flags) {
    m_surface->setAspectRatio(m_plot->getAspectRatio());
    m_surface->setHorizontalAspectRatio(m_plot->getHorizontalRatio());
  }

  if (pos_flags & flags) {
    m_info->setText( QString("%1 %2")
        .arg(m_plot->getXSelected(),6,'f',2)
        .arg(m_plot->getYSelected(),6,'f',3) );
  }

  if( name_flags & flags ) {
    setWindowTitle( m_plot->getDisplayText() );
  }
}

// ----------------------------------------------------------------------------

void Oca3DPlotDock::updateData()
{
  OcaLock lock( m_plot );
  double* pd = m_plot->getData();
  QColor* p = m_plot->getTexture();
  if (NULL == pd) {
    // TODO
    return;
  }

  double v = 0;
  double v_min = 10000;
  double v_max = -10000;
  int nx = m_plot->getXLen();
  int ny = m_plot->getYLen();

  m_xScrollBar->blockSignals(true);
  m_yScrollBar->blockSignals(true);
  m_xScrollBar->setRange(0,nx);
  m_yScrollBar->setRange(0,ny);
  m_xScrollBar->blockSignals(false);
  m_yScrollBar->blockSignals(false);

  QImage img(nx, ny, QImage::Format_ARGB32_Premultiplied);
  QSurfaceDataArray *data = new QSurfaceDataArray;
  img.fill(Qt::white);
  float z_min = m_plot->getZMin();
  float z_scale = m_plot->getZScale();
  float z_colorK = 80.0/z_scale;

  float x_step = m_plot->getXStep();
  float x_origin = m_plot->getXOrigin();
  float y_step = m_plot->getYStep();
  float y_origin = m_plot->getYOrigin();

  for (int jf=0; jf<ny; ++jf) {
    QSurfaceDataRow *dataRow = new QSurfaceDataRow;
    for (int jt=0; jt<nx; ++jt) {
      v = *pd++;
      v_min = std::min(v_min, v);
      v_max = std::max(v_max, v);
      int jj = ny-jf-1;
      if (z_min > v) {
        v = z_min;
      }
      else {
        int vc = (v - z_min)*z_colorK;
        img.setPixel(jt, jj, QColor(qBound(0,50+4*vc,255),
                                    qBound(0,210-vc,255), qBound(0,220-2*vc,255)).rgb() );
      }
      if (NULL != p) {
        if (0 != p->alpha()) {
          img.setPixel(jt, jj, p->rgb() );
        }
        ++p;
      }
      *dataRow << QVector3D(x_origin + jt*x_step, (float)v, y_origin + jf*y_step);
    }
    *data << dataRow;
  }

  // TODO
  img = img.scaled(QSize(nx*2, ny*2), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  img = img.copy(1,1,2*nx-2, 2*ny-2);
  m_seriesData->setTexture( img.scaled(QSize(nx-1, ny-1), Qt::IgnoreAspectRatio, Qt::SmoothTransformation) );
  m_seriesData->dataProxy()->resetArray(data);
}

// ----------------------------------------------------------------------------

void Oca3DPlotDock::handlePositionChange(const QPoint &position)
{
  if (QPoint(-1,-1) != position) {
    m_plot->setXSelected( position.y()*m_plot->getXStep() + m_plot->getXOrigin() );
    m_plot->setYSelected( position.x()*m_plot->getYStep() + m_plot->getYOrigin() );
  }
  else {
    m_plot->setXSelected( NAN );
    m_plot->setYSelected( NAN );
  }
  //fprintf(stderr, "selected point: %f %f\n", position.y()*0.01, position.x()*0.005 );
}

// ----------------------------------------------------------------------------

void Oca3DPlotDock::updateXAxis()
{
  m_plot->setXPos( m_xScrollBar->value()*m_plot->getXStep() + m_plot->getXOrigin() );
}

// ----------------------------------------------------------------------------

void Oca3DPlotDock::updateYAxis()
{
  m_plot->setYPos( m_yScrollBar->value()*m_plot->getYStep() + m_plot->getYOrigin() );
}

// ----------------------------------------------------------------------------

bool Oca3DPlotDock::eventFilter( QObject* obj, QEvent* ev )
{
  if (QEvent::Wheel == ev->type()) {
    QWheelEvent* e = (QWheelEvent*)ev;
    if (obj == m_xScrollBar && Qt::ControlModifier == e->modifiers()) {
      double k = std::pow(2, -e->pixelDelta().y()*0.001);
      m_plot->setXScale( m_plot->getXScale() * k );
      return true;
    }
    if (obj == m_yScrollBar && Qt::ControlModifier == e->modifiers()) {
      double k = std::pow(2, -e->pixelDelta().y()*0.001);
      m_plot->setYScale( m_plot->getYScale() * k );
      return true;
    }
  }
  return false;
}

// -----------------------------------------------------------------------------

void Oca3DPlotDock::openContextMenu( const QPoint& pos )
{
  QMenu* menu = new QMenu( this );
  menu->addAction( "Delete", m_plot, SLOT(close())  );

  menu->addAction( "Properties...", this, SLOT(openProperties()) );
  menu->exec( mapToGlobal(pos) );
}

// -----------------------------------------------------------------------------

void Oca3DPlotDock::openProperties()
{
  OcaDialogProperties3DPlot* dlg = new OcaDialogProperties3DPlot( m_plot );
  dlg->show();
}

// -----------------------------------------------------------------------------

