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


#include "Oca3DPlot.h"

#include <QtCore>
#include <stdint.h>

// ------------------------------------------------------------------------------------

Oca3DPlot::Oca3DPlot(const QString& name)
:
  m_xLen(0),
  m_yLen(0),
  m_data(NULL),
  m_texture(NULL),
  m_xSelected(NAN),
  m_ySelected(NAN),
  m_xPos(0.0),
  m_xScale(10.0),
  m_yPos(0.0),
  m_yScale(10.0),
  m_zLimit(0.0),
  m_zScale(10.0),
  m_aspectRatio(1.6),
  m_horizontalRatio(0.7),
  m_xOrigin(0.0),
  m_xStep(1.0),
  m_yOrigin(0.0),
  m_yStep(1.0)
{
  setName(name);
  m_nameFlag = e_FlagNameChanged;
  m_displayNameFlag = e_FlagNameChanged;
}

// ------------------------------------------------------------------------------------

Oca3DPlot::~Oca3DPlot()
{
  clearData();
}

// ------------------------------------------------------------------------------------

void Oca3DPlot::onClose()
{
  clearData();
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::clearData()
{
  uint flags = 0;
  if (0 != m_xLen || 0 != m_yLen) {
    WLock lock( this );
    if (NULL != m_data) {
      delete [] m_data;
      m_data = NULL;
      flags |= e_FlagDataChanged;
    }
    if (NULL != m_texture) {
      delete [] m_texture;
      m_texture = NULL;
      flags |= e_FlagTextureChanged;
    }
    m_xLen = 0;
    m_yLen = 0;
  }

  emitChanged(flags);
  return 0 != flags;
}

// ------------------------------------------------------------------------------------

static void double_to_texture(QColor* dst, double* src, size_t len) {
  QColor* dst_e = dst + len;
  for(;dst<dst_e;++dst,++src) {
    if (!std::isnan(*src)) {
      uint32_t tmp = *src;
      int r = tmp >> 16;
      int g = (tmp >> 8) & 0xff;
      int b = tmp & 0xff;
      dst->setRgb(r,g,b);
    }
    else {
      dst->setRgb(0,0,0,0);
    }
  }
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setData(int nx, int ny, double* data, double* texture)
{
  uint flags = 0;
  {
    WLock lock( this );
    if (NULL != m_data) {
      delete [] m_data;
    }
    if (NULL != m_texture) {
      delete [] m_texture;
    }
    m_data = new double[nx * ny];
    if (NULL != m_data) {
      m_xLen = nx;
      m_yLen = ny;
      memcpy(m_data, data, sizeof(double)*nx*ny);
      if (NULL == texture) {
        m_texture = NULL;
      }
      else {
        m_texture = new QColor[nx * ny];
        double_to_texture(m_texture, texture, nx*ny);
      }
    }
    else {
      m_xLen = 0;
      m_yLen = 0;
      m_texture = NULL;
    }
    flags = e_FlagDataChanged | e_FlagTextureChanged;
  }

  emitChanged(flags);
  return 0 != flags;
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setTexture(int nx, int ny, double* texture)
{
  uint flags = 0;
  {
    WLock lock( this );
    if (NULL == texture && NULL != m_texture) {
      delete [] m_texture;
      m_texture = NULL;
      flags |= e_FlagTextureChanged;
    }
    if (nx == m_xLen && ny == m_yLen && NULL != texture) {
      if (NULL == m_texture) {
        m_texture = new QColor[nx * ny];
      }
      double_to_texture(m_texture, texture, nx*ny);
      flags |= e_FlagTextureChanged;
    }
  }

  emitChanged(flags);
  return 0 != flags;
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setXPos(double x_pos)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_xPos != x_pos) {
      m_xPos = x_pos;
      flags = e_FlagXViewChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setXScale(double x_scale)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( (m_xScale != x_scale) && (0 < x_scale) ) {
      m_xScale = x_scale;
      flags = e_FlagXViewChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setYPos(double y_pos)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_yPos != y_pos ) {
      m_yPos = y_pos;
      flags = e_FlagYViewChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setYScale(double y_scale)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( (m_yScale != y_scale) && (0 < y_scale) ) {
      m_yScale = y_scale;
      flags = e_FlagYViewChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setZMin(double z_min)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_zLimit != z_min ) {
      m_zLimit = z_min;
      flags = e_FlagZScaleChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setXSelected(double x)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_xSelected != x ) {
      m_xSelected = x;
      flags = e_FlagSelectedPosChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setYSelected(double y)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_ySelected != y ) {
      m_ySelected = y;
      flags = e_FlagSelectedPosChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setZScale(double scale)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_zScale != scale ) {
      m_zScale = scale;
      flags = e_FlagZScaleChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setAspectRatio(double ratio)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_aspectRatio != ratio ) {
      m_aspectRatio = ratio;
      flags = e_FlagAspectRatioChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setHorizontalRatio(double ratio)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_horizontalRatio != ratio ) {
      m_horizontalRatio = ratio;
      flags = e_FlagAspectRatioChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setXOrigin(double x)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_xOrigin != x ) {
      m_xOrigin = x;
      flags = e_FlagXDataChanged;
      if( !std::isnan(m_xSelected) ) {
        m_xSelected = NAN;
        flags |= e_FlagSelectedPosChanged;
      }
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setXStep(double step)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( (m_xStep != step) && (0.0 < step) ) {
      m_xStep = step;
      flags = e_FlagXDataChanged;
      if( !std::isnan(m_xSelected) ) {
        m_xSelected = NAN;
        flags |= e_FlagSelectedPosChanged;
      }
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setYOrigin(double y)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( m_yOrigin != y ) {
      m_yOrigin = y;
      flags = e_FlagYDataChanged;
      if( !std::isnan(m_ySelected) ) {
        m_ySelected = NAN;
        flags |= e_FlagSelectedPosChanged;
      }
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

bool Oca3DPlot::setYStep(double step)
{
  uint flags = 0;
  {
    WLock lock( this );
    if( (m_yStep != step) && (0.0 < step) ) {
      m_yStep = step;
      flags = e_FlagYDataChanged;
      if( !std::isnan(m_ySelected) ) {
        m_ySelected = NAN;
        flags |= e_FlagSelectedPosChanged;
      }
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

