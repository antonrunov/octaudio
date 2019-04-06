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

#include "OcaCommandEditor.h"

#include "OcaPopupList.h"
#include "OcaOctaveController.h"
#include "OcaApp.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------
// OcaSyntaxHighlighter

class OcaSyntaxHighlighter : public QSyntaxHighlighter
{
  public:
    OcaSyntaxHighlighter( QTextEdit * parent ) : QSyntaxHighlighter( parent ) {}
    bool getStringInfo( int idx, int& i1, int &i2 );
    bool getStringInfo( int idx ) { int i1; int i2; return getStringInfo(idx, i1, i2 ); }

  protected:
    virtual void highlightBlock(const QString &text);

  protected:
    struct StringInfo {
      StringInfo( int i1, int i2 ) : m_i1(i1), m_i2(i2) {}
      int m_i1;
      int m_i2;
    };
    struct Data : public QTextBlockUserData {
        QList<StringInfo> m_strings;
    };
};

// -----------------------------------------------------------------------------

bool OcaSyntaxHighlighter::getStringInfo( int idx, int& i1, int &i2 )
{
  QTextBlock tb = document()->findBlock( idx );
  Data* data = static_cast<Data*>( tb.userData() );
  if( NULL != data ) {
    int i0 = tb.position();
    for( int i = 0; i < data->m_strings.size(); i++ ) {
      StringInfo info = data->m_strings.at(i);
      if( ( i0 + info.m_i1 <= idx ) && ( i0 + info.m_i2 >= idx-1 ) ) {
        i1 = i0 + info.m_i1;
        i2 = i0 + info.m_i2;
        return true;
      }
    }
  }
  return false;
}

// -----------------------------------------------------------------------------

void OcaSyntaxHighlighter::highlightBlock( const QString &text )
{
  QTextCharFormat stringFormat;
  stringFormat.setForeground( QColor(0xaa0000) );
  int start = 0;
  int ds = 0;
  QChar ch = qMax( 0, previousBlockState() );

  Data* data = static_cast<Data*>( currentBlockUserData() );
  if( NULL == data ) {
    data = new Data;
    setCurrentBlockUserData( data );
  }
  else {
    data->m_strings.clear();
  }
  for( int i = 0; i < text.length(); i++ ) {
    if( 0 == ch.unicode() ) {
      if( '"' == text[i] ) {
        start = i;
        ch = '"';
        ds = 1;
      }
      if( '\'' == text[i] ) {
        if( 0 < i ) {
          // check for transpose operation
          QChar prev = text[i-1];
          if( prev.isLetterOrNumber() || QString(")]}_.").contains(prev) ) {
            continue;
          }
        }
        start = i;
        ch = '\'';
        ds = 1;
      }
    }
    else if( text[i] == ch ) {
      // check for ''
      if( ( '\'' == ch && text.length() > i+1 ) && ( '\'' == text[i+1] ) ) {
        i++;
      }
      else {
        setFormat( start, i-start+1, stringFormat);
        data->m_strings.append( StringInfo( start+ds, i-1 ) );
        //fprintf( stderr, "highlightBlock: str: %d - %d\n", start+1, i-1 );
        start = -1;
        ch = 0;
      }
    }
    else if( ('"' == ch) && ('\\' == text[i]) ) {
      i++;
    }
  }
  if( 0 != ch.unicode() ) {
    setFormat( start, text.length()-start, stringFormat);
    data->m_strings.append( StringInfo( start+ds, text.length()-1 ) );
    //fprintf( stderr, "highlightBlock: string: %d - %d\n", start+1, text.length()-1 );
  }
  setCurrentBlockState( ch.unicode() );
}


// -----------------------------------------------------------------------------
// OcaCommandEditor

