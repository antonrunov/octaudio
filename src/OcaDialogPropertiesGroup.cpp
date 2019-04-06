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

#include "OcaDialogPropertiesGroup.h"

#include "OcaObjectListener.h"
#include "OcaValidatorDouble.h"
#include "OcaWindowData.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaDialogPropertiesGroup::OcaDialogPropertiesGroup( OcaTrackGroup* group,
                                                    OcaWindowData* window /* = NULL */ )
:
  m_group( group ),
  m_window( window )
{
  const uint mask =   OcaTrackGroup::e_FlagNameChanged
                    | OcaTrackGroup::e_FlagDefaultSampleRateChanged;
  createListener( m_group, mask );
  QGridLayout* layout = new QGridLayout( this );
  layout->addWidget( new QLabel( "Name" ), 0, 0 );
  layout->addWidget( m_editName, 0, 1 );
  layout->addWidget( new QLabel( "Display Name" ), 1, 0 );
  layout->addWidget( m_editDisplayName, 1, 1 );

  m_editSampleRate = new QLineEdit( this );
  m_sampleRateValidator = new OcaValidatorDouble(  0.001, 1e9, 3 );
  m_editSampleRate->setValidator( m_sampleRateValidator );
  connect( m_editSampleRate, SIGNAL(editingFinished()), SLOT(onSampleRateChanged()) );
  layout->addWidget( new QLabel( "Sample Rate for New Tracks" ), 2, 0 );
  layout->addWidget( m_editSampleRate, 2, 1 );

  if( NULL != m_window ) {
    m_listener->addObject( m_window, 0 );
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                      | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    layout->addWidget( buttonBox, 3, 0, 1, -1 );
  }
}

// -----------------------------------------------------------------------------

OcaDialogPropertiesGroup::~OcaDialogPropertiesGroup()
{
  if( NULL != m_window ) {
    if( m_ok ) {
      m_window->addGroup( m_group );
      m_window->setActiveGroup( m_group );
    }
    else {
      m_group->close();
    }
  }
  delete m_sampleRateValidator;
  m_sampleRateValidator = NULL;
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesGroup::onUpdateRequired( uint flags )
{
  updateNames();
  m_sampleRateValidator->setFixupValue( m_group->getDefaultSampleRate() );
  m_editSampleRate->setText( QString::number( m_group->getDefaultSampleRate(), 'f', 3 ) );
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesGroup::onSampleRateChanged()
{
  m_group->setDefaultSampleRate( m_editSampleRate->text().toDouble() );
}

// -----------------------------------------------------------------------------

