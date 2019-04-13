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

#include "OcaMainWindow.h"

#include "OcaConsole.h"
#include "OcaTrackGroupView.h"
#include "OcaTrack.h"
#include "OcaMonitor.h"
#include "OcaOctaveController.h"
#include "OcaApp.h"
#include "OcaWindowData.h"
#include "OcaObjectListener.h"
#include "OcaTrackGroup.h"
#include "OcaMonitorDock.h"
#include "OcaDialogPropertiesGroup.h"
#include "OcaDialogPropertiesTrack.h"
#include "OcaDialogPropertiesSmartTrack.h"
#include "OcaDialogPreferences.h"
#include "OcaDialogAbout.h"
#include "OcaDialogProperties3DPlot.h"
#include "Oca3DPlot.h"
#include "Oca3DPlotDock.h"

#include "images/toolbar_icons.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------

OcaMainWindow::OcaMainWindow( OcaWindowData* data )
:
  m_activeTrack( NULL ),
  m_activeGroup( NULL ),
  m_data( data )
{
  m_listener = new OcaObjectListener( m_data, OcaWindowData::e_FlagALL, 10, this );
  connect(  m_listener,
            SIGNAL(updateRequired(uint,QHash<QString,uint>&,QList<OcaObject*>&)),
            SLOT(onUpdateRequired(uint,QHash<QString,uint>&,QList<OcaObject*>&))  );

  m_console = new OcaConsole();

  const uint audio_mask =   OcaAudioController::e_FlagAudioModeChanged
                          | OcaAudioController::e_FlagStateChanged;
  OcaAudioController* controller = OcaApp::getAudioController();
  m_listener->addObject( controller, audio_mask );
  m_listener->addEvent( controller, audio_mask );

  QDockWidget* dock = new QDockWidget( "Console", this );
  dock->setWindowFlags( Qt::Widget );
  dock->setFeatures(
      QDockWidget::DockWidgetFloatable
      // | QDockWidget::DockWidgetVerticalTitleBar
      | QDockWidget::DockWidgetClosable
      | QDockWidget::DockWidgetMovable
      );
  dock->setWidget( m_console );
  addDockWidget( Qt::BottomDockWidgetArea, dock );
  m_dockConsole = dock;
  connect( m_console, SIGNAL(commandEntered(const QString&)),
                      SLOT(runCommand(const QString&)) );
  m_dockConsole->installEventFilter( this );

  createMenus();
  createToolbars();

  QWidget* w = new QWidget( this );
  setCentralWidget( w );
  m_tabs = new QTabBar();
  m_tabs->setTabsClosable ( false );
  m_tabs->setDocumentMode( true );
  m_tabs->setExpanding( false );
  m_tabs->setFocusPolicy( Qt::NoFocus );
  m_tabs->setDrawBase( false );
  connect( m_tabs, SIGNAL(currentChanged(int)), SLOT(setCurrentGroup(int)) );
  connect( m_tabs, SIGNAL(tabCloseRequested(int)), SLOT(deleteGroup(int)) );
  m_tabs->installEventFilter( this );
  m_tabs->setAcceptDrops(true);
  m_tabs->setContextMenuPolicy( Qt::CustomContextMenu );
  connect( m_tabs, SIGNAL(customContextMenuRequested(const QPoint&)),
                   SLOT(openGroupContextMenu(const QPoint&)) );
  m_stack = new QStackedLayout();

  QPushButton* new_grp = new QPushButton( "+" );
  connect( new_grp, SIGNAL(clicked()), SLOT(openNewGroupDlg()) );
  //new_grp->setFlat( true );
  new_grp->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
  new_grp->setFocusPolicy( Qt::NoFocus );

  QVBoxLayout *layout = new QVBoxLayout;
  QHBoxLayout *layout_h = new QHBoxLayout;
  layout_h->addWidget( m_tabs );
  layout_h->addWidget( new_grp );
  layout->addLayout( layout_h );
  layout->addLayout( m_stack );
  w->setLayout( layout );

  if( 0 == m_data->getGroupCount() ) {
    addGroupDefault();
  }
  else {
    m_listener->addEvent( NULL, OcaWindowData::e_FlagALL );
  }

  m_status = new QLabel();
  m_status->setTextFormat( Qt::PlainText );
  statusBar()->addWidget( m_status );
  m_statusRight = new QLabel();
  m_statusRight->setTextFormat( Qt::PlainText );
  statusBar()->addPermanentWidget( m_statusRight );

  //m_console->setFocus();
  resize( 1200, 800 );
}

// -----------------------------------------------------------------------------

OcaMainWindow::~OcaMainWindow()
{
  Q_ASSERT( m_listener->isClosed() );
}

// -----------------------------------------------------------------------------

