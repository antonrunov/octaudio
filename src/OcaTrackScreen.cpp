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

#include "OcaTrackScreen.h"

#include "OcaScaleControl.h"
#include "OcaTrack.h"
#include "OcaObjectListener.h"
#include "OcaTrackGroup.h"
#include "OcaApp.h"
#include "OcaAudioController.h"
#include "OcaDialogPropertiesTrack.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaTrackScreen::OcaTrackScreen( OcaTrack* trackObj, OcaTrackGroup* group )
: OcaDataScreen( trackObj, group ),
  m_trackObj( trackObj )
{
}

// -----------------------------------------------------------------------------

OcaTrackScreen::~OcaTrackScreen()
{
  clearBlocks( &m_dataBlocks );
}

// -----------------------------------------------------------------------------

void OcaTrackScreen::updateTrackData(  uint flags, uint group_flags )
{
  const uint scale_flags =    OcaTrack::e_FlagScaleChanged
                            | OcaTrack::e_FlagZeroChanged
                            | OcaTrack::e_FlagAbsValueModeChanged;

  bool update_required = false;
  if( OcaTrack::e_FlagTrackDataChanged & flags ) {
    updateData();
    update_required = true;
  }
  if( scale_flags & flags ) {
    updateScale();
    update_required = true;
  }

  if( update_required ) {
    update();
  }
}

// -----------------------------------------------------------------------------

void OcaTrackScreen::updateData()
{
  double scale = m_trackObj->getScale();
  double zero =  m_trackObj->getZero();
  updateBlocks( &m_dataBlocks, m_trackObj, zero, scale, Qt::blue );
}

// -----------------------------------------------------------------------------

void OcaTrackScreen::updateHandleText()
{
  const char* solo_mute = " ";
  if( ! m_trackObj->isAudible() ) {
    solo_mute = "#";
  }
  else if( isSolo() ) {
    solo_mute = "S";
  }
  else if( isSoloMode() ) {
    solo_mute = "-";
  }
  else if( m_trackObj->isMuted() ) {
    solo_mute = "M";
  }
  oca_index index = m_group->getTrackIndex( m_trackObj ) + 1;
  m_label->setText( QString( " %1 %2 - %3" )
                    . arg( solo_mute )
                    . arg( index, 2, 10, QChar('0') )
                    . arg( m_trackObj->getDisplayText() )  );
  m_label->resize( m_label->sizeHint() );
}

// -----------------------------------------------------------------------------

void OcaTrackScreen::updateScale()
{
  double scale = m_trackObj->getScale();
  double zero  = m_trackObj->getZero();
  m_absValueMode   = m_trackObj->getAbsValueMode();

  for( int i = 0; i < m_dataBlocks.size(); i++ ) {
    m_dataBlocks[i]->setScale( scale );
    m_dataBlocks[i]->setZero(  zero  );
    m_dataBlocks[i]->setAbsValueMode( m_absValueMode );
  }
  m_scaleControl->setValue( scale );
  m_zeroControl->setValue( zero );
}

// -----------------------------------------------------------------------------

void OcaTrackScreen::openHandleContextMenu( const QPoint& pos )
{
  QMenu* menu = new QMenu( this );
  QAction* action = NULL;
  menu->addAction( "Delete", m_trackObj, SLOT(close())  );
  menu->addAction( "Hide", this, SLOT(hideTrack()) );
  menu->addAction( "Up", this, SLOT(moveUpTrack()) );
  menu->addAction( "Down", this, SLOT(moveDownTrack())  );

  action = menu->addAction( "Mute", m_trackObj, SLOT(setMute(bool)) );
  action->setCheckable( true );
  action->setChecked( m_trackObj->isMuted() );
  action->setEnabled( m_trackObj->isAudible() );

  action = menu->addAction( "Solo", this, SLOT(soloTrack(bool))  );
  action->setCheckable( true );
  action->setChecked( isSolo() );
  action->setEnabled( m_trackObj->isAudible() );

  action = menu->addAction( "Rec 1", this, SLOT(setRec1(bool))  );
  action->setCheckable( true );
  action->setEnabled( m_trackObj->isAudible() );
  action->setChecked( m_group->getRecTrack1() == m_trackObj );

  action = menu->addAction( "Rec 2", this, SLOT(setRec2(bool))  );
  action->setCheckable( true );
  action->setEnabled( m_trackObj->isAudible() );
  action->setChecked( m_group->getRecTrack2() == m_trackObj );

  action = menu->addAction( "Readonly", m_trackObj, SLOT(setReadonly(bool)) );
  action->setCheckable( true );
  action->setChecked( m_trackObj->isReadonly() );

  menu->addAction( "New Monitor" )->setEnabled( false );
  //menu->addAction( "NewGroup" )->setEnabled( false );
  menu->addAction( "New Smart Track" )->setEnabled( false );
  menu->addAction( "Resample..." )->setEnabled( false );

  action = menu->addAction( "Selected", m_trackObj, SLOT(setSelected(bool))  );
  action->setCheckable( true );
  action->setChecked( m_trackObj->isSelected() );

  action = menu->addAction( "Abs Value Mode", m_trackObj, SLOT(setAbsValueMode(bool)) );
  action->setCheckable( true );
  action->setChecked( m_absValueMode );

  menu->addAction( "Properties...", this, SLOT(openProperties()) );
  menu->exec( mapToGlobal(pos) );
}

// -----------------------------------------------------------------------------

void OcaTrackScreen::setRec1( bool on )
{
  Q_ASSERT( NULL != m_group );
  if( on ) {
    m_group->setRecTrack1( m_trackObj );
  }
  else if( m_group->getRecTrack1() == m_trackObj ) {
    m_group->setRecTrack1( NULL );
  }
}

// -----------------------------------------------------------------------------

void OcaTrackScreen::setRec2( bool on )
{
  Q_ASSERT( NULL != m_group );
  if( on ) {
    m_group->setRecTrack2( m_trackObj );
  }
  else if( m_group->getRecTrack2() == m_trackObj ) {
    m_group->setRecTrack2( NULL );
  }
}

// -----------------------------------------------------------------------------

void OcaTrackScreen::openProperties()
{
  OcaDialogPropertiesTrack* dlg = new OcaDialogPropertiesTrack( m_trackObj );
  dlg->show();
}

// -----------------------------------------------------------------------------

void OcaTrackScreen::updateHandlePalette()
{
  QPalette p = m_label->palette();
  QColor c = p.color( m_label->foregroundRole() );
  if( ! m_active ) {
    c.setAlphaF( 0.8 );
  }
  p.setColor( m_label->foregroundRole(), c );

  c = p.color( m_label->backgroundRole() );
  //c = QRgb( m_readonly ? 0xb0b0b0 : 0xff8080 );
  if( m_trackObj->isReadonly() ) {
    c = QRgb( 0xb0b0b0 );
  }
  else if( m_group->isRecordingTrack( m_trackObj ) ) {
    c = QRgb( 0xffa090 );
  }
  else {
    c = QRgb( 0xc0e0ff );
  }
  if( ! m_active ) {
    c.setAlphaF( 0.4 );
  }
  p.setColor( m_label->backgroundRole(), c );

  m_label->setPalette( p );
  m_label->setAutoFillBackground(true);
}

// -----------------------------------------------------------------------------

void OcaTrackScreen::resizeEvent( QResizeEvent* event )
{
  if( event->oldSize().width() != width() ) {
    updateData();
  }
  OcaDataScreen::resizeEvent( event );
}

// -----------------------------------------------------------------------------

