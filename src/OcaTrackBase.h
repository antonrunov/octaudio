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

#ifndef OcaTrackBase_h
#define OcaTrackBase_h

#include "octaudio.h"
#include "OcaScaleData.h"
#include "OcaObject.h"

#include <QObject>
#include <QMetaType>

class OcaDataScreen;
class OcaTrack;

class OcaTrackBase : public OcaObject
{
  Q_OBJECT ;
  Q_PROPERTY( bool selected READ isSelected  WRITE setSelected );
  Q_PROPERTY( bool hidden READ isHidden WRITE setHidden );
  Q_PROPERTY( double start READ getStartTime );
  Q_PROPERTY( double end READ getEndTime );
  Q_PROPERTY( double duration READ getDuration );
  Q_PROPERTY( double scale READ getScale WRITE setScale );
  Q_PROPERTY( double zero READ getZero WRITE setZero );
  Q_PROPERTY( bool abs_value READ getAbsValueMode WRITE setAbsValueMode );

  public:
    OcaTrackBase( const QString& name );
    virtual ~OcaTrackBase();

  public:
    enum EFlags {
      // Base
      e_FlagNameChanged               = 0x00000001,
      e_FlagHiddenChanged             = 0x00000002,
      e_FlagSelectedChanged           = 0x00000004,
      e_FlagAbsValueModeChanged       = 0x00000008,
      e_FlagScaleChanged              = 0x00000010,
      e_FlagZeroChanged               = 0x00000020,
      e_FlagDurationChanged           = 0x00000040,
      e_FlagHeightChanged             = 0x00000080,
      // Simple
      e_FlagTrackDataChanged          = 0x00000100,
      e_FlagMuteChanged               = 0x00000200,
      e_FlagGainChanged               = 0x00000400,
      e_FlagAudibleChanged            = 0x00000800,
      e_FlagStereoPanChanged          = 0x00001000,
      e_FlagReadonlyChanged           = 0x00002000,
      e_FlagSampleRateChanged         = 0x00004000,
      e_FlagChannelsChanged           = 0x00008000,
      // Smart
      e_FlagSubtrackAdded             = 0x00010000,
      e_FlagSubtrackRemoved           = 0x00020000,
      e_FlagSubtrackMoved             = 0x00040000,
      e_FlagSubtrackChanged           = 0x00080000,
      e_FlagSubtrackColorsChanged     = 0x00100000,
      e_FlagActiveSubtrackChanged     = 0x00200000,
      e_FlagControlModeChanged        = 0x00400000,
      e_FlagAutoscaleChanged          = 0x00800000,
      // Monitor
      e_FlagSizeChanged               = 0x01000000,
      e_FlagPlotDataChanged           = 0x02000000,
      e_FlagYScaleChanged             = 0x04000000,
      e_FlagCursorChanged             = 0x08000000,
      e_FlagGroupChanged              = 0x10000000,

      e_FlagALL                       = 0xffffffff,
    };

  public:
    virtual void setHidden( bool hidden );
    bool isHidden() const;
    bool isSelected() const { return m_selected; }

    virtual const OcaTrack* getCurrentTrack() const = 0;
    virtual bool isAudible() const = 0;
    virtual double  getZero() const = 0;
    virtual double  getScale() const = 0;
    bool    getAbsValueMode() const  { return m_absValueMode; }

    virtual double getStartTime() const = 0;
    virtual double getEndTime() const = 0;
    virtual double getDuration() const = 0;

  public slots:
    virtual void setSelected( bool selected );

    virtual void setScale( double scale ) = 0;
    virtual void setZero( double zero ) = 0;
    virtual void moveScale( double factor ) = 0;
    virtual void moveZero( double relative_step ) = 0;
    void setAbsValueMode( bool on );

  protected:
    bool                m_selected;
    bool                m_absValueMode;
    bool                m_hidden;
    OcaScaleData        m_scaleData;
};

#endif // OcaTrackBase_h
