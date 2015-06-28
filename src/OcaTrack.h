/*
   Copyright 2013-2015 Anton Runov

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

#ifndef OcaTrack_h
#define OcaTrack_h

#include "octaudio.h"
#include "OcaTrackBase.h"
#include "OcaBlockList.h"

#include <QMap>
#include <QPair>
#include <QList>

const double Oca_TIME_TOLERANCE = 1.0e-5 / 7;
class OcaTrackDataBlock;

typedef OcaBlockList<OcaDataVector>  OcaBlockListData;
typedef OcaBlockList<OcaAvgVector>   OcaBlockListAvg;
typedef QList< QPair<double,long> >  OcaBlockListInfo;

class OcaTrack : public OcaTrackBase
{
  Q_OBJECT ;
  Q_PROPERTY( bool muted READ isMuted WRITE setMute );
  Q_PROPERTY( bool readonly READ isReadonly WRITE setReadonly );
  Q_PROPERTY( double rate READ getSampleRate WRITE setSampleRate );
  Q_PROPERTY( bool audible READ isAudible WRITE setAudible );
  Q_PROPERTY( double gain READ getGain WRITE setGain );
  Q_PROPERTY( double stereo_pan READ getStereoPan WRITE setStereoPan );
  Q_PROPERTY( double start READ getStartTime WRITE setStartTime );
  //Q_PROPERTY( bool stereo );

  public:
    OcaTrack( const QString& name, double sr );
    virtual ~OcaTrack();

  public:
    bool isMuted() const { return m_muted; }
    bool isReadonly() const { return m_readonly; }
    virtual const OcaTrack* getCurrentTrack() const { return this; }
    double getSampleRate() const { return m_sampleRate; }
    bool setSampleRate( double rate );
    virtual bool isAudible() const { return m_audible; }
    double getGain() const { return m_gain; }
    void setGain( double gain );
    double getStereoPan() const { return m_stereoPan; }
    void setStereoPan( double pan );
    virtual double getZero() const { return m_scaleData.getZero(); }
    virtual double getScale() const { return m_scaleData.getScale(); }

    virtual double getStartTime() const { return m_startTime; }
    virtual double getEndTime() const { return m_startTime + m_duration; }
    virtual double getDuration() const { return m_duration; }

    bool setStartTime( double t );

    void getDataBlocksInfo( OcaBlockListInfo* info, double t0, double duration ) const;

    void getData( OcaBlockListData* dst, double t0, double duration ) const;
    long getAvgData( OcaBlockListAvg* dst, double t0,
                     double duration, long decimation_hint ) const;
    double setData( const OcaDataVector* src, double t0 );
    void deleteData( double t0, double duration );
    void cutData( OcaBlockListData* dst, double t0, double duration );

    bool validateBlocks() const;

  public slots:
    virtual void setScale( double scale );
    virtual void setZero( double zero );
    virtual void moveScale( double factor );
    virtual void moveZero( double relative_step );
    void setMute( bool on );
    void setReadonly( bool readonly );
    void setAudible( bool on );

  protected:
    double    m_sampleRate;
    double    m_startTime;
    double    m_duration;
    bool      m_muted;
    bool      m_readonly;
    bool      m_audible;
    double    m_gain;
    double    m_stereoPan;

  protected:
    QMap<double,OcaTrackDataBlock*>   m_blocks;

  protected:
    uint updateDuration();

    class DstWrapper;
    void getDataInternal( DstWrapper* dst, double t0, double duration ) const;

    QMap<double,OcaTrackDataBlock*>::const_iterator findBlock( double t0,
                                                                bool bottom_allowed  ) const;
    struct Range {
      long start;
      long end;
      bool isValid() const { return ( 0 < end ); }
    };
    Range getRange( const OcaTrackDataBlock* block, double dt, double duration ) const;
};

#endif // OcaTrack_h
