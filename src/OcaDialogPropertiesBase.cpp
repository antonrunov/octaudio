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

#include "OcaDialogPropertiesBase.h"

#include "OcaObjectListener.h"
#include "OcaObject.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaDialogPropertiesBase::OcaDialogPropertiesBase()
:
  m_listener( NULL ),
  m_ok( false ),
  m_disableAutoClose( false )
{
  setAttribute( Qt::WA_DeleteOnClose );
  m_editName = new QLineEdit( this );
  m_editDisplayName = new QLineEdit( this );
  connect( m_editName, SIGNAL(editingFinished()), SLOT(onNameChanged()) );
  connect( m_editDisplayName, SIGNAL(editingFinished()), SLOT(onDisplayNameChanged()) );
}

// -----------------------------------------------------------------------------

OcaDialogPropertiesBase::~OcaDialogPropertiesBase()
{
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesBase::createListener( OcaObject* obj, uint mask )
{
  m_listener = new OcaObjectListener( obj, mask, 10, this );
  m_listener->addEvent( NULL, mask );

  connect(  m_listener,
            SIGNAL(updateRequired(uint,QHash<QString,uint>&,QList<OcaObject*>&)),
            SLOT(onUpdateRequired(uint))                                          );
  connect( m_listener, SIGNAL(closed()), SLOT(close()) );
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesBase::updateNames()
{
  m_editName->setText( getObject()->getName() );
  m_editDisplayName->setText( getObject()->getDisplayName() );
  m_editDisplayName->setPlaceholderText( getObject()->getName() );
  setWindowTitle( getObject()->getDisplayText() );
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesBase::onNameChanged()
{
  if( m_editName->text().isEmpty() ) {
    m_editName->setText( getObject()->getName() );
  }
  else {
    getObject()->setName( m_editName->text().trimmed() );
  }
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesBase::onDisplayNameChanged()
{
  if( ! m_editDisplayName->text().trimmed().isEmpty() ) {
    getObject()->setDisplayName( m_editDisplayName->text() );
  }
  else {
    m_editDisplayName->clear();
    getObject()->setDisplayName( "" );
  }
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesBase::changeEvent( QEvent* e )
{
  if( ( QEvent::ActivationChange == e->type() ) && ( ! isActiveWindow() ) ) {
    if( ! m_disableAutoClose ) {
      close();
    }
  }
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesBase::keyReleaseEvent( QKeyEvent* event )
{
  int key = ( event->modifiers() & (~Qt::KeypadModifier) ) + event->key();
  if( Qt::Key_Escape == key ) {
    event->accept();
    delete this;
  }
  else if( ( Qt::Key_Enter == key ) || ( Qt::Key_Return == key ) ) {
    event->accept();
    onOk();
  }
}

// -----------------------------------------------------------------------------

void OcaDialogPropertiesBase::onOk()
{
  m_ok = true;
  close();
}

// -----------------------------------------------------------------------------

