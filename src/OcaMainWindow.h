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

#ifndef OcaMainWindow_h
#define OcaMainWindow_h

#include "OcaTrackGroupView.h"
#include "OcaList.h"

#include <QMainWindow>
#include <QList>
#include <QHash>

class OcaConsole;
class OcaMonitor;
class Oca3DPlot;
class QMenu;
class OcaTrackBase;
class OcaWindowData;
class OcaObjectListener;
class OcaObject;
class OcaTrackGroup;
class QStackedLayout;
class QTabBar;
class QLabel;
class QToolButton;

class OcaMainWindow : public QMainWindow
{
  Q_OBJECT ;

  public:
    OcaMainWindow( OcaWindowData* data );
    ~OcaMainWindow();

  protected:
    QDockWidget*                  m_dockConsole;
    QStackedLayout*               m_stack;
    QTabBar*                      m_tabs;

  protected:
    QMenu*                        m_audioMenu;
    QMenu*                        m_audioStartMenu;
    QMenu*                        m_audioPauseMenu;
    QMenu*                        m_audioStopMenu;
    QMenu*                        m_audioStartModeMenu;
    QMenu*                        m_audioStopModeMenu;
    QMenu*                        m_editMenu;
    QMenu*                        m_editRegionMenu;
    QMenu*                        m_editConsoleMenu;
    QMenu*                        m_fileMenu;
    QMenu*                        m_trackMenu;
    QMenu*                        m_trackMenuNew;
    QMenu*                        m_trackMenuShow;
    QMenu*                        m_viewMenu;
    QMenu*                        m_viewCenterMenu;
    QMenu*                        m_viewMonitorMenu;
    QMenu*                        m_helpMenu;

    QHash<QObject*,OcaTrackBase*>   m_trackShowMap;

  protected:
    QToolButton*  m_btnPlayback;
    QToolButton*  m_btnRecording;
    QToolButton*  m_btnStop;
    QToolButton*  m_btnPause;
    QToolButton*  m_btnDuplex;

  protected:
    OcaConsole*                 m_console;
    QHash<QByteArray,QAction*>  m_groupActions;
    QHash<QByteArray,QAction*>  m_audioActions;
    QLabel*                     m_status;
    QLabel*                     m_statusRight;
    OcaTrackBase*               m_activeTrack;
    OcaTrackGroup*              m_activeGroup;

  protected:
    OcaWindowData*      m_data;
    OcaObjectListener*  m_listener;

    QHash<OcaMonitor*,QDockWidget*>           m_monitorDocks;
#ifdef OCA_BUILD_3DPLOT
    QHash<Oca3DPlot*,QDockWidget*>            m_3DPlotDocks;
#endif
    OcaList<OcaTrackGroup,OcaTrackGroupView>  m_views;

  protected:
    void  createMenus();
    void  createToolbars();
    QAction* createAction(  const QString& name,
                            const QKeySequence& shortcutKey,
                            QMenu* menu );
    // TODO - temporary function
    void addDisabledAction( const QString& name,
                            const QKeySequence& shortcutKey,
                            QMenu* menu,
                            const char* slot );
    void addWindowAction( const QString& name,
                          const QKeySequence& shortcutKey,
                          QMenu* menu,
                          const char* slot );
    void addGroupAction( const QString& name,
                         const QKeySequence& shortcutKey,
                         QMenu* menu,
                         const char* slot );
    void addConsoleAction( const QString& name,
                         const QKeySequence& shortcutKey,
                         QMenu* menu,
                         const char* slot );
    void addAudioAction( const QString& name,
                         const QKeySequence& shortcutKey,
                         QMenu* menu,
                         const char* slot );
    void connectGroupView( OcaTrackGroupView* view );

    void addMonitorDock( OcaMonitor* monitor );
#ifdef OCA_BUILD_3DPLOT
    void add3DPlotDock( Oca3DPlot* plot );
#endif

  protected slots:
    void onUpdateRequired( uint flags, QHash<QString,uint>& cum_flags,
                                       QList<OcaObject*>& objects     );

  protected slots:
    void openExportDlg() {}
    void openImportDlg() {}
    void openProperties();
    void openNewGroupDlg();
    void addGroupWithCurrentTrack() {}
    void addGroupWithSelectedTracks() {}
    void activateNextGroup();
    void activatePrevGroup();
    void focusCurrentGroup();

    void copy() {}
    void paste() {}
    void cut() {}
    void undo() {}
    void redo() {}
    void openPreferences();
    void openTrackProperties();
    void openAboutDlg();

    // to console
    void cancelCommand();
    void clearCommand() {}
    void evalCommand() {}
    void findInLog() {}
    void showCommandHistory() {}
    void nextCommand() {}
    void prevCommand() {}

    void openStatusBarFunctionDlg() {}

    void clearMonitor() {}
    void addMonitorDefault();
    void openNewMonitorDlg();
    void addMonitorWithCurrentTrack();
    void addMonitorWithSelectedTracks();
    void openMonitorProperties() {}

    void newTrackDefault();
    void newSTrackDefault();
    void openNewTrackDlg();
    void openNewSTrackDlg();
    void newSTrackWithCurrentTrack();
    void newSTrackWithSelectedTracks();

    void startAudioPlayback();
    void startAudioDuplex();
    void startAudioLooped() {}
    void startAudioRecording();

    void pauseAudioAll();
    void pauseAudioPlayback() {}
    void pauseAudioRecording() {}

    void stopAudioAll();
    void stopAudioPlayback();
    void stopAudioRecording();

#ifdef OCA_BUILD_3DPLOT
    void openNew3DPlotDlg();
#endif

  protected slots:
    void runCommand( const QString& command );
    void addGroupDefault();
    void deleteGroup( int idx );
    void setCurrentGroup( int idx );
    void deleteGroup() { deleteGroup(-1); }
    void showConsole();
    void hideConsole();
    void showTrack();
    void updateAudioModeMenu();
    void openGroupContextMenu( const QPoint& pos );

  protected:
    void updateCurrentGroup();
    void updateStatusBar();
    void updateHiddenMenu();

  public:
    virtual bool eventFilter( QObject* obj, QEvent* ev );

  protected:
    virtual void closeEvent ( QCloseEvent * ev );

};

#endif // OcaMainWindow_h

