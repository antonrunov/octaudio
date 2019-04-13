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

#include "OcaWindowData.h"

#include "OcaMonitor.h"
#include "Oca3DPlot.h"
#include "OcaTrackGroup.h"
#include "OcaApp.h"
#include "OcaInstance.h"
#include "OcaAudioController.h"

#include <QtCore>

// ------------------------------------------------------------------------------------

OcaWindowData::OcaWindowData()
:
  m_activeGroup( NULL ),
  m_defaultSampleRate( 44100 )
{
  m_nameFlag        = e_FlagNameChanged;
  m_displayNameFlag = e_FlagNameChanged;
}

// ------------------------------------------------------------------------------------

OcaWindowData::~OcaWindowData()
{
  fprintf( stderr, "OcaWindowData::~OcaWindowData\n" );
}

// ------------------------------------------------------------------------------------

uint OcaWindowData::getGroupCount() const
{
  OcaLock lock( this );
  return m_groups.getLength();
}

// ------------------------------------------------------------------------------------

oca_index OcaWindowData::getGroupIndex( OcaTrackGroup* group ) const
{
  OcaLock lock( this );
  return m_groups.findItemIndex( group );
}

// ------------------------------------------------------------------------------------

OcaTrackGroup* OcaWindowData::getGroupAt( oca_index idx ) const
{
  OcaLock lock( this );
  return m_groups.getItem( idx );
}

// ------------------------------------------------------------------------------------

QList<OcaTrackGroup*> OcaWindowData::findGroups( const QString& name ) const
{
  OcaLock lock( this );
  return m_groups.findItemsByName( name );
}

// ------------------------------------------------------------------------------------

QList<OcaMonitor*> OcaWindowData::findMonitors( const QString& name ) const
{
  OcaLock lock( this );
  return m_monitors.findItemsByName( name );
}

// ------------------------------------------------------------------------------------

#ifdef OCA_BUILD_3DPLOT
QList<Oca3DPlot*> OcaWindowData::find3DPlots( const QString& name ) const
{
  OcaLock lock( this );
  return m_3DPlots.findItemsByName( name );
}
#endif

// ------------------------------------------------------------------------------------

oca_index OcaWindowData::addGroup( OcaTrackGroup* group )
{
  oca_index idx = -1;
  uint group_flags = 0;
  uint flags = 0;
  {
    WLock lock( this );
    idx = m_groups.appendItem( group );
    if( -1 != idx ) {
      if( ! connectObject( group, SLOT(onGroupClosed(OcaObject*)) ) ) {
        m_groups.removeItem( group );
      }
      else {
        group_flags = e_FlagGroupAdded;
        if( NULL == m_activeGroup ) {
          Q_ASSERT( 0 == idx );
          m_activeGroup = group;
          flags = e_FlagActiveGroupChanged;
        }
      }
    }
  }

  emitChanged( group, group_flags );
  emitChanged( flags );
  return idx;
}

// ------------------------------------------------------------------------------------

oca_index OcaWindowData::removeGroup( OcaTrackGroup* group )
{
  oca_index idx = -1;
  uint group_flags = 0;
  uint flags = 0;
  {
    WLock lock( this );
    idx = m_groups.removeItem( group );
    disconnectObject( group );
    if( -1 != idx ) {
      group_flags = e_FlagGroupRemoved;
    }
    if( m_activeGroup == group ) {
      Q_ASSERT( -1 != idx );
      flags = e_FlagActiveGroupChanged;
      if( 0 < idx ) {
        m_activeGroup = m_groups.getItem( idx - 1 );
      }
      else if( 0 < m_groups.getLength() ) {
        m_activeGroup = m_groups.getItem( 0 );
      }
      else {
        m_activeGroup = NULL;
      }
    }
  }

  emitChanged( group, group_flags );
  emitChanged( flags );
  return idx;
}

// ------------------------------------------------------------------------------------

