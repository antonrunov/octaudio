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

#include "OcaSmartScreen.h"

#include "OcaScaleControl.h"
#include "OcaTrack.h"
#include "OcaSmartTrack.h"
#include "OcaObjectListener.h"
#include "OcaTrackGroup.h"
#include "OcaDialogPropertiesSmartTrack.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaSmartScreen::OcaSmartScreen(  OcaSmartTrack* trackObj, OcaTrackGroup* group )
: OcaDataScreen( trackObj, group ),
  m_trackObj( trackObj )
{
}

// -----------------------------------------------------------------------------

OcaSmartScreen::~OcaSmartScreen()
{
  m_dataBlocks.clear();
  clearHash( &m_trackBlocks );
}

// -----------------------------------------------------------------------------

void OcaSmartScreen::openHandleContextMenu( const QPoint& pos )
{
  QMenu* menu = new QMenu( this );
  QAction* action = NULL;
  menu->addAction( "Delete", m_trackObj, SLOT(close())  );
  menu->addAction( "Hide", this, SLOT(hideTrack()) );
  menu->addAction( "Up", this, SLOT(moveUpTrack()) );
  menu->addAction( "Down", this, SLOT(moveDownTrack())  );

  action = menu->addAction( "Solo", this, SLOT(soloTrack(bool))  );
  action->setCheckable( true );
  action->setChecked( isSolo() );

  action = menu->addAction( "Selected", m_trackObj, SLOT(setSelected(bool))  );
  action->setCheckable( true );
  action->setChecked( m_trackObj->isSelected() );

  action = menu->addAction( "Abs Value Mode", m_trackObj, SLOT(setAbsValueMode(bool)) );
  action->setCheckable( true );
  action->setChecked( m_absValueMode );

  action = menu->addAction( "Common Scale", m_trackObj, SLOT(setCommonScaleOn(bool))  );
  action->setCheckable( true );
  action->setChecked( m_trackObj->isCommonScaleOn() );

  menu->addAction( "Properties...", this, SLOT(openProperties()) );
  menu->exec( mapToGlobal(pos) );
}

// -----------------------------------------------------------------------------

void OcaSmartScreen::openProperties()
{
  OcaDialogPropertiesSmartTrack* dlg = new OcaDialogPropertiesSmartTrack( m_trackObj );
  dlg->show();
}

// -----------------------------------------------------------------------------

void OcaSmartScreen::updateTrackData(  uint flags, uint group_flags )
{
  const uint subtrack_flags =   OcaSmartTrack::e_FlagSubtrackAdded
                              | OcaSmartTrack::e_FlagSubtrackRemoved
                              | OcaSmartTrack::e_FlagSubtrackMoved
                              | OcaSmartTrack::e_FlagActiveSubtrackChanged;

  const uint subtrack_data_flags =    OcaSmartTrack::e_FlagSubtrackChanged
                                    | OcaSmartTrack::e_FlagSubtrackAdded
                                    | OcaSmartTrack::e_FlagTrackDataChanged;

  const uint scale_flags =    OcaSmartTrack::e_FlagAbsValueModeChanged
                            | OcaSmartTrack::e_FlagScaleChanged
                            | OcaSmartTrack::e_FlagZeroChanged
                            | OcaSmartTrack::e_FlagControlModeChanged
                            | OcaSmartTrack::e_FlagAutoscaleChanged;

  const uint color_flags = OcaSmartTrack::e_FlagSubtrackColorsChanged
                           | OcaSmartTrack::e_FlagActiveSubtrackChanged;

  bool update_required = false;

  OcaLock lock( m_trackObj );

  if( subtrack_flags & flags ) {
    updateSubtrackList();
  }

  if( subtrack_data_flags & flags ) {
    // TODO
    updateData();
  }
  if( ( subtrack_flags | subtrack_data_flags ) & flags ) {
    m_dataBlocks.clear();
    int count = m_trackObj->getSubtrackCount();
    OcaTrack* top = m_trackObj->getActiveSubtrack();
    if( 0 < count ) {
      Q_ASSERT( m_trackBlocks.contains( top ) );
      for( int idx = count-1; idx >= 0; idx-- ) {
        OcaTrack* t = m_trackObj->getSubtrack( idx );
        if( t != top ) {
          m_dataBlocks.append( *m_trackBlocks.value( t ) );
        }
      }
      m_dataBlocks.append( *m_trackBlocks.value( top ) );
    }
    update_required = true;
  }

  if( scale_flags & flags ) {
    double scale = m_trackObj->getScale();
    double zero  = m_trackObj->getZero();
    m_absValueMode   = m_trackObj->getAbsValueMode();
    m_scaleControl->setValue( scale );
    m_zeroControl->setValue( zero );

    int count = m_trackObj->getSubtrackCount();
    for( int idx = 0; idx < count; idx++ ) {
      OcaTrack* t = m_trackObj->getSubtrack( idx );
      if( ! m_trackObj->isCommonScaleOn() ) {
        scale = m_trackObj->getSubtrackScale( t );
        zero  = m_trackObj->getSubtrackZero( t );
      }
      QList<DataBlock*>* list = m_trackBlocks.value( t );
      Q_ASSERT( NULL != list );
      for( int i = 0; i < list->size(); i++ ) {
        DataBlock* block = list->at( i );
        block->setScale( scale );
        block->setZero(  zero  );
        block->setAbsValueMode( m_absValueMode );
      }
    }
    update_required = true;
  }

  if( color_flags & flags ) {
    int count = m_trackObj->getSubtrackCount();
    OcaTrack* top = m_trackObj->getActiveSubtrack();
    for( int idx = 0; idx < count; idx++ ) {
      OcaTrack* t = m_trackObj->getSubtrack( idx );
      QColor c = m_trackObj->getSubtrackColor( t );
      if( t != top ) {
        c.setAlphaF( m_trackObj->getAuxTransparency() );
      }
      QList<DataBlock*>* list = m_trackBlocks.value( t );
      Q_ASSERT( NULL != list );
      for( int i = 0; i < list->size(); i++ ) {
        DataBlock* block = list->at( i );
        block->setColor( c );
      }
    }
    update_required = true;
  }

  if( update_required ) {
    update();
  }
}

