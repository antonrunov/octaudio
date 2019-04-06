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

#include "OcaDialogPropertiesSmartTrack.h"

#include "OcaObjectListener.h"
#include "OcaValidatorDouble.h"
#include "OcaTrackGroup.h"
#include "OcaTrack.h"
#include "OcaScaleControl.h"
#include "OcaApp.h"
#include "OcaInstance.h"
#include "OcaWindowData.h"
#include "OcaMonitor.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaDialogPropertiesSmartTrack::OcaDialogPropertiesSmartTrack( OcaSmartTrack* track,
                                                    OcaTrackGroup* group /* = NULL */ )
:
  m_track( track ),
  m_group( group )
{
  const uint mask =   OcaSmartTrack::e_FlagNameChanged
                    | OcaSmartTrack::e_FlagAbsValueModeChanged
                    | OcaSmartTrack::e_FlagSubtrackAdded
                    | OcaSmartTrack::e_FlagSubtrackRemoved
                    | OcaSmartTrack::e_FlagSubtrackMoved
                    | OcaSmartTrack::e_FlagSubtrackChanged
                    | OcaSmartTrack::e_FlagSubtrackColorsChanged
                    | OcaSmartTrack::e_FlagActiveSubtrackChanged
                    | OcaSmartTrack::e_FlagControlModeChanged
                    | OcaSmartTrack::e_FlagAutoscaleChanged
                    | OcaSmartTrack::e_FlagScaleChanged
                    | OcaSmartTrack::e_FlagZeroChanged ;

  createListener( m_track, mask );
  int row = 0;
  QGridLayout* layout = new QGridLayout( this );
  layout->addWidget( new QLabel( "Name" ), row, 0 );
  layout->addWidget( m_editName, row, 1 );

  layout->addWidget( new QLabel( "Display Name" ), ++row, 0 );
  layout->addWidget( m_editDisplayName, row, 1 );

  m_chkAbsValueMode = new QCheckBox( this );
  layout->addWidget( new QLabel( "Abs Value Mode" ), ++row, 0 );
  layout->addWidget( m_chkAbsValueMode, row, 1 );
  m_track->connect( m_chkAbsValueMode, SIGNAL(toggled(bool)), SLOT(setAbsValueMode(bool)) );

  m_listSubtracks = new QListWidget( this );
  m_listSubtracks->setContextMenuPolicy( Qt::CustomContextMenu );
  connect( m_listSubtracks, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
                            SLOT(setActiveSubtrack())   );
  connect( m_listSubtracks, SIGNAL(customContextMenuRequested(const QPoint&)),
                            SLOT(openContextMenu(const QPoint&))   );
  connect( m_listSubtracks, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*) ),
                            SLOT(updateScales())   );

  connect( m_listSubtracks, SIGNAL(itemChanged(QListWidgetItem*) ),
                            SLOT(onItemChanged(QListWidgetItem*))   );

  layout->addWidget( new QLabel( "Edit Track List" ), ++row, 0 );
  m_chkEditTrackList = new QCheckBox( this );
  m_chkEditTrackList->setChecked( NULL != m_group );
  layout->addWidget( m_chkEditTrackList, row, 1 );
  connect( m_chkEditTrackList, SIGNAL(toggled(bool)), SLOT(updateTrackList()) );

  layout->addWidget( m_listSubtracks, ++row, 0, 1, -1 );

  m_regTransparency = new QDoubleSpinBox( this );
  m_regTransparency->setMinimum( 0.0 );
  m_regTransparency->setMaximum( 1.0 );
  m_regTransparency->setSingleStep( 0.05 );
  m_regTransparency->setDecimals( 2 );
  layout->addWidget( new QLabel( "Aux Transparency" ), ++row, 0 );
  layout->addWidget( m_regTransparency, row, 1 );
  m_track->connect( m_regTransparency, SIGNAL(valueChanged(double)), SLOT(setAuxTransparency(double)) );

  m_chkCommonScale = new QCheckBox( this );
  layout->addWidget( new QLabel( "Common Scale" ), ++row, 0 );
  layout->addWidget( m_chkCommonScale, row, 1 );
  m_track->connect( m_chkCommonScale, SIGNAL(toggled(bool)), SLOT(setCommonScaleOn(bool)) );

  m_scale = new OcaScaleControl( this );
  layout->addWidget( new QLabel( "Vertical Scale" ), ++row, 0 );
  layout->addWidget( m_scale, row, 1 );

  m_zero = new OcaScaleControl( this );
  layout->addWidget( new QLabel( "Zero Offset" ), ++row, 0 );
  layout->addWidget( m_zero, row, 1 );

  connect( m_scale, SIGNAL(changed(double)), SLOT(setScale(double)) );
  connect( m_zero, SIGNAL(changed(double)), SLOT(setZero(double)) );
  connect( m_scale, SIGNAL(moved(double)), SLOT(moveScale(double)) );
  connect( m_zero, SIGNAL(moved(double)), SLOT(moveZero(double)) );

  if( NULL != m_group ) {
    m_listener->addObject( m_group, 0 );
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                      | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    layout->addWidget( buttonBox, ++row, 0, 1, -1 );
  }
}