oca_index OcaWindowData::moveGroup( OcaTrackGroup* group, oca_index idx )
{
  oca_index idx_old = -1;
  uint flags = 0;
  {
    WLock lock( this );
    idx_old = m_groups.moveItem( group, idx );
    if( -1 != idx_old ) {
      flags = e_FlagGroupMoved;
    }
  }

  emitChanged( group, flags );
  return idx_old;
}

// ------------------------------------------------------------------------------------

bool OcaWindowData::setActiveGroup( OcaTrackGroup* group )
{
  bool result = false;
  uint flags = 0;
  {
    WLock lock( this );
    if( -1 != m_groups.findItemIndex( group ) ) {
      result = true;
      if( m_activeGroup != group ) {
        m_activeGroup = group;
        flags = e_FlagActiveGroupChanged;
      }
    }
  }

  emitChanged( flags );
  return result;
}

// ------------------------------------------------------------------------------------

uint OcaWindowData::getMonitorCount() const
{
  OcaLock lock( this );
  return m_monitors.getLength();
}

// ------------------------------------------------------------------------------------

oca_index OcaWindowData::getMonitorIndex( OcaObject* id ) const
{
  OcaLock lock( this );
  return m_monitors.findItemIndex( id );
}

// ------------------------------------------------------------------------------------

OcaMonitor* OcaWindowData::getMonitorAt( oca_index idx ) const
{
  OcaLock lock( this );
  return m_monitors.getItem( idx );
}

// ------------------------------------------------------------------------------------

oca_index OcaWindowData::addMonitor( OcaMonitor* monitor )
{
  oca_index idx = -1;
  uint flags = 0;
  {
    WLock lock( this );
    idx = m_monitors.appendItem( monitor );
    if( -1 != idx ) {
      if( ! connectObject( monitor, SLOT(onMonitorClosed(OcaObject*)) ) ) {
        m_monitors.removeItem( monitor );
      }
      else {
        flags = e_FlagMonitorAdded;
      }
    }
  }

  emitChanged( monitor, flags );
  return idx;
}

// ------------------------------------------------------------------------------------

oca_index OcaWindowData::removeMonitor( OcaMonitor* monitor )
{
  oca_index idx = -1;
  uint flags = 0;
  {
    WLock lock( this );
    idx = m_monitors.removeItem( monitor );
    disconnectObject( monitor );
    if( -1 != idx ) {
      flags = e_FlagMonitorRemoved;
    }
  }

  emitChanged( monitor, flags );
  return idx;
}

// ------------------------------------------------------------------------------------

void OcaWindowData::onMonitorClosed( OcaObject* obj )
{
  if( isClosed() ) {
    return;
  }
  OcaMonitor* d = qobject_cast<OcaMonitor*>( obj );
  Q_ASSERT( NULL != d );
  removeMonitor( d );
}

#ifdef OCA_BUILD_3DPLOT
// ------------------------------------------------------------------------------------

uint OcaWindowData::get3DPlotCount() const
{
  OcaLock lock( this );
  return m_3DPlots.getLength();
}

// ------------------------------------------------------------------------------------

oca_index OcaWindowData::get3DPlotIndex( OcaObject* id ) const
{
  OcaLock lock( this );
  return m_3DPlots.findItemIndex( id );
}

// ------------------------------------------------------------------------------------

Oca3DPlot* OcaWindowData::get3DPlotAt( oca_index idx ) const
{
  OcaLock lock( this );
  return m_3DPlots.getItem( idx );
}

// ------------------------------------------------------------------------------------

oca_index OcaWindowData::add3DPlot( Oca3DPlot* plot )
{
  oca_index idx = -1;
  uint flags = 0;
  {
    WLock lock( this );
    idx = m_3DPlots.appendItem( plot );
    if( -1 != idx ) {
      if( ! connectObject( plot, SLOT(on3DPlotClosed(OcaObject*)) ) ) {
        m_3DPlots.removeItem( plot );
      }
      else {
        flags = e_Flag3DPlotAdded;
      }
    }
  }

  emitChanged( plot, flags );
  return idx;
}

