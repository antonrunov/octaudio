/*
   Copyright 2013-2019 Anton Runov

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

#include "OcaDialogAbout.h"

#include "octaudio_configinfo.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// ------------------------------------------------------------------------------------

OcaDialogAbout::OcaDialogAbout()
{
  setAttribute( Qt::WA_DeleteOnClose );
  setModal( true );
  QString s = QString( "<h3 align='center'> %1 </h3>" ).arg( OCA_VERSION_STRING );

  s.append( "<p align='center'>Audio Editor with GNU Octave Console" );
  s.append( "<p align='center'>Copyright &copy; 2013-2019 Anton Runov" );

  s.append( "<pre><table width='100%'>" );
  s.append( QString( "<tr><td align='right'>Config:<td><td>%1</td></tr>").arg( OCA_CONFIG_BUILDTYPE ) );

  int extra_lines = 0;
  if( NULL != OCA_BUILD_TIMESTAMP ) {
    s.append( QString( "<tr><td align='right'>Build Date:<td><td>%1</td></tr>" ).arg( OCA_BUILD_TIMESTAMP ) );
    extra_lines++;
  }
  if( NULL != Oca_BUILD_REVISION ) {
    s.append( QString( "<tr><td align='right'>Revision:<td><td>%1</td></tr>" )
      .arg( QString(OCA_BUILD_REVISION).left( 7 ) ) );
    extra_lines++;
  }
  s.append( "</center>" );

  QTextEdit* text = new QTextEdit( s );
  text->setReadOnly( true );

  QVBoxLayout* layout = new QVBoxLayout( this );
  layout->setContentsMargins( 0, 0, 0, 0 );
  layout->addWidget( text );
  QFontMetrics fm( text->font() );
  setFixedWidth( fm.averageCharWidth() * 50 );
  setFixedHeight( fm.lineSpacing() * ( 9 + extra_lines ) );
}

// ------------------------------------------------------------------------------------

OcaDialogAbout::~OcaDialogAbout()
{
  fprintf( stderr, "OcaDialogAbout::~OcaDialogAbout\n" );
}

// ------------------------------------------------------------------------------------

