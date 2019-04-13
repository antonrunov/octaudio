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

#include "OcaAudioController.h"

#include "OcaTrack.h"
#include "OcaTrackGroup.h"
#include "OcaRingBuffer.h"

#include <QtCore>
#include <QtGui>

#include <portaudio.h>
#include <math.h>

// -----------------------------------------------------------------------------

static int on_pa_callback(  const void*                     input_buffer,
                            void*                           output_buffer,
                            unsigned long                   frames_per_buffer,
                            const PaStreamCallbackTimeInfo* time_info,
                            PaStreamCallbackFlags           status_flags,
                            void*                           user_data           )
{
  OcaRingBuffer* ring = (OcaRingBuffer*)user_data;
  float *out = (float*)output_buffer;

  (void) time_info; /* Prevent unused variable warnings. */
  (void) status_flags;
  (void) input_buffer;

  ring->read( out, frames_per_buffer * 2 );
  return paContinue;
}

// -----------------------------------------------------------------------------

static int on_pa_recording_callback(  const void*                     input_buffer,
                                      void*                           output_buffer,
                                      unsigned long                   frames_per_buffer,
                                      const PaStreamCallbackTimeInfo* time_info,
                                      PaStreamCallbackFlags           status_flags,
                                      void*                           user_data           )
{
  OcaRingBuffer* ring = (OcaRingBuffer*)user_data;
  float *in = (float*)input_buffer;

  (void) time_info; /* Prevent unused variable warnings. */
  (void) status_flags;
  (void) input_buffer;

  ring->write( in, frames_per_buffer * 2 );
  return paContinue;
}

// -----------------------------------------------------------------------------
// OcaAudioController

