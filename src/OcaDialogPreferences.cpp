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

#include "OcaDialogPreferences.h"

#include "OcaObjectListener.h"
#include "OcaValidatorDouble.h"
#include "OcaApp.h"
#include "OcaInstance.h"
#include "OcaAudioController.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaDialogPreferences::OcaDialogPreferences()
:
  m_data( OcaApp::getOcaInstance()->getWindowData() )
{
  const uint mask =   OcaWindowData::e_FlagNameChanged
                    | OcaWindowData::e_FlagDefaultRateChanged;
  createListener( m_data, mask );
  m_listener->addObject( OcaApp::getAudioController(),
                         OcaAudioController::e_FlagSampleRateChanged |
                         OcaAudioController::e_FlagDeviceChanged );

  int row = 0;
  QGridLayout* layout = new QGridLayout( this );
  layout->addWidget( new QLabel( "Window Name" ), row, 0 );
  layout->addWidget( m_editName, row, 1 );

  layout->addWidget( new QLabel( "Window Display Name" ), ++row, 0 );
  layout->addWidget( m_editDisplayName, row, 1 );

  m_devOutput = new QComboBox();
  layout->addWidget( new QLabel( "Output Device" ), ++row, 0 );
  layout->addWidget( m_devOutput, row, 1 );
  connect( m_devOutput, SIGNAL(currentIndexChanged(const QString&)),
                        SLOT(setOutputDevice(const QString&)) );

  m_devInput = new QComboBox();
  layout->addWidget( new QLabel( "Input Device" ), ++row, 0 );
  layout->addWidget( m_devInput, row, 1 );
  connect( m_devInput,  SIGNAL(currentIndexChanged(const QString&)),
                        SLOT(setInputDevice(const QString&)) );
  QFontMetrics fm( m_devInput->font() );
  m_devOutput->setMinimumWidth( fm.averageCharWidth() * 50 );
  m_devInput->setMinimumWidth( fm.averageCharWidth() * 50 );

  m_devOutput->installEventFilter( this );
  m_devInput->installEventFilter( this );

  m_editDefaultRate = new QLineEdit( this );
  m_defaultRateValidator = new OcaValidatorDouble(  0.001, 1e6, 3 );
  m_editDefaultRate->setValidator( m_defaultRateValidator );
  connect( m_editDefaultRate, SIGNAL(editingFinished()), SLOT(setDefaultRate()) );
  layout->addWidget( new QLabel( "Sample Rate for New Groups" ), ++row, 0 );
  layout->addWidget( m_editDefaultRate, row, 1 );

  m_editSampleRate = new QLineEdit( this );
  m_sampleRateValidator = new OcaValidatorDouble(  0.001, 1e6, 3 );
  m_editSampleRate->setValidator( m_sampleRateValidator );
  connect( m_editSampleRate, SIGNAL(editingFinished()), SLOT(setAudioRate()) );
  layout->addWidget( new QLabel( "Audio Driver Sample Rate" ), ++row, 0 );
  layout->addWidget( m_editSampleRate, row, 1 );

  m_listener->addObject( OcaApp::getOcaInstance(),
                         OcaInstance::e_FlagCachePathChanged );
  m_editDataCacheBase = new QLineEdit( this );
  connect( m_editDataCacheBase, SIGNAL(editingFinished()), SLOT(setDataCache()) );
  layout->addWidget( new QLabel( "Data Cache Directory" ), ++row, 0 );
  layout->addWidget( m_editDataCacheBase, row, 1 );

  OcaApp::getAudioController()->checkDevices();
}

// -----------------------------------------------------------------------------

OcaDialogPreferences::~OcaDialogPreferences()
{
}

// -----------------------------------------------------------------------------

void OcaDialogPreferences::onUpdateRequired( uint flags )
{
  updateNames();
  m_sampleRateValidator->setFixupValue( OcaApp::getAudioController()->getSampleRate() );
  m_editSampleRate->setText( QString::number( OcaApp::getAudioController()->getSampleRate(), 'f', 3 ) );

  m_defaultRateValidator->setFixupValue( m_data->getDefaultSampleRate() );
  m_editDefaultRate->setText( QString::number( m_data->getDefaultSampleRate(), 'f', 3 ) );

  m_devOutput->blockSignals( true );
  m_devOutput->clear();
  m_devOutput->addItems( OcaApp::getAudioController()->enumDevices( false ) );
  int idx = m_devOutput->findText( OcaApp::getAudioController()->getDevice( false ) );
  m_devOutput->setCurrentIndex( idx );
  m_devOutput->blockSignals( false );

  m_devInput->blockSignals( true );
  m_devInput->clear();
  m_devInput->addItems( OcaApp::getAudioController()->enumDevices( true ) );
  idx = m_devInput->findText( OcaApp::getAudioController()->getDevice( true ) );
  m_devInput->setCurrentIndex( idx );
  m_devInput->blockSignals( false );

  m_editDataCacheBase->setText( OcaApp::getOcaInstance()->getDataCacheBase() );
}

// -----------------------------------------------------------------------------

void OcaDialogPreferences::setAudioRate()
{
  OcaApp::getAudioController()->setSampleRate( m_editSampleRate->text().toDouble() );
}

// -----------------------------------------------------------------------------

void OcaDialogPreferences::setDefaultRate()
{
  m_data->setDefaultSampleRate( m_editDefaultRate->text().toDouble() );
}

// -----------------------------------------------------------------------------

void OcaDialogPreferences::setInputDevice( const QString& dev_name )
{
  OcaApp::getAudioController()->setDevice( dev_name, true );
}

// -----------------------------------------------------------------------------

void OcaDialogPreferences::setOutputDevice( const QString& dev_name )
{
  OcaApp::getAudioController()->setDevice( dev_name, false );
}

// -----------------------------------------------------------------------------

void OcaDialogPreferences::setDataCache()
{
  OcaApp::getOcaInstance()->setDataCacheBase( m_editDataCacheBase->text() );
}

// -----------------------------------------------------------------------------

bool OcaDialogPreferences::eventFilter( QObject* obj, QEvent* ev )
{
  if( ev->type() == QEvent::KeyRelease ) {
    QKeyEvent *key_event = static_cast<QKeyEvent*>(ev);
    if( Qt::Key_Return == key_event->key() + key_event->modifiers() ) {
      keyReleaseEvent( key_event );
      return true;
    }
    if( Qt::Key_Escape == key_event->key() + key_event->modifiers() ) {
      keyReleaseEvent( key_event );
      return true;
    }
  }
  return OcaDialogPropertiesBase::eventFilter( obj, ev );
}

// -----------------------------------------------------------------------------