OcaCommandEditor::OcaCommandEditor()
:
  m_completionsSelection( false ),
  m_completionsEndPos( -1 )
{
  m_edit = new QTextEdit();
  m_edit->setAcceptRichText( false );
  QHBoxLayout* layout = new QHBoxLayout();
  layout->addWidget( m_edit );
  m_edit->installEventFilter( this );
  m_highlighter = new OcaSyntaxHighlighter( m_edit );

  QVBoxLayout* layoutButtons = new QVBoxLayout();
  layoutButtons->setContentsMargins( 0, 0, 0, 0 );

  m_cancel = new QPushButton( "Cancel" );
  connect( m_cancel, SIGNAL(clicked()),SLOT(cancelCommand()) );
  layoutButtons->addWidget( m_cancel );
  m_cancel->setEnabled( false );

  connect( m_edit, SIGNAL(cursorPositionChanged()), SLOT(updateHighlight()) );

  layout->addLayout( layoutButtons );
  layout->setContentsMargins( 0, 0, 0, 0 );
  setLayout( layout );
  setMinimumHeight( m_edit->fontMetrics().lineSpacing() * 2 );
  setFocusProxy( m_edit );
}

// -----------------------------------------------------------------------------

OcaCommandEditor::~OcaCommandEditor()
{
}

// -----------------------------------------------------------------------------

QSize OcaCommandEditor::sizeHint () const
{
  return QSize( m_edit->sizeHint().width(),  m_edit->fontMetrics().lineSpacing() * 3 );
}

// -----------------------------------------------------------------------------

bool OcaCommandEditor::eventFilter( QObject* obj, QEvent* ev )
{
  static bool enter_ready = false;
  if( obj == m_edit ) {
    if( ev->type() == QEvent::KeyRelease ) {
      QKeyEvent *key_event = static_cast<QKeyEvent*>(ev);
      if( Qt::Key_Return == key_event->key() ) {
        if( enter_ready && ( 0 == key_event->modifiers() ) ) {
          sendCommand();
          return true;
        }
      }
      else if( Qt::Key_Escape + Qt::ShiftModifier == key_event->key() +  key_event->modifiers() ) {
        QTextCursor cursor = m_edit->textCursor();
        cursor.movePosition( QTextCursor::Start );
        cursor.movePosition( QTextCursor::End,  QTextCursor::KeepAnchor );
        cursor.deleteChar();
        return true;
      }
      enter_ready = false;
    }
    else if( ev->type() == QEvent::KeyPress ) {
      QKeyEvent *key_event = static_cast<QKeyEvent*>(ev);
      if( Qt::Key_Return == key_event->key() ) {
        if( 0 == key_event->modifiers() ) {
          enter_ready = true;
          return true;
        }
      }
      else if( Qt::Key_Up == key_event->key() ) {
        if( Qt::ControlModifier == ( key_event->modifiers() & (~Qt::KeypadModifier) ) ) {
          openCommandHistory();
          return true;
        }
      }
      else if( Qt::Key_Tab == key_event->key() ) {
        if( 0 == ( key_event->modifiers() & (~Qt::KeypadModifier) ) ) {
          openCompletions();
          return true;
        }
      }
    }
  }

  return QWidget::eventFilter( obj, ev );
}

// -----------------------------------------------------------------------------

void OcaCommandEditor::sendCommand()
{
  QString cmd = m_edit->toPlainText().trimmed();
  if( ! cmd.isEmpty() ) {
    emit commandEntered( cmd );
  }
  m_edit->clear();
}

// -----------------------------------------------------------------------------

void OcaCommandEditor::cancelCommand()
{
  OcaApp::getOctaveController()->abortCurrentCommand();
}

// -----------------------------------------------------------------------------

int OcaCommandEditor::findMatchedChar( int pos, QChar left_char, QChar right_char ) const
{
  QString str = m_edit->toPlainText();

  int idx = pos;
  int count = 1;
  int i1 = -1;
  int i2 = -1;
  if( m_highlighter->getStringInfo(pos,i1, i2) ) {
    return -1;
  }
  if( right_char == str[ pos ] ) {
    while( 0 <= --idx ) {
      if( m_highlighter->getStringInfo( idx, i1, i2 ) ) {
        idx = i1-1;
        continue;
      }
      if( left_char == str[ idx ] ) {
        if( 0 == --count ) {
          break;
        }
      }
      else if( right_char == str[ idx ] ) {
        ++count;
      }
    }
  }
  else if( left_char == str[ pos ] ) {
    while( str.length() > ++idx ) {
      if( m_highlighter->getStringInfo( idx, i1, i2 ) ) {
        idx = i2+1;
        continue;
      }
      if( right_char == str[ idx ] ) {
        if( 0 == --count ) {
          break;
        }
      }
      else if( left_char == str[ idx ] ) {
        ++count;
      }
    }
  }

  return ( 0 == count ) ? idx : -1;
}

