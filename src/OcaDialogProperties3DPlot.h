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

#pragma once

#include "OcaDialogPropertiesBase.h"
#include "Oca3DPlot.h"

class OcaTrackGroup;

class OcaDialogProperties3DPlot : public OcaDialogPropertiesBase
{
  Q_OBJECT ;

  public:
    OcaDialogProperties3DPlot( Oca3DPlot* plot, bool create=false );
    virtual ~OcaDialogProperties3DPlot();

  protected:
    virtual OcaObject* getObject() const { return m_plot; }

  protected slots:
    virtual void onUpdateRequired( uint flags );

  protected:
    Oca3DPlot*          m_plot;
    bool                m_create;
};