// ------------------------------------------------------------------------------------

oca_index OcaWindowData::remove3DPlot( Oca3DPlot* plot )
{
  oca_index idx = -1;
  uint flags = 0;
  {
    WLock lock( this );
    idx = m_3DPlots.removeItem( plot );
    disconnectObject( plot );
    if( -1 != idx ) {
      flags = e_Flag3DPlotRemoved;
    }
  }

  emitChanged( plot, flags );
  return idx;
}

// ------------------------------------------------------------------------------------

void OcaWindowData::on3DPlotClosed( OcaObject* obj )
{
  if( isClosed() ) {
    return;
  }
  Oca3DPlot* d = qobject_cast<Oca3DPlot*>( obj );
  Q_ASSERT( NULL != d );
  remove3DPlot( d );
}
#endif // OCA_BUILD_3DPLOT

// ------------------------------------------------------------------------------------

void OcaWindowData::onGroupClosed( OcaObject* obj )
{
  if( isClosed() ) {
    return;
  }
  OcaTrackGroup* group = qobject_cast<OcaTrackGroup*>( obj );
  Q_ASSERT( NULL != group );
  removeGroup( group );
}

// ------------------------------------------------------------------------------------

void OcaWindowData::onClose()
{
  QList<OcaObject*> list;
  {
    WLock lock(this);
    for( oca_index idx = m_groups.getLength() - 1; idx >=0; idx-- ) {
      list.append( m_groups.getItem( idx ) );
    }
    m_groups.clear();
    for( oca_index idx = m_monitors.getLength() - 1; idx >=0; idx-- ) {
      list.append( m_monitors.getItem( idx ) );
    }
    m_monitors.clear();

#ifdef OCA_BUILD_3DPLOT
    for( oca_index idx = m_3DPlots.getLength() - 1; idx >=0; idx-- ) {
      list.append( m_3DPlots.getItem( idx ) );
    }
    m_3DPlots.clear();
#endif
    m_activeGroup = NULL;

  }

  for( int i = 0; i < list.size(); i++ ) {
    disconnectObject( list.at(i) );
    list.at(i)->close();
  }
}

// ------------------------------------------------------------------------------------

double OcaWindowData::getAudioSampleRate() const
{
  return OcaApp::getAudioController()->getSampleRate();
}

// ------------------------------------------------------------------------------------

void OcaWindowData::setAudioSampleRate( double rate )
{
  OcaApp::getAudioController()->setSampleRate( rate );
}

// ------------------------------------------------------------------------------------

QString OcaWindowData::getOutputDevice() const
{
  return OcaApp::getAudioController()->getDevice( false );
}

// ------------------------------------------------------------------------------------

QString OcaWindowData::getInputDevice() const
{
  return OcaApp::getAudioController()->getDevice( true );
}

// ------------------------------------------------------------------------------------

bool OcaWindowData::setOutputDevice( const QString& dev_name )
{
  return OcaApp::getAudioController()->setDevice( dev_name, false );
}

// ------------------------------------------------------------------------------------

bool OcaWindowData::setInputDevice( const QString& dev_name )
{
  return OcaApp::getAudioController()->setDevice( dev_name, true );
}

// ------------------------------------------------------------------------------------

bool OcaWindowData::setDefaultSampleRate( double rate )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( ( m_defaultSampleRate != rate ) && ( 0 < rate ) ) {
      m_defaultSampleRate = rate;
      flags = e_FlagDefaultRateChanged;
    }
  }

  return emitChanged( flags );
}

// ------------------------------------------------------------------------------------

QString OcaWindowData::getCacheBase() const
{
  return OcaApp::getOcaInstance()->getDataCacheBase();
}

// ------------------------------------------------------------------------------------

bool OcaWindowData::setCacheBase( const QString& path )
{
  return OcaApp::getOcaInstance()->setDataCacheBase( path );
}

// ------------------------------------------------------------------------------------