void OcaMainWindow::createMenus()
{
  m_fileMenu              = menuBar()->addMenu( tr("File") );

  addWindowAction( tr("New Group"), tr("Ctrl+T"), m_fileMenu, SLOT(addGroupDefault()) );
  addWindowAction( tr("New Group..."), tr("Ctrl+Shift+T"), m_fileMenu, SLOT(openNewGroupDlg()) );
  addWindowAction( tr("Delete Group"), tr("Ctrl+W"), m_fileMenu, SLOT(deleteGroup()) );
  addDisabledAction( tr("Export..."), 0, m_fileMenu, SLOT(openExportDlg()) );
  addDisabledAction( tr("Import..."), 0, m_fileMenu, SLOT(openImportDlg()) );
  addWindowAction( tr("Group Properties..."), tr("Ctrl+G"), m_fileMenu, SLOT(openProperties()) );
  //addWindowAction( tr("NewGroupCurrent"), 0, m_fileMenu, SLOT(addGroupWithCurrentTrack()) );
  //addWindowAction( tr("NewGroupSelected"), 0, m_fileMenu, SLOT(addGroupWithSelectedTracks()) );
  addWindowAction( tr("Next Group"), tr("Ctrl+PgDown"), m_fileMenu, SLOT(activateNextGroup()) );
  addWindowAction( tr("Prev Group"), tr("Ctrl+PgUp"), m_fileMenu, SLOT(activatePrevGroup()) );

  m_editMenu              = menuBar()->addMenu( tr("Edit") );

  addDisabledAction( tr("Copy"), 0, m_editMenu, SLOT(copy()) );
  addDisabledAction( tr("Paste"), 0, m_editMenu, SLOT(paste()) );
  addDisabledAction( tr("Cut"), 0, m_editMenu, SLOT(cut()) );
  addDisabledAction( tr("Join"), 0, m_editMenu, SLOT(joinBlocks()) );
  addDisabledAction( tr("Split"), 0, m_editMenu, SLOT(splitBlock()) );
  addDisabledAction( tr("Zero"), 0, m_editMenu, SLOT(zeroRegion()) );
  addDisabledAction( tr("Undo"), 0, m_editMenu, SLOT(undo()) );
  addDisabledAction( tr("Redo"), 0, m_editMenu, SLOT(redo()) );
  addWindowAction( tr("Preferences"), 0, m_editMenu, SLOT(openPreferences()) );

  m_editRegionMenu       = m_editMenu->addMenu( tr("Region") );

  addGroupAction( tr("Clear"), tr("Ctrl+Shift+A"), m_editRegionMenu, SLOT(clearRegion()) );
  addGroupAction( tr("Set from Group"), 0, m_editRegionMenu, SLOT(selectGroup()) );
  addGroupAction( tr("Set from Track"), 0, m_editRegionMenu, SLOT(selectTrack()) );
  addGroupAction( tr("Set from View"), 0, m_editRegionMenu, SLOT(selectView()) );
  addGroupAction( tr("Set Start"), 0, m_editRegionMenu, SLOT(setRegionStartOnCursor()) );
  addGroupAction( tr("Set End"), 0, m_editRegionMenu, SLOT(setRegionEndOnCursor()) );

  m_editConsoleMenu       = m_editMenu->addMenu( tr("Console") );

  addConsoleAction( tr("Scroll to Bottom"), 0, m_editConsoleMenu, SLOT(scrollToBottom()) );
  addWindowAction( tr("Cancel"), 0, m_editConsoleMenu, SLOT(cancelCommand()) );
  addDisabledAction( tr("Clear"), 0, m_editConsoleMenu, SLOT(clearCommand()) );
  addDisabledAction( tr("Eval"), 0, m_editConsoleMenu, SLOT(evalCommand()) );
  addDisabledAction( tr("Find"), 0, m_editConsoleMenu, SLOT(findInLog()) );
  addDisabledAction( tr("History"), 0, m_editConsoleMenu, SLOT(showCommandHistory()) );
  addDisabledAction( tr("Next Command"), 0, m_editConsoleMenu, SLOT(nextCommand()) );
  addDisabledAction( tr("Prev Command"), 0, m_editConsoleMenu, SLOT(prevCommand()) );
  addWindowAction( tr("Hide"), tr("Ctrl+`"), m_editConsoleMenu, SLOT(hideConsole()) );
  addWindowAction( tr("Show"), tr("Ctrl+1"), m_editConsoleMenu, SLOT(showConsole()) );

  m_viewMenu              = menuBar()->addMenu( tr("View") );

  addGroupAction( tr("Group End"), 0, m_viewMenu, SLOT(showGroupEnd()) );
  addGroupAction( tr("Group Start"), 0, m_viewMenu, SLOT(showGroupStart()) );
  addGroupAction( tr("Set Cursor on Base"), 0, m_viewMenu, SLOT(setCursorOnBasePosition()) );
  addGroupAction( tr("Set Cursor on Center"), 0, m_viewMenu, SLOT(setCursorOnCenter()) );
  addGroupAction( tr("Track End"), 0, m_viewMenu, SLOT(showTrackEnd()) );
  addGroupAction( tr("Track Start"), 0, m_viewMenu, SLOT(showTrackStart()) );
  addGroupAction( tr("Whole Group"), 0, m_viewMenu, SLOT(showWholeGroup()) );
  addGroupAction( tr("Whole Region"), 0, m_viewMenu, SLOT(showWholeRegion()) );
  addGroupAction( tr("Whole Track"), 0, m_viewMenu, SLOT(showWholeTrack()) );
  addDisabledAction( tr("Set Status Bar Function"), 0, m_viewMenu, SLOT(openStatusBarFunctionDlg()) );
  addWindowAction( tr("Focus Tracks"), tr("Ctrl+2"), m_viewMenu, SLOT(focusCurrentGroup()) );


  m_viewCenterMenu        = m_viewMenu->addMenu( tr("Cener") );

  addGroupAction( tr("Audio Position"), tr("Alt+A"), m_viewCenterMenu, SLOT(centerAudioPos()) );
  addGroupAction( tr("Base Position"), 0, m_viewCenterMenu, SLOT(centerBasePos()) );
  addGroupAction( tr("Cursor"), 0, m_viewCenterMenu, SLOT(centerCursor()) );
  addGroupAction( tr("Region"), 0, m_viewCenterMenu, SLOT(centerRegion()) );
  addGroupAction( tr("Region Start"), 0, m_viewCenterMenu, SLOT(centerRegionStart()) );
  addGroupAction( tr("Region End"), 0, m_viewCenterMenu, SLOT(centerRegionEnd()) );

  m_viewMonitorMenu       = m_viewMenu->addMenu( tr("Monitor") );

  addDisabledAction( tr("Clear"), 0, m_viewMonitorMenu, SLOT(clearMonitor()) );
  addWindowAction( tr("New"), 0, m_viewMonitorMenu, SLOT(addMonitorDefault()) );
  addWindowAction( tr("New..."), tr("Alt+Shift+M"), m_viewMonitorMenu, SLOT(openNewMonitorDlg()) );
  addWindowAction( tr("New with Active Track"), 0, m_viewMonitorMenu,
                                                          SLOT(addMonitorWithCurrentTrack()) );
  addWindowAction( tr("New with Selected Tracks"), 0, m_viewMonitorMenu,
                                                        SLOT(addMonitorWithSelectedTracks()) );
  addDisabledAction( tr("Properties"), 0, m_viewMonitorMenu, SLOT(openMonitorProperties()) );

#ifdef OCA_BUILD_3DPLOT
  addWindowAction( tr("New 3DPlot..."), 0, m_viewMenu, SLOT(openNew3DPlotDlg()) );
#endif

  m_trackMenu             = menuBar()->addMenu( tr("Track") );
  m_trackMenuNew          = m_trackMenu->addMenu( tr("New") );

  addWindowAction( tr("Track"), tr("Ctrl+N"), m_trackMenuNew, SLOT(newTrackDefault()) );
  addWindowAction( tr("Track..."), tr("Ctrl+Shift+N"), m_trackMenuNew, SLOT(openNewTrackDlg()) );
  addWindowAction( tr("Smart Track"), 0, m_trackMenuNew, SLOT(newSTrackDefault()) );
  addWindowAction( tr("Smart Track..."), tr("Alt+Shift+S"), m_trackMenuNew,
                                                                      SLOT(openNewSTrackDlg()) );
  addWindowAction( tr("Smart Track with Active Track"), 0, m_trackMenuNew,
                                                              SLOT(newSTrackWithCurrentTrack()) );
  addWindowAction( tr("Smart Track with Selected Tracks"), 0, m_trackMenuNew,
                                                            SLOT(newSTrackWithSelectedTracks()) );

  addGroupAction( tr("Delete"), tr("Ctrl+K"), m_trackMenu, SLOT(deleteCurrentTrack()) );
  addGroupAction( tr("Delete Selected Tracks"), tr("Ctrl+Shift+K"), m_trackMenu,
                                                                  SLOT(deleteSelectedTracks()) );
  addGroupAction( tr("Hide"), tr("Alt+Shift+H"), m_trackMenu, SLOT(hideCurrentTrack()) );
  addGroupAction( tr("Hide Selected Tracks"), tr("Ctrl+Shift+H"), m_trackMenu,
                                                                    SLOT(hideSelectedTracks()) );
  addDisabledAction( tr("Mix"), 0, m_trackMenu, SLOT(openTrackMixDlg()) );
  addDisabledAction( tr("Resample"), 0, m_trackMenu, SLOT(openTrackResampleDlg()) );
  addGroupAction( tr("Toggle Mute"), 0, m_trackMenu, SLOT(toggleTrackMute()) );
  //addGroupAction( tr("Rename"), 0, m_trackMenu, SLOT(renameTrack()) );
  addGroupAction( tr("Toggle Selection"), tr("Ctrl+I"), m_trackMenu, SLOT(toggleTrackSelected()) );
  addGroupAction( tr("Toggle Solo"), 0, m_trackMenu, SLOT(setTrackSolo()) );
  addWindowAction( tr("Properties"), tr("Ctrl+P"), m_trackMenu, SLOT(openTrackProperties()) );

  m_trackMenuShow         = m_trackMenu->addMenu( tr("Show") );

  m_audioMenu             = menuBar()->addMenu( tr("Audio") );
  m_audioStartMenu        = m_audioMenu->addMenu( tr("Start") );

  addWindowAction( tr("Playback"), tr("Ctrl+Shift+P"), m_audioStartMenu,
                                                                  SLOT(startAudioPlayback()) );
  addWindowAction( tr("Duplex"), 0, m_audioStartMenu, SLOT(startAudioDuplex()) );
  addWindowAction( tr("Recording"), tr("Ctrl+Shift+R"), m_audioStartMenu,
                                                                  SLOT(startAudioRecording()) );
  addDisabledAction( tr("Playback Looped"), 0, m_audioStartMenu, SLOT(startAudioLooped()) );

  m_audioPauseMenu        = m_audioMenu->addMenu( tr("Pause") );

  addWindowAction( tr("All"), tr("Ctrl+Shift+Space"), m_audioPauseMenu, SLOT(pauseAudioAll()) );
  addDisabledAction( tr("Playback"), 0, m_audioPauseMenu, SLOT(pauseAudioPlayback()) );
  addDisabledAction( tr("Recording"), 0, m_audioPauseMenu, SLOT(pauseAudioRecording()) );

  m_audioStopMenu         = m_audioMenu->addMenu( tr("Stop") );

  addWindowAction( tr("All"), tr("Ctrl+Shift+S"), m_audioStopMenu, SLOT(stopAudioAll()) );
  addWindowAction( tr("Playback"), 0, m_audioStopMenu, SLOT(stopAudioPlayback()) );
  addWindowAction( tr("Recording"), 0, m_audioStopMenu, SLOT(stopAudioRecording()) );

  m_audioStartModeMenu    = m_audioMenu->addMenu( tr("Start Mode") );

  addAudioAction( tr("Auto"), 0, m_audioStartModeMenu, SLOT(setAudioStartModeAuto()) );
  addAudioAction( tr("Cursor"), 0, m_audioStartModeMenu, SLOT(setAudioStartModeCursor()) );
  addAudioAction( tr("Start"), 0, m_audioStartModeMenu, SLOT(setAudioStartModeStart()) );

  m_audioStopModeMenu     = m_audioMenu->addMenu( tr("Stop Mode") );

  addAudioAction( tr("Auto"), 0, m_audioStopModeMenu, SLOT(setAudioStopModeAuto()) );
  addAudioAction( tr("Duplex"), 0, m_audioStopModeMenu, SLOT(setAudioStopModeDuplex()) );

  m_helpMenu              = menuBar()->addMenu( tr("Help") );

  addWindowAction( tr("About"), 0, m_helpMenu, SLOT(openAboutDlg()) );
}