OcaAudioController::OcaAudioController()
:
  m_state( e_StateStopped ),
  m_timer( NULL ),
  m_sampleRate( 44100 ),
  m_playbackCursor( NAN ),
  m_playbackStopPosition( NAN ),
  m_playbackBuffer( NULL ),
  m_groupPlay( NULL ),
  m_playbackStream( NULL ),

  m_stateRecording( e_StateStopped ),
  m_recordingCursor( NAN ),
  m_recordingStopPosition( NAN ),
  m_recordingBuffer( NULL ),
  m_groupRecording( NULL ),
  m_recordingStream( NULL ),

  m_duplexStopRequested( false ),
  m_endOfData( false ),
  m_startSkipCounter( 0 ),

  m_outputDevice( paNoDevice ),
  m_inputDevice( paNoDevice ),

  m_startMode( 0 ),
  m_stopMode( 0 ),
  m_duplexMode( 1 )
{
  int err = Pa_Initialize();
  Q_ASSERT( paNoError == err );
  // TODO: error handling
  (void) err;
  m_timer = new QTimer( this );
  connect( m_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
}

// -----------------------------------------------------------------------------

OcaAudioController::~OcaAudioController()
{
  Q_ASSERT( e_StateStopped == m_state );
  Q_ASSERT( NULL == m_groupPlay );
  Q_ASSERT( NULL == m_playbackBuffer );
  Q_ASSERT( NULL == m_playbackStream );
}

// -----------------------------------------------------------------------------

double OcaAudioController::startDefaultPlayback( OcaTrackGroup* group )
{
  double t = NAN;
  switch( m_startMode ) {
    case 1:
      // cursor
      t = group->getCursorPosition();
      break;

    case 2:
      //start
      t = group->getStartTime();
      break;

    default:
      if( std::isfinite( group->getRegionStart() ) ) {
        t = group->getRegionStart();
      }
      else {
        t = group->getCursorPosition();
      }
  }

  double duration = INFINITY;
  if( 0 == m_stopMode ) {
    if( ( group->getRegionStart() <= t ) && ( group->getRegionEnd() > t ) ) {
      duration = group->getRegionEnd() - t;
    }
  }

  return startPlayback( group, t, duration );
}

// -----------------------------------------------------------------------------

double OcaAudioController::startPlayback( OcaTrackGroup* group, double t, double duration )
{
  bool result = true;
  uint flags = 0;

  {
    WLock lock( this );
    if( e_StateStopped != m_state ) {
      return false;
    }
    Q_ASSERT( NULL == m_groupPlay );
    Q_ASSERT( NULL != group );

    m_groupPlay = group;
    if( m_groupPlay != m_groupRecording ) {
      connectObject( m_groupPlay, SLOT(onGroupClosed(OcaObject*)), false );
    }

    if( m_groupPlay == m_groupRecording ) {
      Q_ASSERT( e_StateStopped != m_stateRecording );
      Q_ASSERT( NULL != m_recordingBuffer );
      t = m_recordingCursor + m_recordingBuffer->getAvailableLength() / 2 / m_sampleRate;
      Q_ASSERT( std::isfinite( t ) );
    }

    PaStreamParameters output_parameters;

    if( result ) {
      m_playbackCursor = t;
      m_playbackStopPosition = t + duration;
      m_playbackBuffer = new OcaRingBuffer( qRound( m_sampleRate ) * 2 );


      output_parameters.device = ( paNoDevice != m_outputDevice ) ?
                                      m_outputDevice : Pa_GetDefaultOutputDevice();
      if( output_parameters.device == paNoDevice) {
        result = false;
        fprintf(stderr,"OcaAudioController::startPlayback (ERROR): No default output device.\n");
      }
    }

    PaStream *stream = NULL;
    if( result ) {
      output_parameters.channelCount = 2;       /* stereo output */
      output_parameters.sampleFormat = paFloat32; /* 32 bit floating point output */
      output_parameters.suggestedLatency =
                    Pa_GetDeviceInfo( output_parameters.device )->defaultHighOutputLatency;
      output_parameters.hostApiSpecificStreamInfo = NULL;

      int err = Pa_OpenStream(
          &stream,
          NULL, /* no input */
          &output_parameters,
          m_sampleRate,
          paFramesPerBufferUnspecified,
          paNoFlag,
          on_pa_callback,
          m_playbackBuffer );
      if( err != paNoError ) {
        result = false;
      }
    }

    if( result ) {
      result = ( paNoError == Pa_StartStream( stream ) );
    }

    if( ! result ) {
      if( NULL != stream ) {
        Pa_CloseStream( stream );
        stream = 0;
      }
      if( NULL != m_playbackBuffer ) {
        delete m_playbackBuffer;
        m_playbackBuffer = NULL;
      }
      if( m_groupPlay != m_groupRecording ) {
        disconnectObject( m_groupPlay, false );
      }
      m_groupPlay = NULL;
      m_playbackCursor = NAN;
      flags = e_FlagStateChanged;
    }
    else {
      if( ! m_timer->isActive() ) {
        m_timer->start( 50 );
      }
      m_endOfData = false;
      m_startSkipCounter = 0;
      m_playbackStream = stream;
      m_state = e_StatePlaying;
      flags = e_FlagStateChanged | e_FlagCursorChanged;
    }
  }

  emitChanged( flags );
  return m_playbackCursor;
}

// -----------------------------------------------------------------------------

bool OcaAudioController::stopPlayback()
{
  bool result = true;
  uint flags = 0;

  {
    WLock lock( this );
    if( e_StateStopped == m_state ) {
      result = false;
    }

    if( result ) {
      if( e_StateStopped == m_stateRecording ) {
        m_timer->stop();
      }
      else if( 0 != m_duplexMode ) {
        m_duplexStopRequested = true;
      }
      Q_ASSERT( NULL != m_playbackStream );
      Q_ASSERT( NULL != m_playbackBuffer );
      int err = paNoError;
      if( e_StatePlaying == m_state ) {
        err = Pa_StopStream( m_playbackStream );
        Q_ASSERT( paNoError == err );
      }

      err = Pa_CloseStream( m_playbackStream );
      Q_ASSERT( paNoError == err );
      // TODO: error handling
      (void) err;
      m_playbackStream = NULL;

      m_playbackCursor = NAN;
      m_playbackStopPosition = NAN;
      delete m_playbackBuffer;
      m_playbackBuffer = NULL;

      if( m_groupPlay != m_groupRecording ) {
        disconnectObject( m_groupPlay, false );
      }
      m_groupPlay = NULL;
      m_state = e_StateStopped;
      flags = e_FlagStateChanged | e_FlagCursorChanged;
    }
  }

  emitChanged( flags );

  return result;
}

// -----------------------------------------------------------------------------

bool OcaAudioController::pausePlayback()
{
  bool result = true;
  uint flags = 0;

  do {
    WLock lock( this );
    if( e_StatePlaying != m_state ) {
      result = ( e_StatePaused == m_state );
      break;
    }
    Q_ASSERT( NULL != m_playbackStream );
    if( paNoError == Pa_StopStream( m_playbackStream ) ) {
      result = true;
      flags = e_FlagStateChanged;
      m_state = e_StatePaused;
    }

  } while( false );

  emitChanged( flags );

  return result;
}

// -----------------------------------------------------------------------------

bool OcaAudioController::resumePlayback()
{
  bool result = true;
  uint flags = 0;

  do {
    WLock lock( this );
    if( e_StatePaused != m_state ) {
      result = ( e_StatePlaying == m_state );
      break;
    }
    Q_ASSERT( NULL != m_playbackStream );
    if( paNoError == Pa_StartStream( m_playbackStream ) ) {
      result = true;
      flags = e_FlagStateChanged;
      m_state = e_StatePlaying;
    }

  } while( false );

  emitChanged( flags );

  return result;
}

// -----------------------------------------------------------------------------

double OcaAudioController::getPlaybackPosition() const
{
  OcaLock( this );
  double t = NAN;
  if( NULL != m_playbackBuffer ) {
    t = m_playbackCursor - m_playbackBuffer->getAvailableLength() / 2 / m_sampleRate;
  }
  return t;
}

// -----------------------------------------------------------------------------

double OcaAudioController::getRecordingPosition() const
{
  OcaLock( this );
  double t = NAN;
  if( NULL != m_recordingBuffer ) {
    t = m_recordingCursor + m_recordingBuffer->getAvailableLength() / 2 / m_sampleRate;
  }
  return t;
}

// -----------------------------------------------------------------------------

double OcaAudioController::startDefaultRecording( OcaTrackGroup* group )
{
  double t = NAN;
  switch( m_startMode ) {
    case 1:
      // cursor
      t = group->getCursorPosition();
      break;

    case 2:
      //start
      t = group->getStartTime();
      break;

    default:
      if( std::isfinite( group->getRegionStart() ) ) {
        t = group->getRegionStart();
      }
      else {
        t = group->getCursorPosition();
      }
  }

  double duration = INFINITY;
  if( 0 == m_stopMode ) {
    if( ( group->getRegionStart() <= t ) && ( group->getRegionEnd() > t ) ) {
      duration = group->getRegionEnd() - t;
    }
  }

  return startRecording( group, t, duration );
}

// -----------------------------------------------------------------------------

double OcaAudioController::startRecording( OcaTrackGroup* group, double t, double duration )
{
  bool result = false;
  uint flags = 0;
  PaStream *stream = NULL;

  WLock lock( this );
  do {
    if( e_StateStopped != m_stateRecording ) {
      return false;
    }
    Q_ASSERT( NULL == m_groupRecording );
    Q_ASSERT( NULL != group );

    m_duplexStopRequested = false;
    m_groupRecording = group;
    if( m_groupPlay != m_groupRecording ) {
      connectObject( m_groupRecording, SLOT(onGroupClosed(OcaObject*)), false );
    }

    if( m_groupPlay == m_groupRecording ) {
      Q_ASSERT( e_StateStopped != m_state );
      Q_ASSERT( NULL != m_playbackBuffer );
      t = m_playbackCursor - m_playbackBuffer->getAvailableLength() / 2 / m_sampleRate;
      Q_ASSERT( std::isfinite( t ) );
    }

    PaStreamParameters input_parameters;

    m_recordingCursor = t;
    m_recordingStopPosition = t + duration;
    m_recordingBuffer = new OcaRingBuffer( qRound( m_sampleRate ) * 2 );

    input_parameters.device = ( paNoDevice != m_inputDevice ) ?
                                    m_inputDevice : Pa_GetDefaultInputDevice();
    if (input_parameters.device == paNoDevice) {
      fprintf(stderr,"OcaAudioController::startRecording (ERROR): No default input device.\n");
      break;
    }

    input_parameters.channelCount = 2;       /* stereo input */
    input_parameters.sampleFormat = paFloat32; /* 32 bit floating point input */
    input_parameters.suggestedLatency =
                    Pa_GetDeviceInfo( input_parameters.device )->defaultHighOutputLatency;
    input_parameters.hostApiSpecificStreamInfo = NULL;

    int err = Pa_OpenStream(    &stream,
                                &input_parameters,
                                NULL, // no output
                                m_sampleRate,
                                paFramesPerBufferUnspecified,
                                paNoFlag,
                                on_pa_recording_callback,
                                m_recordingBuffer                 );

    if( err != paNoError ) {
      break;
    }

    if( paNoError != Pa_StartStream( stream ) ) {
      break;
    }

    m_recordingCursor = m_groupRecording->writeRecordingData(   m_recordingCursor,
                                                                m_recordingStopPosition,
                                                                m_recordingBuffer,
                                                                m_sampleRate,
                                                                true                       );
    if( ! std::isfinite( m_recordingCursor ) ) {
      break;
    }
    result = true;

    if( ! m_timer->isActive() ) {
      m_timer->start( 50 );
    }
    m_recordingStream = stream;
    m_stateRecording = e_StatePlaying;
    flags = e_FlagStateChanged | e_FlagCursorChanged;

  } while( false );

  if( ! result ) {
    if( NULL != stream ) {
      Pa_CloseStream( stream );
      stream = 0;
    }
    if( NULL != m_recordingBuffer ) {
      delete m_recordingBuffer;
      m_recordingBuffer = NULL;
    }
    if( m_groupRecording != m_groupPlay ) {
      disconnectObject( m_groupRecording, false );
    }
    m_groupRecording = NULL;
    m_recordingCursor = NAN;
    t = NAN;
  }

  lock.unlock();

  emitChanged( flags );
  return t;
}

// -----------------------------------------------------------------------------

bool OcaAudioController::stopRecording()
{
  bool result = true;
  uint flags = 0;

  WLock lock( this );
  do {
    if( e_StateStopped == m_stateRecording ) {
      break;
    }

    if( e_StateStopped == m_state ) {
      m_timer->stop();
    }
    Q_ASSERT( NULL != m_recordingStream );
    Q_ASSERT( NULL != m_recordingBuffer );
    int err = paNoError;
    if( e_StatePlaying == m_stateRecording ) {
      err = Pa_StopStream( m_recordingStream );
      Q_ASSERT( paNoError == err );
    }

    /*
    m_groupRecording->writeRecordingData(   m_recordingCursor,
                                            m_recordingStopPosition,
                                            m_recordingBuffer,
                                            m_sampleRate,
                                            false                       );
    */

    err = Pa_CloseStream( m_recordingStream );
    Q_ASSERT( paNoError == err );
    // TODO: error handling
    (void) err;
    m_recordingStream = NULL;

    m_recordingCursor = NAN;
    m_recordingStopPosition = NAN;
    delete m_recordingBuffer;
    m_recordingBuffer = NULL;

    if( m_groupRecording != m_groupPlay ) {
      disconnectObject( m_groupRecording, false );
    }
    m_groupRecording = NULL;
    m_stateRecording = e_StateStopped;
    flags = e_FlagStateChanged | e_FlagCursorChanged;
    result = true;
  } while( false );

  lock.unlock();

  emitChanged( flags );
  return result;
}

// -----------------------------------------------------------------------------

bool OcaAudioController::pauseRecording()
{
  bool result = true;
  uint flags = 0;

  do {
    WLock lock( this );
    if( e_StatePlaying != m_stateRecording ) {
      result = ( e_StatePaused == m_stateRecording );
      break;
    }
    Q_ASSERT( NULL != m_recordingStream );
    if( paNoError == Pa_StopStream( m_recordingStream ) ) {
      result = true;
      flags = e_FlagStateChanged;
      m_stateRecording = e_StatePaused;
    }

  } while( false );

  emitChanged( flags );

  return result;
}

// -----------------------------------------------------------------------------

bool OcaAudioController::resumeRecording()
{
  bool result = true;
  uint flags = 0;

  do {
    WLock lock( this );
    if( e_StatePaused != m_stateRecording ) {
      result = ( e_StatePlaying == m_stateRecording );
      break;
    }
    Q_ASSERT( NULL != m_recordingStream );
    if( paNoError == Pa_StartStream( m_recordingStream ) ) {
      result = true;
      flags = e_FlagStateChanged;
      m_stateRecording = e_StatePlaying;
    }

  } while( false );

  emitChanged( flags );

  return result;
}

// -----------------------------------------------------------------------------

bool OcaAudioController::setAudioModeStart( int mode )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( ( mode != m_startMode ) && ( -1 < mode ) && ( 3 > mode ) ) {
      m_startMode = mode;
      flags = e_FlagAudioModeChanged;
    }
  }
  return emitChanged( flags );
}