// -----------------------------------------------------------------------------

int OcaCommandEditor::findPare( int pos ) const
{
  int idx = -1;
  if( 0 <= pos ) {
    const int NUM_DELIMITERS = 3;
    const QChar LEFT_DELIMITERS[ NUM_DELIMITERS ]   = { '(', '[', '{', };
    const QChar RIGHT_DELIMITERS[ NUM_DELIMITERS ]  = { ')', ']', '}', };
    for( int i = 0; i < NUM_DELIMITERS; i++ ) {
      idx = findMatchedChar( pos, LEFT_DELIMITERS[i], RIGHT_DELIMITERS[i] );
      if( -1 != idx ) {
        break;
      }
    }
  }
  return idx;
}

// -----------------------------------------------------------------------------

void OcaCommandEditor::updateHighlight()
{
  int pos =  m_edit->textCursor().position() - 1;
  int idx = findPare( pos );

  QList<QTextEdit::ExtraSelection> selections;
  if( -1 != idx ) {
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground( QColor(0x7f7f7f) );
    selection.format.setForeground( QColor(QRgb(0xffffff)) );
    selection.cursor = m_edit->textCursor();
    selection.cursor.setPosition( pos );
    selection.cursor.setPosition( pos+1, QTextCursor::KeepAnchor );
    selections.append( selection );
    selection.cursor.setPosition( idx );
    selection.cursor.setPosition( idx+1, QTextCursor::KeepAnchor );
    selections.append( selection );
  }
  m_edit->setExtraSelections( selections );
}

// -----------------------------------------------------------------------------

void OcaCommandEditor::openCommandHistory()
{
  fprintf( stderr, "openCommandHistory\n" );
  QStringList list = OcaApp::getOctaveController()->getCommandHistory();
  if( ! list.isEmpty() ) {
    OcaPopupList* menu = new OcaPopupList( m_edit, list );
    connect( menu, SIGNAL(itemSelected(const QString&)),
                   SLOT(setCommandText(const QString&))   );
    menu->popup( mapToGlobal( m_edit->pos() ) );
  }
}

// -----------------------------------------------------------------------------

void OcaCommandEditor::setCommandText( const QString& text )
{
  QTextCursor cursor = m_edit->textCursor();
  cursor.movePosition( QTextCursor::End );
  int pos = cursor.position();
  if( 0 < pos ) {
    cursor.insertText( "\n" + text );
    cursor.movePosition( QTextCursor::Start );
    cursor.setPosition( pos+1,  QTextCursor::KeepAnchor );
    cursor.deleteChar();
    cursor.movePosition( QTextCursor::End );
  }
  else {
    cursor.insertText( text );
  }
  m_edit->setTextCursor( cursor );
}

// -----------------------------------------------------------------------------

