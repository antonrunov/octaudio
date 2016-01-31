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

#ifndef OcaInstance_h
#define OcaInstance_h

#include "OcaObject.h"

class OcaOctaveController;
class OcaWindowData;

class OcaInstance : public OcaObject
{
  Q_OBJECT ;

  public:
    OcaInstance();
    ~OcaInstance();
    void setWindow( OcaWindowData* window_data );

  public:
    OcaWindowData*  getWindowData() const { return m_mainWindowData; }
    QString         getDataCacheBase() const;
    bool            setDataCacheBase( const QString& path );

  public:
    enum EFlags {
      e_FlagWindowAdded       = 0x001,
      e_FlagWindowRemoved     = 0x002,
      e_FlagCachePathChanged  = 0x004,

      e_FlagObjectRemoved     = 0x100,
      e_FlagALL               = 0xfff,
    };

  protected slots:
    void onWindowClosed();

  private:
    OcaWindowData*            m_mainWindowData;
    QString                   m_dataCacheBase;

};

#endif // OcaInstance_h
