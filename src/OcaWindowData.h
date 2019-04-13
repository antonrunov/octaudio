/*
   Copyright 2013-2019 Anton Runov

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

#ifndef OcaWindowData_h
#define OcaWindowData_h

#include "OcaObject.h"
#include "OcaList.h"

class OcaMonitor;
class OcaTrackGroup;
class Oca3DPlot;

class OcaWindowData : public OcaObject
{
  Q_OBJECT ;
  Q_PROPERTY( double audio_samplerate READ getAudioSampleRate WRITE setAudioSampleRate );
  Q_PROPERTY( double default_rate READ getDefaultSampleRate WRITE setDefaultSampleRate );
  Q_PROPERTY( OcaTrackGroup* active_group READ getActiveGroup WRITE setActiveGroup );
  Q_PROPERTY( QString output_device READ getOutputDevice WRITE setOutputDevice );
  Q_PROPERTY( QString input_device READ getInputDevice WRITE setInputDevice );
  Q_PROPERTY( QString cache_dir READ getCacheBase WRITE setCacheBase );

  public:
    OcaWindowData();
    ~OcaWindowData();

  public:
    enum EFlags {
      e_FlagGroupAdded          = 0x001,
      e_FlagGroupRemoved        = 0x002,
      e_FlagGroupMoved          = 0x004,
      e_FlagActiveGroupChanged  = 0x008,
      e_FlagMonitorAdded        = 0x010,
      e_FlagMonitorRemoved      = 0x020,
      e_FlagNameChanged         = 0x040,
      e_FlagDefaultRateChanged  = 0x080,
      e_Flag3DPlotAdded         = 0x100,
      e_Flag3DPlotRemoved       = 0x200,

      e_FlagALL                 = 0xfff,
    };

  public:
    OcaTrackGroup*  getActiveGroup() const { return m_activeGroup; }
    uint            getGroupCount() const;
    oca_index       getGroupIndex( OcaTrackGroup* group ) const;
    OcaTrackGroup*  getGroupAt( oca_index idx ) const;

    QList<OcaTrackGroup*> findGroups( const QString& name ) const;

    oca_index       addGroup( OcaTrackGroup* );
    oca_index       removeGroup( OcaTrackGroup* );
    oca_index       moveGroup( OcaTrackGroup* group, oca_index idx );
    bool            setActiveGroup( OcaTrackGroup* group );

    uint            getMonitorCount() const;
    oca_index       getMonitorIndex( OcaObject* id ) const;
    OcaMonitor*     getMonitorAt( oca_index idx ) const;

    QList<OcaMonitor*> findMonitors( const QString& name ) const;

    oca_index       addMonitor( OcaMonitor* );
    oca_index       removeMonitor( OcaMonitor* );

#ifdef OCA_BUILD_3DPLOT
    uint              get3DPlotCount() const;
    oca_index         get3DPlotIndex( OcaObject* id ) const;
    Oca3DPlot*        get3DPlotAt( oca_index idx ) const;
    QList<Oca3DPlot*> find3DPlots( const QString& name ) const;
    oca_index         add3DPlot( Oca3DPlot* );
    oca_index         remove3DPlot( Oca3DPlot* );
#endif

    double  getDefaultSampleRate() const { return m_defaultSampleRate; }
    bool    setDefaultSampleRate( double rate );

  protected:
    double  getAudioSampleRate() const;
    void    setAudioSampleRate( double rate );
    QString getOutputDevice() const;
    QString getInputDevice() const;
    QString getCacheBase() const;
    bool    setOutputDevice( const QString& dev_name );
    bool    setInputDevice( const QString& dev_name );
    bool    setCacheBase( const QString& path );

  protected slots:
    void onMonitorClosed( OcaObject* obj );
    void onGroupClosed( OcaObject* obj );
#ifdef OCA_BUILD_3DPLOT
    void on3DPlotClosed( OcaObject* obj );
#endif

  protected:
    virtual void onClose();

  protected:
    OcaTrackGroup*                m_activeGroup;
    OcaList<OcaTrackGroup,void>   m_groups;
    OcaList<OcaMonitor,void>      m_monitors;
#ifdef OCA_BUILD_3DPLOT
    OcaList<Oca3DPlot,void>       m_3DPlots;
#endif
    double                        m_defaultSampleRate;
};

#endif // OcaWindowData_h