// -----------------------------------------------------------------------------

bool OcaAudioController::setAudioModeStop( int mode )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( ( mode != m_stopMode ) && ( -1 < mode ) && ( 2 > mode ) ) {
      m_stopMode = mode;
      flags = e_FlagAudioModeChanged;
    }
  }
  return emitChanged( flags );
}

// -----------------------------------------------------------------------------

bool OcaAudioController::setAudioModeDuplex( int mode )
{
  uint flags = 0;
  {
    WLock lock( this );
    if( ( mode != m_duplexMode ) && ( -1 < mode ) && ( 2 > mode ) ) {
      m_duplexMode = mode;
      flags = e_FlagAudioModeChanged;
    }
  }
  return emitChanged( flags );
}

// -----------------------------------------------------------------------------

bool OcaAudioController::setSampleRate( double rate ) {
  uint flags = 0;
  {
    WLock lock( this );
    if( ( m_sampleRate != rate ) && ( 0 < rate ) && ( 1e6 > rate ) ) {
      m_sampleRate = rate;
      flags = e_FlagSampleRateChanged;
    }
  }
  if( 0 != flags ) {
    stopPlayback();
    stopRecording();
  }
  return emitChanged( flags );
}

// -----------------------------------------------------------------------------

static QString get_dev_string( PaDeviceIndex idx, bool default_dev = false )
{
  QString s( "unknown" );
  const PaDeviceInfo* device_info = Pa_GetDeviceInfo( idx );
  if( NULL != device_info ) {
    const PaHostApiInfo *host_info = Pa_GetHostApiInfo( device_info->hostApi );
    s = QString("%1: %2").arg( host_info->name ).arg( device_info->name );
    if( default_dev ) {
      s = QString("default <%1>").arg( s );
    }
  }
  return s;
}

