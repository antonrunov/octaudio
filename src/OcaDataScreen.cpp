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

#include "OcaDataScreen.h"

#include "OcaTrackGroup.h"
#include "OcaScaleControl.h"
#include "OcaApp.h"
#include "OcaAudioController.h"
#include "OcaTrack.h"
#include "OcaObjectListener.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

// -----------------------------------------------------------------------------
// OcaDataScreen::DataBlock

OcaDataScreen::DataBlock::DataBlock()
:
  m_color( Qt::black ),
  m_scale( 0 ),
  m_zero( 0 ),
  m_dirtyFlag( false ),
  m_height( 0 ),
  m_absValueMode( false )
{
}

// -----------------------------------------------------------------------------

OcaDataScreen::DataBlock::~DataBlock()
{
}

// -----------------------------------------------------------------------------

const QVector<QLine>& OcaDataScreen::DataBlock::getLines()
{
  if( m_dirtyFlag ) {
    processData();
  }
  return m_cacheLines;
}

// -----------------------------------------------------------------------------

const QVector<QPoint>& OcaDataScreen::DataBlock::getPoints()
{
  if( m_dirtyFlag ) {
    processData();
  }
  return m_cachePoints;
}

// -----------------------------------------------------------------------------

void OcaDataScreen::DataBlock::invalidate()
{
  m_dirtyFlag = true;
  m_cacheLines.clear();
  m_cachePoints.clear();
}

// -----------------------------------------------------------------------------

void OcaDataScreen::DataBlock::setColor( const QColor& color )
{
  m_color = color;
}

// -----------------------------------------------------------------------------

void OcaDataScreen::DataBlock::setScale( double scale )
{
  if( m_scale != scale ) {
    m_scale = scale;
    invalidate();
  }
}

// -----------------------------------------------------------------------------

void OcaDataScreen::DataBlock::setZero( double zero )
{
  if( m_zero != zero ) {
    m_zero = zero;
    invalidate();
  }
}

// -----------------------------------------------------------------------------

void OcaDataScreen::DataBlock::setLines( const QVector<QLineF>& lines )
{
  m_lines = lines;
  invalidate();
}

// -----------------------------------------------------------------------------

void OcaDataScreen::DataBlock::setPoints( const QVector<QPointF>& points )
{
  m_points = points;
  invalidate();
}

// -----------------------------------------------------------------------------

void OcaDataScreen::DataBlock::setHeight( int height )
{
  if( m_height != height ) {
    m_height = height;
    invalidate();
  }
}

// -----------------------------------------------------------------------------

void OcaDataScreen::DataBlock::setAbsValueMode( bool mode )
{
  if( m_absValueMode != mode ) {
    m_absValueMode = mode;
    invalidate();
  }
}

// -----------------------------------------------------------------------------

void OcaDataScreen::DataBlock::processData() {
  double k = -m_height / 2 / m_scale;
  double offset = m_height / 2 - m_zero * k;
  if( m_absValueMode ) {
    k *= 2;
    offset = ( m_height - 1 )  - m_zero * k;
  }
  {
    m_cachePoints.resize( m_points.size() );
    QPoint* dst = m_cachePoints.data();
    QPointF* src = m_points.data();
    QPoint* dst_max = dst + m_points.size();
    while( dst < dst_max ) {
      dst->setX( src->x() );
      dst->setY( qBound( 0, qRound(src->y() * k + offset), m_height-1 ) );
      dst++;
      src++;
    }
  }
  {
    m_cacheLines.resize( m_lines.size() );
    QLine* dst = m_cacheLines.data();
    QLineF* src = m_lines.data();
    QLine* dst_max = dst + m_lines.size();
    while( dst < dst_max ) {
      int y1 = qBound( 0, qRound(src->y1() * k + offset), m_height-1 );
      int y2 = qBound( 0, qRound(src->y2() * k + offset), m_height-1 );
      int x1 = src->x1();
      int x2 = src->x2();
      if( ( y1 == y2 ) && ( x1 == x2 ) ) {
        x2++;
      }
      dst->setLine( x1, y1, x2, y2 );
      dst++;
      src++;
    }
  }
  m_dirtyFlag = false;
}


// -----------------------------------------------------------------------------
// OcaDataScreen

