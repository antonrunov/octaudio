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

#ifndef OcaAudioController_h
#define OcaAudioController_h

#include "OcaObject.h"

#include <QList>
#include <QStringList>

class OcaTrack;
class OcaTrackGroup;
class OcaRingBuffer;
class QTimer;

class OcaAudioController : public OcaObject
{
  Q_OBJECT ;

  public:
    OcaAudioController();

  protected:
    ~OcaAudioController();

  public:
    enum EState {
      e_StateStopped = 0,
      e_StatePlaying,
      e_StatePaused,
    };

  public:
    enum EFlags {
      e_FlagStateChanged        = 0x0001,
      e_FlagCursorChanged       = 0x0002,
      e_FlagAudioModeChanged    = 0x0004,
      e_FlagSampleRateChanged   = 0x0008,
      e_FlagDeviceChanged       = 0x0010,

      e_FlagALL                 = 0x00ff,
    };

  public slots:
    double startPlayback( OcaTrackGroup* group, double t, double duration );
    double startDefaultPlayback( OcaTrackGroup* group );
    bool stopPlayback();
    bool pausePlayback();
    bool resumePlayback();

    double startRecording( OcaTrackGroup* group, double t, double duration );
    double startDefaultRecording( OcaTrackGroup* group );
    bool stopRecording();
    bool pauseRecording();
    bool resumeRecording();

    void setAudioStartModeAuto() { setAudioModeStart( 0 ); }
    void setAudioStartModeCursor() { setAudioModeStart( 1 ); }
    void setAudioStartModeStart() { setAudioModeStart( 2 ); }

    void setAudioStopModeAuto() { setAudioModeStop( ( 0 == m_stopMode ) ? 1 : 0 ); }
    void setAudioStopModeDuplex() { setAudioModeDuplex( ( 0 == m_duplexMode ) ? 1 : 0 ); }

  public:
    int getState() const { return m_state; }
    int getStateRecording() const { return m_stateRecording; }
    double getPlaybackPosition() const;
    const OcaTrackGroup* getPlayedGroup() const { return m_groupPlay; }
    double getRecordingPosition() const;
    const OcaTrackGroup* getRecordingGroup() const { return m_groupRecording; }

  public:
    bool setAudioModeStart( int mode );
    bool setAudioModeStop( int mode );
    bool setAudioModeDuplex( int mode );
    int  getAudioModeStart() const { return m_startMode; }
    int  getAudioModeStop() const { return m_stopMode; }
    int  getAudioModeDuplex() const { return m_duplexMode; }

    double  getSampleRate() const { return m_sampleRate; }
    bool    setSampleRate( double rate );

  public:
    void          checkDevices();
    QStringList   enumDevices( bool recording ) const;
    QString       getDevice( bool recording ) const;
    bool          setDevice( QString dev_name, bool recording );

  protected:
    int               m_state;
    QTimer*           m_timer;
    double            m_sampleRate;
    double            m_playbackCursor;
    double            m_playbackStopPosition;
    OcaRingBuffer*    m_playbackBuffer;
    OcaTrackGroup*    m_groupPlay;
    void*             m_playbackStream;

    // recording
    int               m_stateRecording;
    double            m_recordingCursor;
    double            m_recordingStopPosition;
    OcaRingBuffer*    m_recordingBuffer;
    OcaTrackGroup*    m_groupRecording;
    void*             m_recordingStream;

    bool              m_duplexStopRequested;
    bool              m_endOfData;
    int               m_startSkipCounter;

  protected:
    int   m_outputDevice;
    int   m_inputDevice;

  protected:
    int   m_startMode;
    int   m_stopMode;
    int   m_duplexMode;

  protected:
    bool fillPlaybackBuffer();

  protected slots:
    void onGroupClosed( OcaObject* obj );
    void onTimer();

};

#endif // OcaAudioController_h
