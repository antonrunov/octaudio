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

#ifndef OcaTrackGroupView_h
#define OcaTrackGroupView_h

#include "octaudio.h"
#include "OcaAudioController.h"
#include "OcaTrackGroup.h"
#include "OcaList.h"

#include <QAbstractScrollArea>
#include <QHash>
#include <QList>
#include <QMutex>

#include <math.h>

class OcaTimeRuller;
class OcaTrackWidget;
class OcaTrack;
class OcaSmartTrack;
class OcaTrackBase;
class OcaTrackGroup;
class OcaDataScreen;
class OcaObjectListener;

class OcaTrackGroupView : public QAbstractScrollArea
{
  Q_OBJECT ;

  public:
    OcaTrackGroupView( OcaTrackGroup* group );
    ~OcaTrackGroupView();

  protected slots:
    void moveCursor( double dt );
    void moveCursorAndView( double dt );
    void moveView( double dt );
    void setTimeScale( double scale );
    void moveTimeScale( double factor );
    void moveRegion( double dt );
    int  getTrackMargin() const { return 3; }
    int  getTrackWidth() const { return viewport()->width() - 2 * getTrackMargin(); }
    void setRegionStart( double t );
    void setRegionEnd( double t );

    void splitBlock() {} // main window
    void joinBlocks() {} // main window
    void zeroRegion() {} // main window

    void clearRegion() { m_group->setRegion( NAN, NAN ); } // main window
    void selectGroup() { m_group->setRegion( m_group->getStartTime(), m_group->getEndTime() ); } // main window
    void selectTrack(); // -> main window
    void selectView() { m_group->setRegion(  m_group->getViewPosition(),  m_group->getViewRightPosition()  ); } // main window
    void setRegionStartOnCursor() { setRegionStart( m_group->getCursorPosition() ); } // main window
    void setRegionEndOnCursor() { setRegionEnd( m_group->getCursorPosition() ); } // main window

    void showGroupEnd() { m_group->setViewEnd( m_group->getEndTime() ); } // main window
    void showGroupStart() { m_group->setViewPosition( m_group->getStartTime() ); } // main window
    void setCursorOnBasePosition() { m_group->setCursorPosition( m_basePosition ); } // main window
    void setCursorOnCenter() { m_group->setCursorPosition( m_group->getViewCenterPosition() ); } // main window
    void showTrackEnd(); // -> main window
    void showTrackStart(); // -> main window
    void showWholeGroup() { m_group->setView( m_group->getStartTime(), m_group->getEndTime() ); } // main window
    void showWholeRegion() { m_group->setView( m_group->getRegionStart(), m_group->getRegionEnd() ); } // main window
    void showWholeTrack(); // -> main window

    void centerAudioPos(); // -> mainwindow
    void centerBasePos() { m_group->setViewCenter( m_basePosition ); } // -> main window
    void centerCursor() { m_group->setViewCenter( m_group->getCursorPosition() ); } // -> main window
    void centerRegion() { m_group->setViewCenter( m_group->getRegionCenter() ); } // -> main window
    void centerRegionStart() { m_group->setViewCenter( m_group->getRegionStart() ); } // -> main window
    void centerRegionEnd() { m_group->setViewCenter( m_group->getRegionEnd() ); } // -> main window

    void deleteCurrentTrack(); // -> main window
    void deleteSelectedTracks(); // -> main window
    void hideCurrentTrack(); // -> main window
    void hideSelectedTracks(); // -> main window
    void showAllHiddenTracks(); // -> main window
    void openTrackMixDlg() {} // -> main window
    void openTrackResampleDlg() {} // -> main window
    void toggleTrackMute(); // -> main window
    void renameTrack() {} // -> main window
    void toggleTrackSelected(); // -> main window

    void setTrackSolo(); // -> main window

  protected:
    void setBasePositionByRegion( double t );
    bool checkPosition( double t, double margine_left, double margin_right );
    void setBasePosition( double t, bool autobase = false );
    void setBasePositionAuto( bool force );

  protected slots:
    void onUpdateRequired( uint flags, QHash<QString,uint>& cum_flags, QList<OcaObject*>& objects );
    void updateAudioPosition(); // tmp

  protected:
    OcaTrackGroup*                        m_group;
    OcaList<OcaTrackBase,OcaDataScreen>   m_screens;
    OcaObjectListener*                    m_listener;

  protected:
    enum EAudioPositionState {
      e_AudioPositionNone       = 0,
      e_AudioPositionPlayback,
      e_AudioPositionRecording,
    };

  protected:
    OcaTimeRuller*  m_timeRuller;
    double          m_timeScale;
    double          m_audioPosition;
    int             m_audioPositionState;
    double          m_basePosition;
    bool            m_basePosAuto;
    double          m_autoBaseLeft;
    double          m_autoBaseRight;
    double          m_timeScrollbarScale;
    bool            m_timeScrollbarEnabled;

  protected:
    class TrackFrame : public QWidget
    {
      public:
        TrackFrame( OcaTrackGroupView* view );

      public:
        void setActiveFrame( const QRect& r ) { m_activeFrame = r; update(); }
        void setSelectedFrames( const QList<QRect>& frames ) { m_selectedFrames = frames; update(); }

      protected:
        virtual void paintEvent ( QPaintEvent * event );

      protected:
        OcaTrackGroupView*  m_view;
        QRect               m_activeFrame;
        QList<QRect>        m_selectedFrames;
    };

    TrackFrame*                m_widget;

  protected:
    int                         m_totalHeight;
    int                         m_resizedTrack;
    int                         m_resizeRef;
    int                         m_resizedRegion;

  public:
    virtual bool eventFilter( QObject* obj, QEvent* ev );

  protected:
    int checkTrackResizeArea( QPoint pos, int* resize_ref = NULL );
    int checkRegionResizeArea( QPoint pos );

  protected:
    OcaDataScreen* createScreen( OcaTrackBase* track );
    QRect getTrackFrameRect( const OcaTrackBase* track ) const;
    void  checkVisibleTracks();
    bool  isTrackVisible( OcaDataScreen* s );

  protected:
    virtual void resizeEvent( QResizeEvent* event );
    virtual void scrollContentsBy(int dx, int dy);
    virtual void keyPressEvent( QKeyEvent* key_event );
    virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseReleaseEvent( QMouseEvent* event );
    virtual void wheelEvent( QWheelEvent* event );
    virtual void mouseDoubleClickEvent( QMouseEvent* event );

  protected:
    void updateTimeScrollBar();
    void updateVerticalScrollBar();
};

#endif // OcaTrackGroupView_h

