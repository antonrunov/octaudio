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

#ifndef OcaScaleControl_h
#define OcaScaleControl_h

#include <QLineEdit>

class OcaScaleControl : public QLineEdit
{
  Q_OBJECT ;
  public:
    OcaScaleControl( QWidget* parent = NULL );
    ~OcaScaleControl();

  public:
    void    setStep( double step );
    void    setFastStep( double step );
    void    setFineStep( double step );
    void    setTransparency( double alpha_bg, double alpha_fg = 1.0 );

  public slots:
    void setValue( double value );

  signals:
    void changed( double value );
    void moved( double step );

  protected:
    virtual void enterEvent( QEvent* event );
    virtual void leaveEvent( QEvent* event );
    virtual void focusOutEvent( QFocusEvent* event );
    virtual void keyPressEvent( QKeyEvent* key_event );
    virtual void wheelEvent( QWheelEvent* event );

  protected slots:
    void onTextChanged( const QString& text );

  protected:
    void  updateText();
    void  updateTransparency();
    void  updateWidth();

  protected:
    double  m_value;
    double  m_step;
    double  m_fastStep;
    double  m_fineStep;
    double  m_transparencyBg;
    double  m_transparencyFg;
};

#endif // ScaleControl_h