OcaDataScreen::OcaDataScreen( OcaTrackBase* track, OcaTrackGroup* group )
:
  m_cursorPosition( 0 ),
  m_regionStart( -1 ),
  m_regionEnd( -1 ),
  m_audioPosition( -1 ),
  m_basePosition( 0 ),
  m_absValueMode( false ),
  m_audioCursorState( e_AudioCursorNone ),
  m_group( group ),
  m_label( NULL ),
  m_active( false ),
  m_postponedFlags( 0 ),
  m_postponedGroupFlags( 0 ),
  m_listener( NULL ),
  m_viewPosition( 0.0 ),
  m_timeScale( 1.0 ),
  m_scaleControl( NULL ),
  m_zeroControl( NULL )
{
  m_scaleControl = new OcaScaleControl( this );
  m_scaleControl->setValue( track->getScale() );
  m_scaleControl->setTransparency( 0.2, 0.5 );

  m_zeroControl =  new OcaScaleControl( this );
  m_zeroControl->setValue( track->getZero() );
  m_zeroControl->setTransparency( 0.2, 0.5 );

  m_scaleControl->move( width() - m_scaleControl->width(), 0 );
  m_zeroControl->move(  width() - m_zeroControl->width(),
                        height() -  m_zeroControl->height()   );

  m_listener = new OcaObjectListener( track, OcaTrackBase::e_FlagALL, 10, this );
  m_listener->addEvent( NULL, OcaTrackBase::e_FlagALL );
  if( NULL != m_group ) {
    m_listener->addObject( m_group, OcaTrackGroup::e_FlagALL );
    m_listener->addEvent( m_group, OcaTrackGroup::e_FlagALL );
  }

  m_listener->addObject( OcaApp::getAudioController(), OcaAudioController::e_FlagStateChanged );
  m_listener->addEvent( OcaApp::getAudioController(), OcaAudioController::e_FlagStateChanged );

  connect(  m_listener,
            SIGNAL(updateRequired(uint,QHash<QString,uint>&,QList<OcaObject*>&)),
            SLOT(onUpdateRequired(uint,QHash<QString,uint>&,QList<OcaObject*>&))  );

  m_label = new QLabel( this );
  m_label->setTextFormat( Qt::PlainText );
  m_label->installEventFilter( this );
  m_label->setContextMenuPolicy( Qt::CustomContextMenu );
  connect( m_label, SIGNAL(customContextMenuRequested(const QPoint&)),
                            SLOT(openHandleContextMenu(const QPoint&))  );


  track->connect( m_scaleControl, SIGNAL(changed(double)), SLOT(setScale(double)) );
  track->connect( m_zeroControl, SIGNAL(changed(double)), SLOT(setZero(double)) );
  track->connect( m_scaleControl, SIGNAL(moved(double)), SLOT(moveScale(double)) );
  track->connect( m_zeroControl, SIGNAL(moved(double)), SLOT(moveZero(double)) );


  setMinimumHeight( 30 );
  setBackgroundRole( QPalette::Light );
  setMouseTracking ( true );
  setAcceptDrops(true);
}

// -----------------------------------------------------------------------------

OcaDataScreen::~OcaDataScreen()
{
  Q_ASSERT( m_dataBlocks.isEmpty() );
}

// -----------------------------------------------------------------------------

int OcaDataScreen::mapFromView( OcaTrackGroup* view, double t ) const
{
  return std::isfinite( t ) ? ( t - view->getViewPosition() ) / m_timeScale : -1;
}

// -----------------------------------------------------------------------------

