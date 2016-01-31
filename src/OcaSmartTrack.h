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

#ifndef OcaSmartTrack_h
#define OcaSmartTrack_h

#include "octaudio.h"
#include "OcaTrackBase.h"
#include "OcaList.h"

#include <QColor>

class OcaTrack;

class OcaSmartTrack : public OcaTrackBase
{
  Q_OBJECT ;
  Q_PROPERTY( bool common_scale READ isCommonScaleOn  WRITE setCommonScaleOn );
  Q_PROPERTY( OcaTrack* active_subtrack READ getActiveSubtrack WRITE setActiveSubtrack );
  Q_PROPERTY( double aux_transparency READ getAuxTransparency WRITE setAuxTransparency );

  public:
    OcaSmartTrack( const QString& name );
    virtual ~OcaSmartTrack();

  public:
    bool        isCommonScaleOn() const { return m_commonScale; }
    uint        getSubtrackCount() const;
    oca_index   findSubtrack( OcaTrack* t ) const;
    bool        isAutoScaleOn() const;

  public:
    OcaTrack*   getActiveSubtrack() const;
    OcaTrack*   getSubtrack( oca_index idx ) const;
    QList<OcaTrack*> findSubtracks( const QString& name ) const;
    double      getAuxTransparency() const { return m_auxTransparency; }

  public slots:
    void        setCommonScaleOn( bool on );
    bool        setActiveSubtrack( OcaTrack* t );
    oca_index   addSubtrack( OcaTrack* track );
    oca_index   removeSubtrack( OcaTrack* t );
    oca_index   moveSubtrack( OcaTrack* t, oca_index idx );
    bool        setAutoScaleOn( bool on );
    bool        setAuxTransparency( double value );

  // subtrack operations
  public:
    QColor  getSubtrackColor( OcaTrack* t ) const;
    double  getSubtrackScale( OcaTrack* t ) const;
    double  getSubtrackZero( OcaTrack* t ) const;
    bool    isSubtrackAutoScaleOn( OcaTrack* t ) const;

  public slots:
    bool    setSubtrackColor( OcaTrack* t, QColor color );
    bool    setSubtrackScale( OcaTrack* t, double scale );
    bool    setSubtrackZero( OcaTrack* t, double zero );
    bool    moveSubtrackScale( OcaTrack* t, double step );
    bool    moveSubtrackZero( OcaTrack* t, double step );
    bool    setSubtrackAutoScaleOn( OcaTrack* t, bool on );

  protected:
    struct SubtrackInfo {
      OcaTrack*       track;
      OcaScaleData    scaleData;
      bool            autoScale;
      QColor          color;
    };


  protected:
    OcaList<OcaTrack,SubtrackInfo>            m_subtracks;

  protected:
    const OcaScaleData* getScaleData() const;
    OcaScaleData* getScaleData();
    uint updateDuration();

  protected slots:
    void onSubtrackClosed( OcaObject* obj );
    void onSubtrackChanged( OcaObject* obj, uint flags );

  protected:
    virtual void onClose();

  protected:
    SubtrackInfo*   m_activeSubtrack;

  protected:
    bool    m_commonScale;
    bool    m_autoScale;
    double  m_startTime;
    double  m_duration;
    int     m_addIdx;
    double  m_auxTransparency;

  // reimplemented
  public:
    virtual const   OcaTrack* getCurrentTrack() const;
    virtual bool    isAudible() const;
    virtual double  getZero() const;
    virtual double  getScale() const;
    virtual double  getStartTime() const { return m_startTime; }
    virtual double  getEndTime() const { return m_startTime + m_duration; }
    virtual double  getDuration() const { return m_duration; }

  public slots:
    virtual void setScale( double scale );
    virtual void setZero( double zero );
    virtual void moveScale( double step );
    virtual void moveZero( double step );

};

#endif // OcaSmartTrack_h