// -----------------------------------------------------------------------------

void OcaMainWindow::createToolbars()
{
  QToolBar* audioToolbar = addToolBar(tr("Audio"));
  QFontMetrics fm( audioToolbar->font() );
  int n = fm.lineSpacing();
  audioToolbar->setIconSize( QSize(n,n) );

  m_btnPlayback = new QToolButton( this );
  {
    QIcon icon( (QPixmap)playback_off_xpm );
    icon.addPixmap( QPixmap(playback_on_xpm), QIcon::Disabled, QIcon::Off );
    m_btnPlayback->setIcon( icon );
  }
  m_btnPlayback->setToolTip( tr("Play") );
  connect( m_btnPlayback, SIGNAL(clicked()), SLOT(startAudioPlayback()) );
  audioToolbar->addWidget( m_btnPlayback );

  m_btnPause = new QToolButton( this );
  m_btnPause->setIcon( QIcon(QPixmap(pause_xpm)) );
  m_btnPause->setCheckable( true );
  m_btnPause->setToolTip( tr("Pause") );
  connect( m_btnPause, SIGNAL(clicked()), SLOT(pauseAudioAll()) );
  audioToolbar->addWidget( m_btnPause );

  m_btnStop = new QToolButton( this );
  m_btnStop->setIcon( QIcon(QPixmap(stop_xpm)) );
  m_btnStop->setToolTip( tr("Stop") );
  connect( m_btnStop, SIGNAL(clicked()), SLOT(stopAudioAll()) );
  audioToolbar->addWidget( m_btnStop );

  m_btnRecording = new QToolButton( this );
  {
    QIcon icon( (QPixmap)recording_off_xpm );
    icon.addPixmap( QPixmap(recording_on_xpm), QIcon::Disabled, QIcon::Off );
    m_btnRecording->setIcon( icon );
  }
  m_btnRecording->setToolTip( tr("Record") );
  connect( m_btnRecording, SIGNAL(clicked()), SLOT(startAudioRecording()) );
  audioToolbar->addWidget( m_btnRecording );

  m_btnDuplex = new QToolButton( this );
  m_btnDuplex->setIcon( QIcon(QPixmap(duplex_xpm)) );
  m_btnDuplex->setToolTip( tr("Duplex") );
  connect( m_btnDuplex, SIGNAL(clicked()), SLOT(startAudioDuplex()) );
  audioToolbar->addWidget( m_btnDuplex );
}

