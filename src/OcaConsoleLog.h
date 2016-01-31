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

#ifndef OcaConsoleLog_h
#define OcaConsoleLog_h

#include <QPlainTextEdit>

class OcaConsoleLog : public QPlainTextEdit
{
  Q_OBJECT ;

  public:
    OcaConsoleLog();
    ~OcaConsoleLog();

  public slots:
    void appendCommand( const QString& command );
    void appendOutput( const QString& text, int error );

  protected:
    void setColors( QRgb background, QRgb foreground );
};

#endif // OcaConsoleLog_h

