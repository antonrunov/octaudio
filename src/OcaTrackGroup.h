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

#ifndef OcaTrackGroup_h
#define OcaTrackGroup_h

#include "OcaObject.h"
#include "OcaList.h"
#include "OcaTrackBase.h"

#include <QHash>

class OcaTrackBase;
class OcaTrack;
class OcaRingBuffer;
class OcaTrackWriter;
class OcaTrackReader;

class OcaTrackGroup : public OcaObject
{
  Q_OBJECT ;
  Q_PROPERTY( OcaTrackBase* active_track READ getActiveTrack WRITE setActiveTrack );
  Q_PROPERTY( OcaTrackBase* solo_track READ getSoloTrack WRITE setSoloTrack );
  Q_PROPERTY( OcaTrack* rec_track1 READ getRecTrack1 WRITE setRecTrack1 );
  Q_PROPERTY( OcaTrack* rec_track2 READ getRecTrack2 WRITE setRecTrack2 );
  Q_PROPERTY( double rate READ getDefaultSampleRate WRITE setDefaultSampleRate );
  Q_PROPERTY( double start READ getStartTime );
  Q_PROPERTY( double end READ getEndTime );
  Q_PROPERTY( double duration READ getDuration );
  Q_PROPERTY( double cursor READ getCursorPosition WRITE setCursorPosition );
  Q_PROPERTY( double region_start READ getRegionStart WRITE setRegionStart );
  Q_PROPERTY( double region_end READ getRegionEnd WRITE setRegionEnd );
  Q_PROPERTY( double region_duration READ getRegionDuration );
  Q_PROPERTY( double view_position READ getViewPosition WRITE setViewPosition );
  Q_PROPERTY( double view_duration READ getViewDuration WRITE setViewDuration );

  public:
    OcaTrackGroup( const QString name, double rate );
    virtual ~OcaTrackGroup();

  public:
    enum EFlags {
      e_FlagTimeScaleChanged          = 0x00000001,
      e_FlagViewPositionChanged       = 0x00000002,
      e_FlagCursorChanged             = 0x00000004,
      e_FlagRegionChanged             = 0x00000008,
      e_FlagBasePositionChanged       = 0x00000010,
      e_FlagActiveTrackChanged        = 0x00000020,
      e_FlagTrackAdded                = 0x00000040,
      e_FlagTrackRemoved              = 0x00000080,
      e_FlagTrackMoved                = 0x00000100,
      e_FlagRecordingTracksChanged    = 0x00000200,
      e_FlagSoloTrackChanged          = 0x00000400,
      e_FlagNameChanged               = 0x00000800,
      e_FlagDefaultSampleRateChanged  = 0x00001000,

      e_FlagSoloModeChanged           = 0x00002000,
      e_FlagHiddenTracksChanged       = 0x00004000,
      e_FlagDurationChanged           = 0x00008000,
      e_FlagSelectedTracksChanged     = 0x00010000,
      e_FlagAudioTracksChanged        = 0x00020000,

      e_FlagALL                       = 0x000fffff,
    };

  public:
    // time
    void setCursorPosition( double t );
    double getCursorPosition() const { return m_cursorPosition; }

    void setView( double start, double end );
    void setViewPosition( double start );
    void setViewEnd( double end );
    void setViewCenter( double center );
    void setViewDuration( double duration );

    double getViewPosition() const { return m_viewPosition; }
    double getViewDuration() const { return m_viewPositionEnd - m_viewPosition; }
    double getViewCenterPosition() const { return ( m_viewPosition + m_viewPositionEnd ) / 2; }
    double getViewRightPosition() const { return m_viewPositionEnd; }

    void setRegion( double start, double end );
    void setRegionStart( double start ) { setRegion( start, m_regionEnd ); }
    void setRegionEnd( double end ) { setRegion( m_regionStart, end ); }
    double getRegionStart() const { return m_regionStart; }
    double getRegionEnd() const { return m_regionEnd; }
    double getRegionCenter() const { return ( m_regionStart + m_regionEnd ) / 2; }
    double getRegionDuration() const;

    double getStartTime() const { return m_startTime; }
    double getDuration() const { return m_duration; }
    double getEndTime() const { return m_startTime + m_duration; }

    // track list
    oca_index       addTrack( OcaTrackBase* track );
    oca_index       removeTrack( OcaTrackBase* track );
    oca_index       moveTrack( OcaTrackBase* track, oca_index idx );
    oca_index       getTrackIndex( OcaTrackBase* track ) const;
    OcaTrackBase*   getTrackAt( oca_index idx ) const;
    uint            getTrackCount() const;

    QList<OcaTrackBase*>          findTracks( QString name ) const;
    const QList<OcaTrackBase*>    getHiddenList() const;
    QList<const OcaTrack*>        getAudioTracks() const;

    oca_index getNextVisibleTrack( OcaTrackBase* track, int direction = 0 ) const;

    // track metadata
    int   getTrackHeight( OcaTrackBase* track ) const;
    bool  setTrackHeight( OcaTrackBase* track, int height );

    // active track
    bool setActiveTrack( OcaTrackBase* track );
    OcaTrackBase* getActiveTrack() const { return m_activeTrack; }

    // solo track
    OcaTrackBase* getSoloTrack() const { return m_soloTrack; }
    void          setSoloTrack( OcaTrackBase* track );

    // recording tracks
    OcaTrack* getRecTrack1() const { return m_recordingTrack1; }
    OcaTrack* getRecTrack2() const { return m_recordingTrack2; }
    void      setRecTrack1( OcaTrack* track );
    void      setRecTrack2( OcaTrack* track );

    bool isRecordingTrack( OcaTrack* track ) const;

    // default sample rate
    void setDefaultSampleRate( double sr );
    double getDefaultSampleRate() const { return m_defaultSampleRate; }

    // audio
    double  readPlaybackData( double t, double t_max, OcaRingBuffer* rbuff,
                                                      double rate, bool duplex );
    double  writeRecordingData( double t, double t_max, OcaRingBuffer* rbuff,
                                                        double rate, bool first );

  protected:
    void  updateAudioTracks();
    uint  updateDuration();
    uint  updateActiveTrack( bool force = false );
    uint  setViewInternal( double start, double end );
    oca_index getNextVisibleTrackInternal( OcaTrackBase* track, int direction ) const;

  protected slots:
    void onTrackClosed( OcaObject* obj );
    void onTrackChanged( OcaObject* obj, uint flags );
    void onAudioControllerEvent(OcaObject* obj, uint flags );

  protected:
    virtual void onClose();

  protected:
    double  m_startTime;
    double  m_duration;
    double  m_cursorPosition;
    double  m_regionStart;
    double  m_regionEnd;
    double  m_viewPosition;
    double  m_viewPositionEnd;

    struct TrackData {
      TrackData() : height( 100 ) {}
      int height;
    };

    OcaList<OcaTrackBase,TrackData>  m_tracks;
    OcaList<OcaTrackBase,void>  m_tracksHidden;
    OcaTrackBase*               m_activeTrack;
    double                      m_defaultSampleRate;

    OcaTrackBase*               m_soloTrack;
    OcaTrack*                   m_recordingTrack1;
    OcaTrack*                   m_recordingTrack2;
    bool                        m_autoRecordingTracks;

    QHash<const OcaTrack*,OcaTrackWriter*> m_writers;
    QHash<const OcaTrack*,OcaTrackReader*> m_readers;

};

#endif // OcaTrackGroup_h

