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

#include "OcaObject.h"
#include <QColor>

class Oca3DPlot : public OcaObject
{
  Q_OBJECT ;
  Q_PROPERTY( int x_len READ getXLen );
  Q_PROPERTY( int y_len READ getYLen );
  Q_PROPERTY( double x_pos READ getXPos WRITE setXPos );
  Q_PROPERTY( double x_scale READ getXScale WRITE setXScale );
  Q_PROPERTY( double y_pos READ getYPos WRITE setYPos );
  Q_PROPERTY( double y_scale READ getYScale WRITE setYScale );
  Q_PROPERTY( double z_min READ getZMin WRITE setZMin );
  Q_PROPERTY( double z_scale READ getZScale WRITE setZScale );
  Q_PROPERTY( double x_sel_pos READ getXSelected );
  Q_PROPERTY( double y_sel_pos READ getYSelected );
  Q_PROPERTY( double aspect_ratio READ getAspectRatio WRITE setAspectRatio );
  Q_PROPERTY( double horiz_ratio READ getHorizontalRatio WRITE setHorizontalRatio );
  Q_PROPERTY( double x_origin READ getXOrigin WRITE setXOrigin );
  Q_PROPERTY( double x_step READ getXStep WRITE setXStep );
  Q_PROPERTY( double y_origin READ getYOrigin WRITE setYOrigin );
  Q_PROPERTY( double y_step READ getYStep WRITE setYStep );

  public:
    Oca3DPlot(const QString& name);
    ~Oca3DPlot();

  public:
    enum EFlags {
      e_FlagNameChanged         = 0x0001,
      e_FlagDataChanged         = 0x0002,
      e_FlagTextureChanged      = 0x0004,
      e_FlagXViewChanged        = 0x0008,
      e_FlagYViewChanged        = 0x0010,
      e_FlagZScaleChanged       = 0x0020,
      e_FlagSelectedPosChanged  = 0x0040,
      e_FlagAspectRatioChanged  = 0x0080,
      e_FlagXDataChanged        = 0x0100,
      e_FlagYDataChanged        = 0x0200,

      e_FlagALL                 = 0xffff,
    };

  public:
    bool setData(int nx, int ny, double* data, double* texture=NULL);
    bool setTexture(int nx, int ny, double* texture);
    bool clearData();

    double* getData() const {return m_data;}
    QColor* getTexture() const {return m_texture;}


  public:
    int getXLen() const {return m_xLen;}
    int getYLen() const {return m_yLen;}
    
    double getXPos() const {return m_xPos;}
    bool setXPos(double x_pos);

    double getXScale() const {return m_xScale;}
    bool setXScale(double x_scale);

    double getYPos() const {return m_yPos;}
    bool setYPos(double y_pos);

    double getYScale() const {return m_yScale;}
    bool setYScale(double y_scale);

    double getZMin() const {return m_zLimit;}
    bool setZMin(double z_min);

    double getZScale() const {return m_zScale;}
    bool setZScale(double scale);

    double getXSelected() const {return m_xSelected;}
    bool setXSelected(double x);

    double getYSelected() const {return m_ySelected;}
    bool setYSelected(double y);

    double getAspectRatio() const {return m_aspectRatio;}
    bool setAspectRatio(double ratio);

    double getHorizontalRatio() const {return m_horizontalRatio;}
    bool setHorizontalRatio(double ratio);

    double getXOrigin() const {return m_xOrigin;}
    bool setXOrigin(double x);

    double getXStep() const {return m_xStep;}
    bool setXStep(double step);

    double getYOrigin() const {return m_yOrigin;}
    bool setYOrigin(double y);

    double getYStep() const {return m_yStep;}
    bool setYStep(double step);

  protected:
    virtual void onClose();

  protected:
    int               m_xLen;
    int               m_yLen;
    double*           m_data;
    QColor*           m_texture;
    double            m_xSelected;
    double            m_ySelected;

    double            m_xPos;
    double            m_xScale;
    double            m_yPos;
    double            m_yScale;
    double            m_zLimit;
    double            m_zScale;
    double            m_aspectRatio;
    double            m_horizontalRatio;
    double            m_xOrigin;
    double            m_xStep;
    double            m_yOrigin;
    double            m_yStep;
};

