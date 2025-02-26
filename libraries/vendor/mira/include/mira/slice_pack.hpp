#pragma once

#include <rocksdb/slice.h>

#include <boost/type_traits/integral_constant.hpp>
#include <boost/tuple/tuple.hpp>

namespace mira {

using ::rocksdb::Slice;
using ::rocksdb::PinnableSlice;

template< typename T > struct is_static_length : public boost::false_type {};
template<> struct is_static_length< bool > : public boost::true_type {};
template<> struct is_static_length< char > : public boost::true_type {};
template<> struct is_static_length< int16_t > : public boost::true_type {};
template<> struct is_static_length< uint16_t > : public boost::true_type {};
template<> struct is_static_length< int32_t > : public boost::true_type {};
template<> struct is_static_length< uint32_t > : public boost::true_type {};
template<> struct is_static_length< int64_t > : public boost::true_type {};
template<> struct is_static_length< uint64_t > : public boost::true_type {};

template<> struct is_static_length< boost::tuples::null_type > : public boost::true_type {};

template< typename HT, typename TT >
struct is_static_length< boost::tuples::cons< HT, TT > >
{
   static const bool value = is_static_length< HT >::value && is_static_length< TT >::value;
};

template< typename Serializer, typename T > struct slice_packer
{
   static void pack( PinnableSlice& s, const T& t )
   {
      if( is_static_length< T >::value )
      {
         s.PinSelf( Slice( (char*)&t, sizeof(t) ) );
      }
      else
      {
         auto v = Serializer::to_binary_vector( t );
         s.PinSelf( Slice( v.data(), v.size() ) );
      }
   }

   static void unpack( const Slice& s, T& t )
   {
      if( is_static_length< T >::value )
      {
         t = *(T*)s.data();
      }
      else
      {
         Serializer::template from_binary_array< T >( s.data(), s.size(), t );
      }
   }
};

template< typename Serializer, typename T >
void pack_to_slice( PinnableSlice& s, const T& t )
{
   slice_packer< Serializer, T >::pack( s, t );
}

template< typename Serializer, typename T >
void unpack_from_slice( const Slice& s, T& t )
{
   slice_packer< Serializer, T >::unpack( s, t );
}

template< typename Serializer, typename T >
T unpack_from_slice( const Slice& s )
{
   T t;
   unpack_from_slice< Serializer >( s, t );
   return t;
}

} // mira
