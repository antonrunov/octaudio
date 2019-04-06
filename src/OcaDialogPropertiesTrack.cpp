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

#include "OcaDialogPropertiesTrack.h"

#include "OcaObjectListener.h"
#include "OcaValidatorDouble.h"
#include "OcaTrackGroup.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaDialogPropertiesTrack::OcaDialogPropertiesTrack( OcaTrack* track,
                                                    OcaTrackGroup* group /* = NULL */ )
:
  m_track( track ),
  m_group( group )
{
  const uint mask =   OcaTrack::e_FlagNameChanged
                    | OcaTrack::e_FlagSampleRateChanged
                    | OcaTrack::e_FlagAbsValueModeChanged
                    | OcaTrack::e_FlagGainChanged
                    | OcaTrack::e_FlagStereoPanChanged
                    | OcaTrack::e_FlagAudibleChanged
                    | OcaTrack::e_FlagStereoPanChanged
                    | OcaTrack::e_FlagChannelsChanged
                    | OcaTrack::e_FlagReadonlyChanged;

  createListener( m_track, mask );
  int row = 0;
  QGridLayout* layout = new QGridLayout( this );
  layout->addWidget( new QLabel( "Name" ), row, 0 );
  layout->addWidget( m_editName, row, 1 );

  layout->addWidget( new QLabel( "Display Name" ), ++row, 0 );
  layout->addWidget( m_editDisplayName, row, 1 );

  m_editSampleRate = new QLineEdit( this );
  m_sampleRateValidator = new OcaValidatorDouble(  0.001, 1e9, 3 );
  m_editSampleRate->setValidator( m_sampleRateValidator );
  connect( m_editSampleRate, SIGNAL(editingFinished()), SLOT(onSampleRateChanged()) );
  layout->addWidget( new QLabel( "Sample Rate" ), ++row, 0 );
  layout->addWidget( m_editSampleRate, row, 1 );

  m_chkReadonly = new QCheckBox( this );
  layout->addWidget( new QLabel( "Readonly" ), ++row, 0 );
  layout->addWidget( m_chkReadonly, row, 1 );
  m_track->connect( m_chkReadonly, SIGNAL(toggled(bool)), SLOT(setReadonly(bool)) );

  m_chkAbsValueMode = new QCheckBox( this );
  layout->addWidget( new QLabel( "Abs Value Mode" ), ++row, 0 );
  layout->addWidget( m_chkAbsValueMode, row, 1 );
  m_track->connect( m_chkAbsValueMode, SIGNAL(toggled(bool)), SLOT(setAbsValueMode(bool)) );

  m_chkAudible = new QCheckBox( this );
  layout->addWidget( new QLabel( "Audible" ), ++row, 0 );
  layout->addWidget( m_chkAudible, row, 1 );
  m_track->connect( m_chkAudible, SIGNAL(toggled(bool)), SLOT(setAudible(bool)) );

  m_regGain = new QDoubleSpinBox( this );
  m_regGain->setMinimum( -100 );
  m_regGain->setMaximum( 100 );
  m_regGain->setDecimals( 2 );
  layout->addWidget( new QLabel( "Gain (dB)" ), ++row, 0 );
  layout->addWidget( m_regGain, row, 1 );
  connect( m_regGain, SIGNAL(valueChanged(double)), SLOT(onGainChanged(double)) );

  m_regStereoPan = new QDoubleSpinBox( this );
  m_regStereoPan->setMinimum( -1 );
  m_regStereoPan->setMaximum( 1 );
  m_regStereoPan->setDecimals( 2 );
  m_regStereoPan->setSingleStep ( 0.05 );
  layout->addWidget( new QLabel( "Stereo Pan" ), ++row, 0 );
  layout->addWidget( m_regStereoPan, row, 1 );
  connect( m_regStereoPan, SIGNAL(valueChanged(double)), SLOT(onPanChanged(double)) );

  m_cmbChannels = new QComboBox( this );
  m_cmbChannels->setEditable( true );
  m_cmbChannels->setValidator( new QIntValidator( 1, 32768 ) );
  for( int i = 1; i <= 8; i++ ) {
    m_cmbChannels->addItem( QString::number(i) );
  }
  m_cmbChannels->setInsertPolicy( QComboBox::NoInsert );
  layout->addWidget( new QLabel( "Channels" ), ++row, 0 );
  layout->addWidget( m_cmbChannels, row, 1 );
  connect( m_cmbChannels, SIGNAL(editTextChanged(const QString&)),
                          SLOT(onChannelsChanged(const QString&)) );

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

OcaDialogPropertiesTrack::~OcaDialogPropertiesTrack()
{
  if( NULL != m_group ) {
    if( m_ok ) {
      m_group->addTrack( m_track );
      m_group->setActiveTrack( m_track );
    }
    else {
      m_track->close();
    }
  }
  delete m_sampleRateValidator;
  m_sampleRateValidator = NULL;
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesTrack::onUpdateRequired( uint flags )
{
  updateNames();
  m_sampleRateValidator->setFixupValue( m_track->getSampleRate() );
  m_editSampleRate->setText( QString::number( m_track->getSampleRate(), 'f', 3 ) );
  m_chkReadonly->setChecked( m_track->isReadonly() );
  m_chkAbsValueMode->setChecked( m_track->getAbsValueMode() );
  m_chkAudible->setChecked( m_track->isAudible() );
  m_regGain->setEnabled( m_track->isAudible() );
  m_regGain->blockSignals( true );
  m_regGain->setValue( 20 * log10( m_track->getGain() ) );
  m_regGain->blockSignals( false );
  m_regStereoPan->setEnabled( m_track->isAudible() );
  m_regStereoPan->blockSignals( true );
  m_regStereoPan->setValue( m_track->getStereoPan() );
  m_regStereoPan->blockSignals( false );
  int ch = m_track->getChannels();
  if( ch <= m_cmbChannels->count() ) {
    m_cmbChannels->setCurrentIndex( ch - 1 );
  }
  else {
    m_cmbChannels->setCurrentIndex( -1 );
    m_cmbChannels->setEditText( QString::number( ch ) );
  }
  m_regStereoPan->setEnabled( 1 == ch );
  m_cmbChannels->setEnabled( 0.0 == m_track->getDuration() );
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesTrack::onSampleRateChanged()
{
  m_track->setSampleRate( m_editSampleRate->text().toDouble() );
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesTrack::onGainChanged( double value )
{
  m_track->setGain( pow( 10, value/20 ) );
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesTrack::onPanChanged( double value )
{
  m_track->setStereoPan( value );
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesTrack::onChannelsChanged( const QString& value)
{
  m_track->setChannels( value.toUInt() );
}

// -----------------------------------------------------------------------------