// -----------------------------------------------------------------------------

OcaDialogPropertiesSmartTrack::~OcaDialogPropertiesSmartTrack()
{
  if( NULL != m_group ) {
    if( m_ok ) {
      OcaMonitor* monitor = qobject_cast<OcaMonitor*>( m_track );
      if( NULL != monitor ) {
        OcaApp::getOcaInstance()->getWindowData()->addMonitor( monitor );
      }
      else {
        m_group->addTrack( m_track );
        m_group->setActiveTrack( m_track );
      }
    }
    else {
      m_track->close();
    }
  }
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesSmartTrack::onUpdateRequired( uint flags )
{
  updateNames();
  m_chkAbsValueMode->setChecked( m_track->getAbsValueMode() );
  m_chkCommonScale->setChecked( m_track->isCommonScaleOn() );
  m_regTransparency->setValue( m_track->getAuxTransparency() );

  const uint mask =   OcaSmartTrack::e_FlagSubtrackAdded
                    | OcaSmartTrack::e_FlagSubtrackRemoved
                    | OcaSmartTrack::e_FlagSubtrackMoved
                    | OcaSmartTrack::e_FlagSubtrackColorsChanged
                    | OcaSmartTrack::e_FlagActiveSubtrackChanged;

  if( mask & flags ) {
    updateTrackList();
  }
  updateScales();
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesSmartTrack::openContextMenu( const QPoint& pos )
{
  QMenu* menu = new QMenu( this );
  //QAction* action = NULL;
  menu->addAction( "Set Color", this, SLOT(setSubtrackColor()) );
  menu->addAction( "Set Active", this, SLOT(setActiveSubtrack()) );
  menu->addAction( "Remove", this, SLOT(removeSubtrack())  );
  menu->exec( m_listSubtracks->mapToGlobal(pos) );
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesSmartTrack::setActiveSubtrack()
{
  OcaTrack* t = getCurrentSubtrack();
  if( NULL != t ) {
    m_track->setActiveSubtrack( t );
  }
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesSmartTrack::setSubtrackColor()
{
  OcaTrack* t = getCurrentSubtrack();
  if( NULL != t ) {
    m_disableAutoClose = true;
    QColor c = QColorDialog::getColor( m_track->getSubtrackColor( t ), this );
    m_disableAutoClose = false;
    if( c.isValid() ) {
      m_track->setSubtrackColor( t, c );
    }
  }
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesSmartTrack::removeSubtrack()
{
  OcaTrack* t = getCurrentSubtrack();
  if( NULL != t ) {
    m_track->removeSubtrack( t );
  }
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesSmartTrack::updateTrackList()
{
  m_listSubtracks->blockSignals( true );
  m_listSubtracks->clear();
  if( m_chkEditTrackList->isChecked() ) {
    OcaTrack* active = m_track->getActiveSubtrack();
    OcaTrackGroup* group = OcaApp::getOcaInstance()->getWindowData()->getActiveGroup();
    Q_ASSERT( NULL != group );
    OcaLock lock( group );
    for( uint i = 0; i < group->getTrackCount(); i++ ) {
      OcaTrack* t = qobject_cast<OcaTrack*>( group->getTrackAt(i) );
      if( NULL != t ) {
        QListWidgetItem* item = new QListWidgetItem( t->getName(), m_listSubtracks );
        item->setData( Qt::UserRole, (qlonglong)t->getId() );
        if( t == active ) {
          //m_listSubtracks->setCurrentItem( item );
        }
        if( -1 != m_track->findSubtrack( t ) ) {
          item->setCheckState( Qt::Checked );
        }
        else {
          item->setCheckState( Qt::Unchecked );
        }
        item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsUserCheckable );
      }
    }
  }
  else {
    OcaLock lock( m_track );
    OcaTrack* active = m_track->getActiveSubtrack();
    QListWidgetItem* item = NULL;
    for( uint i = 0; i < m_track->getSubtrackCount(); i++ ) {
      OcaTrack *t =  m_track->getSubtrack( i );
      Q_ASSERT( NULL != t );
      item = new QListWidgetItem( t->getName(), m_listSubtracks );
      item->setData( Qt::ForegroundRole, m_track->getSubtrackColor( t ) );
      item->setData( Qt::UserRole, (qlonglong)t->getId() );
      if( t == active ) {
        m_listSubtracks->setCurrentItem( item );
      }
      item->setCheckState( (t == active ) ? Qt::Checked : Qt::Unchecked );
      item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
    }
  }
  m_listSubtracks->blockSignals( false );
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesSmartTrack::updateScales()
{
  bool enabled = true;
  if( m_track->isCommonScaleOn() ) {
    m_scale->setValue( m_track->getScale() );
    m_zero->setValue( m_track->getZero() );
  }
  else {
    OcaTrack* t = getCurrentSubtrack();
    if( NULL != t ) {
      m_scale->setValue( m_track->getSubtrackScale( t ) );
      m_zero->setValue( m_track->getSubtrackZero( t ) );
    }
    else {
      m_scale->setValue( m_track->getScale() );
      m_zero->setValue( m_track->getZero() );
      enabled = false;
    }
  }
  m_scale->setEnabled( enabled );
  m_zero->setEnabled( enabled );
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesSmartTrack::setScale( double scale )
{
  if( m_track->isCommonScaleOn() ) {
    m_track->setScale( scale );
  }
  else {
    OcaTrack* t = getCurrentSubtrack();
    if( NULL != t ) {
       m_track->setSubtrackScale( t, scale );
    }
  }
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesSmartTrack::setZero( double zero )
{
  if( m_track->isCommonScaleOn() ) {
    m_track->setZero( zero );
  }
  else {
    OcaTrack* t = getCurrentSubtrack();
    if( NULL != t ) {
       m_track->setSubtrackZero( t, zero );
    }
  }
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesSmartTrack::moveScale( double step )
{
  if( m_track->isCommonScaleOn() ) {
    m_track->moveScale( step );
  }
  else {
    OcaTrack* t = getCurrentSubtrack();
    if( NULL != t ) {
       m_track->moveSubtrackScale( t, step );
    }
  }
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesSmartTrack::moveZero( double step )
{
  if( m_track->isCommonScaleOn() ) {
    m_track->moveZero( step );
  }
  else {
    OcaTrack* t = getCurrentSubtrack();
    if( NULL != t ) {
       m_track->moveSubtrackZero( t, step );
    }
  }
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesSmartTrack::onItemChanged( QListWidgetItem* item )
{
  if( m_chkEditTrackList->isChecked() ) {
    OcaTrack* t = getSubtrack( item );
    if( Qt::Checked == item->checkState() ) {
      m_track->addSubtrack( t );
    }
    else {
      m_track->removeSubtrack( t );
    }
  }
}

// -----------------------------------------------------------------------------

OcaTrack* OcaDialogPropertiesSmartTrack::getCurrentSubtrack() const
{
  return getSubtrack( m_listSubtracks->currentItem() );
}

// -----------------------------------------------------------------------------

OcaTrack* OcaDialogPropertiesSmartTrack::getSubtrack( QListWidgetItem* item ) const
{
  OcaTrack* t = NULL;
  if( NULL != item ) {
    t = qobject_cast<OcaTrack*>(
          OcaObject::getObject( (oca_ulong)item->data( Qt::UserRole ).toULongLong() ) );
  }
  return t;
}

// -----------------------------------------------------------------------------