uint OcaDataScreen::updateViewport( uint group_flags )
{
  Q_ASSERT( NULL != m_group );

  const uint viewport_flags1 =    OcaTrackGroup::e_FlagViewPositionChanged
                                | OcaTrackGroup::e_FlagTimeScaleChanged;

  const uint viewport_flags2 =    viewport_flags1
                                | OcaTrackGroup::e_FlagCursorChanged
                                | OcaTrackGroup::e_FlagRegionChanged
                                | OcaTrackGroup::e_FlagBasePositionChanged;

  const uint viewport_flags3 =    OcaTrackGroup::e_FlagTrackMoved
                                | OcaTrackGroup::e_FlagTrackRemoved;

  uint extra_flags = 0;

  if( viewport_flags1 & group_flags ) {
    extra_flags = OcaTrackBase::e_FlagTrackDataChanged;
  }

  if( viewport_flags2 & group_flags ) {
    m_viewPosition = m_group->getViewPosition();
    m_timeScale = m_group->getViewDuration() / width();
    m_cursorPosition = mapFromView( m_group, m_group->getCursorPosition() );
    if( e_AudioCursorPlayback == m_audioCursorState ) {
      m_audioPosition = mapFromView( m_group, OcaApp::getAudioController()->getPlaybackPosition() );
    }
    else if( e_AudioCursorRecording == m_audioCursorState ) {
      m_audioPosition = mapFromView( m_group, OcaApp::getAudioController()->getRecordingPosition() );
    }
    else {
      m_audioPosition = -1;
    }
    if( 0 < m_group->getRegionEnd() - m_group->getRegionStart() ) {
      m_regionStart = mapFromView( m_group, m_group->getRegionStart() );
      m_regionEnd = mapFromView( m_group, m_group->getRegionEnd() );
      if( m_regionStart == m_regionEnd ) {
        m_regionEnd++;
      }
    }
    else {
      m_regionStart = -1;
      m_regionEnd = -1;
    }
    update();
  }

  if( viewport_flags3 & group_flags ) {
    extra_flags |= OcaTrackBase::e_FlagNameChanged;
  }

  return extra_flags;
}

// -----------------------------------------------------------------------------

void OcaDataScreen::onUpdateRequired( uint flags, QHash<QString,uint>&, QList<OcaObject*>& )
{
  uint group_flags = 0;
  if( NULL != m_group ) {
    group_flags = m_listener->getFlags( m_group );
  }

  const uint metadata_flags =   OcaTrackBase::e_FlagNameChanged
                              | OcaTrackBase::e_FlagMuteChanged
                              | OcaTrackBase::e_FlagAudibleChanged
                              | OcaTrackBase::e_FlagReadonlyChanged
                              | OcaTrackBase::e_FlagSelectedChanged
                              | OcaTrackBase::e_FlagActiveSubtrackChanged;

  const uint group_handle_flags =   OcaTrackGroup::e_FlagSoloTrackChanged
                                  | OcaTrackGroup::e_FlagRecordingTracksChanged;

  uint audio_flags = m_listener->getFlags( OcaApp::getAudioController() );

  const uint monitor_flags =    OcaTrackBase::e_FlagCursorChanged
                              | OcaTrackBase::e_FlagYScaleChanged;

  OcaAudioController* controller = OcaApp::getAudioController();
  if( OcaAudioController::e_FlagStateChanged & audio_flags ) {
    if( controller->getRecordingGroup() == m_group ) {
      m_listener->setObjectMask( controller, OcaAudioController::e_FlagALL );
      m_audioCursorState = e_AudioCursorRecording;
      group_flags |= OcaTrackGroup::e_FlagCursorChanged;
    }
    else if( controller->getPlayedGroup() == m_group ) {
      m_listener->setObjectMask( controller, OcaAudioController::e_FlagALL );
      m_audioCursorState = e_AudioCursorPlayback;
      group_flags |= OcaTrackGroup::e_FlagCursorChanged;
    }
    else {
      m_listener->setObjectMask( controller, OcaAudioController::e_FlagStateChanged );
      m_audioCursorState = e_AudioCursorNone;
    }
  }
  if( OcaAudioController::e_FlagCursorChanged & audio_flags ) {
    group_flags |= OcaTrackGroup::e_FlagCursorChanged;
  }

  if( monitor_flags & flags ) {
    group_flags |= OcaTrackGroup::e_FlagCursorChanged;
  }

  if( group_flags ) {
    flags |= updateViewport( group_flags );
  }

  if( ( metadata_flags & flags ) || ( group_handle_flags & group_flags ) ) {
    // TODO
    updateHandleText();
    updateHandlePalette();
  }

  if( isVisible() ) {
    updateTrackData( flags, group_flags );
    m_postponedFlags = 0;
    m_postponedGroupFlags = 0;;
  }
  else {
    m_postponedFlags |= flags;
    m_postponedGroupFlags |= group_flags;
  }
}

