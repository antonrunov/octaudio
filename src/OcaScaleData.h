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

#ifndef OcaScaleData_h
#define OcaScaleData_h

class OcaScaleData
{
  public:
    OcaScaleData();
    ~OcaScaleData();

  public:
    double  getScale() const { return m_scale; }
    double  getZero() const { return m_zero; }
    double  moveScale( double step );
    double  moveZero( double step );
    bool    setScale( double scale );
    bool    setZero( double zero );

  protected:
    double  m_scale_log;
    double  m_scale;
    double  m_zero;
};

#endif // OcaScaleData_h