// -----------------------------------------------------------------------------

static bool is_proper_device( PaDeviceIndex idx, bool recording )
{
  bool result = false;
  const PaDeviceInfo* device_info = Pa_GetDeviceInfo( idx );
  if( NULL != device_info ) {
    if( recording ) {
      result = ( 2 <= device_info->maxInputChannels );
    }
    else {
      result = ( 2 <= device_info->maxOutputChannels );
    }
  }
  return result;
}

// -----------------------------------------------------------------------------

void OcaAudioController::checkDevices()
{
  WLock lock( this );
  if( ( e_StateStopped == m_state ) && ( e_StateStopped == m_stateRecording ) ) {
    QString output_name;
    QString input_name;
    bool remap_required = false;
    if( paNoDevice != m_outputDevice ) {
      output_name = get_dev_string( m_outputDevice );
      remap_required = true;
    }
    if( paNoDevice != m_inputDevice ) {
      input_name = get_dev_string( m_inputDevice );
      remap_required = true;
    }
    Pa_Terminate();
    int err = Pa_Initialize();
    Q_ASSERT( paNoError == err );
    // TODO: error handling
    (void) err;
    if( remap_required ) {
      m_outputDevice = paNoDevice;
      m_inputDevice = paNoDevice;
      int num_devices = Pa_GetDeviceCount();
      for( int i = 0; i < num_devices; i++ ) {
        if( is_proper_device( i, false ) && ( get_dev_string( i ) == output_name ) ) {
          m_outputDevice = i;
        }
        if( is_proper_device( i, true ) && ( get_dev_string( i ) == input_name ) ) {
          m_inputDevice = i;
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------

QStringList OcaAudioController::enumDevices( bool recording ) const
{
  OcaLock lock( this );
  int num_devices = Pa_GetDeviceCount();
  QStringList list;
  PaDeviceIndex default_dev = recording ?
                              Pa_GetDefaultInputDevice() : Pa_GetDefaultOutputDevice();
  if( paNoDevice != default_dev ) {
    list.append( get_dev_string( default_dev, true ) );
    for( int i = 0; i < num_devices; i++ ) {
      if( is_proper_device( i, recording ) ) {
        list.append( get_dev_string( i ) );
      }
    }
  }
  return list;
}

// -----------------------------------------------------------------------------

QString OcaAudioController::getDevice( bool recording ) const
{
  OcaLock lock( this );
  PaDeviceIndex current_dev = recording ? m_inputDevice : m_outputDevice;
  PaDeviceIndex default_dev = recording ?
                              Pa_GetDefaultInputDevice() : Pa_GetDefaultOutputDevice();
  QString s( "none" );
  if( paNoDevice != default_dev ) {
    if( paNoDevice != current_dev ) {
      s = get_dev_string( current_dev );
    }
    else {
      s = get_dev_string( default_dev, true );
    }
  }
  return s;
}

// -----------------------------------------------------------------------------

bool OcaAudioController::setDevice( QString dev_name, bool recording )
{
  uint flags = 0;
  {
    WLock lock( this );
    PaDeviceIndex dev = paNoDevice;
    int num_devices = Pa_GetDeviceCount();
    for( int i = 0; i < num_devices; i++ ) {
      if( is_proper_device( i, recording ) && ( get_dev_string( i ) == dev_name ) ) {
        dev = i;
        break;
      }
    }
    int* dev_old = ( recording ? &m_inputDevice : &m_outputDevice );
    if( *dev_old != dev ) {
      *dev_old = dev;
      flags = e_FlagDeviceChanged;
    }
  }
  if( 0 != flags ) {
    stopPlayback();
    stopRecording();
  }

  return emitChanged( flags );
}

// -----------------------------------------------------------------------------

bool OcaAudioController::fillPlaybackBuffer()
{
  if( ( NULL == m_playbackBuffer ) || ( NULL == m_groupPlay ) ) {
    return false;
  }

  // a tricky workaround for dropping initial parts of audio by some systems
  // we are waiting a little bit before starting feedding the audio driver
  if (4 > m_startSkipCounter) {
    if (0 < m_playbackBuffer->getReadCount()) {
      ++m_startSkipCounter;
    }
    return true;
  }

  bool duplex = ( m_groupPlay == m_groupRecording );
  m_playbackCursor = m_groupPlay->readPlaybackData( m_playbackCursor,
                                                    m_playbackStopPosition,
                                                    m_playbackBuffer,
                                                    m_sampleRate,
                                                    duplex                );
  return ( 0 < m_playbackBuffer->getAvailableLength() );
}

// -----------------------------------------------------------------------------

void OcaAudioController::onTimer()
{
  Q_ASSERT( ( e_StateStopped != m_state ) || ( e_StateStopped != m_stateRecording ) );
  bool stop_playback = false;
  bool stop_recording = false;
  uint flags = 0;
  {
    OcaLock lock( this );
    if( e_StateStopped != m_state ) {
      Q_ASSERT( NULL != m_playbackBuffer );
      Q_ASSERT( NULL != m_groupPlay );
      if( fillPlaybackBuffer() ) {
        flags |= e_FlagCursorChanged;
      }
      else {
        flags |= e_FlagCursorChanged;
        if( m_endOfData ) {
          stop_playback = true;
        }
        m_endOfData = true;
      }
    }

    if( e_StateStopped != m_stateRecording ) {
      Q_ASSERT( NULL != m_recordingBuffer );
      Q_ASSERT( NULL != m_groupRecording );
      m_recordingCursor = m_groupRecording->writeRecordingData(   m_recordingCursor,
                                                                  m_recordingStopPosition,
                                                                  m_recordingBuffer,
                                                                  m_sampleRate,
                                                                  false                       );
      if( std::isfinite( m_recordingCursor ) && ( ! m_duplexStopRequested ) ) {
        flags |= e_FlagCursorChanged;
      }
      else {
        stop_recording = true;
      }
    }
  }


  if( stop_playback  ) {
    stopPlayback();
  }
  if( stop_recording  ) {
    stopRecording();
  }

  emitChanged( flags );
}

// -----------------------------------------------------------------------------

void OcaAudioController::onGroupClosed( OcaObject* obj )
{
  if( obj == m_groupPlay ) {
    stopPlayback();
  }
  if( obj == m_groupRecording ) {
    stopRecording();
  }
}

// -----------------------------------------------------------------------------