// -----------------------------------------------------------------------------

int OcaDataScreen::checkX( int x ) const
{
  int result = 0;
  if( 0 > x ) {
    result = -1;
  }
  else if( width() <= x ) {
    result = 1;
  }
  return result;
}

// -----------------------------------------------------------------------------

void OcaDataScreen::paintEvent ( QPaintEvent * event )
{
  QPainter painter(this);
  painter.eraseRect( rect() );
  int start = qBound( 0, m_regionStart, width() - 1 );
  int end = qBound( 0, m_regionEnd, width() - 1 );
  if( start < end ) {
    painter.fillRect( start, 0, end - start + 1, height(), QColor(QRgb(0xdddddd)) );
  }

  for( int i = 0; i < m_dataBlocks.size(); i++ ) {
    painter.setPen( QPen( m_dataBlocks[i]->getColor(), 3 ) );
    painter.drawPoints( m_dataBlocks[i]->getPoints() );

    painter.setPen( QPen( m_dataBlocks[i]->getColor(), 1 ) );
    painter.drawLines( m_dataBlocks[i]->getLines() );
  }
  if( 0 == checkX( m_cursorPosition ) ) {
    QColor c( QRgb( 0x000000 ) );
    c.setAlphaF( 0.8 );
    painter.setPen( c );
    painter.drawLine( m_cursorPosition, 0, m_cursorPosition, height()-1 );
  }
  if( 0 == checkX( m_audioPosition ) ) {
    QColor c( ( e_AudioCursorRecording == m_audioCursorState ) ? QRgb( 0x800000 ) : QRgb( 0x008080 )  );
    painter.setPen( c );
    painter.drawLine( m_audioPosition, 0, m_audioPosition, height()-1 );
  }
  if( ! m_absValueMode ) {
    QColor c( QRgb( 0x000000 ) );
    c.setAlphaF( 0.2 );
    painter.setPen( c );
    painter.drawLine( 0, height()/2, width()-1, height()/2 );
  }

}

// -----------------------------------------------------------------------------

void OcaDataScreen::resizeEvent( QResizeEvent* event )
{
  m_scaleControl->move( width() - m_scaleControl->width(), 0 );
  if( height() > 2 * m_scaleControl->height() ) {
    m_zeroControl->show();
    m_zeroControl->move( width() - m_zeroControl->width(), height() -  m_zeroControl->height() );
  }
  else {
    m_zeroControl->hide();
  }
  if( event->oldSize().height() != height() ) {
    for( int i = 0; i < m_dataBlocks.size(); i++ ) {
      m_dataBlocks[i]->setHeight( height() );
    }
  }
  QWidget::resizeEvent( event );
}

// -----------------------------------------------------------------------------

void OcaDataScreen::mousePressEvent( QMouseEvent* event )
{
  if( NULL != m_group ) {
    m_group->setActiveTrack( getTrackObject() );
  }
  QWidget::mousePressEvent( event );
}

// -----------------------------------------------------------------------------

void OcaDataScreen::wheelEvent( QWheelEvent* event )
{
  bool processed = true;
  static double distance = 0;
  distance += event->delta();
  const int MIN_WHEEL_STEP=40;

  int n = round(distance / MIN_WHEEL_STEP);
  distance -= n*MIN_WHEEL_STEP;
  double d = -n*MIN_WHEEL_STEP/120.0;


  switch( event->orientation() + event->modifiers() ) {

    // scroll_V_3 - Track:VScale:MoveScale
    case  Qt::Vertical + Qt::ShiftModifier:
      getTrackObject()->moveScale( d );
      break;

    // scroll_V_4 - Track:VScale:MoveZero
    case  Qt::Vertical + Qt::AltModifier:
      getTrackObject()->moveZero( d );
      break;
    default:
      processed = false;
  }
  if( ! processed ) {
    QWidget::wheelEvent( event );
  }
  else {
    event->accept();
  }
}

// -----------------------------------------------------------------------------

