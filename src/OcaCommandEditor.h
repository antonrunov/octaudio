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

#ifndef OcaCommandEditor_h
#define OcaCommandEditor_h

#include <QWidget>

class QTextEdit;
class QPushButton;
class OcaSyntaxHighlighter;

class OcaCommandEditor : public QWidget
{
  Q_OBJECT ;

  public:
    OcaCommandEditor();
    ~OcaCommandEditor();

  public:
    virtual QSize sizeHint () const;
    virtual bool eventFilter( QObject* obj, QEvent* ev );

  public:
    void setReadyState( bool state );

  protected slots:
    void sendCommand();
    void cancelCommand();
    void updateHighlight();
    void openCommandHistory();
    void setCommandText( const QString& text );
    void openCompletions();
    void applayCompletion( const QString& text );
    void cancelCompletion();

  signals:
    void commandEntered( const QString& command );

  protected:
    bool completeFilePath();
    int findMatchedChar( int pos, QChar left_char, QChar right_char ) const;
    int findPare( int pos ) const;

  protected:
    QTextEdit*            m_edit;
    QPushButton*          m_cancel;
    OcaSyntaxHighlighter* m_highlighter;
    bool                  m_completionsSelection;
    int                   m_completionsEndPos;
};

#endif // OcaCommandEditor_h


