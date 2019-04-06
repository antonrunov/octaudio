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

#include "OcaPopupList.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaPopupList::ItemDelegate::ItemDelegate( const OcaPopupList* list )
:
  m_list( list )
{
}

// -----------------------------------------------------------------------------

OcaPopupList::ItemDelegate::~ItemDelegate()
{
}

// -----------------------------------------------------------------------------

void OcaPopupList::ItemDelegate::paint( QPainter*                     painter,
                                        const QStyleOptionViewItem&   option,
                                        const QModelIndex&            index ) const
{
  QVariant data = index.data();

  QStyleOptionViewItemV4 opt = setOptions(index, option);

  const QStyleOptionViewItemV2 *v2 =
    qstyleoption_cast<const QStyleOptionViewItemV2 *>(&option);
  opt.features = v2 ? v2->features
    : QStyleOptionViewItemV2::ViewItemFeatures(QStyleOptionViewItemV2::None);
  const QStyleOptionViewItemV3 *v3 =
                  qstyleoption_cast<const QStyleOptionViewItemV3 *>(&option);
  opt.locale = v3 ? v3->locale : QLocale();
  opt.widget = v3 ? v3->widget : 0;

  painter->save();
  painter->setClipRect(option.rect);

  QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                            ? QPalette::Normal : QPalette::Disabled;
  if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
      cg = QPalette::Inactive;
  if (option.state & QStyle::State_Selected) {
      painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
      painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
  } else {
      painter->setPen(option.palette.color(cg, QPalette::Text));
  }

  QString pattern = m_list->m_pattern;
  QStringList lines = data.toString().split( '\n' );
  int y = option.rect.y() + painter->fontMetrics().ascent();

  for( int i = 0; i < lines.size(); i++ ) {
    QString line = lines.at( i );
    int x = option.rect.x() + 2;
    while( ! pattern.isEmpty() ) {
      int idx = line.indexOf( pattern );
      if( -1 == idx ) {
        break;
      }

      int idx_end = idx + pattern.length();
      painter->drawText( x, y, line.left( idx ) );
      painter->save();
      painter->setPen( QRgb(0xff0000) );
      x += painter->fontMetrics().width( line.left( idx ) );
      QRect rectPattern(  x,
                          y - painter->fontMetrics().ascent(),
                          painter->fontMetrics().width( pattern ),
                          painter->fontMetrics().height() );
      painter->fillRect( rectPattern, QRgb( 0x00ffff ) );
      painter->drawText( x, y, line.mid( idx, pattern.length() ) );
      painter->restore();
      x += painter->fontMetrics().width( pattern );
      line = line.mid( idx_end );
    }
    painter->drawText( x, y, line );
    y += painter->fontMetrics().lineSpacing();
  }

  painter->restore();
}


// -----------------------------------------------------------------------------
// OcaPopupList

OcaPopupList::OcaPopupList( QWidget* parent, QStringList list )
: QListWidget( parent ),
  m_list( list )
{
  Qt::WindowFlags flags = windowFlags();
  flags &= ( ~Qt::WindowType_Mask );
  flags |= Qt::Popup;
  setWindowFlags( flags );
  setAttribute( Qt::WA_DeleteOnClose );
  // Is that needed?
  //setAttribute(Qt::WA_X11NetWmWindowTypeMenu );
  setItemDelegate( new ItemDelegate( this ) );

  addItems( list );
  setCurrentItem( item(count()-1) );
  connect( this, SIGNAL(itemActivated(QListWidgetItem*) ), SLOT(selectItem()) );
  connect( this, SIGNAL(clicked (const QModelIndex&) ), SLOT(selectItem()) );
}

// -----------------------------------------------------------------------------

OcaPopupList::~OcaPopupList()
{
  // printf( "OcaPopupList::~OcaPopupList\n" );
}

// -----------------------------------------------------------------------------

QSize OcaPopupList::sizeHint () const
{
  if( NULL == parentWidget() ) {
    return QListWidget::sizeHint();
  }
  return QSize( parentWidget()->width(), QListWidget::sizeHint().height() );
}

// -----------------------------------------------------------------------------

void OcaPopupList::popup( QPoint pos )
{
  move( pos - QPoint( 0, sizeHint().height() ) );
  show();
  setFocus(Qt::PopupFocusReason);
}

// -----------------------------------------------------------------------------

void OcaPopupList::keyReleaseEvent ( QKeyEvent * event )
{
  if( Qt::Key_Return == event->key() ) {
    selectItem();
  }
  else if( Qt::Key_Escape == event->key() ) {
    if( m_pattern.isEmpty() ) {
      emit cancelled();
      close();
    }
    else {
      m_pattern.clear();
      applyPattern();
    }
  }
  QListWidget::keyReleaseEvent( event );
}

// -----------------------------------------------------------------------------

void OcaPopupList::keyPressEvent ( QKeyEvent * event )
{
  if( Qt::Key_Return == event->key() ) {
  }
  else if( Qt::Key_Backspace == event->key() ) {
    if( ! m_pattern.isEmpty() ) {
      m_pattern.remove( m_pattern.length()-1, 1 );
      //fprintf( stderr, "OcaPopupList::keyPressEvent: '%s'\n", m_pattern.toLocal8Bit().data() );
      applyPattern();
    }
  }
  else if( event->text()[0].isPrint() ) {
    m_pattern += event->text();
    //fprintf( stderr, "OcaPopupList::keyPressEvent: '%s'\n", m_pattern.toLocal8Bit().data() );
    applyPattern();
  }
  else {
    QListWidget::keyPressEvent( event );
  }
}

// -----------------------------------------------------------------------------

bool OcaPopupList::event( QEvent * event )
{
  if( QEvent::MouseButtonPress == event->type() ) {
    QMouseEvent* ev = (QMouseEvent*) event;
    if( ! rect().contains( ev->pos() ) ) {
      close();
    }
  }
  return QListWidget::event( event );
}

// -----------------------------------------------------------------------------

void OcaPopupList::selectItem()
{
  if( NULL != currentItem() ) {
    emit itemSelected( currentItem()->text() );
  }
  close();
}

// -----------------------------------------------------------------------------

void OcaPopupList::applyPattern()
{
  clear();
  addItems( m_list.filter( m_pattern ) );
  setCurrentItem( item(count()-1) );
}

// -----------------------------------------------------------------------------