// -----------------------------------------------------------------------------

void OcaSmartScreen::updateSubtrackList()
{
  QHash<OcaTrack*, QList<DataBlock*>*> old_blocks = m_trackBlocks;
  m_trackBlocks.clear();
  int count = m_trackObj->getSubtrackCount();
  for( int idx = 0; idx < count; idx++ ) {
    OcaTrack* t =  m_trackObj->getSubtrack( idx );
    if( old_blocks.contains( t ) ) {
      m_trackBlocks[ t ] = old_blocks.value( t );
      old_blocks.remove( t );
    }
    else {
      m_trackBlocks[ t ] = new QList<DataBlock*>;
    }
  }
  clearHash( &old_blocks );
  //fprintf( stderr, "OcaSmartScreen::updateSubtrackList - count = %d\n", count );
}

// -----------------------------------------------------------------------------

void OcaSmartScreen::clearHash( QHash<OcaTrack*, QList<DataBlock*>*>* hash )
{
  QHashIterator<OcaTrack*, QList<DataBlock*>*> it( *hash );
  while (it.hasNext()) {
    it.next();
    clearBlocks( it.value() );
    delete it.value();
  }
  hash->clear();
}

// -----------------------------------------------------------------------------

void OcaSmartScreen::updateData()
{
  double scale = m_trackObj->getScale();
  double zero =  m_trackObj->getZero();
  int count = m_trackObj->getSubtrackCount();
  OcaTrack* top = m_trackObj->getActiveSubtrack();
  for( int idx = 0; idx < count; idx++ ) {
    OcaTrack* t =  m_trackObj->getSubtrack( idx );
    // TODO if( flags || track_flags )

    QColor color = m_trackObj->getSubtrackColor( t );
    if( t != top ) {
      color.setAlphaF( m_trackObj->getAuxTransparency() );
    }
    if( ! m_trackObj->isCommonScaleOn() ) {
      scale = m_trackObj->getSubtrackScale( t );
      zero = m_trackObj->getSubtrackZero( t );
    }
    updateBlocks( m_trackBlocks.value( t ), t, zero, scale, color );
  }
}

// -----------------------------------------------------------------------------

void OcaSmartScreen::updateHandleText()
{
  const char* solo_mute = " ";
  OcaTrack* t = m_trackObj->getActiveSubtrack();

  if( ( NULL != t ) && ( ! t->isAudible() ) ) {
    solo_mute = "#";
  }
  else if( isSolo() ) {
    solo_mute = "S";
  }
  else if( isSoloMode() ) {
    solo_mute = "-";
  }

  oca_index index = -1;
  if( NULL != m_group ) {
    index = m_group->getTrackIndex( m_trackObj ) + 1;
  }
  m_label->setText( QString( " %1 %2 - %3 <%4>" )
                    . arg( solo_mute )
                    . arg( index, 2, 10, QChar('0') )
                    . arg( m_trackObj->getDisplayText() )
                    . arg( ( NULL != t ) ? t->getDisplayText() : QString() )  );
  m_label->resize( m_label->sizeHint() );
}

// -----------------------------------------------------------------------------

void OcaSmartScreen::updateHandlePalette()
{
  QPalette p = m_label->palette();
  QColor c = p.color( m_label->foregroundRole() );
  if( ! m_active ) {
    c.setAlphaF( 0.8 );
  }
  p.setColor( m_label->foregroundRole(), c );

  c = QRgb( 0xc0ff40 );
  if( ! m_active ) {
    c.setAlphaF( 0.4 );
  }
  p.setColor( m_label->backgroundRole(), c );

  m_label->setPalette( p );
  m_label->setAutoFillBackground(true);
}

// -----------------------------------------------------------------------------

void OcaSmartScreen::resizeEvent( QResizeEvent* event )
{
  if( event->oldSize().width() != width() ) {
    //updateData();
    // TODO
    m_listener->addEvent( NULL, OcaSmartTrack::e_FlagTrackDataChanged );
  }
  OcaDataScreen::resizeEvent( event );
}

// -----------------------------------------------------------------------------

