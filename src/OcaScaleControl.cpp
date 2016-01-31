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

#include "OcaScaleControl.h"

#include <QtCore>
#include <QtGui>

// -----------------------------------------------------------------------------

OcaScaleControl::OcaScaleControl( QWidget* parent /*= NULL*/ )
: QLineEdit( parent ),
  m_value( 0.0 ),
  m_step( 1.0 ),
  m_fastStep( 4.0 ),
  m_fineStep( 0.25 )
{
  setFrame( false );
  setFocusPolicy( Qt::ClickFocus );
  setAlignment( Qt::AlignRight );
  setAttribute( Qt::WA_MacShowFocusRect, false );
  connect( this, SIGNAL(textChanged(const QString &)),
                 SLOT(onTextChanged(const QString&)) );
  resize( sizeHint() );
  updateText();
}

// -----------------------------------------------------------------------------

OcaScaleControl::~OcaScaleControl()
{
}

// -----------------------------------------------------------------------------

void OcaScaleControl::setTransparency( double alpha_bg, double alpha_fg /*= 1.0*/ )
{
  QPalette p = palette();
 
  QColor c = p.color( backgroundRole() );
  c.setAlphaF( alpha_bg );
  p.setColor( backgroundRole(), c );
  
  c = p.color( foregroundRole() );
  c.setAlphaF( alpha_fg );
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

QSize OcaScaleControl::sizeHint() const
{
  QSize hint = QLineEdit::sizeHint();
  hint.setWidth( fontMetrics().width( "00000000000" ) );
  return hint;
}

// -----------------------------------------------------------------------------

void OcaScaleControl::focusOutEvent( QFocusEvent* event )
{
  updateText();
  QLineEdit::focusOutEvent( event );
}

// -----------------------------------------------------------------------------

void OcaScaleControl::updateText()
{
  blockSignals( true );
  setText( QString::number( m_value, 'f', 4 ) );
  blockSignals( false );
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
  double d = ( 0 < event->delta() ? -0.5 : 0.5 );
  setCursor( Qt::ArrowCursor );
  switch( event->orientation() + event->modifiers() ) {
    case Qt::Vertical:
      emit moved( d * m_step );
      break;

    case Qt::Vertical + Qt::ControlModifier:
      emit moved( d * m_fineStep );
      break;

    case Qt::Vertical + Qt::ShiftModifier:
      emit moved( d * m_fastStep );
      break;
  }
}

// -----------------------------------------------------------------------------

void OcaScaleControl::onTextChanged( const QString& text )
{
  double value = text.toDouble();
  if( value != m_value ) {
    m_value = value;
    emit changed( m_value );
  }
}

// -----------------------------------------------------------------------------

