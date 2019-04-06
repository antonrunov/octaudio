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

#include "OcaConsoleLog.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaConsoleLog::OcaConsoleLog()
{
  setReadOnly( true );
  setMaximumBlockCount( 2000 );
  //setFocusPolicy( Qt::NoFocus );
}

// -----------------------------------------------------------------------------

OcaConsoleLog::~OcaConsoleLog()
{
}

// -----------------------------------------------------------------------------

void OcaConsoleLog::appendCommand( const QString& command )
{
  //setColors( 0xffe0e0, 0x000000 );
  setColors( 0xffffff, 0x0000ff );
  appendPlainText( QString("> %1").arg( command ) );
}

// -----------------------------------------------------------------------------

void OcaConsoleLog::appendOutput( const QString& text, int error )
{
  if( 0 != error ) {
    setColors( 0xffffff, 0xff0000 );
  }
  else {
    setColors( 0xffffff, 0x000000 );
  }
  appendPlainText( text );
}

// -----------------------------------------------------------------------------

void OcaConsoleLog::setColors( QRgb background, QRgb foreground )
{
  QTextCharFormat fmt =  currentCharFormat();
  fmt.setBackground( QColor(background) );
  fmt.setForeground( QColor(foreground) );
  setCurrentCharFormat( fmt );
}

// -----------------------------------------------------------------------------

