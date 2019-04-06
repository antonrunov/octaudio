/*
   Copyright 2013-2018 Anton Runov

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

#include "OcaScaleControl.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaScaleControl::OcaScaleControl( QWidget* parent /*= NULL*/ )
: QLineEdit( parent ),
  m_value( 0.0 ),
  m_step( 1.0 ),
  m_fastStep( 4.0 ),
  m_fineStep( 0.25 ),
  m_transparencyBg( 1.0 ),
  m_transparencyFg( 1.0 )
{
  setFrame( false );
  setFocusPolicy( Qt::ClickFocus );
  setAlignment( Qt::AlignRight );
  setAttribute( Qt::WA_MacShowFocusRect, false );
  connect( this, SIGNAL(textChanged(const QString &)),
                 SLOT(onTextChanged(const QString&)) );
  updateText();
}

// -----------------------------------------------------------------------------

OcaScaleControl::~OcaScaleControl()
{
}

// -----------------------------------------------------------------------------

void OcaScaleControl::setTransparency( double alpha_bg, double alpha_fg /*= 1.0*/ )
{
  m_transparencyBg = alpha_bg;
  m_transparencyFg = alpha_fg;
  updateTransparency();
}

// -----------------------------------------------------------------------------

void OcaScaleControl::updateTransparency()
{
  QPalette p = palette();

  bool active = underMouse() || hasFocus();
  QColor c = p.color( backgroundRole() );
  c.setAlphaF( active ? 1.0 : m_transparencyBg );
  p.setColor( backgroundRole(), c );
  
  c = p.color( foregroundRole() );
  c.setAlphaF( active ? 1.0 : m_transparencyFg );
  p.setColor( foregroundRole(), c );
  
  setPalette( p );
}

// -----------------------------------------------------------------------------

void OcaScaleControl::setValue( double value )
{
  if( value != m_value ) {
    m_value = value;
    updateText();
  }
}

// -----------------------------------------------------------------------------

void OcaScaleControl::enterEvent( QEvent* event )
{
  updateTransparency();
}

// -----------------------------------------------------------------------------

void OcaScaleControl::leaveEvent( QEvent* event )
{
  updateTransparency();
}

// -----------------------------------------------------------------------------

void OcaScaleControl::focusOutEvent( QFocusEvent* event )
{
  updateText();
  QLineEdit::focusOutEvent( event );
  updateTransparency();
}

// -----------------------------------------------------------------------------

void OcaScaleControl::updateText()
{
  blockSignals( true );
  setText( QString::number( m_value, 'f', 4 ) );
  updateWidth();
  blockSignals( false );
}

// -----------------------------------------------------------------------------

void OcaScaleControl::updateWidth()
{
  QPoint p = pos();
  int w_cur = width();
  int extra_width = minimumSizeHint().width() - fontMetrics().maxWidth() + 2;
  resize( fontMetrics().width( text() ) + extra_width, height() );
  p.setX( p.x() + w_cur - width() );
  move( p );
}

// -----------------------------------------------------------------------------

void OcaScaleControl::keyPressEvent( QKeyEvent* key_event )
{
  bool  processed = true;
  int   d = 1;

  switch( ( key_event->modifiers() & (~Qt::KeypadModifier) ) + key_event->key() ) {

    case Qt::Key_Escape:
      if( NULL != parentWidget() ) {
        parentWidget()->setFocus();
      }
      break;

    // scrollUD_1
    case Qt::Key_Up:
      d = -1;
    case Qt::Key_Down:
      emit moved( d * m_step );
      break;
    
    // scrollUD_2
    case Qt::Key_Up + Qt::ShiftModifier:
      d = -1;
    case Qt::Key_Down + Qt::ShiftModifier:
      emit moved( d * m_fastStep );
      break;

    // scrollUD_2
    case Qt::Key_Up + Qt::ControlModifier:
      d = -1;
    case Qt::Key_Down + Qt::ControlModifier:
      emit moved( d * m_fineStep );
      break;

    default:
      processed = false;
  }

  if( ! processed ) {
    QLineEdit::keyPressEvent( key_event );
  }
}

// -----------------------------------------------------------------------------

void OcaScaleControl::wheelEvent( QWheelEvent* event )
{  
  static double distance = 0;
  distance += event->delta();
  const int MIN_WHEEL_STEP=40;

  int n = round(distance / MIN_WHEEL_STEP);
  distance -= n*MIN_WHEEL_STEP;
  double d = -0.5*n*MIN_WHEEL_STEP/120.0;
  switch( event->orientation() + event->modifiers() ) {
    case Qt::Vertical:
      d *= m_step;
      event->accept();
      break;

    case Qt::Vertical + Qt::ControlModifier:
      d *= m_fineStep;
      event->accept();
      break;

    case Qt::Vertical + Qt::ShiftModifier:
      d *= m_fastStep;
      event->accept();
      break;

    default:
      d = 0;
  }

  if (0 != d) {
    emit moved(d);
  }
}

// -----------------------------------------------------------------------------

void OcaScaleControl::onTextChanged( const QString& str )
{
  double value = str.toDouble();
  if( value != m_value ) {
    m_value = value;
    emit changed( m_value );
  }
  updateWidth();
}

// -----------------------------------------------------------------------------

