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

#ifndef OcaConsole_h
#define OcaConsole_h

#include <QSplitter>

class OcaConsoleLog;
class OcaCommandEditor;

class OcaConsole : public QSplitter
{
  Q_OBJECT ;

  public:
    OcaConsole();
    ~OcaConsole();

  signals:
    void commandEntered( const QString& command );

  public slots:
    void setReadyState( bool state );
    void focusEditor();
    void scrollToBottom();

  protected:
    OcaConsoleLog*       m_log;
    OcaCommandEditor*    m_commandEditor;
};

# endif // OcaConsole_h

