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

#ifndef OcaDialogPropertiesSmartTrack_h
#define OcaDialogPropertiesSmartTrack_h

#include "OcaDialogPropertiesBase.h"
#include "OcaSmartTrack.h"

class OcaTrackGroup;
class OcaValidatorDouble;
class QCheckBox;
class QDoubleSpinBox;
class QListWidget;
class QListWidgetItem;
class OcaScaleControl;

class OcaDialogPropertiesSmartTrack : public OcaDialogPropertiesBase
{
  Q_OBJECT ;

  public:
    OcaDialogPropertiesSmartTrack( OcaSmartTrack* track, OcaTrackGroup* group = NULL );
    virtual ~OcaDialogPropertiesSmartTrack();

  protected:
    virtual OcaObject* getObject() const { return m_track; }

  protected slots:
    virtual void onUpdateRequired( uint flags );
    void openContextMenu( const QPoint& pos );
    void setActiveSubtrack();
    void setSubtrackColor();
    void removeSubtrack();
    void updateTrackList();
    void updateScales();
    void setScale( double scale );
    void setZero( double zero );
    void moveScale( double step );
    void moveZero( double step );
    void onItemChanged( QListWidgetItem* item );

  protected:
    OcaTrack* getCurrentSubtrack() const;
    OcaTrack* getSubtrack( QListWidgetItem* item ) const;

  protected:
    OcaSmartTrack*      m_track;
    OcaTrackGroup*      m_group;
    QCheckBox*          m_chkAbsValueMode;
    QCheckBox*          m_chkCommonScale;
    QCheckBox*          m_chkEditTrackList;
    QListWidget*        m_listSubtracks;
    QDoubleSpinBox*     m_regTransparency;
    OcaScaleControl*    m_scale;
    OcaScaleControl*    m_zero;
};

#endif // OcaDialogPropertiesSmartTrack_h


