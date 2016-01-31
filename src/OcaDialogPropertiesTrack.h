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

#ifndef OcaDialogPropertiesTrack_h
#define OcaDialogPropertiesTrack_h

#include "OcaDialogPropertiesBase.h"
#include "OcaTrack.h"

class OcaTrackGroup;
class OcaValidatorDouble;
class QCheckBox;
class QDoubleSpinBox;
class QComboBox;

class OcaDialogPropertiesTrack : public OcaDialogPropertiesBase
{
  Q_OBJECT ;

  public:
    OcaDialogPropertiesTrack( OcaTrack* track, OcaTrackGroup* group = NULL );
    virtual ~OcaDialogPropertiesTrack();

  protected:
    virtual OcaObject* getObject() const { return m_track; }

  protected slots:
    virtual void onUpdateRequired( uint flags );
    void onSampleRateChanged();
    void onGainChanged( double value );
    void onPanChanged( double value );
    void onChannelsChanged( const QString& value);

  protected:
    OcaTrack*     m_track;
    OcaTrackGroup*      m_group;
    QLineEdit*          m_editSampleRate;
    OcaValidatorDouble* m_sampleRateValidator;
    QCheckBox*          m_chkReadonly;
    QCheckBox*          m_chkAbsValueMode;
    QCheckBox*          m_chkAudible;
    QDoubleSpinBox*     m_regGain;
    QDoubleSpinBox*     m_regStereoPan;
    QComboBox*          m_cmbChannels;
    // TODO
    // stereo
};

#endif // OcaDialogPropertiesTrack_h

