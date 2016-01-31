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

#ifndef OcaValidatorDouble_h
#define OcaValidatorDouble_h

#include <QDoubleValidator>

class OcaValidatorDouble : public QDoubleValidator
{
  Q_OBJECT ;

  public:
    OcaValidatorDouble( double min, double max, int decimals );
    virtual ~OcaValidatorDouble();

  public:
    virtual void  fixup ( QString & input ) const;
    void setFixupValue( double value );

  protected:
    double m_fixupValue;
};

#endif // OcaValidatorDouble_h
