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

#ifndef OcaStringWrapper_h
#define OcaStringWrapper_h

#include <QString>
#include <QVariant>
#include <octave/oct.h>

#define OCA_STR( x )  OcaStringWrapper( x )
#define OCA_CSTR( x ) (const char*)OcaStringWrapper(x)

class OcaStringWrapper
{
  public:
    OcaStringWrapper() { m_wrapper = new Wrapper; }
    OcaStringWrapper( const QString& str ) { m_wrapper = new WrapperQString( str ); }
    OcaStringWrapper( const QVariant& var ) { m_wrapper = new WrapperQString( var.toString() ); }
    OcaStringWrapper( const std::string& str ) { m_wrapper = new WrapperStdString( str ); }
    OcaStringWrapper( const octave_value& str ) { m_wrapper = new WrapperOctaveValue( str ); }
    OcaStringWrapper( const char* str ) { m_wrapper = new WrapperCString( str ); }
    ~OcaStringWrapper() { delete m_wrapper; m_wrapper = NULL; }

  public:
    operator QString() const  { return m_wrapper->getQString(); }
    operator std::string() const  { return m_wrapper->getStdString(); }
    operator octave_value() const  { return m_wrapper->getOctaveValue(); }
    operator const char*() const { return m_wrapper->getCString(); }
    operator QVariant() const { return m_wrapper->getQString(); }

  protected:
    class Wrapper {
      public:
        virtual ~Wrapper() {}
      public:
        virtual QString       getQString() const { return QString(); }
        virtual std::string   getStdString() const { return std::string(); }
        virtual octave_value  getOctaveValue() const { return octave_value(); }
        virtual const char*   getCString() const { return NULL; }
    };

    Wrapper* m_wrapper;

    class WrapperQString : public Wrapper {
      public:
        WrapperQString( const QString& str ) : m_str( str ) {}
      public:
        virtual QString       getQString() const { return m_str; }
        virtual std::string   getStdString() const { return std::string( m_str.toLocal8Bit().data() ); }
        virtual octave_value  getOctaveValue() const { return octave_value( getStdString() ); }
        virtual const char*   getCString() const { m_ba = m_str.toLocal8Bit(); return  m_ba.data(); }
      protected:
        const QString       m_str;
        mutable QByteArray  m_ba;
    };

    class WrapperStdString : public Wrapper {
      public:
        WrapperStdString( const std::string& str ) : m_str( str ) {}
      public:
        virtual QString       getQString() const { return QString::fromLocal8Bit( getCString() ); }
        virtual std::string   getStdString() const { return m_str; }
        virtual octave_value  getOctaveValue() const { return octave_value( m_str ); }
        virtual const char*   getCString() const { return m_str.c_str(); }
      protected:
        const std::string& m_str;
    };

    class WrapperOctaveValue : public Wrapper {
      public:
        WrapperOctaveValue( const octave_value& str ) : m_str( str ) {}
      public:
        virtual QString       getQString() const { return QString::fromLocal8Bit( getStdString().c_str() ); }
        virtual std::string   getStdString() const { return m_str.is_string() ? m_str.string_value() : std::string() ; }
        virtual octave_value  getOctaveValue() const { return octave_value( getStdString() ); }
        virtual const char*   getCString() const { return getStdString().c_str(); }
      protected:
        const octave_value& m_str;
    };

    class WrapperCString : public Wrapper {
      public:
        WrapperCString( const char* str ) : m_str( str ) {}
      public:
        virtual QString       getQString() const { return QString::fromLocal8Bit( m_str ); }
        virtual std::string   getStdString() const { return std::string( m_str ); }
        virtual octave_value  getOctaveValue() const { return octave_value( getStdString() ); }
        virtual const char*   getCString() const { return m_str; }
      protected:
        const char* m_str;
    };
};

#endif // OcaStringWrapper_h
