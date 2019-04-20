/*
   Copyright 2013-2019 Anton Runov

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

#include "OcaOctaveHost.h"

#include "OcaPropProxySubtrack.h"
#include "OcaPropProxyTrack.h"
#include "OcaTrackGroup.h"
#include "OcaTrack.h"
#include "OcaSmartTrack.h"
#include "OcaStringWrapper.h"
#include "OcaInstance.h"
#include "OcaApp.h"
#include "OcaWindowData.h"
#include "OcaMonitor.h"
#include "Oca3DPlot.h"
#include "OcaAudioController.h"

#include "octaudio_configinfo.h"

#include <QtCore>

#define CHECK_OCTAVE_VERSION(major,minor) ( (OCTAVE_MAJOR_VERSION > major) || (OCTAVE_MAJOR_VERSION == major && OCTAVE_MINOR_VERSION >= minor) )

#include <octave/oct.h>
#include <octave/parse.h>
#include <octave/octave.h>
#if CHECK_OCTAVE_VERSION(4,2)
#include <octave/interpreter.h>
#else
#include <octave/toplev.h>
#endif
#include <octave/input.h>
#include <octave/quit.h>
#include <octave/cmd-edit.h>
#include <octave/oct-env.h>

Q_DECLARE_METATYPE( OcaObject* );
Q_DECLARE_METATYPE( OcaTrackBase* );
Q_DECLARE_METATYPE( OcaTrack* );
Q_DECLARE_METATYPE( OcaSmartTrack* );
Q_DECLARE_METATYPE( OcaMonitor* );
Q_DECLARE_METATYPE( Oca3DPlot* );
Q_DECLARE_METATYPE( OcaTrackGroup* );

// changing API is never nice
#if CHECK_OCTAVE_VERSION(4,4)
  #define is_empty isempty
  #define is_cellstr iscellstr
  #define is_cell iscell
  #define is_map isstruct
  #define is_vector isvector
  #define is_real_type isreal
  #define is_numeric_type isnumeric
  #define is_integer_type isinteger
  #define eval_string octave::eval_string
  #define feval octave::feval
  #define recover_from_exception octave::interpreter::recover_from_exception
#endif

#if CHECK_OCTAVE_VERSION(4,2)
  #define octave_execution_exception octave::execution_exception
  #define octave_interrupt_exception octave::interrupt_exception
#endif

// ----------------------------------------------------------------------------
// static octave functions

// @deftypefn {OctAudio Function} {retval =} __version_info__ (@var{name}, @var{version}, @var{release}, @var{date})
const char* DOC_STRING_STUB =
"-*- texinfo -*-\n"
"@deftypefn {OctAudio Function}\n"
"@end deftypefn";

QHash<OcaObject*,octave_scalar_map*>   OcaOctaveHost::s_context;
OcaOctaveHost*                         OcaOctaveHost::s_instance = NULL;

// ----------------------------------------------------------------------------

#define OCA_BUILTIN( name, doc ) \
static const char* name##_doc_string = doc; \
static octave_value_list name( const octave_value_list& args, int nargout )

// ----------------------------------------------------------------------------

static void validate_Track( const OcaTrack* track )
{
  if( NULL != track ) {
    track->validateBlocks();
  }
}

// ----------------------------------------------------------------------------

static NDArray ndarray_from_id_tmp( oca_ulong id )
{
  NDArray ar( dim_vector(1,sizeof(oca_ulong) / 2) );
  for( uint i = 0; i < sizeof(oca_ulong) / 2; i++ ) {
    ar( i ) = ( id >> ( 16*i ) ) & 0xffff;
  }
  return ar;
}

// ----------------------------------------------------------------------------

static NDArray ndarray_from_ocaobj( const OcaObject* obj )
{
  return ndarray_from_id_tmp( ( NULL == obj ) ? 0 : obj->getId() );
}

// ----------------------------------------------------------------------------

static oca_ulong id_from_ndarray_tmp( NDArray ar )
{
  oca_ulong id = 0;
  if( ar.numel() == sizeof(oca_ulong)/2 ) {
    for( uint i = 0; i < sizeof(oca_ulong)/2; i++ ) {
      id += ((oca_ulong)ar(i)) << ( 16*i );
    }
  }
  return id;
}

// ----------------------------------------------------------------------------

static bool ocaobj_from_ndarray( NDArray ar, OcaObject** obj )
{
  bool result = true;
  oca_ulong id = id_from_ndarray_tmp( ar );
  if( 0 != id ) {
    *obj = OcaObject::getObject( id );
    result = ( NULL != *obj );
  }
  return result;
}

// ----------------------------------------------------------------------------

static NDArray ndarray_from_track_info( const OcaBlockListInfo& data, double rate )
{
  NDArray ar;
  if( ! data.isEmpty() ) {
    ar = NDArray( dim_vector( 2, data.size() ) );
    double* vec = ar.fortran_vec();
    for( int i = 0; i < data.size(); i++ ) {
      *(vec++) = data.at(i).first;
      *(vec++) = data.at(i).second / rate;
    }
  }
  return ar;
}

// ----------------------------------------------------------------------------

static octave_value qvariant_to_octave_value( const QVariant& var )
{
  octave_value result;
  switch( var.type() ) {

    case QVariant::Bool:
      result = var.toBool();
      break;

    case QVariant::Double:
      result = var.toDouble();
      break;

    case QVariant::Int:
      result = var.toInt();
      break;

    case QVariant::UInt:
      result = var.toUInt();
      break;

    case QVariant::LongLong:
      result = var.toLongLong();
      break;

    case QVariant::ULongLong:
      result = var.toULongLong();
      break;

    case QVariant::String:
      result = OCA_STR( var );
      break;

    case QVariant::Color:
      {
        NDArray ar( dim_vector( 1, 3 ) );
        QColor c = var.value<QColor>();
        ar(0) = c.red();
        ar(1) = c.green();
        ar(2) = c.blue();
        result = ar;
      }
      break;

    case QVariant::UserType:
      if( qMetaTypeId<OcaObject*>() == var.userType() ) {
        result = ndarray_from_ocaobj( var.value<OcaObject*>() );
      }
      else if( qMetaTypeId<OcaTrackBase*>() == var.userType() ) {
        result = ndarray_from_ocaobj( var.value<OcaTrackBase*>() );
      }
      else if( qMetaTypeId<OcaTrack*>() == var.userType() ) {
        result = ndarray_from_ocaobj( var.value<OcaTrack*>() );
      }
      else if( qMetaTypeId<OcaTrackGroup*>() == var.userType() ) {
        result = ndarray_from_ocaobj( var.value<OcaTrackGroup*>() );
      }
      break;

    default:
      break;
  }

  return result;
}

// ----------------------------------------------------------------------------

static octave_value get_oca_property( QObject* obj, const std::string& prop_name,
                                                                QObject* obj_aux  )
{
  octave_value result;
  QVariant val;

  if( NULL != obj_aux ) {
    val = obj_aux->property( OCA_STR(prop_name) );
  }
  if( ! val.isValid() ) {
    val = obj->property( OCA_STR(prop_name) );
  }
  result = qvariant_to_octave_value( val );
  return result;
}

// ----------------------------------------------------------------------------

static void enum_oca_properties( octave_scalar_map* propmap, QObject* obj )
{
  if( NULL != obj ) {
    const QMetaObject* meta_object = obj->metaObject();
    for( int i = OcaObject::staticMetaObject.propertyOffset();
                                  i < meta_object->propertyCount(); i++ ) {
      QMetaProperty p = meta_object->property(i);
      if( p.isReadable() ) {
        propmap->assign( OCA_STR( p.name() ), qvariant_to_octave_value( p.read( obj ) ) );
      }
    }
  }
}

// ----------------------------------------------------------------------------

template <typename T>
static octave_value get_oca_properties( QList<T*> obj_list,
                                        octave_value_list args,
                                        OcaPropProxy<T>* prop_proxy = NULL )
{
  octave_value retval;

  bool all = false;
  if( 0 == args.length() ) {
    all = true;
  }
  else if( args(0).is_string() ) {
    if( args(0).string_value().empty() ) {
      all = true;
    }
    else {
      Cell c( 1, obj_list.size() );
      for( int i = 0; i < obj_list.size(); i++ ) {
        T* obj = obj_list.at( i );
        if( NULL != prop_proxy ) {
          prop_proxy->setItem( obj );
        }
        OcaLock lock( obj );
        c( 0, i ) = get_oca_property( obj, args(0).string_value(), prop_proxy );
      }
      if( 1 == obj_list.size() ) {
        retval = c( 0, 0 );
      }
      else {
        retval = c;
      }
    }
  }

  if( ! retval.is_defined() ) {
    if( 1 == obj_list.size() ) {
      T* obj = obj_list.first();
      if( NULL != prop_proxy ) {
        prop_proxy->setItem( obj );
      }
      OcaLock lock( obj );
      if( args(0).is_cellstr() ) {
        Array<std::string> names = args(0).cellstr_value();
        Cell props( 1, names.numel() );
        for( int i = 0; i < names.numel(); i++ ) {
          props( 0, i ) = get_oca_property( obj, names(i), prop_proxy );
        }
        retval = props;
      }
      if( all ) {
        octave_scalar_map props;
        enum_oca_properties( &props, obj );
        enum_oca_properties( &props, prop_proxy );
        retval = props;
      }
    }
    else if( ! obj_list.isEmpty() ) {
      error( "complex getprop for multiple objects" );
    }
  }

  return retval;
}

// ----------------------------------------------------------------------------

static QVariant octave_value_to_qvariant( const octave_value& val, int type )
{
  QVariant result;
  switch( type ) {
    case QVariant::Bool:
      if( val.is_bool_scalar() ) {
        result = val.bool_value();
      }
      break;

    case QVariant::Double:
      if( val.is_real_scalar() && val.is_double_type() ) {
        result = val.double_value();
      }
      break;

    case QVariant::Int:
      if( val.is_real_scalar() || val.is_integer_type() ) {
        result = val.int_value();
      }
      break;

    case QVariant::UInt:
      if( val.is_real_scalar() || val.is_integer_type() ) {
        result = val.uint_value();
      }
      break;

    case QVariant::LongLong:
      if( val.is_real_scalar() || val.is_integer_type() ) {
        result = (qlonglong)val.long_value();
      }
      break;

    case QVariant::ULongLong:
      if( val.is_real_scalar() || val.is_integer_type() ) {
        result = (qulonglong)val.ulong_value();
      }
      break;

    case QVariant::String:
      if( val.is_string() ) {
        result = OCA_STR( val );
      }
      break;

    case QVariant::Color:
      if( val.is_real_matrix() ) {
        int32NDArray ar = val.array_value();
        if( 3 == ar.numel() ) {
          result = QColor( ar(0), ar(1), ar(2) );
        }
      }
      break;

    default:
      if( val.is_real_matrix() && val.is_numeric_type()  ) {
        OcaObject* obj = NULL;
        if( ocaobj_from_ndarray( val.array_value(), &obj ) ) {
          if( qMetaTypeId<OcaObject*>() == type ) {
            result.setValue<OcaObject*>( obj );
          }
          else if( qMetaTypeId<OcaTrackBase*>() == type ) {
            OcaTrackBase* t = qobject_cast<OcaTrackBase*>( obj );
            if( ( NULL != t ) || ( NULL == obj ) ) {
              result.setValue<OcaTrackBase*>( t );
            }
          }
          else if( qMetaTypeId<OcaTrack*>() == type ) {
            OcaTrack* t = qobject_cast<OcaTrack*>( obj );
            if( ( NULL != t ) || ( NULL == obj ) ) {
              result.setValue<OcaTrack*>( t );
            }
          }
          else if( qMetaTypeId<OcaTrackGroup*>() == type ) {
            OcaTrackGroup* g = qobject_cast<OcaTrackGroup*>( obj );
            if( ( NULL != g ) || ( NULL == obj ) ) {
              result.setValue<OcaTrackGroup*>( g );
            }
          }
        }
      }
      break;

  }
  return result;
}

// ----------------------------------------------------------------------------

static bool set_oca_property( QObject* obj, const std::string& prop,
                                            octave_value val, QObject* obj_aux )
{
  bool result = false;
  const QMetaObject* meta_object = NULL;
  int i = -1;
  if( NULL != obj_aux ) {
    meta_object = obj_aux->metaObject();
    i = meta_object->indexOfProperty( OCA_STR( prop ) );
  }
  if( -1 == i ) {
    meta_object = obj->metaObject();
    i = meta_object->indexOfProperty( OCA_STR( prop ) );
  }
  else {
    obj = obj_aux;
  }

  if( -1 != i ) {
    QMetaProperty p = meta_object->property(i);
    if( p.isWritable() ) {
      result = p.write( obj, octave_value_to_qvariant( val, p.userType() ) );
    }
  }

  return result;
}

// ----------------------------------------------------------------------------

static bool is_prop_args_map( const octave_value_list& args ) {
  return ( 0 < args.length() ) && ( args(0).is_map() );
}

// ----------------------------------------------------------------------------

template <typename T>
static int set_oca_properties(  QList<T*> obj_list,
                                octave_value_list args,
                                OcaPropProxy<T>* prop_proxy = NULL )
{
  int result = 0;

  for( int i = 0; i < obj_list.size(); i++ ) {
    T* obj = obj_list.at( i );
    if( NULL != prop_proxy ) {
      prop_proxy->setItem( obj );
    }
    if( is_prop_args_map( args ) ) {
      octave_scalar_map props = args(0).scalar_map_value();
      for( octave_scalar_map::const_iterator it = props.begin(); it != props.end(); it++ ) {
        if( set_oca_property( obj, props.key( it ), props.contents( it ), prop_proxy ) ) {
          result++;
        }
      }
    }
    else if( ( 1 < args.length() ) && ( args(0).is_string() ) ) {
      if( set_oca_property( obj, args(0).string_value(), args(1), prop_proxy ) ) {
        result++;
      }
    }
  }
  return result;
}

// ----------------------------------------------------------------------------

template <typename Item, typename Container>
QList<Item*> get_objects( const octave_value& id_val, Container* container, bool unique = true )
{
  QList<Item*> result;
  if( id_val.is_real_matrix() && ( ! id_val.is_string() ) ) {
    OcaObject* obj = NULL;
    if( ocaobj_from_ndarray( id_val.array_value(), &obj ) ) {
      Item* item = qobject_cast<Item*>( obj );
      if( NULL != item ) {
        result.append( item );
      }
      else {
        error( "invalid id" );
      }
    }
  }
  else if( id_val.is_cell() ) {
    Cell c = id_val.cell_value();
    for( oca_index idx = 0; idx < c.numel(); idx++ ) {
      result.append( get_objects<Item,Container>( c(idx), container, false ) );
    }
  }
  else if( container && ( ! container->isNull() ) ) {
    if( ! id_val.is_defined() ) {
      Item* t = container->getActiveItem();
      if( NULL != t ) {
        result.append( t );
      }
    }
    else if( id_val.is_string() ) {
      result = container->findItems( OCA_STR( id_val ) );
      if( unique && ( 1 < result.size() ) ) {
        error( "duplicated names" );
        result.clear();
      }
    }
    else if( id_val.is_real_scalar() ) {
      oca_index idx = id_val.int_value();
      Item* t = NULL;
      if( 0 == idx ) {
        t = container->getActiveItem();
      }
      else if ( 0 < idx ) {
        t = container->getItemAt( idx - 1 );
        if( NULL == t ) {
          error( "invalid index" );
        }
      }
      if( NULL != t ) {
        result.append( t );
      }
    }
  }

  return result;
}

// ----------------------------------------------------------------------------

template <typename Item, typename Container>
Item* get_object( const octave_value& id_val, Container* container )
{
  Item* result = NULL;
  QList<Item*> list = get_objects<Item,Container>( id_val, container );
  if( 1 == list.size() ) {
    result = list.first();
  }
  else if( ! list.isEmpty() ) {
    error( "ambiguous id sepcification" );
  }

  return result;
}

// ----------------------------------------------------------------------------

static octave_value safe_arg( const octave_value_list& args, int idx )
{
  octave_value result;
  if( ( 0 <= idx ) && ( args.length() > idx ) ) {
    result = args( idx );
  }
  return result;
}

// ----------------------------------------------------------------------------

class OcaWrapperGroup
{
  public:
    OcaWrapperGroup( OcaWindowData* data ) : m_data( data ) {}
    bool            isNull() const { return ( NULL == m_data ); }
    OcaTrackGroup*  getActiveItem() const { return m_data->getActiveGroup(); }
    OcaTrackGroup*  getItemAt( oca_index idx ) const { return m_data->getGroupAt( idx ); }

    QList<OcaTrackGroup*> findItems( const QString& name ) const
    {
      return m_data->findGroups( name );
    }

  protected:
    OcaWindowData* m_data;
};

// ----------------------------------------------------------------------------

static OcaTrackGroup* id_to_group( octave_value_list args, int id )
{
  OcaWrapperGroup container( OcaOctaveHost::getWindowData() );
  return get_object<OcaTrackGroup,OcaWrapperGroup>( safe_arg(args,id), &container );
}

// ----------------------------------------------------------------------------

static QList<OcaTrackGroup*> id_to_group_list( octave_value_list args, int id )
{
  OcaWrapperGroup container( OcaOctaveHost::getWindowData() );
  return get_objects<OcaTrackGroup,OcaWrapperGroup>( safe_arg(args,id), &container );
}

// ----------------------------------------------------------------------------

class OcaWrapperMonitor
{
  public:
    OcaWrapperMonitor( OcaWindowData* data ) : m_data( data ) {}
    bool            isNull() const { return ( NULL == m_data ); }
    OcaMonitor*     getActiveItem() const { return m_data->getMonitorAt( 0 ); }
    OcaMonitor*     getItemAt( oca_index idx ) const { return m_data->getMonitorAt( idx ); }

    QList<OcaMonitor*> findItems( const QString& name ) const
    {
      return m_data->findMonitors( name );
    }

  protected:
    OcaWindowData* m_data;
};

// ----------------------------------------------------------------------------

static OcaMonitor* id_to_monitor( octave_value_list args, int id )
{
  OcaWrapperMonitor container( OcaOctaveHost::getWindowData() );
  return get_object<OcaMonitor,OcaWrapperMonitor>( safe_arg(args,id), &container );
}

// ----------------------------------------------------------------------------

static QList<OcaMonitor*> id_to_monitor_list( octave_value_list args, int id )
{
  OcaWrapperMonitor container( OcaOctaveHost::getWindowData() );
  return get_objects<OcaMonitor,OcaWrapperMonitor>( safe_arg(args,id), &container );
}

#ifdef OCA_BUILD_3DPLOT
// ----------------------------------------------------------------------------

class OcaWrapper3DPlot
{
  public:
    OcaWrapper3DPlot( OcaWindowData* data ) : m_data( data ) {}
    bool            isNull() const { return ( NULL == m_data ); }
    Oca3DPlot*      getActiveItem() const { return m_data->get3DPlotAt( 0 ); }
    Oca3DPlot*      getItemAt( oca_index idx ) const { return m_data->get3DPlotAt( idx ); }

    QList<Oca3DPlot*> findItems( const QString& name ) const
    {
      return m_data->find3DPlots( name );
    }

  protected:
    OcaWindowData* m_data;
};

// ----------------------------------------------------------------------------

static Oca3DPlot* id_to_3dplot( octave_value_list args, int id )
{
  OcaWrapper3DPlot container( OcaOctaveHost::getWindowData() );
  return get_object<Oca3DPlot,OcaWrapper3DPlot>( safe_arg(args,id), &container );
}

// ----------------------------------------------------------------------------

static QList<Oca3DPlot*> id_to_3dplot_list( octave_value_list args, int id )
{
  OcaWrapper3DPlot container( OcaOctaveHost::getWindowData() );
  return get_objects<Oca3DPlot,OcaWrapper3DPlot>( safe_arg(args,id), &container );
}
#endif //OCA_BUILD_3DPLOT

// ----------------------------------------------------------------------------

class OcaWrapperTrack
{
  public:
    OcaWrapperTrack( OcaTrackGroup* group ) : m_group( group ) {}
    bool            isNull() const { return ( NULL == m_group ); }
    OcaTrackBase*   getActiveItem() const { return m_group->getActiveTrack(); }
    OcaTrackBase*   getItemAt( oca_index idx ) const { return m_group->getTrackAt( idx ); }

    QList<OcaTrackBase*> findItems( const QString& name ) const { return m_group->findTracks( name ); }

  protected:
    OcaTrackGroup* m_group;
};

// ----------------------------------------------------------------------------

static OcaTrackBase* id_to_track( octave_value_list args, int id, int group_id,
                                                          OcaTrackGroup** grp = NULL )
{
  OcaTrackGroup* group = id_to_group( args, group_id );
  OcaWrapperTrack container( group );
  if( NULL != grp ) {
    *grp = group;
  }
  return get_object<OcaTrackBase,OcaWrapperTrack>( safe_arg(args,id), &container );
}

// ----------------------------------------------------------------------------

static QList<OcaTrackBase*> id_to_track_list( octave_value_list args, int id,
                                              int group_id, OcaTrackGroup** grp = NULL )
{
  OcaTrackGroup* group = id_to_group( args, group_id );
  OcaWrapperTrack container( group );
  if( NULL != grp ) {
    *grp = group;
  }
  return get_objects<OcaTrackBase,OcaWrapperTrack>( safe_arg(args,id), &container );
}

// ----------------------------------------------------------------------------

static OcaSmartTrack* id_to_smarttrack( octave_value_list args, int id, int group_id )
{
  octave_value group_val = safe_arg( args, group_id );
  if( group_val.is_real_scalar() && ( -1 == group_val.int_value() ) ) {
    return id_to_monitor( args, id );
  }
  OcaWrapperTrack container( id_to_group( args, group_id ) );
  OcaTrackBase* t = get_object<OcaTrackBase,OcaWrapperTrack>( safe_arg(args,id), &container );
  return qobject_cast<OcaSmartTrack*>( t );
}

// ----------------------------------------------------------------------------

template< typename T, typename T1>
QList<T*> cast_list( const QList<T1*>& src )
{
  QList<T*> result;
  for( int i = 0; i < src.size(); i++ ) {
    T* t = qobject_cast<T*>( src.at( i ) );
    if( NULL != t ) {
      result.append( t );
    }
  }
  return result;
}

// ----------------------------------------------------------------------------

class OcaWrapperTrackSimple
{
  public:
    OcaWrapperTrackSimple( OcaTrackGroup* group ) : m_group( group ) {}
    bool              isNull() const { return ( NULL == m_group ); }

    OcaTrack*   getActiveItem() const
    {
      return qobject_cast<OcaTrack*>( m_group->getActiveTrack() );
    }

    OcaTrack*   getItemAt( oca_index idx ) const
    {
      return qobject_cast<OcaTrack*>( m_group->getTrackAt( idx ) );
    }

    QList<OcaTrack*> findItems( const QString& name ) const
    {
      return cast_list<OcaTrack,OcaTrackBase>( m_group->findTracks( name ) );
    }

  protected:
    OcaTrackGroup* m_group;
};

// ----------------------------------------------------------------------------

static OcaTrack* id_to_datatrack( octave_value_list args, int id,
                                        int group_id, OcaTrackGroup** grp = NULL )
{
  OcaTrackGroup* group = id_to_group( args, group_id );
  OcaWrapperTrackSimple container( group );
  if( NULL != grp ) {
    *grp = group;
  }
  return get_object<OcaTrack,OcaWrapperTrackSimple>( safe_arg(args,id), &container );
}

// ----------------------------------------------------------------------------

static QList<OcaTrack*> id_to_datatrack_list( octave_value_list args, int id,
                                                    int group_id, OcaTrackGroup** grp = NULL )
{
  OcaTrackGroup* group = id_to_group( args, group_id );
  OcaWrapperTrackSimple container( group );
  if( NULL != grp ) {
    *grp = group;
  }
  return get_objects<OcaTrack,OcaWrapperTrackSimple>( safe_arg(args,id), &container );
}


// ----------------------------------------------------------------------------

class OcaWrapperSubtrack
{
  public:
    OcaWrapperSubtrack( OcaSmartTrack* strack ) : m_strack( strack ) {}
    bool            isNull() const { return ( NULL == m_strack ); }
    OcaTrack* getActiveItem() const { return m_strack->getActiveSubtrack(); }
    OcaTrack* getItemAt( oca_index idx ) const { return m_strack->getSubtrack( idx ); }

    QList<OcaTrack*> findItems( const QString& name ) const
    {
      return m_strack->findSubtracks( name );
    }

  protected:
    OcaSmartTrack* m_strack;
};

// ----------------------------------------------------------------------------

static OcaTrack* id_to_subtrack( octave_value_list args, int id,
                                int strack_id, int group_id, OcaSmartTrack** strk = NULL )
{
  OcaSmartTrack* strack = id_to_smarttrack( args, strack_id, group_id );
  if( NULL == strack ) {
    error( "invalid smarttrack" );
  }
  OcaWrapperSubtrack container( strack );
  if( NULL != strk ) {
    *strk = strack;
  }
  return get_object<OcaTrack,OcaWrapperSubtrack>( safe_arg(args,id), &container );
}

// ----------------------------------------------------------------------------

static QList<OcaTrack*> id_to_subtrack_list( octave_value_list args, int id,
                                  int strack_id, int group_id, OcaSmartTrack** strk = NULL )
{
  OcaSmartTrack* strack = id_to_smarttrack( args, strack_id, group_id );
  if( NULL == strack ) {
    error( "invalid smarttrack" );
  }
  OcaWrapperSubtrack container( strack );
  if( NULL != strk ) {
    *strk = strack;
  }
  return get_objects<OcaTrack,OcaWrapperSubtrack>( safe_arg(args,id), &container );
}

// ----------------------------------------------------------------------------

static NDArray get_time_spec( octave_value t_spec_val, OcaTrack* track, OcaTrackGroup* group )
{
  NDArray result;
  int type = -1;  // 0 - default, 1 - cursor, 2 - region

  if( ! t_spec_val.is_defined() ) {
    type = 0;
  }
  else if( t_spec_val.is_string() ) {
    const std::string s = t_spec_val.string_value();
    if( s.empty() ) {
      type = 0;
    }
    else if( "cursor" == s ) {
      type = 1;
    }
    else if( "region" == s ) {
      type = 2;
    }
    else if( "all" == s ) {
      result = NDArray( dim_vector( 1, 2 ) );
      result(0) = -INFINITY;
      result(1) = INFINITY;
    }
    /*
    else if( "track" == s ) {
      result = NDArray( dim_vector( 1, 2 ) );
      result(0) = track_d->getStartTime();
      result(1) = track_d->getDuration();
    }
    else if( "group" == s ) {
      result = NDArray( dim_vector( 1, 2 ) );
      result(0) = group->getStartTime();
      result(1) = group->getDuration();
    }
    */
    else {
      error( "invalid t_spec" );
    }
  }
  else if( t_spec_val.is_real_matrix() ) {
    NDArray t_spec = t_spec_val.array_value();
    if( 2 != t_spec.numel() ) {
      error( "invalid t_spec" );
    }
    else {
      result = t_spec;
    }
  }
  else {
    error( "invalid t_spec" );
  }

  if( -1 != type ) {
    result = NDArray( dim_vector( 1, 2 ) );
    if( 0 == type ) {
      type = ( 0 < group->getRegionDuration() ) ? 2 : 1;
    }
    if( 1 == type ) {
      result(1) = 0;
      result(0) = group->getCursorPosition() + 0.5 / track->getSampleRate();
    }
    else if( 2 == type ) {
      if( 0 == group->getRegionDuration() ) {
        error( "no region defined" );
      }
      else {
        result(1) = group->getRegionDuration();
        result(0) = group->getRegionStart() + 0.9 / track->getSampleRate();
      }
    }
    else {
      Q_ASSERT( false );
    }
  }

  return result;
}



