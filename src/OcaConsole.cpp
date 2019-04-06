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

#include "OcaConsole.h"

#include "OcaConsoleLog.h"
#include "OcaCommandEditor.h"
#include "OcaOctaveController.h"
#include "OcaApp.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaConsole::OcaConsole()
: QSplitter( Qt::Vertical )
{
  {
    /*
    QFontDatabase database;
    foreach (QString family, database.families()) {
      fprintf( stderr, "%s\n", family.toLocal8Bit().data() );
    }
    */
  }
  QFont fnt( "monospace, monaco", 11 );
  fnt.setStyleHint( QFont::TypeWriter, QFont::PreferDefault );
  fnt.setFixedPitch( true );
  fprintf( stderr, "using console font: %s\n",
                                  QFontInfo(fnt).family().toLocal8Bit().data() );
  m_log = new OcaConsoleLog();
  m_log->setFont( fnt );
  addWidget( m_log );
  m_commandEditor = new OcaCommandEditor();
  m_commandEditor->setFont( fnt );
  addWidget( m_commandEditor );
  setChildrenCollapsible ( false );
  setStretchFactor( 0, 100 );
  setStretchFactor( 1, 0 );
  connect( m_commandEditor, SIGNAL(commandEntered(const QString&)),
                            SIGNAL(commandEntered(const QString&)) );
  m_log->connect( m_commandEditor, SIGNAL(commandEntered(const QString&)),
                                      SLOT(appendCommand(const QString&))  );
  m_log->connect(
      OcaApp::getOctaveController(),
      SIGNAL(outputReceived(const QString&,int)),
      SLOT(appendOutput(const QString&,int))
  );
  connect(
      OcaApp::getOctaveController(),
      SIGNAL(readyStateChanged(bool,int)),
      SLOT(setReadyState(bool))
  );
  setFocusProxy( m_commandEditor );
}

// -----------------------------------------------------------------------------

OcaConsole::~OcaConsole()
{
}

// -----------------------------------------------------------------------------

void OcaConsole::setReadyState( bool state )
{
  m_commandEditor->setReadyState( state );
}

// -----------------------------------------------------------------------------

void OcaConsole::focusEditor()
{
  m_commandEditor->setFocus();
}

// -----------------------------------------------------------------------------

void OcaConsole::scrollToBottom()
{
  m_log->moveCursor( QTextCursor::End );
  m_log->ensureCursorVisible();
}

// -----------------------------------------------------------------------------