// -----------------------------------------------------------------------------

QAction* OcaMainWindow::createAction( const QString& name,
                                      const QKeySequence& shortcutKey,
                                                          QMenu* menu   )
{
  QAction* action = new QAction( name, this );
  if( ! shortcutKey.isEmpty() ) {
    action->setShortcut( shortcutKey );
  }
  addAction( action );
  if( NULL != menu ) {
    menu->addAction( action );
  }
  return action;
}

// -----------------------------------------------------------------------------

void OcaMainWindow::addDisabledAction(  const QString& name,
                                        const QKeySequence& shortcutKey,
                                        QMenu* menu,
                                        const char* slot  )
{
  QAction* action = createAction( name, shortcutKey, menu );
  action->setEnabled( false );
}

// -----------------------------------------------------------------------------

void OcaMainWindow::addWindowAction(  const QString& name,
                                      const QKeySequence & shortcutKey,
                                      QMenu* menu,
                                      const char* slot  )
{
  QAction* action = createAction( name, shortcutKey, menu );
  connect( action, SIGNAL(triggered()), slot );
}

// -----------------------------------------------------------------------------

void OcaMainWindow::addGroupAction( const QString& name,
                                    const QKeySequence & shortcutKey,
                                    QMenu* menu,
                                    const char* slot )
{
  m_groupActions.insert( slot, createAction( name, shortcutKey, menu ) );
}

// -----------------------------------------------------------------------------

void OcaMainWindow::addConsoleAction( const QString& name,
                                      const QKeySequence & shortcutKey,
                                      QMenu* menu,
                                      const char* slot )
{
  m_console->connect( createAction( name, shortcutKey, menu ), SIGNAL(triggered()), slot );
}

// -----------------------------------------------------------------------------

void OcaMainWindow::addAudioAction( const QString& name,
                                    const QKeySequence & shortcutKey,
                                    QMenu* menu,
                                    const char* slot  )
{
  QAction* action = createAction( name, shortcutKey, menu );
  OcaApp::getAudioController()->connect( action, SIGNAL(triggered()), slot );
  m_audioActions.insert( slot, action );
}

// -----------------------------------------------------------------------------

