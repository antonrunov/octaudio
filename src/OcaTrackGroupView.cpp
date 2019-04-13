/*
   Copyright 2013-2018 Anton Runov

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

#include "OcaTrackGroupView.h"

#include "OcaTimeRuller.h"
#include "OcaOctaveController.h"
#include "OcaTrackBase.h"
#include "OcaTrack.h"
#include "OcaSmartTrack.h"
#include "OcaTrackScreen.h"
#include "OcaApp.h"
#include "OcaTrackGroup.h"
#include "OcaObjectListener.h"
#include "OcaSmartScreen.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

static const int TIME_SCROLLBAR_RES = INT_MAX / 2 - 1000;

// ------------------------------------------------------------------------------------
// OcaTrackGroupView::TrackFrame

OcaTrackGroupView::TrackFrame::TrackFrame( OcaTrackGroupView* view )
: QWidget( view->viewport() ),
  m_view( view )
{
  setMouseTracking( true );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::TrackFrame::paintEvent ( QPaintEvent * event )
{
  QPainter painter(this);
  painter.eraseRect( rect() );
  if( m_activeFrame.isValid() ) {
    painter.fillRect( m_activeFrame, QColor(QRgb(0xffe000)) );
  }
  for( int idx = 0; idx < m_selectedFrames.size(); idx++ ) {
    const QRect& r = m_selectedFrames.at( idx );
    Q_ASSERT( r.isValid() );
    painter.fillRect( r, QColor(QRgb(0xd00000)) );
  }
}


// ------------------------------------------------------------------------------------
// OcaTrackGroupView

OcaTrackGroupView::OcaTrackGroupView( OcaTrackGroup* group )
:
  m_group( group ),
  m_listener( NULL ),
  m_timeRuller( NULL ),
  m_timeScale( 0.0 ),
  m_audioPosition( NAN ),
  m_basePosition( 0.0 ),
  m_basePosAuto( true ),
  m_autoBaseLeft( -INFINITY ),
  m_autoBaseRight( INFINITY ),
  m_timeScrollbarScale( 0.0 ),
  m_timeScrollbarEnabled( false ),
  m_widget( NULL ),
  m_totalHeight( 0 ),
  m_resizedTrack( -1 ),
  m_resizeRef( -1 ),
  m_resizedRegion( -1 )
{
  m_listener = new OcaObjectListener( m_group, OcaTrackGroup::e_FlagALL, 10, this );
  connect(  m_listener,
            SIGNAL(updateRequired(uint,QHash<QString,uint>&,QList<OcaObject*>&)),
            SLOT(onUpdateRequired(uint,QHash<QString,uint>&,QList<OcaObject*>&))  );
  m_listener->addEvent( NULL, OcaTrackGroup::e_FlagALL );

  OcaAudioController* controller = OcaApp::getAudioController();
  m_listener->addObject( controller, OcaAudioController::e_FlagStateChanged );
  m_listener->addEvent( controller, OcaAudioController::e_FlagStateChanged );

  //setLineWidth( 15 );
  //setFrameStyle( QFrame::Panel | QFrame::Sunken );
  setFrameStyle( QFrame::NoFrame );
  m_timeRuller = new OcaTimeRuller( m_group );
  m_timeRuller->setParent( this );
  m_timeRuller->setGeometry( 0, 0, viewport()->width(), m_timeRuller->height() );

  //m_timeScale = m_group->getViewDuration() / getTrackWidth();
  m_basePosition = 0;
  m_basePosAuto = true;

  setViewportMargins ( 0, m_timeRuller->height(), 0, 0 );
  viewport()->setBackgroundRole( QPalette::Dark );
  setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
  //setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
  m_widget = new TrackFrame( this );
  m_widget->resize( viewport()->width(), 0 );
  m_widget->installEventFilter( this );
  viewport()->setMouseTracking ( true );

  m_timeRuller->setZeroOffset( getTrackMargin() + viewport()->x() );

  verticalScrollBar()->setValue( 0 );

  horizontalScrollBar()->setRange( -200, TIME_SCROLLBAR_RES + 200 );
  updateTimeScrollBar();
}

// ------------------------------------------------------------------------------------

OcaTrackGroupView::~OcaTrackGroupView()
{
  m_group = NULL;
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::moveCursor( double dt )
{
  double t = m_group->getCursorPosition() + dt;
  m_group->setCursorPosition( t );
  setBasePosition( t );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::moveCursorAndView( double dt )
{
  if( ! checkPosition( m_group->getCursorPosition(), 0.125, 0.125 ) ) {
    setCursorOnCenter();
  }
  moveCursor( dt );
  moveView( dt );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::moveView( double dt )
{
  m_group->setViewPosition( m_group->getViewPosition() + dt );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::setTimeScale( double scale )
{
  if( ( 1e-6 < scale ) && ( 1e4 > scale ) ) {
    double new_duration = scale * getTrackWidth();
    double start = m_basePosition - ( m_basePosition - m_group->getViewPosition() )
                                                                    / m_timeScale * scale;
    m_autoBaseLeft = start;
    m_autoBaseRight = start + new_duration;
    m_group->setView( start, start + new_duration );
    m_timeScale = m_group->getViewDuration() / getTrackWidth();
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::moveTimeScale( double factor )
{
  setTimeScale( m_timeScale * factor );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::moveRegion( double dt )
{
  // TODO
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::setRegionStart( double t )
{
  m_group->setRegion( t, m_group->getRegionEnd() );
  setBasePositionByRegion( t );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::setRegionEnd( double t )
{
  m_group->setRegion( m_group->getRegionStart(), t );
  setBasePositionByRegion( t );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::setBasePositionByRegion( double t )
{
  if(  checkPosition( m_group->getRegionStart(), 0.15, 0.15 )
        && checkPosition( m_group->getRegionEnd(), 0.15, 0.15 )  ) {
    t =  m_group->getRegionCenter();
  }
  setBasePosition( t );
}

// ------------------------------------------------------------------------------------

bool OcaTrackGroupView::checkPosition( double t, double margine_left, double margin_right )
{
  bool result = false;
  if( 0 < m_group->getViewDuration() ) {
    double x = ( t - m_group->getViewPosition() ) / m_group->getViewDuration();
    result = ( x > margine_left ) && ( x < ( 1 - margin_right ) );
  }
  return result;
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::setBasePosition( double t, bool autobase /* = false */ )
{
  if( checkPosition( t, 0.0, 0.0 ) ) {
    m_basePosition = t;
    m_autoBaseLeft = m_group->getViewPosition();
    m_autoBaseRight = m_group->getViewRightPosition();
    m_basePosAuto = autobase;
    m_timeRuller->setBasePosition( m_basePosition );
  }
  else {
    setBasePositionAuto( true );
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::setBasePositionAuto( bool force )
{
  if(   force ||
        ( m_group->getViewPosition() != m_autoBaseLeft ) ||
        ( m_group->getViewRightPosition() != m_autoBaseRight )    ) {

    if( m_basePosAuto || ( ! checkPosition( m_basePosition, 0.15, 0.15 ) ) ) {
      if(   std::isfinite( m_audioPosition ) &&
          ( checkPosition( m_audioPosition, 0.15, 0.15 ) )  ) {
        m_basePosition = m_audioPosition;
      }
      else if( checkPosition( m_group->getCursorPosition(), 0.4, 0.4 ) ) {
        m_basePosition = m_group->getCursorPosition();
      }
      else if (   checkPosition( m_group->getRegionStart(), 0.4, 0.4 )
                  && checkPosition( m_group->getRegionEnd(), 0.4, 0.4 )   ) {
        m_basePosition =  m_group->getRegionCenter();
      }
      else if ( checkPosition( m_group->getRegionStart(), 0.4, 0.4 ) ) {
        m_basePosition =  m_group->getRegionStart();
      }
      else if ( checkPosition( m_group->getRegionEnd(), 0.4, 0.4 ) ) {
        m_basePosition =  m_group->getRegionEnd();
      }
      else {
        m_basePosition = m_group->getViewCenterPosition();
      }
      m_basePosAuto = true;
    }

    m_autoBaseLeft = m_group->getViewPosition();
    m_autoBaseRight = m_group->getViewRightPosition();
    m_timeRuller->setBasePosition( m_basePosition );
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::selectTrack()
{
  OcaTrackBase* w = m_group->getActiveTrack();
  if( NULL != w ) {
    if( 0 < w->getDuration() ) {
      m_group->setRegion( w->getStartTime(), w->getEndTime() );
    }
    else {
      clearRegion();
    }
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::showTrackEnd()
{
  OcaTrackBase* w = m_group->getActiveTrack();
  if( NULL != w ) {
    if( std::isfinite( w->getEndTime() ) ) {
      m_group->setViewEnd( w->getEndTime() );
    }
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::showTrackStart()
{
  OcaTrackBase* w = m_group->getActiveTrack();
  if( NULL != w ) {
    if( std::isfinite( w->getStartTime() ) ) {
      m_group->setViewPosition( w->getStartTime() );
    }
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::showWholeTrack()
{
  OcaTrackBase* w = m_group->getActiveTrack();
  if( NULL != w ) {
    if( 0 < w->getDuration() ) {
      m_group->setView( w->getStartTime(), w->getEndTime() );
    }
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::centerAudioPos()
{
  if( std::isfinite( m_audioPosition ) ) {
    m_group->setViewCenter( m_audioPosition );
    setBasePosition( m_audioPosition, true );
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::deleteCurrentTrack()
{
  OcaTrackBase* t = m_group->getActiveTrack();
  if( NULL != t ) {
    t->close();
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::deleteSelectedTracks()
{
  QList<OcaTrackBase*> list;
  {
    OcaLock lock( m_group );
    for( int idx = m_group->getTrackCount() - 1; 0 <= idx; idx-- ) {
      OcaTrackBase* t = m_group->getTrackAt( idx );
      Q_ASSERT( NULL != t );
      if( t->isSelected() ) {
        Q_ASSERT( ! t->isHidden() );
        list.append( t );
      }
    }
  }
  while( ! list.isEmpty() ) {
    list.takeFirst()->close();
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::hideCurrentTrack()
{
  OcaTrackBase* w = m_group->getActiveTrack();
  if( NULL != w ) {
    w->setHidden( true );
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::hideSelectedTracks()
{
  QList<OcaTrackBase*> list;
  {
    OcaLock lock( m_group );
    for( uint idx = 0; idx < m_group->getTrackCount(); idx++ ) {
      OcaTrackBase* w = m_group->getTrackAt( idx );
      if( w->isSelected() ) {
        list.append( w );
      }
    }
  }
  while( ! list.isEmpty() ) {
    list.takeFirst()->setHidden( true );
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::showAllHiddenTracks()
{
  QList<OcaTrackBase*> list;
  {
    OcaLock lock( m_group );
    for( uint i = 0; i < m_group->getTrackCount(); i++ ) {
      OcaTrackBase* w = m_group->getTrackAt( i );
      list.append( w );
    }
  }
  while( ! list.isEmpty() ) {
    list.takeFirst()->setHidden( false );
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::toggleTrackMute()
{
  OcaTrack* w = qobject_cast<OcaTrack*>( m_group->getActiveTrack() );
  if( NULL != w ) {
    w->setMute( ! w->isMuted() );
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::toggleTrackSelected()
{
  OcaTrackBase* w = m_group->getActiveTrack();
  if( NULL != w ) {
    w->setSelected( ! w->isSelected() );
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::setTrackSolo()
{
  OcaTrackBase* t = m_group->getActiveTrack();
  if( m_group->getSoloTrack() == t ) {
    m_group->setSoloTrack( NULL );
  }
  else {
    m_group->setSoloTrack( t );
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::updateAudioPosition()
{
  OcaAudioController*  controller = OcaApp::getAudioController();
  bool visible = checkPosition( m_audioPosition, 0.0, 0.0 );

  if( ( e_AudioPositionRecording == m_audioPositionState ) &&
      ( controller->getStateRecording() == OcaAudioController::e_StatePlaying ) ) {
    m_audioPosition = controller->getRecordingPosition();
  }
  else if( ( e_AudioPositionPlayback == m_audioPositionState ) &&
      ( controller->getState() == OcaAudioController::e_StatePlaying ) ) {
    m_audioPosition = controller->getPlaybackPosition();
  }
  else {
    visible = false;
  }

  if( m_basePosAuto && visible ) {
    if( ! checkPosition( m_audioPosition, 0.0, 0.125 ) ) {
      m_group->setViewPosition( m_audioPosition - 0.125 * m_group->getViewDuration() );
    }
    setBasePosition( m_audioPosition, true );
  }
}

// ------------------------------------------------------------------------------------

bool OcaTrackGroupView::eventFilter( QObject* obj, QEvent* ev )
{
  // TOD
  if( obj == m_widget ) {
    if( QEvent::MouseMove == ev->type() ) {
      /*
      QMouseEvent* event = (QMouseEvent*)ev;
      //fprintf( stderr, "MOVE: %d x %d\n", event->x(), event->y() );
      if( -1 != checkTrackResizeArea( event->globalPos() ) ) {
        setCursor( Qt::SplitVCursor );
      }
      else {
        setCursor( Qt::ArrowCursor );
      }
      */
      //return -1 == m_resizedTrack;
    }
    else if( QEvent::Leave == ev->type() ) {
      setCursor( Qt::ArrowCursor );
    }
    else if( QEvent::Resize == ev->type() ) {
      updateVerticalScrollBar();
    }
  }
  else {
#if 0
    if( QEvent::MouseButtonPress == ev->type() ) {
      /*
      OcaTrack* w = ((OcaTrack::ScreenSimple*)obj)->getUiTrack();
      setActiveTrackWidget( w );
      */
    }
    else if( QEvent::DragEnter == ev->type() ) {
      //OcaTrack* w = (OcaTrack*)obj;
      //fprintf( stderr, "drag enter - %d\n", (int)w->getIndex() );
      QDragEnterEvent* event = (QDragEnterEvent*)ev;
      event->setDropAction( Qt::MoveAction );
      if( event->mimeData()->hasFormat("application/x-octaudio-id") ) {
        event->acceptProposedAction();
      }
    }
    else if( QEvent::Drop == ev->type() ) {
      /*
      OcaTrack* w = ((OcaTrack::ScreenSimple*)obj)->getUiTrack();
      //fprintf( stderr, "drop - %d\n", (int)w->getIndex() );
      QDropEvent* event = (QDropEvent*)ev;
      oca_ulong id = event->mimeData()->data("application/x-octaudio-id").toULong();
      //oca_ulong id = (oca_ulong)(OcaTrack*)event->source();
      if( 0 != id ) {
        event->acceptProposedAction();
        moveTrack( id, w->getIndex() );
      }
      */
    }
    else
#endif
    if( QEvent::Enter == ev->type() ) {
      setCursor( Qt::ArrowCursor );
    }
  }
  return false;
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::onUpdateRequired(   uint flags,
                                            QHash<QString,uint>& /* cum_flags */,
                                            QList<OcaObject*>& /* objects */      )
{
  const uint move_flags =   OcaTrackGroup::e_FlagTrackMoved
                          | OcaTrackGroup::e_FlagTrackRemoved
                          | OcaTrackGroup::e_FlagTrackAdded
                          | OcaTrackGroup::e_FlagHiddenTracksChanged;

  const uint frame_flags =    move_flags
                            | OcaTrackGroup::e_FlagActiveTrackChanged
                            | OcaTrackGroup::e_FlagSelectedTracksChanged;

  const uint time_flags =   OcaTrackGroup::e_FlagTimeScaleChanged
                          | OcaTrackGroup::e_FlagViewPositionChanged
                          | OcaTrackGroup::e_FlagDurationChanged;

  const uint base_pos_flags = OcaTrackGroup::e_FlagViewPositionChanged;

  const uint list_flags =   OcaTrackGroup::e_FlagTrackRemoved
                          | OcaTrackGroup::e_FlagTrackAdded
                          | move_flags;

  if( list_flags & flags ) {
    OcaLock lock( m_group );
    // remove screens
    for( int idx = m_screens.getLength() - 1; idx >= 0; idx-- ) {
      OcaDataScreen* s = NULL;
      OcaTrackBase* t = m_screens.getItem( idx, &s );
      if( -1 == m_group->getTrackIndex( t ) ) {
        m_screens.removeItem( t );
        Q_ASSERT( NULL != s );
        delete s;
        s = NULL;
      }
    }

    // add screens
    for( uint idx = 0; idx < m_group->getTrackCount(); idx++ ) {
      OcaTrackBase* t = m_group->getTrackAt( idx );
      Q_ASSERT( NULL != t );
      if( -1 == m_screens.findItemIndex( t ) ) {
        OcaDataScreen* s = createScreen( t );
        Q_ASSERT( NULL != s );
        m_screens.appendItem( t, s );
      }
    }

    Q_ASSERT( m_group->getTrackCount() == m_screens.getLength() );
    // move screens
    int h = 0;
    for( uint idx = 0; idx < m_group->getTrackCount(); idx++ ) {
      OcaTrackBase* t = m_group->getTrackAt( idx );
      oca_index idx_screen = m_screens.findItemIndex( t );
      if( idx_screen != idx ) {
        m_screens.moveItem( t, idx );
      }
      Q_ASSERT( m_screens.getItem( idx ) == t );
      OcaDataScreen* s = m_screens.getItemData( idx );
      if( ! t->isHidden() ) {
        s->show();
        s->move( getTrackMargin(), h + 2 );
        s->resize( getTrackWidth(), m_group->getTrackHeight( t ) );
        h += s->height() + 5;
        s->setVisible( isTrackVisible( s ) );
      }
      else {
        s->hide();
      }
    }
    if( h != m_widget->height() ) {
      m_widget->resize( viewport()->width(), h );
    }
  }

  // set frames (active, selected)
  if( frame_flags & flags ) {
    OcaTrackBase* t = m_group->getActiveTrack();
    QRect r;
    if( NULL != t ) {
      r = getTrackFrameRect( t ).adjusted( -3, -3, 3, 3 );
    }
    QList<QRect> selected;
    for( int idx = m_screens.getLength() - 1; idx >= 0; idx-- ) {
      OcaDataScreen* s = NULL;
      OcaTrackBase* t = m_screens.getItem( idx, &s );
      Q_ASSERT( NULL != t );
      Q_ASSERT( NULL != s );
      if( t->isSelected() ) {
        Q_ASSERT( ! t->isHidden() );
        QRect r1 = s->geometry().adjusted( -5, 0, 5, 0 ) ;
        selected.append( r1 );
      }
    }
    m_widget->setActiveFrame( r );
    m_widget->setSelectedFrames( selected );
  }

  // audio
  bool auto_pos_after_audio = false;
  OcaAudioController* controller = OcaApp::getAudioController();
  uint audio_flags = m_listener->getFlags( controller );
  if( OcaAudioController::e_FlagStateChanged & audio_flags ) {
    bool autopos = false;
    if( controller->getRecordingGroup() == m_group ) {
      m_listener->setObjectMask( controller, OcaAudioController::e_FlagALL );
      //autopos = ! std::isfinite( m_audioPosition );
      autopos = true; // TODO
      m_audioPositionState = e_AudioPositionRecording;
      m_audioPosition = controller->getRecordingPosition();
    }
    else if( controller->getPlayedGroup() == m_group ) {
      m_listener->setObjectMask( controller, OcaAudioController::e_FlagALL );
      //autopos = ! std::isfinite( m_audioPosition );
      autopos = true; // TODO
      m_audioPositionState = e_AudioPositionPlayback;
      m_audioPosition = controller->getPlaybackPosition();
    }
    else {
      m_listener->setObjectMask( controller, OcaAudioController::e_FlagStateChanged );
      auto_pos_after_audio = true;
      m_audioPositionState = e_AudioPositionNone;
      m_audioPosition = NAN;
    }

    if( autopos && std::isfinite( m_audioPosition ) ) {
      if( checkPosition( m_audioPosition, 0.0, 0.0 ) ) {
        setBasePosition( m_audioPosition, true );
      }
      else {
        m_group->setViewPosition( m_audioPosition - 0.125 * m_group->getViewDuration() );
      }
    }
  }

  if( OcaAudioController::e_FlagCursorChanged & audio_flags ) {
    updateAudioPosition();
  }

  // base position
  if( ( base_pos_flags && flags ) || auto_pos_after_audio ) {
    setBasePositionAuto( auto_pos_after_audio );
  }

  // update time scrollbar
  if( time_flags & flags ) {
    m_timeScale = m_group->getViewDuration() / getTrackWidth();
    updateTimeScrollBar();
  }
}

// ------------------------------------------------------------------------------------

QRect OcaTrackGroupView::getTrackFrameRect( const OcaTrackBase* track ) const
{
  QRect r;
  OcaDataScreen* s = m_screens.findItemData( track );
  if( NULL != s ) {
    r = s->geometry();
  }
  return r;
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::checkVisibleTracks()
{
  for( uint idx = 0; idx < m_screens.getLength(); ++idx ) {
    OcaDataScreen* ws = NULL;
    OcaTrackBase* w = m_screens.getItem( idx, &ws );
    Q_ASSERT( NULL != ws );
    if( ! w->isHidden() ) {
      ws->setVisible( isTrackVisible( ws ) );
    }
  }
}

// ------------------------------------------------------------------------------------

bool OcaTrackGroupView::isTrackVisible( OcaDataScreen* s )
{
  int y = m_widget->mapTo( viewport(), s->pos() ).y();
  return ( 0 <= y + s->height() ) && ( viewport()->height() > y );
}

// ------------------------------------------------------------------------------------

OcaDataScreen* OcaTrackGroupView::createScreen( OcaTrackBase* track )
{
  OcaDataScreen* s = NULL;
  OcaTrack* t = qobject_cast<OcaTrack*>( track );
  if( NULL != t ) {
    s = new OcaTrackScreen( t, m_group );
  }
  else {
    OcaSmartTrack* smart = qobject_cast<OcaSmartTrack*>( track );
    Q_ASSERT( NULL != smart );
    s = new OcaSmartScreen( smart, m_group );
  }
  s->setParent( m_widget );
  s->installEventFilter( this );
  s->resize( getTrackWidth(), m_group->getTrackHeight( track ) );
  return s;
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::resizeEvent( QResizeEvent* event )
{
  m_timeRuller->setGeometry( 0, 0, viewport()->width(), m_timeRuller->height() );
  m_widget->resize( viewport()->width(), m_widget->height() );
  for( uint idx = 0; idx < m_screens.getLength(); idx++ ) {
    OcaDataScreen* s = m_screens.getItemData( idx );
    Q_ASSERT( NULL != s );
    s->resize( getTrackWidth(), s->height() );
  }
  m_listener->addEvent( NULL, OcaTrackGroup::e_FlagActiveTrackChanged );
  if( 0.0 != m_timeScale ) {
    double t0 = m_group->getViewPosition();
    m_group->setView( t0, t0 + m_timeScale * getTrackWidth() );
  }
  updateTimeScrollBar();
  checkVisibleTracks();
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::scrollContentsBy( int dx, int dy )
{
  if( 0 != dx && m_timeScrollbarEnabled ) {
     int n =  horizontalScrollBar()->value();
     if( ( 1.0 < m_timeScrollbarScale ) && ( abs(dx) < 5000 ) ) {
       moveView( -dx * m_timeScale );
     }
     else {
       m_group->setViewPosition( m_group->getStartTime() + ( n * m_timeScrollbarScale - getTrackWidth() ) * m_timeScale );
     }
  }
  if( 0 != dy ) {
    m_widget->move( 0, -verticalScrollBar()->value() );
    checkVisibleTracks();
  }
}

static const double STEP = pow( 2.0, -0.25 );

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::keyPressEvent ( QKeyEvent* key_event )
{
  // TOD
  // int modifiers = ( key_event->modifiers() & (~Qt::KeypadModifier) );

  bool processed = true;
  bool tmp_flag = false;
  int  d = 1;

  switch( ( key_event->modifiers() & (~Qt::KeypadModifier) ) + key_event->key() ) {

    // move track
    case Qt::Key_U + Qt::ControlModifier:
      tmp_flag = true;
    case Qt::Key_D + Qt::ControlModifier:
      {
        OcaTrackBase* t = m_group->getActiveTrack();
        if( NULL != t ) {
          int d = ( tmp_flag ? -1 : 1 );
          oca_index idx = m_group->getNextVisibleTrack( t, d );
          if( -1 != idx ) {
            m_group->moveTrack( t, idx );
          }
        }
      }
      break;

    // scrollUD_1 - Group:Tracks:Activate
    case Qt::Key_Up:
      tmp_flag = true;
    case Qt::Key_Down:
      {
        OcaTrackBase* t = m_group->getActiveTrack();
        if( NULL != t ) {
          int d = ( tmp_flag ? -1 : 1 );
          oca_index idx = m_group->getNextVisibleTrack( t, d );
          if( -1 != idx ) {
            m_group->setActiveTrack( m_group->getTrackAt( idx ) );
          }
        }
      }
      break;

    // scrollUD_2 - Group:TimeScale:Move
    case  Qt::Key_Up + Qt::ControlModifier:
      tmp_flag = true;
    case  Qt::Key_Down + Qt::ControlModifier:
      moveTimeScale( tmp_flag ? STEP : 1/STEP );
      break;

    // scrollUD_3 - Track:VScale:MoveScale
    case  Qt::Key_Up + Qt::ShiftModifier:
      d = -1;
    case  Qt::Key_Down + Qt::ShiftModifier:
      {
        OcaTrackBase* t = m_group->getActiveTrack();
        if( NULL != t ) {
          t->moveScale( d );
        }
      }
      break;

    // scrollUD_4 - Track:VScale:MoveZero
    case  Qt::Key_Up + Qt::AltModifier:
      d = -1;
    case  Qt::Key_Down + Qt::AltModifier:
      {
        OcaTrackBase* t = m_group->getActiveTrack();
        if( NULL != t ) {
          t->moveZero( d );
        }
      }
      break;

      // scrollLR_1 - Group:Cursor:Move
    case  Qt::Key_Left:
      tmp_flag = true;
    case  Qt::Key_Right:
      moveCursor( ( tmp_flag ? -10 : 10 ) * m_timeScale );
      break;

    // scrollLR_2 - Group:Cursor:Move(Fine)
    case  Qt::Key_Left + Qt::ControlModifier:
      tmp_flag = true;
    case  Qt::Key_Right + Qt::ControlModifier:
      moveCursor( ( tmp_flag ? -1 : 1 ) * m_timeScale );
      break;

    // scrollLR_4 - Group:View:Move
    case  Qt::Key_Left + Qt::ShiftModifier:
      tmp_flag = true;
    case  Qt::Key_Right + Qt::ShiftModifier:
      moveView( ( tmp_flag ? -10 : 10 ) * m_timeScale );
      break;

    // scrollLR_5 - Group:View:Move(Fast)
    case  Qt::Key_Left + Qt::ShiftModifier + Qt::AltModifier:
      tmp_flag = true;
    case  Qt::Key_Right + Qt::ShiftModifier + Qt::AltModifier:
      moveView( ( tmp_flag ? -50 : 50 ) * m_timeScale );
      break;

    // scrollLR_6 - Group:View:Move(Fine)
    case  Qt::Key_Left + Qt::ShiftModifier + Qt::ControlModifier:
      tmp_flag = true;
    case  Qt::Key_Right + Qt::ShiftModifier + Qt::ControlModifier:
      moveView( ( tmp_flag ? -1 : 1 ) * m_timeScale );
      break;



    // scrollPM_N - Group:Cursor:Move(Sample)
    case  Qt::Key_BracketLeft + Qt::ControlModifier:
      tmp_flag = true;
    case  Qt::Key_BracketRight + Qt::ControlModifier:
      moveCursor( ( tmp_flag ? -1 : 1 ) / m_group->getDefaultSampleRate() );
      break;

    // scrollPM_N - Group:Cursor:MoveWithView
    case  Qt::Key_BracketLeft + Qt::AltModifier:
      tmp_flag = true;
    case  Qt::Key_BracketRight + Qt::AltModifier:
      moveCursorAndView( ( tmp_flag ? -10 : 10 ) * m_timeScale );
      break;

    // scrollPM_N - Group:Cursor:MoveWithView(Fast)
    case  Qt::Key_BracketLeft + Qt::AltModifier + Qt::ShiftModifier:
    case  Qt::Key_BraceLeft + Qt::AltModifier + Qt::ShiftModifier:
      tmp_flag = true;
    case  Qt::Key_BracketRight + Qt::AltModifier + Qt::ShiftModifier:
    case  Qt::Key_BraceRight + Qt::AltModifier + Qt::ShiftModifier:
      moveCursorAndView( ( tmp_flag ? -50 : 50 ) * m_timeScale );
      break;

    // scrollPM_N - Group:Region:Move
    case  Qt::Key_Less + Qt::ControlModifier:
    case  Qt::Key_Greater + Qt::ControlModifier:
      break;

    // scrollPM_N - Group:Region:Move(Fine)
    case  Qt::Key_Less + Qt::AltModifier:
    case  Qt::Key_Greater + Qt::AltModifier:
      break;

    // scrollPM_N - Group:Region:Move(Sample)
    case  Qt::Key_Less + Qt::ShiftModifier + Qt::ControlModifier:
    case  Qt::Key_Greater + Qt::ShiftModifier + Qt::ControlModifier:
      break;

    case Qt::Key_N + Qt::AltModifier:
      tmp_flag = true;
    case Qt::Key_P + Qt::AltModifier:
      {
        OcaSmartTrack* t = qobject_cast<OcaSmartTrack*>( m_group->getActiveTrack() );
        if( NULL != t ) {
          oca_index idx = t->findSubtrack( t->getActiveSubtrack() );
          int c = t->getSubtrackCount();
          if( ( 0 <= idx ) && ( 1 < c ) ) {
            idx = ( idx + ( tmp_flag ? 1 : -1 ) ) % c;
            if( 0 > idx ) {
              idx += c;
            }
            t->setActiveSubtrack( t->getSubtrack( idx ) );
          }
        }
      }
      break;

    case Qt::Key_Home:
      showGroupStart();
      m_group->setCursorPosition( m_group->getStartTime() );
      break;

    case Qt::Key_End:
      showGroupEnd();
      m_group->setCursorPosition( m_group->getEndTime() );
      break;

    case Qt::Key_Home + Qt::ControlModifier:
    case Qt::Key_End + Qt::ControlModifier:
      {
        OcaTrackBase* w = m_group->getActiveTrack();
        if( NULL != w ) {
          double t = ( Qt::Key_Home == key_event->key() ) ? w->getStartTime() : w->getEndTime();
          if( std::isfinite( t ) ) {
            m_group->setCursorPosition( t );
            m_group->setViewPosition( t - 0.125 * m_group->getViewDuration() );
          }
        }
      }
      break;

    default:
      processed = false;
  }

  if( ! processed ) {
    QAbstractScrollArea::keyPressEvent( key_event );
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::mouseMoveEvent( QMouseEvent* event )
{
  double t = m_group->getViewPosition()
      + ( m_widget->mapFrom( viewport(), event->pos() ).x() - getTrackMargin() ) * m_timeScale;
  if( -1 != m_resizedTrack ) {
    int new_height = event->globalY() - m_resizeRef;
    if( 20 > new_height ) {
      new_height = 20;
    }
    OcaTrackBase* track = m_screens.getItem( m_resizedTrack );
    Q_ASSERT( NULL != track );
    m_group->setTrackHeight( track, new_height );

  }
  else if( 1 == m_resizedRegion ) {
    setRegionStart( t );

  }
  else if( 2 == m_resizedRegion ) {
    setRegionEnd( t );
  }
  else {
    if( -1 != checkRegionResizeArea( event->pos() ) ) {
      setCursor( Qt::SplitHCursor );
    }
    else if( -1 != checkTrackResizeArea( event->globalPos() ) ) {
      setCursor( Qt::SplitVCursor );
    }
    else {
      setCursor( Qt::ArrowCursor );
    }
  }
  QAbstractScrollArea::mouseMoveEvent( event );
}

// ------------------------------------------------------------------------------------

int OcaTrackGroupView::checkTrackResizeArea( QPoint pos, int* resize_ref /* = NULL */ )
{
  int track_idx = -1;
  for( uint idx = 0; idx < m_screens.getLength(); ++idx ) {
    OcaDataScreen* ws = NULL;
    OcaTrackBase* w = m_screens.getItem( idx, &ws );
    if( ( NULL != ws ) && ( ! w->isHidden() ) ) {
      QPoint track_pos = m_widget->mapToGlobal( ws->pos() );

      int dist = pos.y() - ( track_pos.y() + ws->height() );
      if( 0 <= dist && dist < 5 ) {
        track_idx = idx;
        if( NULL != resize_ref ) {
          *resize_ref = track_pos.y() + dist;
        }
        break;
      }
    }
  }
  return track_idx;
}

// ------------------------------------------------------------------------------------

int OcaTrackGroupView::checkRegionResizeArea( QPoint pos )
{
  double t = m_group->getViewPosition()
            + ( m_widget->mapFrom( viewport(), pos ).x() - getTrackMargin() ) * m_timeScale;
  int result = -1;
  if( 2 > fabs( t - m_group->getRegionStart() ) / m_timeScale ) {
    result = 1;
  }
  else if( 2 > fabs( t - m_group->getRegionEnd() ) / m_timeScale ) {
    result = 2;
  }
  return result;
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::mousePressEvent( QMouseEvent* event )
{
  bool processed = true;
  double t = m_group->getViewPosition()
    + ( m_widget->mapFrom( viewport(), event->pos() ).x() - getTrackMargin() ) * m_timeScale;

  switch( event->button() + event->modifiers() ) {

    // click_1
    case Qt::LeftButton:
      m_resizedRegion = checkRegionResizeArea( event->pos() );
      if( -1 == m_resizedRegion ) {
        m_resizedTrack = checkTrackResizeArea( event->globalPos(), &m_resizeRef );
        if( -1 == m_resizedTrack ) {
          m_group->setCursorPosition( t );
          setBasePosition( t );
        }
      }
      break;

    // click_N - Group:Region:Set(StartOnPos)
    case Qt::LeftButton + Qt::ControlModifier + Qt::ShiftModifier:
      setRegionStart( t );
      break;

    // click_N - Group:Region:Set(EndOnPos)
    case Qt::LeftButton + Qt::AltModifier + Qt::ShiftModifier:
      setRegionEnd( t );
      break;

    default:
      processed = false;
  }

  /*
  if( ( Qt::LeftButton == event->button() ) && ( Qt::NoModifier == event->modifiers() ) ) {
    m_resizedTrack = checkTrackResizeArea( event->globalPos(), &m_resizeRef );
    if( -1 == m_resizedTrack ) {
      setCursorPosition( m_viewPosition + event->x() * m_timeScale );
    }
  }
  */

  if( ! processed ) {
    QAbstractScrollArea::mousePressEvent( event );
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::mouseReleaseEvent( QMouseEvent* event )
{
  m_resizedTrack = -1;
  m_resizedRegion = -1;
  QAbstractScrollArea::mouseReleaseEvent( event );
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::wheelEvent( QWheelEvent* event )
{
  bool processed = true;
  static double distance = 0;
  distance += event->delta();
  const int MIN_WHEEL_STEP=40;

  int n = round(distance / MIN_WHEEL_STEP);
  distance -= n*MIN_WHEEL_STEP;
  double d = -n*MIN_WHEEL_STEP/120.0;

  setCursor( Qt::ArrowCursor );
  switch( event->orientation() + event->modifiers() ) {

    // scroll_V_2 - Group:TimeScale:Move
    case Qt::Vertical + Qt::ControlModifier:
      {
        double t = m_group->getViewPosition()
            + ( m_widget->mapFrom( this, event->pos() ).x() - getTrackMargin() ) * m_timeScale;
        setBasePosition( t, true );
        moveTimeScale( pow(STEP, -d) );
      }
      break;


    // scroll_H_1 - Group:View:Move
    case Qt::Horizontal:
      moveView( m_timeScale*20*d );
      break;

    // scroll_H_2 - Group:View:Move(Fast)
    case Qt::Horizontal + Qt::ShiftModifier:
      moveView( m_timeScale*100*d );
      break;

    // scroll_H_6 - Group:View:Move(Fine)
    case Qt::Horizontal + Qt::AltModifier + Qt::ShiftModifier:
      moveView( 0 < event->delta() ? -m_timeScale : m_timeScale );
      break;


    // scroll_H_3 - Group:Cursor:MoveWithView
    case Qt::Horizontal + Qt::ControlModifier:
      moveCursorAndView( 0 < event->delta() ? -m_timeScale * 10 : m_timeScale * 10 );
      break;

    // scroll_H_3 - Group:Cursor:MoveWithView(Fast)
    case Qt::Horizontal + Qt::ControlModifier + Qt::ShiftModifier:
      moveCursorAndView( 0 < event->delta() ? -m_timeScale * 50 : m_timeScale * 50 );
      break;

    // scroll_H_3 - Group:Cursor:MoveWithView(Fine)
    case Qt::Horizontal + Qt::ControlModifier + Qt::AltModifier:
      moveCursorAndView( 0 < event->delta() ? -m_timeScale : m_timeScale );
      break;


    default:
      processed = false;
  }

  if( ! processed ) {
    QAbstractScrollArea::wheelEvent( event );
  }
  else {
    event->accept();
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::mouseDoubleClickEvent( QMouseEvent* event )
{
  bool processed = true;

  switch( event->button() + event->modifiers() ) {
    /*
     * unconvinient - commented out
    case Qt::LeftButton:
      //moveView( ( event->x() - viewport()->width() / 2 ) * m_timeScale );
      moveView( ( m_widget->mapFrom( viewport(), event->pos() ).x() - getTrackMargin() - getTrackWidth() / 2 ) * m_timeScale );
    */

    default:
      processed = false;
  }

  if( ! processed ) {
    QAbstractScrollArea::mouseDoubleClickEvent( event );
  }
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::updateTimeScrollBar()
{
  m_timeScrollbarScale = ( m_group->getDuration() / m_timeScale + getTrackWidth() ) / TIME_SCROLLBAR_RES;
  double t_pos =  ( ( m_group->getViewPosition() - m_group->getStartTime() )
                                          / m_timeScale + getTrackWidth() ) / m_timeScrollbarScale;
  double tmp_scale = qMin( 1.0, m_timeScrollbarScale );

  m_timeScrollbarEnabled = false;
  horizontalScrollBar()->setPageStep( getTrackWidth() / tmp_scale );
  horizontalScrollBar()->setSingleStep( 10 / tmp_scale );
  horizontalScrollBar()->setValue( t_pos );
  m_timeScrollbarEnabled = true;
}

// ------------------------------------------------------------------------------------

void OcaTrackGroupView::updateVerticalScrollBar()
{
  /*
  int last_height = 0;
  if( ! m_tracks.isEmpty() ) {
    last_height = qMin( m_tracks.last()->height(), viewport()->height() / 2 );
  }
  verticalScrollBar()->setRange( 0, m_widget->height() - last_height );
  */
  verticalScrollBar()->setRange( 0, m_widget->height() );
  verticalScrollBar()->setPageStep( viewport()->height() );
  verticalScrollBar()->setSingleStep( 10 );
}

// ------------------------------------------------------------------------------------

