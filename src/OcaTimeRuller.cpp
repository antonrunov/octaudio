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

#include "OcaTimeRuller.h"

#include "OcaTrackGroup.h"
#include "OcaObjectListener.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaTimeRuller::OcaTimeRuller( OcaTrackGroup* group )
:
  m_group( group ),
  m_zeroOffset( 0 ),
  m_timeScale( 1.0 ),
  m_offset( 0 ),
  m_timeMs( 0 ),
  m_step( 100 ),
  m_stepMs( 1000 ),
  m_labelPrecision( 0 ),
  m_subTicks( 5 ),
  m_cursorPosition( 0 ),
  m_selectionLeft( NAN ),
  m_selectionRight( NAN ),
  m_basePosition( -1 ),
  m_basePositionTime( NAN )
{
  m_listener = new OcaObjectListener( m_group, OcaTrackGroup::e_FlagALL, 10, this );
  m_listener->addEvent( NULL, OcaTrackGroup::e_FlagALL );

  connect(  m_listener,
            SIGNAL(updateRequired(uint,QHash<QString,uint>&,QList<OcaObject*>&)),
            SLOT(onUpdateRequired(uint))  );

  setFixedHeight( 20 );
  setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
  setFocusPolicy( Qt::NoFocus );
}

// -----------------------------------------------------------------------------

OcaTimeRuller::~OcaTimeRuller()
{
}

// -----------------------------------------------------------------------------

void OcaTimeRuller::onUpdateRequired( uint flags )
{
  setViewport( m_group );
}

// -----------------------------------------------------------------------------

int OcaTimeRuller::mapFromView( OcaTrackGroup* group, double t ) const
{
  return std::isfinite( t ) ? ( t - group->getViewPosition() ) / m_timeScale + m_zeroOffset : -1;
}

// -----------------------------------------------------------------------------

void OcaTimeRuller::setBasePosition( double t )
{
  m_basePositionTime = t;
  m_basePosition = mapFromView( m_group, m_basePositionTime );
  update();
}

// -----------------------------------------------------------------------------

void OcaTimeRuller::setViewport( OcaTrackGroup* group )
{
  OcaLock lock( group );
  m_timeScale = group->getViewDuration() / ( width() - 2 * m_zeroOffset );
  const int scales[] = { 1, 2, 5, };
  const int scales2[] = { 5, 4, 5, };
  int order3 = (int)floor( log10( m_timeScale * 200 ) * 3 );
  int ord = floor( order3 / 3.0 );
  m_stepMs = pow( 10, ord ) * scales[ order3 - 3 * ord ] * 1000;
  m_step = m_stepMs / m_timeScale / 1000;
  m_subTicks = scales2[ order3 - 3 * ord ];

  m_labelPrecision = 0;
  if( 0 > ord ) {
    m_labelPrecision = -ord;
  }

  double n = floor( group->getViewPosition() * 1000 / m_stepMs );
  m_timeMs = n * m_stepMs;
  m_offset = mapFromView( group, m_timeMs / 1000 );

  m_cursorPosition = mapFromView( group, group->getCursorPosition() );
  m_selectionLeft = mapFromView( group, group->getRegionStart() );
  m_selectionRight = mapFromView( group, group->getRegionEnd() );
  m_basePosition = mapFromView( m_group, m_basePositionTime );

  update();
}

// -----------------------------------------------------------------------------

void OcaTimeRuller::paintEvent ( QPaintEvent* )
{
  QPainter painter(this);
  painter.eraseRect( rect() );
  if( ( 0 < m_selectionRight ) && ( m_selectionLeft < width() ) ) {
    painter.fillRect( m_selectionLeft, 0, m_selectionRight - m_selectionLeft,
                                              height(), QColor(QRgb(0xcccccc)) );
  }
  painter.setBrush( QBrush( QRgb( 0xa0a0a0 ) ) );
  if( ( 0 < m_selectionLeft ) && ( m_selectionLeft < width() ) ) {
    QPointF points[3] = {
      QPointF(m_selectionLeft, 0),
      QPointF(m_selectionLeft, height() - 1 ),
      QPointF(m_selectionLeft - 5, ( height() - 1 ) / 2 ),
    };
    painter.drawPolygon(points, 3);
  }
  if( ( 0 < m_selectionRight ) && ( m_selectionRight < width() ) ) {
    QPointF points[3] = {
      QPointF(m_selectionRight, 0),
      QPointF(m_selectionRight, height() - 1 ),
      QPointF(m_selectionRight + 5, ( height() - 1 ) / 2 ),
    };
    painter.drawPolygon(points, 3);
  }

  painter.setPen(Qt::black);
  painter.drawLine( 0, height() - 2, width(), height() - 2 );

  double x = m_offset;
  double time_ms = m_timeMs;

  while( x < width() ) {
    painter.drawText( QPointF( x, height() / 2 ),
                      QString::number( time_ms / 1000, 'f', m_labelPrecision ) );
    painter.drawLine( x, height()/2, x, height() - 1 );
    for( int i = 1; i < m_subTicks; i++ ) {
      double x1 = x + i * m_step / m_subTicks;
      painter.drawLine( x1, height() - 5, x1, height() - 1 );
    }
    x += m_step;
    time_ms += m_stepMs;
  }

  painter.setPen(Qt::black);
  if( ( 0 < m_cursorPosition ) && ( m_cursorPosition < width() ) ) {
    painter.drawLine( m_cursorPosition, 0, m_cursorPosition, height() - 1 );
  }

  if( ( 0 < m_basePosition ) && ( m_basePosition < width() ) ) {
    painter.setPen( QRgb(0xf06000) );
    painter.drawLine( m_basePosition, height()/2, m_basePosition, height() - 1 );
  }
}

// -----------------------------------------------------------------------------