// ----------------------------------------------------------------------------
// track

OCA_BUILTIN(  track_add,
              "[ID, idx ] = oca_track_add( name, [rate], [groip_id] )         # rate = sr, \"\" (default)\n"
              "[ID, idx ] = oca_track_add( name, track_ids,  [group_id]  )    # add smart track"    )
{
  octave_value_list retval;
  octave_value name = safe_arg( args, 0 );
  if( ! name.is_string() ) {
    print_usage();
  }
  else {
    OcaTrackGroup* group = id_to_group( args, 2 );
    if( NULL != group ) {
      OcaTrackBase* track = NULL;
      octave_value val = safe_arg( args, 1 );
      if( ( ! val.is_defined() ) || ( val.is_string() && val.string_value().empty() ) ) {
        track = new OcaTrack( OCA_STR(name), group->getDefaultSampleRate() );
      }
      else if( val.is_real_scalar() ) {
        track = new OcaTrack( OCA_STR(name), val.double_value() );
      }
      else {
        QList<OcaTrack*> list = id_to_datatrack_list( args, 1, -1 );
        OcaSmartTrack* strack = new OcaSmartTrack( OCA_STR(name) );
        for( int i = 0; i < list.size(); i++ ) {
          strack->addSubtrack( list.at(i) );
        }
        track = strack;
      }
      Q_ASSERT( NULL != track );
      retval(0) = ndarray_from_ocaobj( track );
      retval(1) = group->addTrack( track ) + 1;
    }
    else {
      error( "invalid group" );
    }
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  track_remove,
              "ret = oca_track_remove( [ids], [group_id] )            # returns counter"  )
{
  octave_value retval;
  QList<OcaTrackBase*> list = id_to_track_list( args, 0, 1 );
  int counter = 0;
  for( int i = 0; i < list.size(); i++ ) {
    list.at(i)->close();
    counter++;
  }
  retval = counter;
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  track_move,
              "idx = oca_track_move( idx, [id], [target_group_id], [group_id] )"  )
{
  octave_value retval;
  octave_value idx_val = safe_arg( args, 0 );
  if( ! idx_val.is_real_scalar() ) {
    print_usage();
  }
  else {
    oca_index idx = idx_val.int_value();
    OcaTrackGroup* target = id_to_group( args, 2 );
    OcaTrackBase* track = id_to_track( args, 1, 3 );
    if( NULL == target ) {
      error( "invalid target group" );
    }
    else if( NULL == track ) {
      error( "invalid track" );
    }
    else {
      oca_index idx_old = target->getTrackIndex( track );
      if( -1 == idx_old ) {
        // intergroup moving
        OcaTrackGroup* old_group = qobject_cast<OcaTrackGroup*>( track->getContainer() );
        if( ( old_group != target ) && ( NULL != old_group ) ) {
          idx_old = old_group->removeTrack( track );
          if( -1 < idx_old ) {
            target->addTrack( track );
            if( 0 < idx ) {
              target->moveTrack( track, idx - 1 );
            }
            retval = idx_old + 1;
          }
          else {
            error( "remove track failed (move)" );
          }
        }
        else {
          error( "invalid groups" );
        }
      }
      else {
        idx_old = target->moveTrack( track, idx - 1 );
        if( -1 != idx_old ) {
          retval = idx_old + 1;
        }
        else {
          retval = -1;
        }
      }
    }
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  track_list,
              "[list] = oca_track_list( [group_id] )"   )
{
  octave_value retval;
  OcaTrackGroup* group = id_to_group( args, 0 );
  if( NULL == group ) {
    error( "invalid group" );
  }
  else {
    OcaLock lock( group );
    Cell c( 1, group->getTrackCount() );
    for( uint i = 0; i < group->getTrackCount(); i++ ) {
      if( 0 != nargout ) {
        c(i) = ndarray_from_ocaobj( group->getTrackAt(i) );
      }
      else {
        printf( "%4d    %s\n", i, OCA_CSTR( group->getTrackAt(i)->getName() ) );
      }
    }
    if( 0 != nargout ) {
      retval = c;
    }
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  track_count,
              "count = oca_track_count( [group_id] )"   )
{
  octave_value retval;
  OcaTrackGroup* group = id_to_group( args, 0 );
  if( NULL == group ) {
    error( "invalid group" );
  }
  else {
    retval = group->getTrackCount();
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  track_find,
              "IDs = oca_track_find( ids, [group_id] )"   )
{
  octave_value retval;
  octave_value val_id = safe_arg( args, 0 );
  if( ! val_id.is_defined() ) {
    print_usage();
  }
  else if ( val_id.is_cell() ) {
    QList<OcaTrackBase*> tracks = id_to_track_list( args, 0, 1 );
    Cell c( 1, tracks.size() );
    for( int i = 0; i < tracks.size(); i++ ) {
      c(i) = ndarray_from_ocaobj( tracks.at(i) );
    }
    retval = c;
  }
  else {
    OcaTrackBase* track = id_to_track( args, 0, 1 );
    retval = ndarray_from_ocaobj( track );
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  track_getprop,
              "vals = oca_track_getprop( [names], [ids], [group_id] )" )
{
  octave_value retval;
  OcaTrackGroup* group = NULL;
  QList<OcaTrackBase*> list = id_to_track_list( args, 1, 2, &group );
  if( NULL != group ) {
    OcaPropProxyTrack prop_proxy( group );
    retval = get_oca_properties( list, args, &prop_proxy );
  }
  else {
    error( "invalid group" );
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  track_setprop,
              "ret = oca_track_setprop( name, val, [ids], [group_id] )\n"
              "ret = oca_track_setprop( props, [ids], [group_id] )          # props = struct( \"name1\", \"val1\", ... )"  )
{
  octave_value retval;
  int idx0 = is_prop_args_map( args ) ? 1 : 2;
  OcaTrackGroup* group = NULL;
  QList<OcaTrackBase*> list = id_to_track_list( args, idx0, idx0+1, &group );
  if( NULL != group ) {
    OcaPropProxyTrack prop_proxy( group );
    retval = set_oca_properties( list, args, &prop_proxy );
  }
  else {
    error( "invalid group" );
  }
  return retval;
}


// ----------------------------------------------------------------------------
// data

OCA_BUILTIN(  data_get,
              "[data, t0] = oca_data_get( [t_spec], [id], [group_id] )    # t_spec = [t, duration]\n"
              "     # \"\" (auto-default), \"cursor\", \"region\", \"all\""  )
{
  octave_value_list result;
  OcaTrackGroup* group = NULL;
  OcaTrack* track = id_to_datatrack( args, 1, 2, &group );
  if( NULL == track ) {
    error( "invalid track" );
  }
  else {
    Q_ASSERT( NULL != group );
    NDArray t_spec = get_time_spec( safe_arg( args, 0 ), track, group );
    NDArray ar;
    double t0_true = NAN;
    if( 2 == t_spec.numel() ) {
      OcaBlockListData data;
      track->getData( &data, t_spec(0), t_spec(1) );

      if( ! data.isEmpty() ) {
        const OcaDataVector* block = data.getBlock( 0 );
        if( 0 < block->length() ) {
          ar = NDArray( dim_vector( block->channels(), block->length() ) );
          memcpy( ar.fortran_vec(), block->constData(), ar.numel() * sizeof(double) );
          t0_true = data.getTime( 0 );
        }
      }
    }
    result(0) = ar;
    result(1) = t0_true;
  }

  return result;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  data_set,
              "t_next = oca_data_set( t, data, [id], [group_id] )"  )
{
  double t_next = NAN;
  OcaTrackGroup* group = NULL;
  OcaTrack* track = id_to_datatrack( args, 2, 3, &group );
  if( NULL == track ) {
    error( "invalid track" );
  }
  else {
    Q_ASSERT( NULL != group );
    octave_value val_t = safe_arg( args, 0 );
    octave_value val_data = safe_arg( args, 1 );
    if( track->isReadonly() ) {
      error( "readonly track '%s'", OCA_CSTR( track->getName() ) );
    }
    if( ( ! val_t.is_defined() ) || ( ! val_data.is_defined() ) ) {
      print_usage();
    }
    else if( ! val_t.is_real_scalar() ) {
      error( "invalid t" );
    }
    else if( ! val_data.is_real_type() ) {
      error( "invalid data" );
    }
    else {
      double t = val_t.double_value();
      NDArray ar = val_data.array_value();
      int channels = track->getChannels();
      int length = ar.numel();
      if( 1 < channels ) {
        if( ar.dim1() == channels ) {
          length = ar.dim2();
        }
        else {
          error( "invalid number of channels (%d)", ar.dim1() );
          length = 0;
        }
      }
      else {
        Q_ASSERT( 1 == channels );
        if( ! ar.is_vector() ) {
          error( "data is not a vector" );
          length = 0;
        }
      }
      if( 0 < length ) {
        OcaDataVector block( channels, length );
        memcpy( block.data(), ar.fortran_vec(), length * channels * sizeof(double) );
        t_next = track->setData( &block, t );

        validate_Track( track );
      }
    }
  }

  return octave_value( t_next );
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  data_clear,
              "ret = oca_data_clear( [ids], [group_id] )"   )
{
  int retval = 0;
  QList<OcaTrack*> list = id_to_datatrack_list( args, 0, 1 );
  for( int i = 0; i < list.size(); i++ ) {
    OcaTrack* track = list.at(i);
    if( track->isReadonly() ) {
      error( "readonly track '%s'", OCA_CSTR( track->getName() ) );
    }
    else {
      track->deleteData( -INFINITY, INFINITY );
      retval++;
      validate_Track( track );
    }
  }
  return octave_value( retval );
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  data_listblocks,
              "list = oca_data_listblocks( [t_spec], [id], [group_id] )"    )
{
  NDArray ar;
  OcaTrackGroup* group = NULL;
  OcaTrack* track = id_to_datatrack( args, 1, 2, &group );
  if( NULL == track ) {
    error( "invalid track" );
  }
  else {
    Q_ASSERT( NULL != group );
    octave_value t_spec_val = safe_arg( args, 0 );
    if( ! t_spec_val.is_defined() ) {
      t_spec_val = "all";
    }
    NDArray t_spec = get_time_spec( t_spec_val, track, group );
    if( 2 == t_spec.numel() ) {
      OcaBlockListInfo data;
      track->getDataBlocksInfo( &data, t_spec(0), t_spec(1) );
      ar = ndarray_from_track_info( data, track->getSampleRate() );
    }
  }

  return octave_value( ar );
}

// ----------------------------------------------------------------------------

static octave_value_list process_data_blocks( const octave_value_list& args, int nargout, bool cut )
{
  octave_value_list result;
  OcaTrackGroup* group = NULL;
  OcaTrack* track = id_to_datatrack( args, 1, 2, &group );
  if( NULL == track ) {
    error( "invalid track" );
  }
  else {
    Q_ASSERT( NULL != group );
    octave_value t_spec_val = safe_arg( args, 0 );
    NDArray t_spec = get_time_spec( t_spec_val, track, group );
    if( 2 == t_spec.numel() ) {
      OcaBlockListData data;
      if( cut ) {
        if( 0 < nargout ) {
          track->cutData( &data, t_spec(0), t_spec(1) );
        }
        else {
          track->deleteData( t_spec(0), t_spec(1) );
        }
      }
      else {
        track->getData( &data, t_spec(0), t_spec(1) );
      }

      if( ( 0 < nargout ) || ( ! cut ) ) {
        Cell c( 1, data.getSize() );
        NDArray starts( dim_vector( 1, data.getSize() ) );
        for( int i = 0; i < data.getSize(); i++ ) {
          const OcaDataVector* block = data.getBlock( i );
          if( 0 < block->length() ) {
            NDArray ar( dim_vector( block->channels(), block->length() ) );
            memcpy( ar.fortran_vec(), block->constData(), ar.numel() * sizeof(double) );
            starts(i) = data.getTime( i );
            c(i) = ar;
          }
        }
        result(0) = c;
        result(1) = starts;
      }
    }
  }
  return result;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  data_getblocks,
              "[blocks, starts] = oca_data_getblocks( [t_spec], [id], [group_id] )"   )
{
  return process_data_blocks( args, nargout, false );
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  data_delete,
              "oca_data_delete( [t_spec], [id], [group_id] )\n"
              "[blocks, starts] = oca_data_delete( [t_spec], [id], [group_id] )"   )
{
  return process_data_blocks( args, nargout, true );
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  data_split,
              "t_split = oca_data_split( [\"cursor\"], [id], [group_id] )\n"
              "t_split = oca_data_split( t, [id], [group_id] )"               )
{
  OcaTrackGroup* group = NULL;
  double t = NAN;
  OcaTrack* track = id_to_datatrack( args, 1, 2, &group );
  if( NULL == track ) {
    error( "invalid track" );
  }
  else {
    octave_value t_spec_val = safe_arg( args, 0 );
    bool use_cursor = ( ! t_spec_val.is_defined() );
    if( t_spec_val.is_real_scalar() ) {
      t = t_spec_val.double_value();
    }
    else if( t_spec_val.is_string() ) {
      if( "cursor" == t_spec_val.string_value() ) {
        use_cursor = true;
      }
    }
    if( use_cursor ) {
      t = group->getCursorPosition() + 0.5 / track->getSampleRate();
    }
    if( ! std::isfinite(t) ) {
      error( "invalid time point" );
    }
    else {
      t = track->splitBlock( t );
    }
  }
  return octave_value( t );
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  data_join,
              "ret = oca_data_join( [t_spec], [id], [group_id] )"   )
{
  OcaTrackGroup* group = NULL;
  OcaTrack* track = id_to_datatrack( args, 1, 2, &group );
  int result = 0;
  if( NULL == track ) {
    error( "invalid track" );
  }
  else {
    Q_ASSERT( NULL != group );
    octave_value t_spec_val = safe_arg( args, 0 );
    if( ! t_spec_val.is_defined() ) {
      t_spec_val = "region";
    }
    NDArray t_spec = get_time_spec( t_spec_val, track, group );
    if( 2 == t_spec.numel() ) {
      if( 0 == t_spec(1) ) {
        error( "zero duration specified" );
      }
      else {
        result = track->joinBlocks(  t_spec(0), t_spec(1) );
      }
    }
  }

  return octave_value( result );
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  data_moveblocks,
              "dt = oca_data_moveblocks( dt, [t_spec], [id], [group_id] )"   )
{
  OcaTrackGroup* group = NULL;
  OcaTrack* track = id_to_datatrack( args, 2, 3, &group );
  double dt = NAN;
  octave_value dt_val = safe_arg( args, 0 );
  if( NULL == track ) {
    error( "invalid track" );
  }
  else if( ! dt_val.is_real_scalar() ) {
    error( "invalid dt" );
  }
  else {
    Q_ASSERT( NULL != group );
    octave_value t_spec_val = safe_arg( args, 1 );
    NDArray t_spec = get_time_spec( t_spec_val, track, group );
    if( 2 == t_spec.numel() ) {
      dt = track->moveBlocks( dt_val.double_value(), t_spec(0), t_spec(1) );
    }
  }

  return octave_value( dt );
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  data_fill,
              "t_next = oca_data_fill( pattern, [t_spec], [id], [group_id] )"   )
{
  OcaTrackGroup* group = NULL;
  OcaTrack* track = id_to_datatrack( args, 2, 3, &group );
  double t_next = NAN;
  octave_value dt_pat = safe_arg( args, 0 );
  if( NULL == track ) {
    error( "invalid track" );
  }
  else if( ! dt_pat.is_real_type() ) {
    error( "invalid pattern" );
  }
  else {
    Q_ASSERT( NULL != group );
    octave_value t_spec_val = safe_arg( args, 1 );
    NDArray t_spec = get_time_spec( t_spec_val, track, group );
    if( 2 == t_spec.numel() ) {
      double t = t_spec(0);
      double dur = t_spec(1);
      if( ( ! std::isfinite( dur ) ) || ( 0.0 > dur ) ) {
        error( "invalid duration" );
      }
      NDArray pat = dt_pat.array_value();
      int channels = track->getChannels();
      int length = pat.numel();
      if( 1 < channels ) {
        if( pat.dim1() == channels ) {
          length = pat.dim2();
        }
        else {
          error( "invalid number of channels (%d)", pat.dim1() );
          length = 0;
        }
      }
      else {
        Q_ASSERT( 1 == channels );
        if( ! pat.is_vector() ) {
          error( "data is not a vector" );
          length = 0;
        }
      }
      if( 0 < length ) {
        OcaDataVector block( channels, length );
        memcpy( block.data(), pat.fortran_vec(), length * channels * sizeof(double) );
        t_next = track->setData( &block, t, dur );
      }
    }
  }

  return octave_value( t_next );
}

// ----------------------------------------------------------------------------
// group

OCA_BUILTIN(  group_add,
              "[ID, idx] = oca_group_add( name )"   )
{
  octave_value_list retval;
  octave_value name = safe_arg( args, 0 );
  if( ! name.is_string() ) {
    print_usage();
  }
  else {
    OcaTrackGroup* group = new OcaTrackGroup( OCA_STR(name),
                            OcaOctaveHost::getWindowData()->getDefaultSampleRate() );
    retval(0) = ndarray_from_ocaobj( group );
    retval(1) = OcaOctaveHost::getWindowData()->addGroup( group ) + 1;
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  group_remove,
              "ret = oca_group_remove( [ids] )"   )
{
  octave_value retval;
  QList<OcaTrackGroup*> list = id_to_group_list( args, 0 );
  int counter = 0;
  for( int i = 0; i < list.size(); i++ ) {
    list.at(i)->close();
    counter++;
  }
  retval = counter;
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  group_move,
              "idx = oca_group_move( idx, [id] )"    )
{
  octave_value retval;
  octave_value idx_val = safe_arg( args, 0 );
  if( ! idx_val.is_real_scalar() ) {
    print_usage();
  }
  else {
    oca_index idx = idx_val.int_value() - 1;
    OcaTrackGroup* group = id_to_group( args, 1 );
    if( NULL == group ) {
      error( "invalid group" );
    }
    else {
      oca_index idx_old = OcaOctaveHost::getWindowData()->moveGroup( group, idx );
      if( -1 == idx_old ) {
        error( "unknown group" );
      }
      retval = idx_old + 1;
    }
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  group_list,
              "[list] = oca_group_list()"   )
{
  octave_value retval;
  OcaWindowData* wd = OcaOctaveHost::getWindowData();
  OcaLock lock( wd );
  Cell c( 1, wd->getGroupCount() );
  for( uint i = 0; i < wd->getGroupCount(); i++ ) {
    if( 0 != nargout ) {
      c(i) = ndarray_from_ocaobj( wd->getGroupAt(i) );
    }
    else {
      printf( "%4d    %s\n", i, OCA_CSTR( wd->getGroupAt(i)->getName() ) );
    }
  }
  if( 0 != nargout ) {
    retval = c;
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  group_count,
              "count = oca_group_count()"   )
{
  octave_value retval;
  retval = OcaOctaveHost::getWindowData()->getGroupCount();
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  group_find,
              "IDs = oca_group_find( ids )"   )
{
  octave_value retval;
  octave_value val_id = safe_arg( args, 0 );
  if( ! val_id.is_defined() ) {
    print_usage();
  }
  else if ( val_id.is_cell() ) {
    QList<OcaTrackGroup*> list = id_to_group_list( args, 0 );
    Cell c( 1, list.size() );
    for( int i = 0; i < list.size(); i++ ) {
      c(i) = ndarray_from_ocaobj( list.at(i) );
    }
    retval = c;
  }
  else {
    OcaTrackGroup* group = id_to_group( args, 0 );
    retval = ndarray_from_ocaobj( group );
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  group_getprop,
              "vals = oca_group_getprop( [names], [ids] )" )
{
  octave_value retval;
  QList<OcaTrackGroup*> list = id_to_group_list( args, 1 );
  retval = get_oca_properties( list, args );
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  group_setprop,
              "ret = oca_group_setprop( name, val, [ids] )\n"
              "ret = oca_group_setprop( props, [ids] )"  )
{

  QList<OcaTrackGroup*> list = id_to_group_list( args, is_prop_args_map( args ) ? 1 : 2 );
  return octave_value( set_oca_properties( list, args ) );
}

// ----------------------------------------------------------------------------
// monitor

OCA_BUILTIN(  monitor_add,
              "[ID, idx] = oca_monitor_add( name, [group_id] )"  )
{
  octave_value_list retval;
  octave_value name = safe_arg( args, 0 );
  if( ! name.is_string() ) {
    print_usage();
  }
  else {
    OcaTrackGroup* group = id_to_group( args, 1 );
    OcaMonitor* monitor = new OcaMonitor( OCA_STR(name), group );
    retval(0) = ndarray_from_ocaobj( monitor );
    retval(1) = OcaOctaveHost::getWindowData()->addMonitor( monitor ) + 1;
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  monitor_remove,
              "ret = oca_monitor_remove( [ids] )"   )
{
  octave_value retval;
  QList<OcaMonitor*> list = id_to_monitor_list( args, 0 );
  int counter = 0;
  for( int i = 0; i < list.size(); i++ ) {
    list.at(i)->close();
    counter++;
  }
  retval = counter;
  return retval;
}

// ----------------------------------------------------------------------------

/*
OCA_BUILTIN(  monitor_move,
              "idx = oca_monitor_move( idx, [id] )"    )
{
  octave_value retval;
  return retval;
}
*/
// ----------------------------------------------------------------------------

OCA_BUILTIN(  monitor_list,
              "[list] = oca_monitor_list()"   )
{
  octave_value retval;
  OcaWindowData* wd = OcaOctaveHost::getWindowData();
  OcaLock lock( wd );
  Cell c( 1, wd->getMonitorCount() );
  for( uint i = 0; i < wd->getMonitorCount(); i++ ) {
    if( 0 != nargout ) {
      c(i) = ndarray_from_ocaobj( wd->getMonitorAt(i) );
    }
    else {
      printf( "%4d    %s\n", i, OCA_CSTR( wd->getMonitorAt(i)->getName() ) );
    }
  }
  if( 0 != nargout ) {
    retval = c;
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  monitor_count,
              "count = oca_monitor_count()"   )
{
  octave_value retval;
  retval = OcaOctaveHost::getWindowData()->getMonitorCount();
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  monitor_find,
              "IDs = oca_monitor_find( ids )"   )
{
  octave_value retval;
  octave_value val_id = safe_arg( args, 0 );
  if( ! val_id.is_defined() ) {
    print_usage();
  }
  else if ( val_id.is_cell() ) {
    QList<OcaMonitor*> list = id_to_monitor_list( args, 0 );
    Cell c( 1, list.size() );
    for( int i = 0; i < list.size(); i++ ) {
      c(i) = ndarray_from_ocaobj( list.at(i) );
    }
    retval = c;
  }
  else {
    OcaMonitor* monitor = id_to_monitor( args, 0 );
    retval = ndarray_from_ocaobj( monitor );
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  monitor_getprop,
              "vals = oca_monitor_getprop( [names], [ids] )" )
{
  octave_value retval;
  QList<OcaMonitor*> list = id_to_monitor_list( args, 1 );
  retval = get_oca_properties( list, args );
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  monitor_setprop,
              "ret = oca_monitor_setprop( name, val, [ids] )\n"
              "ret = oca_monitor_setprop( props, [ids] )"  )
{

  QList<OcaMonitor*> list = id_to_monitor_list( args, is_prop_args_map( args ) ? 1 : 2 );
  return octave_value( set_oca_properties( list, args ) );
}

#ifdef OCA_BUILD_3DPLOT
// ----------------------------------------------------------------------------
// 3dplot

OCA_BUILTIN(  plot3d_add,
              "[ID, idx] = oca_plot3d_add( name, [group_id] )"  )
{
  octave_value_list retval;
  octave_value name = safe_arg( args, 0 );
  if( ! name.is_string() ) {
    print_usage();
  }
  else {
    //OcaTrackGroup* group = id_to_group( args, 1 ); // TODO
    Oca3DPlot* plot = new Oca3DPlot( OCA_STR(name) /* TODO: , group*/ );
    retval(0) = ndarray_from_ocaobj( plot );
    retval(1) = OcaOctaveHost::getWindowData()->add3DPlot( plot ) + 1;
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  plot3d_remove,
              "ret = oca_plot3d_remove( [ids] )"   )
{
  octave_value retval;
  QList<Oca3DPlot*> list = id_to_3dplot_list( args, 0 );
  int counter = 0;
  for( int i = 0; i < list.size(); i++ ) {
    list.at(i)->close();
    counter++;
  }
  retval = counter;
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  plot3d_list,
              "[list] = oca_plot3d_list()"   )
{
  octave_value retval;
  OcaWindowData* wd = OcaOctaveHost::getWindowData();
  OcaLock lock( wd );
  Cell c( 1, wd->get3DPlotCount() );
  for( uint i = 0; i < wd->get3DPlotCount(); i++ ) {
    if( 0 != nargout ) {
      c(i) = ndarray_from_ocaobj( wd->get3DPlotAt(i) );
    }
    else {
      printf( "%4d    %s\n", i, OCA_CSTR( wd->get3DPlotAt(i)->getName() ) );
    }
  }
  if( 0 != nargout ) {
    retval = c;
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  plot3d_count,
              "count = oca_plot3d_count()"   )
{
  octave_value retval;
  retval = OcaOctaveHost::getWindowData()->get3DPlotCount();
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  plot3d_find,
              "IDs = oca_plot3d_find( ids )"   )
{
  octave_value retval;
  octave_value val_id = safe_arg( args, 0 );
  if( ! val_id.is_defined() ) {
    print_usage();
  }
  else if ( val_id.is_cell() ) {
    QList<Oca3DPlot*> list = id_to_3dplot_list( args, 0 );
    Cell c( 1, list.size() );
    for( int i = 0; i < list.size(); i++ ) {
      c(i) = ndarray_from_ocaobj( list.at(i) );
    }
    retval = c;
  }
  else {
    Oca3DPlot* plot = id_to_3dplot( args, 0 );
    retval = ndarray_from_ocaobj( plot );
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  plot3d_getprop,
              "vals = oca_plot3d_getprop( [names], [ids] )" )
{
  octave_value retval;
  QList<Oca3DPlot*> list = id_to_3dplot_list( args, 1 );
  retval = get_oca_properties( list, args );
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  plot3d_setprop,
              "ret = oca_plot3d_setprop( name, val, [ids] )\n"
              "ret = oca_plot3d_setprop( props, [ids] )"  )
{

  QList<Oca3DPlot*> list = id_to_3dplot_list( args, is_prop_args_map( args ) ? 1 : 2 );
  return octave_value( set_oca_properties( list, args ) );
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  plot3d_set,
              "vals = oca_plot3d_set( data, [id] )" )
{
  octave_value retval;
  Oca3DPlot* p = id_to_3dplot( args, 1 );
  if (NULL == p) {
    error( "no 3d plot" );
    return retval;
  }
  octave_value val_data = safe_arg( args, 0 );
  //octave_value val_texture = safe_arg( args, 1 );
  NDArray arData;
  NDArray arTexture;
  if(!val_data.is_real_type()) {
    error( "invalid data" );
  }
  /*
  else if (val_texture.is_defined() && !val_texture.is_real_type() ) {
    error( "invalid texture" );
  }
  */
  else {
    arData = val_data.array_value();
    int nx = arData.dim1();
    int ny = arData.dim2();
    double* data = arData.fortran_vec();
    double* texture = NULL;
    /*
    if (val_texture.is_real_type()) {
      arTexture = val_texture.array_value();
      if ( arTexture.dim1() != nx || arTexture.dim2() != ny ) {
        error( "invalid texture" );
      }
      else {
        texture = arTexture.fortran_vec();
      }
    }
    */
    if( ! p->setData(nx, ny, data, texture) ) {
      error("failed");
    }
  }

  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  plot3d_settexture,
              "vals = oca_plot3d_settexture( texture, [id] )" )
{
  octave_value retval;
  Oca3DPlot* p = id_to_3dplot( args, 1 );
  if (NULL == p) {
    error( "no 3d plot" );
    return retval;
  }
  octave_value val_texture = safe_arg( args, 0 );
  NDArray arTexture;
  if (!val_texture.is_real_type() ) {
    error( "invalid texture" );
  }
  else {
    arTexture = val_texture.array_value();
    int nx = arTexture.dim1();
    int ny = arTexture.dim2();
    double* texture = NULL;
    if ( p->getXLen() == nx || p->getYLen() == ny ) {
      texture = arTexture.fortran_vec();
    }
    else if(!arTexture.is_empty()) {
      error( "invalid texture size" );
    }
    if( ! p->setTexture(nx, ny, texture) ) {
      error("failed");
    }
  }

  return retval;
}
#endif // OCA_BUILD_3DPLOT

// ----------------------------------------------------------------------------
// subtrack

OCA_BUILTIN(  subtrack_add,
              "idx = oca_subtrack_add( track_ids, [smart_track_id], [group_id] )" )
{
  octave_value retval = -1;
  OcaSmartTrack* strack = id_to_smarttrack( args, 1, 2 );
  if( NULL == strack ) {
    error( "invalid strack" );
  }
  else {
    QList<OcaTrack*> list = id_to_datatrack_list( args, 0, -1 );
    NDArray result( dim_vector(1,list.size()) );
    for( int i = 0; i < list.size(); i++ ) {
      result(i) = strack->addSubtrack( list.at(i) ) + 1;
    }
    retval = result;
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  subtrack_remove,
              "idx = oca_subtrack_remove( [ids], [smart_track_id], [group_id] )" )
{
  octave_value retval = -1;
  OcaSmartTrack* strack = NULL;
  QList<OcaTrack*> list = id_to_subtrack_list( args, 0, 1, 2, &strack );

  if( NULL != strack ) {
    NDArray result( dim_vector(1,list.size()) );
    for( int i = 0; i < list.size(); i++ ) {
      result(i) = strack->removeSubtrack( list.at(i) ) + 1;
    }
    retval = result;
  }
  else {
    error( "invalid strack" );
  }

  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  subtrack_move,
              "idx = oca_subtrack_move( idx, [id], [smart_track_id], [group_id] )" )
{
  octave_value retval = -1;
  octave_value idx_val = safe_arg( args, 0 );
  if( ! idx_val.is_real_scalar() ) {
    print_usage();
  }
  else {
    oca_index idx = idx_val.int_value();
    OcaSmartTrack* strack = NULL;
    OcaTrack* track = id_to_subtrack( args, 1, 2, 3, &strack );
    if( NULL == track ) {
      error( "invalid subtrack" );
    }
    else if ( 0 < idx ) {
      retval = strack->moveSubtrack( track, idx - 1 ) + 1;
    }
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  subtrack_list,
              "[list] = oca_subtrack_list( [smart_track_id], [group_id] )" )
{
  octave_value retval;
  OcaSmartTrack* strack = id_to_smarttrack( args, 0, 1 );
  if( NULL == strack ) {
    error( "invalid strack" );
  }
  else {
    Cell c( 1, strack->getSubtrackCount() );
    for( uint i = 0; i < strack->getSubtrackCount(); i++ ) {
      if( 0 != nargout ) {
        c(i) = ndarray_from_ocaobj( strack->getSubtrack(i) );
      }
      else {
        printf( "%4d    %s\n", i+1, OCA_CSTR( strack->getSubtrack(i)->getName() ) );
      }
    }
    if( 0 != nargout ) {
      retval = c;
    }
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  subtrack_count,
              "count = oca_subtrack_count( [strack_id], [group_id] )" )
{
  octave_value retval;
  OcaSmartTrack* strack = id_to_smarttrack( args, 0, 1 );
  if( NULL == strack ) {
    error( "invalid strack" );
  }
  else {
    retval = strack->getSubtrackCount();
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  subtrack_find,
              "TRACK_IDs = oca_subtrack_find( ids, [smart_track_id], [group_id] )" )
{
  octave_value retval;
  octave_value val_id = safe_arg( args, 0 );
  if( ! val_id.is_defined() ) {
    print_usage();
  }
  else if ( val_id.is_cell() ) {
    QList<OcaTrack*> tracks = id_to_subtrack_list( args, 0, 1, 2 );
    Cell c( 1, tracks.size() );
    for( int i = 0; i < tracks.size(); i++ ) {
      c(i) = ndarray_from_ocaobj( tracks.at(i) );
    }
    retval = c;
  }
  else {
    OcaTrack* track = id_to_subtrack( args, 0, 1, 2 );
    retval = ndarray_from_ocaobj( track );
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  subtrack_getprop,
              "vals = oca_subtrack_getprop( [names], [ids], [smart_track_id], [group_id] )" )
{
  octave_value retval;
  OcaSmartTrack* strack = NULL;
  QList<OcaTrack*> list = id_to_subtrack_list( args, 1, 2, 3, &strack );

  if( NULL != strack ) {
    OcaPropProxySubtrack prop_proxy( strack );
    retval = get_oca_properties( list, args, &prop_proxy );
  }
  else {
    error( "invalid strack" );
  }

  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  subtrack_setprop,
              "ret = oca_subtrack_setprop( [name], [val], [ids], [smart_track_id], [group_id] )\n"
              "ret = oca_subtrack_setprop( [props], [ids], [smart_track_id], [group_id] )" )
{
  octave_value retval;
  int pos_base = is_prop_args_map( args ) ? 1 : 2;
  OcaSmartTrack* strack = NULL;
  QList<OcaTrack*> list = id_to_subtrack_list( args, pos_base, pos_base+1, pos_base+2, &strack );

  if( NULL != strack ) {
    OcaPropProxySubtrack prop_proxy( strack );
    retval = set_oca_properties( list, args, &prop_proxy );
  }
  else {
    error( "invalid strack" );
  }

  return retval;
}

// ----------------------------------------------------------------------------
// context

static octave_scalar_map* get_context( const octave_value_list& args )
{
  OcaObject* obj = NULL;
  octave_scalar_map* context = NULL;
  if( ( 0 < args.length() ) && args(0).is_real_matrix() ) {
    ocaobj_from_ndarray( args(0).array_value(), &obj );
  }

  if( NULL == obj ) {
    error( "invalid object" );
  }
  else {
    context = OcaOctaveHost::getObjectContext( obj );
    Q_ASSERT( NULL != context );
  }
  return context;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  context_get,
              "[val, ret] = oca_context_get( ID, name, [default_value] )\n"
              "context = oca_context_get( ID )"         )
{
  octave_value_list retval;
  octave_scalar_map* context = get_context( args );
  if( NULL != context ) {
    if( 1 == args.length() ) {
      retval(0) = *context;
    }
    else {
      Q_ASSERT( 1 < args.length() );
      if( ! args(1).is_string() ) {
        error( "invalid context field name" );
      }
      else {
        retval(0) = context->getfield( args(1).string_value() );
        if( ! retval(0).is_defined() ) {
          retval(1) = false;
          if( 2 < args.length() ) {
            retval(0) = args(2);
          }
        }
        else {
          retval(1) = true;
        }
      }
    }
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  context_set,
              "oca_context_set( ID, name, val )\n"
              "oca_context_set( ID, context )"      )
{
  octave_value retval;
  octave_scalar_map* context = get_context( args );
  if( NULL != context ) {
    if( ( 2 == args.length() ) && ( args(1).is_map() ) ) {
      *context = args(1).scalar_map_value();
    }
    else if ( ( 2 < args.length() ) && args(1).is_string() ) {
      context->assign( args(1).string_value(), args(2) );
    }
    else {
      print_usage();
    }
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  context_remove,
              "ret = oca_context_remove( ID, name )"    )
{
  octave_value retval = false;
  octave_scalar_map* context = get_context( args );
  if( NULL != context ) {
    if( ( 1 < args.length() ) && args(1).is_string() ) {
      std::string key = args(1).string_value();
      if( context->isfield( key ) ) {
        context->rmfield( key );
        retval = true;
      }
    }
    else {
      print_usage();
    }
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  global_getprop,
              "vals = oca_global_getprop( [names] )" )
{
  octave_value retval;
  QList<OcaWindowData*> list;
  list.append( OcaOctaveHost::getWindowData() );
  retval = get_oca_properties( list, args );
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  global_setprop,
              "ret = oca_global_setprop( name, val )\n"
              "ret = oca_global_setprop( props )"  )
{
  QList<OcaWindowData*> list;
  list.append( OcaOctaveHost::getWindowData() );
  return octave_value( set_oca_properties( list, args ) );
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  global_getinfo,
              "ret = oca_global_getinfo( name )\n"
              "ret = oca_global_getinfo()"  )
{
  octave_value retval;
  if( 0 == args.length() ) {
    retval = *OcaOctaveHost::getInfo();
  }
  else if( args(0).is_string() ) {
    retval = OcaOctaveHost::getInfo()->getfield( args(0).string_value() );
  }
  else {
    print_usage();
  }
  return retval;
}

// ----------------------------------------------------------------------------

OCA_BUILTIN(  global_listaudiodevs,
              "list = global_listaudiodevs( type )\n"
              "     type: 'input' or 'output'"  )
{
  octave_value ret;
  std::string type;
  if( ( 0 < args.length() ) && ( args(0).is_string() ) ) {
    type = args(0).string_value();
  }
  if( ( "input" != type ) && ( "output" != type ) ) {
    print_usage();
  }
  else {
    OcaApp::getAudioController()->checkDevices();
    bool rec = ( "input" == type );
    QStringList list = OcaApp::getAudioController()->enumDevices( rec );
    Cell c( 1, list.size() );
    for( int i = 0; i < list.size(); i++ ) {
      c( i ) = OCA_STR( list.at(i) );
    }
    ret = c;
  }

  return ret;
}

// ----------------------------------------------------------------------------

static void install_octaudio_builtin(   octave_builtin::fcn function,
                                        const std::string& function_name,
                                        const char* doc_string = DOC_STRING_STUB )
{
  octave_function* fcn = new octave_builtin ( function, function_name, doc_string );
#if CHECK_OCTAVE_VERSION(4,4)
  octave::interpreter::the_interpreter()->get_symbol_table().install_built_in_function ( function_name, fcn );
#else
  symbol_table::install_built_in_function ( function_name, fcn );
#endif
}

// ----------------------------------------------------------------------------

#define INSTALL_OCA_BUILTIN( name ) install_octaudio_builtin( name, "oca_"#name, name##_doc_string )

#if CHECK_OCTAVE_VERSION(4,2)
static  octave::application*   octave_app = NULL;
static  octave::interpreter*   octave_interpreter = NULL;
#endif

void OcaOctaveHost::initialize()
{
  qRegisterMetaType<OcaObject*>("OcaObject*");
  qRegisterMetaType<OcaTrackBase*>("OcaTrackBase*");
  qRegisterMetaType<OcaTrack*>("OcaTrack*");
  qRegisterMetaType<OcaSmartTrack*>("OcaSmartTrack*");
  qRegisterMetaType<OcaMonitor*>("OcaMonitor*");
  qRegisterMetaType<OcaTrackGroup*>("OcaTrackGroup*");
#ifdef OCA_BUILD_3DPLOT
  qRegisterMetaType<Oca3DPlot*>("Oca3DPlot*");
#endif

  QString prefix = OCA_STR( octave_env::getenv( "OCTAUDIO_PREFIX" ) );
  if( prefix.isEmpty() ) {
    prefix = QFileInfo( qApp->applicationDirPath() ).dir().canonicalPath();
    octave_env::putenv( "OCTAUDIO_PREFIX", OCA_STR( QDir::toNativeSeparators( prefix ) ) );
  }
  QString octaudiorc = OCA_STR( octave_env::getenv( "OCTAUDIO_VERSION_INITFILE" ) );
  if( octaudiorc.isEmpty() ) {
    octaudiorc = prefix + "/share/octaudio/m/startup/octaudiorc";
  }
  if( QFileInfo(octaudiorc).exists() ) {
    octave_env::putenv( "OCTAVE_VERSION_INITFILE", OCA_STR( QDir::toNativeSeparators( octaudiorc ) ) );
  }

  setlocale( LC_NUMERIC, "C" );
  setlocale( LC_TIME, "C" );
  octave_env::putenv( "LC_NUMERIC", "C" );
  octave_env::putenv( "LC_TIME", "C" );

  // It seems like Octave guys decided to make incompatible changes in the embedding API every release
#if CHECK_OCTAVE_VERSION(4,4)
  octave::cmdline_options opts;
  opts.forced_interactive(false);
  opts.forced_line_editing(true);
  opts.read_history_file(false);
  opts.inhibit_startup_message(true);
  // just a hack as I haven't found any better way yet
  opts.code_to_eval("1;");

  octave_app = new octave::cli_application(opts);
  octave_interpreter = new octave::interpreter(octave_app);

  octave_interpreter->initialize_history(false);
  octave_interpreter->initialize_load_path(true);
  octave_interpreter->read_site_files(true);
  octave_interpreter->read_init_files(true);
  octave_interpreter->interactive(false);
  octave_interpreter->initialize();
#elif CHECK_OCTAVE_VERSION(4,2)
  octave::cmdline_options opts;
  opts.forced_interactive(false);
  opts.forced_line_editing(true);
  opts.read_history_file(false);
  opts.inhibit_startup_message(true);
  octave_app = new octave::embedded_application(opts);
#else
  octave_exit = NULL;
  string_vector oct_argv(4);
  oct_argv(0) = "embedded";
  oct_argv(1) = "-q";
  oct_argv(2) = "--line-editing";
  oct_argv(3) = "--no-history";
#endif

  INSTALL_OCA_BUILTIN( track_add );
  INSTALL_OCA_BUILTIN( track_remove );
  INSTALL_OCA_BUILTIN( track_move );
  INSTALL_OCA_BUILTIN( track_list );
  INSTALL_OCA_BUILTIN( track_count );
  INSTALL_OCA_BUILTIN( track_getprop );
  INSTALL_OCA_BUILTIN( track_setprop );
  INSTALL_OCA_BUILTIN( track_find );

  INSTALL_OCA_BUILTIN( data_get );
  INSTALL_OCA_BUILTIN( data_set );
  INSTALL_OCA_BUILTIN( data_clear );
  INSTALL_OCA_BUILTIN( data_listblocks );
  INSTALL_OCA_BUILTIN( data_getblocks );
  INSTALL_OCA_BUILTIN( data_delete );
  INSTALL_OCA_BUILTIN( data_split );
  INSTALL_OCA_BUILTIN( data_join );
  INSTALL_OCA_BUILTIN( data_moveblocks );
  INSTALL_OCA_BUILTIN( data_fill );

  INSTALL_OCA_BUILTIN( group_add );
  INSTALL_OCA_BUILTIN( group_remove );
  INSTALL_OCA_BUILTIN( group_move );
  INSTALL_OCA_BUILTIN( group_list );
  INSTALL_OCA_BUILTIN( group_count );
  INSTALL_OCA_BUILTIN( group_find );
  INSTALL_OCA_BUILTIN( group_getprop );
  INSTALL_OCA_BUILTIN( group_setprop );

  INSTALL_OCA_BUILTIN( monitor_add );
  INSTALL_OCA_BUILTIN( monitor_remove );
  //INSTALL_OCA_BUILTIN( monitor_move );
  INSTALL_OCA_BUILTIN( monitor_list );
  INSTALL_OCA_BUILTIN( monitor_count );
  INSTALL_OCA_BUILTIN( monitor_getprop );
  INSTALL_OCA_BUILTIN( monitor_setprop );
  INSTALL_OCA_BUILTIN( monitor_find );

#ifdef OCA_BUILD_3DPLOT
  INSTALL_OCA_BUILTIN( plot3d_add );
  INSTALL_OCA_BUILTIN( plot3d_remove );
  INSTALL_OCA_BUILTIN( plot3d_list );
  INSTALL_OCA_BUILTIN( plot3d_count );
  INSTALL_OCA_BUILTIN( plot3d_find );
  INSTALL_OCA_BUILTIN( plot3d_getprop );
  INSTALL_OCA_BUILTIN( plot3d_setprop );
  INSTALL_OCA_BUILTIN( plot3d_set );
  INSTALL_OCA_BUILTIN( plot3d_settexture );
#endif

  INSTALL_OCA_BUILTIN( subtrack_add );
  INSTALL_OCA_BUILTIN( subtrack_remove );
  INSTALL_OCA_BUILTIN( subtrack_move );
  INSTALL_OCA_BUILTIN( subtrack_list );
  INSTALL_OCA_BUILTIN( subtrack_count );
  INSTALL_OCA_BUILTIN( subtrack_find );
  INSTALL_OCA_BUILTIN( subtrack_getprop );
  INSTALL_OCA_BUILTIN( subtrack_setprop );

  INSTALL_OCA_BUILTIN( context_get );
  INSTALL_OCA_BUILTIN( context_set );
  INSTALL_OCA_BUILTIN( context_remove );

  INSTALL_OCA_BUILTIN( global_getprop );
  INSTALL_OCA_BUILTIN( global_setprop );
  INSTALL_OCA_BUILTIN( global_getinfo );
  INSTALL_OCA_BUILTIN( global_listaudiodevs );

  octave_scalar_map* info = s_instance->m_info;

  info->assign( "version", OCA_VERSION_STRING );
  info->assign( "build_type", OCA_CONFIG_BUILDTYPE );

  if( NULL != OCA_BUILD_TIMESTAMP ) {
    info->assign( "build_time", OCA_BUILD_TIMESTAMP );
  }
  if( NULL != Oca_BUILD_REVISION ) {
    info->assign( "build_revision", OCA_BUILD_REVISION );
  }

#if CHECK_OCTAVE_VERSION(4,4)
  octave_interpreter->execute();
#elif CHECK_OCTAVE_VERSION(4,2)
  octave_app->execute();
#else
  octave_main( 4, oct_argv.c_str_vec(), 1 );
#endif
}

// ----------------------------------------------------------------------------

void OcaOctaveHost::shutdown() {
#if ! CHECK_OCTAVE_VERSION(4,2)
  quitting_gracefully = true;
  clean_up_and_exit( 0 );
#else
  if (NULL != octave_interpreter) {
    delete octave_interpreter;
    octave_interpreter = NULL;
  }
  if (NULL != octave_app) {
    delete octave_app;
    octave_app = NULL;
  }
#endif
  fprintf( stderr, "OcaOctaveHost::shutdown - %d objects in context\n", s_context.size() );
}

// ----------------------------------------------------------------------------

OcaWindowData* OcaOctaveHost::getWindowData()
{
  return OcaApp::getOcaInstance()->getWindowData();
}

// ----------------------------------------------------------------------------

octave_scalar_map*  OcaOctaveHost::getObjectContext( OcaObject* obj )
{
  OcaLock lock( obj );
  if( obj->isClosed() ) {
    return NULL;
  }
  octave_scalar_map* d = s_context.value( obj );
  if( NULL == d ) {
    d = new octave_scalar_map;
    s_context.insert( obj, d );
    s_instance->connect( obj, SIGNAL(closed(OcaObject*)), SLOT(removeObjectContext(OcaObject*)) );
  }
  return d;
}

// ----------------------------------------------------------------------------

void OcaOctaveHost::removeObjectContext( OcaObject* obj )
{
  octave_scalar_map* d = s_context.take( obj );
  delete d;
  d = NULL;
  obj->disconnect( this );
}

// ----------------------------------------------------------------------------

void OcaOctaveHost::userInterruptionHanler( int /* sig */ )
{
 fprintf( stderr, "OcaOctaveHost::userInterruptionHanler %p\n", QThread::currentThread() );
#if ! CHECK_OCTAVE_VERSION(4,2)
 //if (can_interrupt)
    {
      if (octave_interrupt_immediately)
        {
          if (octave_interrupt_state == 0)
            octave_interrupt_state = 1;

          octave_jump_to_enclosing_context ();
        }
      else
        {
          // If we are already cleaning up from a previous interrupt,
          // take note of the fact that another interrupt signal has
          // arrived.

          if (octave_interrupt_state < 0)
            octave_interrupt_state = 0;

          octave_signal_caught = 1;
          octave_interrupt_state++;

          if (interactive && octave_interrupt_state == 2)
            std::cerr << "Press Control-C again to abort." << std::endl;

          /*
          if (octave_interrupt_state >= 3)
            my_friendly_exit (sig_name, sig_number, true);

          */
        }
   }
#endif
}

//static
OcaTrackGroup* OcaOctaveHost::s_group = NULL;

// ----------------------------------------------------------------------------

OcaOctaveHost::OcaOctaveHost()
{
  Q_ASSERT( NULL == s_instance );
  s_instance = this;
  m_info = new octave_scalar_map;
}

// ----------------------------------------------------------------------------

OcaOctaveHost::~OcaOctaveHost()
{
  Q_ASSERT( this == s_instance );
  s_instance = NULL;
  delete m_info;
  m_info = NULL;
}

// ----------------------------------------------------------------------------

void OcaOctaveHost::start() {
#ifndef Q_OS_WIN32
  m_threadId = pthread_self();
#endif
  emit stateChanged( 1 );
}

// ----------------------------------------------------------------------------

bool OcaOctaveHost::evalCommand( const QString& command, OcaTrackGroup* group )
{
  if( ! m_command.isEmpty() ) {
    return false;
  }
  Q_ASSERT( NULL == s_group );

  if( command.isEmpty() ) {
    return false;
  }

  m_command = command;
  s_group = group;
  QCoreApplication::postEvent( this, new QEvent(QEvent::User) );
  return true;
}

// ----------------------------------------------------------------------------

void OcaOctaveHost::customEvent( QEvent* /* event */ )
{
  Q_ASSERT( ! m_command.isEmpty() );
  processCommand();
  m_command.clear();
  s_group = NULL;
}

// ----------------------------------------------------------------------------

void OcaOctaveHost::processCommand()
{
  fprintf(
      stderr,
      "Command_Listener::runCommand, thread = %p, command = %s\n",
      QThread::currentThread(),
      m_command.toLocal8Bit().data()
      );
  // sleep( 10 );

  int status = 0;
  try {
#ifndef Q_OS_WIN32
    signal( SIGUSR1, userInterruptionHanler );
    eval_string( m_command.toLocal8Bit().data(), false, status, 0 );
    signal( SIGUSR1, SIG_IGN );
#else
    eval_string( m_command.toLocal8Bit().data(), false, status, 0 );
#endif
  }
  catch( octave_interrupt_exception ) {
    fprintf( stderr, "exception\n" );
    //recover_from_exception();
    emit commandFailed( "interrupted", 129 );
  }
  catch( octave_execution_exception& ) {
    //recover_from_exception();
    //emit commandFailed( "failed", 129 );
    error_state = 129;
  }
  catch( std::bad_alloc& ) {
    //recover_from_exception();
    emit commandFailed( "memory exception", 129 );
  }
  catch( ... ) {
    fprintf( stderr, "unknown exception\n" );
    Q_ASSERT( false );
    //recover_from_exception();
    emit commandFailed( "unknown exception", 129 );
  }
  recover_from_exception();

  if( ( 0 != status ) || ( 0 != error_state ) ) {
    int error_state_saved = error_state;
    error_state = 0;
    octave_scalar_map info = feval( "lasterror" )(0).scalar_map_value();
    QString msg = OCA_STR( info.getfield("message") );

    octave_map stack = info.getfield("stack").map_value();
    if( ! stack.is_empty() ) {
      msg += "\ncalled from:";
    }
    for( int i = 0; i < stack.numel(); i++ ) {
      octave_value file = stack(i).contents("file");
      octave_value name = stack(i).contents("name");
      msg += QString( "\n  %2 %3 at line %4 column %5" )
            . arg( OCA_STR(name) )
            . arg( OCA_STR(file) )
            . arg( stack(i).getfield("line").int_value() )
            . arg( stack(i).getfield("column").int_value() );
    }
    emit commandFailed( QString("ERROR: %1").arg( msg ), error_state_saved );
  }
  Vlast_prompt_time.stamp();
  if( Vdrawnow_requested ) {
    feval ("drawnow");
    Vdrawnow_requested = false;
  }
  fflush( stdout );

  emit stateChanged( 1 );
}

// ----------------------------------------------------------------------------

QStringList OcaOctaveHost::getCompletions( const QString& hint ) const
{
  Q_ASSERT( m_command.isEmpty() );
  QStringList list;
  command_editor::completion_fcn fcn = command_editor::get_completion_function();
  //fprintf( stderr, "completion_fcn: %p\n", fcn );
  if( NULL != fcn ) {
    int k = 0;
    try {
      feval ("rehash");
      while( true ) {
        QString s = QString::fromStdString( (*fcn)( hint.toStdString(), k++ ) );
        if( s.isEmpty() ) {
          break;
        }
        list.append( s );
      }
    }
    catch( ... ) {
      recover_from_exception();
      error_state = 0;
    }
  }
  return list;
}

// ----------------------------------------------------------------------------

void OcaOctaveHost::abortCurrentCommand()
{
  fprintf( stderr, "OcaOctaveHost::abortCurrentCommand, thread = %p\n", QThread::currentThread() );
#ifndef Q_OS_WIN32
  pthread_kill( m_threadId, SIGUSR1 );
#else
  //raise( SIGTERM );
#endif
  octave_signal_caught = 1;
  octave_interrupt_state = 1;
}

// ----------------------------------------------------------------------------
