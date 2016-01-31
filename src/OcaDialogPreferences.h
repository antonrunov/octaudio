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

#ifndef OcaDialogPreferences_h
#define OcaDialogPreferences_h

#include "OcaDialogPropertiesBase.h"
#include "OcaWindowData.h"

class OcaValidatorDouble;
class QLineEdit;
class QComboBox;

class OcaDialogPreferences : public OcaDialogPropertiesBase
{
  Q_OBJECT ;
  public:
    OcaDialogPreferences();
    virtual ~OcaDialogPreferences();

  protected:
    virtual OcaObject* getObject() const { return m_data; }

  protected slots:
    virtual void onUpdateRequired( uint flags );
    void setAudioRate();
    void setDefaultRate();
    void setInputDevice( const QString& dev_name );
    void setOutputDevice( const QString& dev_name );
    void setDataCache();

  public:
    virtual bool eventFilter( QObject* obj, QEvent* ev );

  protected:
    OcaWindowData*      m_data;
    QLineEdit*          m_editSampleRate;
    OcaValidatorDouble* m_sampleRateValidator;
    QComboBox*          m_devOutput;
    QComboBox*          m_devInput;
    QLineEdit*          m_editDefaultRate;
    OcaValidatorDouble* m_defaultRateValidator;
    QLineEdit*          m_editDataCacheBase;
};


#endif // OcaDialogPreferences_h
