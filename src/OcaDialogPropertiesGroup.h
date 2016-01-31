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

#ifndef OcaDialogPropertiesGroup_h
#define OcaDialogPropertiesGroup_h

#include "OcaDialogPropertiesBase.h"
#include "OcaTrackGroup.h"

class OcaWindowData;
class OcaValidatorDouble;

class OcaDialogPropertiesGroup : public OcaDialogPropertiesBase
{
  Q_OBJECT ;

  public:
    OcaDialogPropertiesGroup( OcaTrackGroup* group, OcaWindowData* window = NULL );
    virtual ~OcaDialogPropertiesGroup();

  protected:
    virtual OcaObject* getObject() const { return m_group; }

  protected slots:
    virtual void onUpdateRequired( uint flags );
    void onSampleRateChanged();

  protected:
    OcaTrackGroup*      m_group;
    OcaWindowData*      m_window;
    QLineEdit*          m_editSampleRate;
    OcaValidatorDouble* m_sampleRateValidator;
};

#endif // OcaDialogPropertiesGroup_h
