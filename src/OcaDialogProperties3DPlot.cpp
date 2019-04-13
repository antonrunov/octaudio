/*
   Copyright 2018-2019 Anton Runov

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
#include "OcaDialogProperties3DPlot.h"

#include "OcaApp.h"
#include "OcaInstance.h"
#include "OcaWindowData.h"
#include "OcaObjectListener.h"
#include "OcaTrackGroup.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaDialogProperties3DPlot::OcaDialogProperties3DPlot( Oca3DPlot* plot, bool create /*=false*/ )
:
  m_plot(plot),
  m_create(create)
{
  const uint mask =   Oca3DPlot::e_FlagNameChanged;
  createListener( m_plot, mask );
  int row = 0;
  QGridLayout* layout = new QGridLayout( this );
  layout->addWidget( new QLabel( "Name" ), row, 0 );
  layout->addWidget( m_editName, row, 1 );

  layout->addWidget( new QLabel( "Display Name" ), ++row, 0 );
  layout->addWidget( m_editDisplayName, row, 1 );

  if( m_create ) {
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                      | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    layout->addWidget( buttonBox, ++row, 0, 1, -1 );
  }
}

// -----------------------------------------------------------------------------

OcaDialogProperties3DPlot::~OcaDialogProperties3DPlot()
{
  if (m_create) {
    if( m_ok ) {
      OcaApp::getOcaInstance()->getWindowData()->add3DPlot(m_plot);
    }
    else {
      m_plot->close();
    }
  }
}

// -----------------------------------------------------------------------------

void OcaDialogProperties3DPlot::onUpdateRequired( uint flags )
{
  updateNames();
}

// -----------------------------------------------------------------------------

