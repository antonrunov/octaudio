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

#ifndef OcaDataScreen_h
#define OcaDataScreen_h

#include <QWidget>
#include <QVector>
#include <QList>

class OcaScaleControl;
class OcaTrack;
class OcaObjectListener;
class OcaObject;
class OcaTrackGroup;
class OcaTrackBase;
class QLabel;

class OcaDataScreen : public QWidget
{
  Q_OBJECT ;

  public:
    OcaDataScreen( OcaTrackBase* track, OcaTrackGroup* group );
    ~OcaDataScreen();

  protected:
    virtual uint updateViewport( uint group_flags );

  protected:
    int   checkX( int x ) const;
    int   mapFromView( OcaTrackGroup* view, double t ) const;

  protected:
    virtual void paintEvent ( QPaintEvent * event );
    virtual void resizeEvent( QResizeEvent* event );
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void wheelEvent( QWheelEvent* event );
    virtual void dragEnterEvent( QDragEnterEvent* event );
    virtual void dropEvent( QDropEvent* event );
    virtual void showEvent( QShowEvent * event );

    //virtual void keyPressEvent( QKeyEvent* key_event );
    //virtual void enterEvent( QEvent* event );
    //virtual void leaveEvent( QEvent* event );

  public:
    virtual bool eventFilter( QObject* obj, QEvent* ev );

  protected slots:
    void moveUpTrack();
    void moveDownTrack();
    void soloTrack( bool on );
    void hideTrack();

  protected slots:
    virtual void openHandleContextMenu( const QPoint& pos ) = 0;
    virtual void onUpdateRequired(  uint flags,
                                    QHash<QString,uint>& cum_flags,
                                    QList<OcaObject*>& objects      );
  protected:
    class DataBlock
    {
      public:
        DataBlock();
        ~DataBlock();

      public:
        const QVector<QLine>&     getLines();
        const QVector<QPoint>&    getPoints();
        const QColor&             getColor() const { return m_color; }

        void  invalidate();
        void  setColor( const QColor& color );
        void  setScale( double scale );
        void  setZero( double zero );
        void  setLines( const QVector<QLineF>& lines );
        void  setPoints( const QVector<QPointF>& points );
        void  setHeight( int height );
        void  setAbsValueMode( bool mode );

      protected:
        void  processData();

      protected:
        QVector<QLineF>   m_lines;
        QVector<QPointF>  m_points;
        QColor            m_color;
        double            m_scale;
        double            m_zero;
        bool              m_dirtyFlag;
        int               m_height;
        bool              m_absValueMode;
        QVector<QLine>    m_cacheLines;
        QVector<QPoint>   m_cachePoints;
    };

  protected:
    bool  isSolo() const;
    bool  isSoloMode() const;

  protected:
    static void clearBlocks( QList<DataBlock*>* list );
    void updateBlocks(  QList<DataBlock*>* list,
                        const OcaTrack* track,
                        double zero,
                        double scale,
                        QColor color                  );

  protected:
    virtual OcaTrackBase* getTrackObject() const = 0;
    virtual void updateHandleText() = 0;
    virtual void updateHandlePalette() = 0;
    virtual void updateTrackData(  uint flags, uint group_flags ) = 0;

  protected:
    QList<DataBlock*>     m_dataBlocks;
    int                   m_cursorPosition;
    int                   m_regionStart;
    int                   m_regionEnd;
    int                   m_audioPosition;
    int                   m_basePosition;
    bool                  m_absValueMode;
    bool                  m_solo;
    int                   m_audioCursorState;

    enum EAudioCursorState {
      e_AudioCursorNone       = 0,
      e_AudioCursorPlayback,
      e_AudioCursorRecording,
    };

  protected:
    OcaTrackGroup*        m_group;
    QLabel*               m_label;
    bool                  m_active;
    int                   m_postponedFlags;
    int                   m_postponedGroupFlags;

  protected:
    OcaObjectListener*  m_listener;

    double              m_viewPosition;
    double              m_timeScale;
    OcaScaleControl*    m_scaleControl;
    OcaScaleControl*    m_zeroControl;


};

#endif // OcaDataScreen_h