void OcaMainWindow::connectGroupView( OcaTrackGroupView* view )
{
  QHashIterator<QByteArray, QAction*> it( m_groupActions );
  while( it.hasNext() ) {
    it.next();
    it.value()->disconnect();
    if( NULL != view ) {
      view->connect( it.value(), SIGNAL(triggered()), it.key().data() );
    }
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::openProperties()
{
  OcaTrackGroup* group = m_data->getActiveGroup();
  if( NULL != group ) {
    OcaDialogPropertiesGroup* dlg = new OcaDialogPropertiesGroup( group );
    dlg->show();
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::openNewGroupDlg()
{
  OcaTrackGroup* group = new OcaTrackGroup( NULL, m_data->getDefaultSampleRate() );
  OcaDialogPropertiesGroup* dlg = new OcaDialogPropertiesGroup( group, m_data );
  dlg->show();
}

// -----------------------------------------------------------------------------

void OcaMainWindow::activateNextGroup()
{
  if( NULL != m_activeGroup ) {
    oca_index idx = m_data->getGroupIndex( m_activeGroup ) + 1;
    OcaTrackGroup* group = m_data->getGroupAt( idx );
    if( NULL != group ) {
      m_data->setActiveGroup( group );
    }
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::activatePrevGroup()
{
  if( NULL != m_activeGroup ) {
    oca_index idx = m_data->getGroupIndex( m_activeGroup ) - 1;
    OcaTrackGroup* group = m_data->getGroupAt( idx );
    if( NULL != group ) {
      m_data->setActiveGroup( group );
    }
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::focusCurrentGroup()
{
  if( m_dockConsole->isFloating() ) {
    m_dockConsole->hide();
  }
  OcaTrackGroupView* w = m_views.findItemData( m_activeGroup );
  if( NULL != w ) {
    w->setFocus();
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::runCommand( const QString& command )
{
  m_console->scrollToBottom();
  OcaApp::getOctaveController()->runCommand( command, m_activeGroup );
}

// -----------------------------------------------------------------------------

void OcaMainWindow::onUpdateRequired( uint flags, QHash<QString,uint>& cum_flags,
                                                  QList<OcaObject*>& objects      )
{
  Q_ASSERT( ! m_listener->isClosed() );
  //fprintf( stderr, "OcaMainWindow::onUpdateRequired - flags = %d\n", flags );

  const uint monitor_flags =    OcaWindowData::e_FlagMonitorRemoved
                              | OcaWindowData::e_FlagMonitorAdded;

  const uint group_list_flags =   OcaWindowData::e_FlagGroupRemoved
                                | OcaWindowData::e_FlagGroupAdded
                                | OcaWindowData::e_FlagGroupMoved;

  const uint group_tabs_flags =   group_list_flags
                                | OcaWindowData::e_FlagActiveGroupChanged;

  if( monitor_flags & flags ) {
    OcaLock lock( m_data );
    // remove monitor docks
    QMutableHashIterator<OcaMonitor*,QDockWidget*> it(m_monitorDocks);
    while (it.hasNext()) {
      it.next();
      if( 0 > m_data->getMonitorIndex( it.key() ) ) {
        delete it.value();
        it.remove();
      }
    }
    // add monitor docks
    for( uint idx = 0; idx < m_data->getMonitorCount(); idx++ ) {
      OcaMonitor*d = m_data->getMonitorAt( idx );
      Q_ASSERT( NULL != d );
      if( ! m_monitorDocks.contains( d ) ) {
        addMonitorDock( d );
      }
    }
  }

#ifdef OCA_BUILD_3DPLOT
  const uint plot_flags =    OcaWindowData::e_Flag3DPlotRemoved
                              | OcaWindowData::e_Flag3DPlotAdded;

  if( plot_flags & flags ) {
    OcaLock lock( m_data );
    // remove 3dplot docks
    QMutableHashIterator<Oca3DPlot*,QDockWidget*> it(m_3DPlotDocks);
    while (it.hasNext()) {
      it.next();
      if( 0 > m_data->get3DPlotIndex( it.key() ) ) {
        delete it.value();
        it.remove();
      }
    }
    // add 3dplot docks
    for( uint idx = 0; idx < m_data->get3DPlotCount(); idx++ ) {
      Oca3DPlot* d = m_data->get3DPlotAt( idx );
      Q_ASSERT( NULL != d );
      if( ! m_3DPlotDocks.contains( d ) ) {
        add3DPlotDock( d );
      }
    }
  }
#endif

  bool update_tabs = false;
  bool update_current_group = false;
  bool update_title = ( OcaWindowData::e_FlagNameChanged & flags );
  bool update_status = false;
  bool update_active_track = false;


  if( group_tabs_flags & flags ) {
    OcaLock lock( m_data );
    if( group_list_flags & flags ) {
      for( int i = m_views.getLength() - 1; i >= 0; i-- ) {
        OcaTrackGroupView* w = NULL;
        OcaTrackGroup* group = m_views.getItem( i, &w );
        if( -1 == m_data->getGroupIndex( group ) ) {
          m_listener->removeObject( group );
          m_views.removeItem( group );
          Q_ASSERT( NULL != w );
          delete w;
          w = NULL;
        }
      }
      for( uint idx = 0; idx < m_data->getGroupCount(); idx++ ) {
        OcaTrackGroup* group = m_data->getGroupAt( idx );
        Q_ASSERT( NULL != group );
        oca_index idx_group = m_views.findItemIndex( group );
        if( -1 == idx_group ) {
          bool tmp = m_listener->addObject( group, OcaTrackGroup::e_FlagALL );
          Q_ASSERT( tmp );
          (void) tmp;
          OcaTrackGroupView* w = new OcaTrackGroupView( group );
          idx_group = m_views.appendItem( group, w );
          m_stack->addWidget( w );
        }
        if( idx_group != idx ) {
          m_views.moveItem( group, idx );
        }
        Q_ASSERT( m_views.getItem( idx ) == group );
      }
    }
    if(  m_data->getActiveGroup() != m_activeGroup ) {
      OcaTrackGroupView* w = m_views.findItemData( m_activeGroup );
      if( NULL != w ) {
      }
      m_activeGroup = m_data->getActiveGroup();
      if( NULL != m_activeGroup ) {
        m_stack->setCurrentWidget( m_views.findItemData( m_activeGroup ) );
      }
      update_current_group = true;
      update_title = true;
      update_status = true;
      update_active_track = true;
    }
    update_tabs = true;
  }

  uint group_flags = cum_flags.value( "OcaTrackGroup" );
  if( group_flags ) {
    if( OcaTrackGroup::e_FlagNameChanged & group_flags ) {
      update_tabs = true;
    }
    if( NULL != m_activeGroup ) {
      uint cur_flags = m_listener->getFlags( m_activeGroup );
      if( OcaTrackGroup::e_FlagNameChanged & cur_flags ) {
        update_title = true;
      }
      if( OcaTrackGroup::e_FlagHiddenTracksChanged & cur_flags ) {
        updateHiddenMenu();
      }
      if( OcaTrackGroup::e_FlagActiveTrackChanged & cur_flags ) {
        update_active_track = true;
      }
      if( cur_flags ) {
        update_status = true;
      }
    }
  }

  if( update_tabs ) {
    m_tabs->blockSignals( true );
    for( int i = m_tabs->count()-1; i >=0; i-- ) {
      m_tabs->removeTab(i);
    }
    for( uint i = 0; i < m_views.getLength(); i++ ) {
      OcaTrackGroup* group = m_views.getItem(i);
      m_tabs->addTab( QString("%1 - ").arg(i+1) + group->getDisplayText() );
      if( group == m_activeGroup ) {
        m_tabs->setCurrentIndex( i );
      }
    }
    m_tabs->blockSignals( false );
  }

  if( update_current_group ) {
    updateCurrentGroup();
  }

  if( update_active_track ) {
    if( NULL != m_activeTrack ) {
      m_listener->removeObject( m_activeTrack );
    }
    if( NULL != m_activeGroup ) {
      m_activeTrack = m_activeGroup->getActiveTrack();
      if( NULL != m_activeTrack ) {
        const uint track_mask =   OcaTrackBase::e_FlagNameChanged
                                | OcaTrackBase::e_FlagTrackDataChanged
                                | OcaTrackBase::e_FlagSampleRateChanged;
        m_listener->addObject( m_activeTrack, track_mask );
      }
    }
  }
  else if( ( NULL != m_activeTrack ) &&  ( 0 != m_listener->getFlags( m_activeTrack ) ) ) {
    update_status = true;
  }

  if( update_status ) {
    updateStatusBar();
  }

  if( update_title ) {
    if( NULL != m_activeGroup ) {
      setWindowTitle( QString( "%1 - %2" ). arg( m_activeGroup->getDisplayText() )
                                          . arg( m_data->getDisplayText() )  );
    }
    else {
      setWindowTitle( m_data->getDisplayText() );
    }
  }
  OcaAudioController* controller = OcaApp::getAudioController();
  uint audio_flags = m_listener->getFlags( controller );

  if( OcaAudioController::e_FlagAudioModeChanged & audio_flags ) {
    updateAudioModeMenu();
  }

  if( OcaAudioController::e_FlagStateChanged & audio_flags ) {
    uint play_st = controller->getState();
    uint rec_st = controller->getStateRecording();
    m_btnPlayback->setEnabled( play_st != OcaAudioController::e_StatePlaying );
    m_btnRecording->setEnabled( rec_st != OcaAudioController::e_StatePlaying );
    m_btnPause->setEnabled(    ( play_st != OcaAudioController::e_StateStopped )
                            || ( rec_st != OcaAudioController::e_StateStopped )  );

    m_btnStop->setEnabled(    ( play_st != OcaAudioController::e_StateStopped )
                           || ( rec_st != OcaAudioController::e_StateStopped )  );

    m_btnPause->setChecked(    ( play_st == OcaAudioController::e_StatePaused )
                            || ( rec_st == OcaAudioController::e_StatePaused )  );

    m_btnDuplex->setEnabled(    ( play_st == OcaAudioController::e_StateStopped )
                             && ( rec_st == OcaAudioController::e_StateStopped )  );

  }

}

// -----------------------------------------------------------------------------

void OcaMainWindow::addMonitorDock( OcaMonitor* monitor )
{
  OcaMonitorDock* dock = new OcaMonitorDock( monitor );
  addDockWidget( Qt::RightDockWidgetArea, dock );
  m_monitorDocks.insert( monitor, dock );
}

// -----------------------------------------------------------------------------

void OcaMainWindow::addMonitorDefault()
{
  if( NULL != m_activeGroup ) {
    m_data->addMonitor( new OcaMonitor( "monitor", m_activeGroup ) );
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::openNewMonitorDlg()
{
  if( NULL != m_activeGroup ) {
    OcaMonitor* d = new OcaMonitor( "monitor", m_activeGroup );
    OcaDialogPropertiesSmartTrack* dlg = new OcaDialogPropertiesSmartTrack( d, m_activeGroup );
    dlg->show();
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::addMonitorWithCurrentTrack()
{
  if( NULL != m_activeGroup ) {
    OcaTrack* t =  qobject_cast<OcaTrack*>( m_activeGroup->getActiveTrack() );
    if( NULL != t ) {
      OcaMonitor* d = new OcaMonitor( "monitor", m_activeGroup );
      d->addSubtrack( t );
      m_data->addMonitor( d );
    }
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::addMonitorWithSelectedTracks()
{
  OcaTrackGroup* group = m_data->getActiveGroup();
  QList<OcaTrack*> list;
  if( NULL != group ) {
    OcaLock lock( group );
    for( uint i = 0; i < group->getTrackCount(); i++ ) {
      OcaTrack* t = qobject_cast<OcaTrack*>( group->getTrackAt(i) );
      if( t && t->isSelected() ) {
        list.append( t );
      }
    }
  }
  if( ! list.isEmpty() ) {
    OcaMonitor* d = new OcaMonitor( "monitor", m_activeGroup );
    for( int i = 0; i < list.count(); i++ ) {
      d->addSubtrack( list.at(i) );
    }
    m_data->addMonitor( d );
  }
}

// -----------------------------------------------------------------------------

#ifdef OCA_BUILD_3DPLOT
void OcaMainWindow::openNew3DPlotDlg()
{
  Oca3DPlot* plot = new Oca3DPlot("3dplot") ;
  OcaDialogProperties3DPlot* dlg = new OcaDialogProperties3DPlot( plot, true );
  dlg->show();
}
#endif

// -----------------------------------------------------------------------------


void OcaMainWindow::newTrackDefault()
{
  OcaTrackGroup* group = m_data->getActiveGroup();
  if( NULL != group ) {
    OcaTrack* t = new OcaTrack( NULL, group->getDefaultSampleRate() );
    group->addTrack( t );
    group->setActiveTrack( t );
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::openNewTrackDlg()
{
  OcaTrackGroup* group = m_data->getActiveGroup();
  if( NULL != group ) {
    OcaTrack* track = new OcaTrack( NULL, group->getDefaultSampleRate() );
    OcaDialogPropertiesTrack* dlg = new OcaDialogPropertiesTrack( track, group );
    dlg->show();
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::newSTrackDefault()
{
  OcaTrackGroup* group = m_data->getActiveGroup();
  if( NULL != group ) {
    OcaSmartTrack* t = new OcaSmartTrack( NULL );
    group->addTrack( t );
    group->setActiveTrack( t );
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::openNewSTrackDlg()
{
  OcaTrackGroup* group = m_data->getActiveGroup();
  if( NULL != group ) {
    OcaSmartTrack* t = new OcaSmartTrack( NULL );
    OcaDialogPropertiesSmartTrack* dlg = new OcaDialogPropertiesSmartTrack( t, group );
    dlg->show();
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::newSTrackWithCurrentTrack()
{
  OcaTrackGroup* group = m_data->getActiveGroup();
  if( NULL != group ) {
    OcaTrack* t = qobject_cast<OcaTrack*>( group->getActiveTrack() );
    if( NULL != t ) {
      OcaSmartTrack* strack = new OcaSmartTrack( NULL );
      strack->addSubtrack( t );
      group->addTrack( strack );
      group->setActiveTrack( strack );
    }
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::newSTrackWithSelectedTracks()
{
  OcaTrackGroup* group = m_data->getActiveGroup();
  QList<OcaTrack*> list;
  if( NULL != group ) {
    OcaLock lock( group );
    for( uint i = 0; i < group->getTrackCount(); i++ ) {
      OcaTrack* t = qobject_cast<OcaTrack*>( group->getTrackAt(i) );
      if( t && t->isSelected() ) {
        list.append( t );
      }
    }
  }
  if( ! list.isEmpty() ) {
    OcaSmartTrack* strack = new OcaSmartTrack( NULL );
    for( int i = 0; i < list.count(); i++ ) {
      strack->addSubtrack( list.at(i) );
    }
    group->addTrack( strack );
    group->setActiveTrack( strack );
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::startAudioPlayback()
{
  OcaAudioController* controller = OcaApp::getAudioController();
  OcaTrackGroup* group = m_data->getActiveGroup();
  if( NULL != group ) {
    if( OcaAudioController::e_StatePaused == controller->getState() ) {
      controller->stopPlayback();
      controller->stopRecording();
    }
    controller->startDefaultPlayback( group );
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::startAudioDuplex()
{
  OcaAudioController* controller = OcaApp::getAudioController();
  OcaTrackGroup* group = m_data->getActiveGroup();
  if( NULL != group ) {
    double audioPosition = controller->startDefaultRecording( group );
    if( std::isfinite( audioPosition ) ) {
      audioPosition = controller->startDefaultPlayback( group );
    }
    if( ! std::isfinite( audioPosition ) ) {
      stopAudioAll();
    }
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::startAudioRecording()
{
  OcaAudioController* controller = OcaApp::getAudioController();
  OcaTrackGroup* group = m_data->getActiveGroup();
  if( NULL != group ) {
    controller->startDefaultRecording( group );
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::pauseAudioAll()
{
  OcaAudioController* controller = OcaApp::getAudioController();
  if( OcaAudioController::e_StatePaused == controller->getState()
      || OcaAudioController::e_StatePaused == controller->getStateRecording() ) {
    controller->resumePlayback();
    controller->resumeRecording();
  }
  else {
    controller->pausePlayback();
    controller->pauseRecording();
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::stopAudioAll()
{
  OcaApp::getAudioController()->stopPlayback();
  OcaApp::getAudioController()->stopRecording();
}

// -----------------------------------------------------------------------------

void OcaMainWindow::stopAudioPlayback()
{
  OcaApp::getAudioController()->stopPlayback();
}

// -----------------------------------------------------------------------------

void OcaMainWindow::stopAudioRecording()
{
  OcaApp::getAudioController()->stopRecording();
}

// -----------------------------------------------------------------------------

void OcaMainWindow::cancelCommand()
{
  OcaApp::getOctaveController()->abortCurrentCommand();
}

// -----------------------------------------------------------------------------

void OcaMainWindow::addGroupDefault()
{
  OcaTrackGroup* group = new OcaTrackGroup( NULL, m_data->getDefaultSampleRate() );
  m_data->addGroup( group );
  m_data->setActiveGroup( group );
}

// -----------------------------------------------------------------------------

void OcaMainWindow::updateCurrentGroup()
{
  OcaTrackGroupView* w = m_views.findItemData( m_activeGroup );
  connectGroupView( w );
  updateHiddenMenu();
  if( ! m_console->hasFocus() ) {
    focusCurrentGroup();
  }
  // show / hide monitors
  QMutableHashIterator<OcaMonitor*,QDockWidget*> it(m_monitorDocks);
  while (it.hasNext()) {
    it.next();
    OcaTrackGroup* g = it.key()->getGroup();
    it.value()->setHidden( !((NULL == g ) || ( m_activeGroup == g )) );
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::deleteGroup( int idx )
{
  OcaTrackGroup* group = NULL;
  if( -1 == idx ) {
    group = m_data->getActiveGroup();
  }
  else {
    group = m_data->getGroupAt( idx );
  }
  if( NULL != group ) {
    group->close();
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::setCurrentGroup( int idx )
{
  if( -1 != idx ) {
    OcaTrackGroup* group = m_views.getItem( idx );
    Q_ASSERT( NULL != group );
    m_data->setActiveGroup( group );
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::updateStatusBar()
{
  OcaTrackGroup* group = m_data->getActiveGroup();
  if( NULL != group ) {
    m_statusRight->setText( QString("%1 Hz") . arg( group->getDefaultSampleRate() ) );

    OcaTrack* track = qobject_cast<OcaTrack*>( group->getActiveTrack() );
    QString s;
    if( NULL != track ) {
      double dt = 1.0 / track->getSampleRate();
      double x = NAN;
      OcaBlockListData data;
      track->getData( &data, group->getCursorPosition() + 0.5*dt, 0 );

      if( ! data.isEmpty() ) {
        const OcaDataVector* block = data.getBlock(0);
        if( ! block->isEmpty() ) {
          x = *block->constData();
        }
      }
      s = QString( "%1 Hz - %2 ch   %3  |  v = %4  |  " )
                          .arg( track->getSampleRate() )
                          .arg( track->getChannels() )
                          .arg( track->getDisplayText() )
                          .arg( x, 0, 'f', 5 );
    }
    m_status->setText(  QString( "%1t = %3  |  (%4-%5) = %6" )
                        .arg( s )
                        .arg( group->getCursorPosition(), 0, 'f', 5 )
                        .arg( group->getRegionStart(), 0, 'f', 5 )
                        .arg( group->getRegionEnd(), 0, 'f', 5 )
                        .arg( group->getRegionDuration(), 0, 'f', 5 )     );
  }
  else {
    m_status->clear();
    m_statusRight->clear();
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::showConsole()
{
  m_dockConsole->show();
  m_dockConsole->activateWindow();
  m_console->setFocus();
}

// -----------------------------------------------------------------------------

void OcaMainWindow::hideConsole()
{
  m_dockConsole->hide();
  focusCurrentGroup();
}

// -----------------------------------------------------------------------------

void OcaMainWindow::openTrackProperties()
{
  OcaTrackGroup* group = m_data->getActiveGroup();
  OcaTrackBase* t = group->getActiveTrack();
  if( NULL != group ) {
    OcaTrack* track = qobject_cast<OcaTrack*>( t );
    if( NULL != track ) {
      OcaDialogPropertiesTrack* dlg = new OcaDialogPropertiesTrack( track );
      dlg->show();
    }
    else {
      OcaSmartTrack* strack = qobject_cast<OcaSmartTrack*>( t );
      if( NULL != strack ) {
        OcaDialogPropertiesSmartTrack* dlg = new OcaDialogPropertiesSmartTrack( strack );
        dlg->show();
      }
    }
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::openAboutDlg()
{
  OcaDialogAbout* about = new OcaDialogAbout();
  about->show();
}

// -----------------------------------------------------------------------------

void OcaMainWindow::openPreferences()
{
  OcaDialogPreferences* dlg = new OcaDialogPreferences();
  dlg->show();

}

// -----------------------------------------------------------------------------

void OcaMainWindow::updateHiddenMenu()
{
  m_trackMenuShow->clear();
  m_trackShowMap.clear();
  if( NULL == m_activeGroup ) {
    m_trackMenuShow->setEnabled( false );
  }
  else {
    const QList<OcaTrackBase*>& hidden_tracks = m_activeGroup->getHiddenList();
    if( hidden_tracks.isEmpty() ) {
      m_trackMenuShow->setEnabled( false );
    }
    else {
      OcaTrackGroupView* w = m_views.findItemData( m_activeGroup );
      m_trackMenuShow->setEnabled( true );
      m_trackMenuShow->addAction( "All", w, SLOT(showAllHiddenTracks()) );
      m_trackMenuShow->addSeparator();
      for( int i = 0; i < hidden_tracks.size(); i++ ) {
        QAction* action = m_trackMenuShow->addAction(   hidden_tracks.at(i)->getName(),
            this,
            SLOT(showTrack() )                );
        m_trackShowMap.insert( (QObject*)action, hidden_tracks.at(i) );
      }
    }
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::showTrack()
{
  OcaTrackBase* w = m_trackShowMap.value( sender(), 0 );
  if( NULL != w ) {
    Q_ASSERT( OcaObject::isValidObject( w ) );
    w->setHidden( false );
  }
}

// -----------------------------------------------------------------------------

bool OcaMainWindow::eventFilter( QObject* obj, QEvent* ev )
{
  bool processed = false;
  if( obj == m_dockConsole ) {
    if( ( QEvent::ZOrderChange == ev->type() ) && m_dockConsole->underMouse() ) {
      m_dockConsole->activateWindow();
    }
  }
  if( obj == m_tabs ) {
    if( QEvent::MouseMove == ev->type() ) {
      QMouseEvent* event = (QMouseEvent*)ev;
      OcaTrackGroup* group = m_views.getItem( m_tabs->tabAt( event->pos() ) );
      if( NULL != group ) {
        QDrag *drag = new QDrag( this );
        QMimeData *mimeData = new QMimeData;
        mimeData->setText( group->getName() );
        mimeData->setData( "application/x-octaudio-id",
                            QByteArray::number( (qulonglong)group->getId() ) );
        drag->setMimeData(mimeData);
        drag->exec( /*Qt::CopyAction |*/ Qt::MoveAction);
        processed = true;
      }
    }
    else if( QEvent::DragEnter == ev->type() ) {
      QDragEnterEvent* event = (QDragEnterEvent*)ev;

      event->setDropAction( Qt::MoveAction );
      OcaObject* obj = OcaObject::getObject(
          (oca_ulong)event->mimeData()->data("application/x-octaudio-id").toULongLong() );
      if( NULL != obj ) {
        OcaTrackGroup* g = qobject_cast<OcaTrackGroup*>( obj );
        if( NULL != g ) {
          event->acceptProposedAction();
        }
      }
      processed = true;
    }

    else if( QEvent::Drop == ev->type() ) {
      QDropEvent* event = (QDropEvent*)ev;

      OcaObject* obj = OcaObject::getObject(
          (oca_ulong)event->mimeData()->data("application/x-octaudio-id").toULongLong() );
      if( NULL != obj ) {
        OcaTrackGroup* g = qobject_cast<OcaTrackGroup*>( obj );
        if( NULL != g ) {
          m_data->moveGroup( g,  m_tabs->tabAt( event->pos() ) );
        }
      }
      processed = true;
    }
  }
  return processed;
}

// -----------------------------------------------------------------------------

void OcaMainWindow::updateAudioModeMenu()
{
  OcaAudioController* controller = OcaApp::getAudioController();
  int start = controller->getAudioModeStart();
  int stop = controller->getAudioModeStop();
  int duplex = controller->getAudioModeDuplex();

  m_audioActions.value( SLOT(setAudioStartModeAuto()) )->setCheckable( true );
  m_audioActions.value( SLOT(setAudioStartModeAuto()) )->setChecked( 0 == start );


  m_audioActions.value( SLOT(setAudioStartModeCursor()) )->setCheckable( true );
  m_audioActions.value( SLOT(setAudioStartModeCursor()) )->setChecked( 1 == start );


  m_audioActions.value( SLOT(setAudioStartModeStart()) )->setCheckable( true );
  m_audioActions.value( SLOT(setAudioStartModeStart()) )->setChecked( 2 == start );


  m_audioActions.value( SLOT(setAudioStopModeAuto()) )->setCheckable( true );
  m_audioActions.value( SLOT(setAudioStopModeAuto()) )->setChecked( 0 == stop );


  m_audioActions.value( SLOT(setAudioStopModeDuplex()) )->setCheckable( true );
  m_audioActions.value( SLOT(setAudioStopModeDuplex()) )->setChecked( 1 == duplex );
}

// -----------------------------------------------------------------------------

void OcaMainWindow::openGroupContextMenu( const QPoint& pos )
{
  int i = m_tabs->tabAt( pos );
    OcaTrackGroup* group = m_views.getItem( i );
    if( NULL != group ) {
    QMenu* menu = new QMenu( this );
    QAction* action = NULL;
    menu->addAction( "Delete", group, SLOT(close())  );
    QAction* action_props = menu->addAction( "Properties"  );
    action = menu->exec( mapToGlobal(pos) );
    if( action == action_props ) {
      OcaDialogPropertiesGroup* dlg = new OcaDialogPropertiesGroup( group );
      dlg->show();
    }
  }
}

// -----------------------------------------------------------------------------

void OcaMainWindow::closeEvent ( QCloseEvent * ev )
{
  if( NULL != m_data ) {
    ev->ignore();
    m_data->close();
  }
};

// -----------------------------------------------------------------------------

#ifdef OCA_BUILD_3DPLOT
void OcaMainWindow::add3DPlotDock(Oca3DPlot* plot)
{
  Oca3DPlotDock* dock = new Oca3DPlotDock(plot);
  addDockWidget( Qt::RightDockWidgetArea, dock );
  m_3DPlotDocks.insert( plot, dock );
}
#endif

// -----------------------------------------------------------------------------