void OcaDataScreen::dragEnterEvent( QDragEnterEvent* event )
{
  event->setDropAction( Qt::MoveAction );
  OcaObject* obj = OcaObject::getObject(
          (oca_ulong)event->mimeData()->data("application/x-octaudio-id").toULongLong() );
  if( NULL != obj ) {
    OcaTrackBase* t = qobject_cast<OcaTrackBase*>( obj );
    if( NULL != t ) {
      event->acceptProposedAction();
    }
  }
}

// -----------------------------------------------------------------------------

void OcaDataScreen::dropEvent( QDropEvent* event )
{
  OcaObject* obj = OcaObject::getObject( 
          (oca_ulong)event->mimeData()->data("application/x-octaudio-id").toULongLong() );
  if( ( NULL != obj ) && ( NULL != m_group ) ) {
    OcaTrackBase* t = qobject_cast<OcaTrackBase*>( obj );
    if( NULL != t ) {
      oca_index idx = m_group->getTrackIndex( getTrackObject() );
      if( ( -1 != idx ) && ( -1 != m_group->getTrackIndex( t ) ) ) {
        event->acceptProposedAction();
        m_group->moveTrack( t, idx );
      }
    }
  }
}

// -----------------------------------------------------------------------------

void OcaDataScreen::showEvent( QShowEvent * event )
{
  QWidget::showEvent( event );
  if( ( 0 != m_postponedFlags ) || ( 0 != m_postponedGroupFlags ) ) {
    updateTrackData( m_postponedFlags, m_postponedGroupFlags );
    m_postponedFlags = 0;
    m_postponedGroupFlags = 0;;
  }
}

// -----------------------------------------------------------------------------

bool OcaDataScreen::eventFilter( QObject* obj, QEvent* ev )
{
  bool processed = false;
  if( m_label == obj ) {
    if( QEvent::Enter == ev->type() ) {
      m_active = true;
      updateHandlePalette();
    }
    else if( QEvent::Leave == ev->type() ) {
      m_active = false;
      updateHandlePalette();
    }
    else if( QEvent::MouseMove == ev->type() ) {
      QDrag *drag = new QDrag( this );
      QMimeData *mimeData = new QMimeData;
      mimeData->setText( getTrackObject()->getName() );
      mimeData->setData( "application/x-octaudio-id",
                          QByteArray::number( (qulonglong)getTrackObject()->getId() ) );
      drag->setMimeData(mimeData);
      drag->exec( /*Qt::CopyAction |*/ Qt::MoveAction);
      processed = true;
    }
    else if( QEvent::MouseButtonPress == ev->type() ) {
      if( NULL != m_group ) {
        m_group->setActiveTrack( getTrackObject() );
      }

      processed = true;
    }
  }
  return processed;
}

// -----------------------------------------------------------------------------

void OcaDataScreen::moveUpTrack()
{
  oca_index idx = m_group->getNextVisibleTrack( getTrackObject(), -1 );
  if( -1 != idx ) {
    m_group->moveTrack( getTrackObject(), idx );
  }
}

// -----------------------------------------------------------------------------

void OcaDataScreen::moveDownTrack()
{
  oca_index idx = m_group->getNextVisibleTrack( getTrackObject(), 1 );
  if( -1 != idx ) {
    m_group->moveTrack( getTrackObject(), idx );
  }
}

// -----------------------------------------------------------------------------

void OcaDataScreen::soloTrack( bool on )
{
  Q_ASSERT( NULL != m_group );
  if( on ) {
    m_group->setSoloTrack( getTrackObject() );
  }
  else if( m_group->getSoloTrack() == getTrackObject() ) {
    m_group->setSoloTrack( NULL );
  }
}

// -----------------------------------------------------------------------------

void OcaDataScreen::hideTrack()
{
  getTrackObject()->setHidden( true );
}

// -----------------------------------------------------------------------------

bool OcaDataScreen::isSolo() const
{
  bool result = false;
  if( NULL != m_group ) {
    result = ( NULL != m_group->getSoloTrack() );
    result = result && ( m_group->getSoloTrack() == getTrackObject() );
  }
  return result;
}

// -----------------------------------------------------------------------------

bool OcaDataScreen::isSoloMode() const
{
  bool result = false;
  if( NULL != m_group ) {
    result = ( NULL != m_group->getSoloTrack() );
  }
  return result;
}

// -----------------------------------------------------------------------------