void OcaCommandEditor::openCompletions()
{
  m_completionsSelection = false;
  QTextCursor cursor = m_edit->textCursor();
  m_completionsEndPos = cursor.selectionEnd();
  if( cursor.selection().isEmpty() && 0 < cursor.position() ) {
    if( completeFilePath() ) {
      return;
    }
    QChar c = m_edit->toPlainText()[ cursor.position() - 1 ];
    if( c.isLetterOrNumber() || '_' == c || '.' == c ) {
      cursor.movePosition( QTextCursor::WordLeft, QTextCursor::KeepAnchor );
      if( '.' == c ) {
        cursor.movePosition( QTextCursor::WordLeft, QTextCursor::KeepAnchor );
      }
      m_edit->setTextCursor( cursor );
      m_completionsSelection = true;
    }
  }

  QStringList list = OcaApp::getOctaveController()->getCompletions( cursor.selectedText() );

  cursor.setPosition( cursor.selectionStart() );
  cursor.setPosition( m_completionsEndPos,  QTextCursor::KeepAnchor );
  cursor.movePosition( QTextCursor::EndOfWord, QTextCursor::KeepAnchor );

  if( ! cursor.selection().isEmpty() && list.contains( cursor.selectedText() ) ) {
    m_completionsEndPos = cursor.selectionEnd();
  }

  OcaPopupList* menu = new OcaPopupList( NULL, list );
  connect( menu, SIGNAL(itemSelected(const QString&)),
                 SLOT(applayCompletion(const QString&)) );
  connect( menu, SIGNAL(cancelled()), SLOT(cancelCompletion()) );

  menu->popup( mapToGlobal( m_edit->pos() ) + m_edit->cursorRect().topRight() );
}

// -----------------------------------------------------------------------------

void OcaCommandEditor::applayCompletion( const QString& text )
{
  QTextCursor cursor = m_edit->textCursor();
  cursor.setPosition( cursor.selectionStart() );
  cursor.setPosition( m_completionsEndPos, QTextCursor::KeepAnchor );
  cursor.insertText( text );
  m_edit->setTextCursor( cursor );
}

// -----------------------------------------------------------------------------

void OcaCommandEditor::cancelCompletion()
{
  if( m_completionsSelection ) {
    QTextCursor cursor = m_edit->textCursor();
    cursor.setPosition( cursor.anchor() );
    m_edit->setTextCursor( cursor );
  }
}

// -----------------------------------------------------------------------------

void OcaCommandEditor::setReadyState( bool state )
{
  m_cancel->setEnabled( ! state );
}

// -----------------------------------------------------------------------------

bool OcaCommandEditor::completeFilePath()
{
  int i1 = -1;
  int i2 = -1;
  bool result = false;
  QTextCursor cursor = m_edit->textCursor();
  if( m_highlighter->getStringInfo( cursor.position(), i1, i2 ) ) {
    cursor.setPosition( i1 );
    cursor.setPosition( i2+1, QTextCursor::KeepAnchor );
    QFileInfo info( cursor.selectedText() );
    if( info.exists() ) {
      m_completionsEndPos = i2+1;
      //fprintf( stderr, "completeFilePath: %s\n", cursor.selectedText().toLocal8Bit().data() );
    }
    cursor = m_edit->textCursor();
    cursor.setPosition( i1, QTextCursor::KeepAnchor );
    QDir dir0 = info.dir();
    QDir dir = QFileInfo(cursor.selectedText()).dir();
    fprintf( stderr, "  dir: %s\n", dir.path().toLocal8Bit().data() );
    result = true;
    m_completionsSelection = true;
    QFileDialog dlg( this );
    if( info.exists() && info.isFile() ) {
      while( ( info.dir() != dir ) && ( ! info.isRoot() ) ) {
        info = info.dir().path();
        //fprintf( stderr, "  : %s\n", info.filePath().toLocal8Bit().data() );
      }
      dlg.selectFile( info.filePath() );
    }
    else {
      dlg.setDirectory( dir );
    }
    if( dlg.exec() ) {
      QStringList list = dlg.selectedFiles();
      if( ! list.isEmpty() ) {
        Q_ASSERT( 1 == list.size() );
        cursor.setPosition( i1 );
        cursor.setPosition( m_completionsEndPos, QTextCursor::KeepAnchor );
        info.setFile( list.at(0) );
        if( info.dir() == dir0 ) {
          if( dir0.path() == "." ) {
            cursor.insertText( info.fileName() );
          }
          else {
            cursor.insertText( dir0.path() + "/" + info.fileName() );
          }
        }
        else {
          cursor.insertText( list.at(0) );
        }
      }
    }
  }
  return result;
}

// -----------------------------------------------------------------------------