void OcaDataScreen::clearBlocks( QList<DataBlock*>* list )
{
  while( ! list->isEmpty() ) {
    delete list->takeFirst();
  }
}

// -----------------------------------------------------------------------------

void OcaDataScreen::updateBlocks(   QList<DataBlock*>* list,
                                    const OcaTrack* track,
                                    double zero,
                                    double scale,
                                    QColor color      )

{
  clearBlocks( list );
  double duration = width() * m_timeScale;
  double dt = 1.0 / track->getSampleRate();

  OcaBlockListAvg data;
  long decimation = floor( track->getSampleRate() * m_timeScale );
  decimation = track->getAvgData( &data, m_viewPosition, duration + 2*dt, decimation );

  for( int idx = 0; idx < data.getSize(); idx++ ) {
    const OcaAvgVector* src_block = data.getBlock( idx );
    double t = data.getTime( idx );
    if( 0 == src_block->length() ) {
      continue;
    }
    for( int ch=0; ch < src_block->channels(); ch++ ) {
      DataBlock* dst_block = new DataBlock;
      dst_block->setZero( zero );
      dst_block->setScale( scale );
      dst_block->setHeight( height() );
      dst_block->setColor( color );
      dst_block->setAbsValueMode( m_absValueMode );

      double x = ( t - m_viewPosition ) / m_timeScale;
      double dx = decimation / ( track->getSampleRate() * m_timeScale );
      bool force_points = false;

      QVector<QLineF> lines;
      if( 1 < decimation ) {
        int x_min = qMax( 0, (int)floor(x) );
        int x_max = qMin( width()-1, (int)ceil(x + dx * src_block->length() ) ) + 1;
        int x_cur = x_min;
        if( x_min < x_max ) {
          lines.resize( x_max - x_min );
          QLineF* l = lines.data();
          const OcaAvgData* v = src_block->constData() + ch;
          double v_min = v->min;
          double v_max = v->max;
          for( int i = 0; i < src_block->length(); i++ ) {
            if( floor(x + dx) > x_cur ) {
              Q_ASSERT( x_cur < x_max );
              v_max = qMax( v_max, v->min );
              v_min = qMin( v_min, v->max );
              l->setLine( x_cur, v_min, x_cur, v_max );
              v_min = v->min;
              v_max = v->max;
              l++;
              if( ++x_cur == x_max ) {
                break;
              }
            }
            else {
              v_min = qMin( v_min, v->min );
              v_max = qMax( v_max, v->max );
            }
            v += src_block->channels();
            x += dx;
          }
          if( x_cur < x_max ) {
            l->setLine( x_cur, v_min, x_cur, v_max );
          }
        }
      }
      /*
      else if( 2 > dx  ){
      }
      */
      else {
        int idx_start = qBound( 0, (int)floor( -x / dx ), (int)src_block->length() - 1 );
        int idx_end = qBound( 0, (int)ceil( ( width() - 1 - x ) / dx ), (int)src_block->length() - 1 );
        Q_ASSERT( idx_start <= idx_end );
        int len = idx_end - idx_start;
        const OcaAvgData* v = src_block->constData() + idx_start * src_block->channels() + ch;
        x += idx_start * dx;
        if( 0 < len ) {
          lines.resize( len );
          QLineF* l = lines.data();
          for( int i = 0; i < len; i++ ) {
            l->setLine( round(x), v->avg, round(x+dx), (v + src_block->channels())->avg );
            x += dx;
            v += src_block->channels();
            l++;
          }
        }
        else {
          force_points = true;
        }
      }
      dst_block->setLines( lines );

      x = ( t - m_viewPosition ) / m_timeScale;
      if( ( 0.2 > track->getSampleRate() * m_timeScale ) || force_points ) {
        QVector<QPointF> points( src_block->length() );
        const OcaAvgData* v = src_block->constData() + ch;
        QPointF* p = points.data();
        for( int i = 0; i < src_block->length(); i++ ) {
          p->setX( round(x) );
          p->setY( v->avg );
          v += src_block->channels();
          x += dx;
          p++;
        }
        dst_block->setPoints( points );
      }
      list->append( dst_block );
    }
  }

}

// -----------------------------------------------------------------------------

