#pragma once
#include <koinos/exception.hpp>

#include <koinos/pack/rt/basetypes.hpp>
#include <koinos/pack/rt/binary.hpp>

#include <openssl/evp.h>

#define CRYPTO_SHA1_ID     uint64_t(0x11)
#define CRYPTO_SHA2_256_ID uint64_t(0x12)
#define CRYPTO_SHA2_512_ID uint64_t(0x13)

/*
 There are not codes for RIPEMD in the multicodec definition but
 0x52 is currently unassigned as of 05/15/2020.
 0x52 was chosen as the ASCII value for the character 'R'.
 A PR was made against multiformats:
 https://github.com/multiformats/multicodec/pull/175

 https://github.com/multiformats/multicodec/blob/master/table.csv
*/
#define CRYPTO_RIPEMD160_ID     uint64_t(0x1053)

namespace koinos::crypto {

using koinos::protocol::multihash_type;
using koinos::protocol::multihash_vector;
using koinos::protocol::vl_blob;

DECLARE_KOINOS_EXCEPTION( unknown_hash_algorithm );
DECLARE_KOINOS_EXCEPTION( multihash_size_mismatch );
DECLARE_KOINOS_EXCEPTION( multihash_size_limit_exceeded );

struct multihash
{
   multihash() = delete;

   static void set_id( multihash_type& mh,    uint64_t code );
   static void set_id( multihash_vector& mhv, uint64_t code );

   static uint64_t get_id( const multihash_type& mh );
   static uint64_t get_id( const multihash_vector& mhv );

   static void set_size( multihash_type& mh,    uint64_t size );
   static void set_size( multihash_vector& mhv, uint64_t size );

   static uint64_t get_size( const multihash_type& mh );
   static uint64_t get_size( const multihash_vector& mhv );

   static bool validate( const multihash_type& mh,    uint64_t code = 0, uint64_t size = 0 );
   static bool validate( const multihash_vector& mhv, uint64_t code = 0, uint64_t size = 0 );

   static inline bool validate_sha256( const multihash_type& mh )    { return validate( mh,  CRYPTO_SHA2_256_ID, 32 ); }
   static inline bool validate_sha256( const multihash_vector& mhv ) { return validate( mhv, CRYPTO_SHA2_256_ID, 32 ); }
};

bool operator ==( const multihash_type& mha, const multihash_type& mhb )
{
   return ( multihash::validate( mha ) && multihash::validate( mhb ) )
       && ( multihash::get_id( mha )   == multihash::get_id( mhb ) )
       && ( multihash::get_size( mha ) == multihash::get_size( mhb ) )
       && ( memcmp( mha.digest.data.data(), mhb.digest.data.data(), mhb.digest.data.size() ) == 0 );
}

bool operator !=( const multihash_type& mha, const multihash_type& mhb )
{
   return !(mha == mhb);
}

struct encoder
{
   encoder( uint64_t code );
   ~encoder();

   void write( const char* d, size_t len );
   void put( char c ) { write( &c, 1 ); }
   void reset();
   size_t get_result( vl_blob& v, size_t size = 0 );
   inline size_t get_result( multihash_type& mh ) { return get_result( mh.digest ); }

   private:
      const EVP_MD* md = nullptr;
      EVP_MD_CTX* mdctx = nullptr;
};

template< typename T >
multihash_type hash( uint64_t code, T& t, size_t size = 0 )
{
   encoder e( code );
   koinos::pack::to_binary( e, t );
   multihash_type mh;
   multihash::set_id( mh, code );
   size_t hash_size = e.get_result( mh.digest, size );

   if( size )
      KOINOS_ASSERT( size == hash_size, multihash_size_mismatch, "OpenSSL Hash size does not match expected multihash size", () );
   KOINOS_ASSERT( hash_size <= std::numeric_limits< uint8_t >::max(), multihash_size_limit_exceeded, "Multihash size exceeds max", () );

   multihash::set_size( mh, hash_size );
   return mh;
};

multihash_type hash( uint64_t code, const char* data, size_t len, size_t size = 0 )
{
   encoder e( code );
   e.write( data, len );
   multihash_type mh;
   multihash::set_id( mh, code );
   size_t hash_size = e.get_result( mh.digest, size );

   if( size )
      KOINOS_ASSERT( size == hash_size, multihash_size_mismatch, "OpenSSL Hash size does not match expected multihash size", () );
   KOINOS_ASSERT( hash_size <= std::numeric_limits< uint8_t >::max(), multihash_size_limit_exceeded, "Multihash size exceeds max", () );

   multihash::set_size( mh, hash_size );
   return mh;
}

template< typename Iter >
multihash_vector hash( uint64_t code, Iter first, Iter last, size_t size = 0 )
{
   encoder e( code );
   multihash_vector mhv;
   multihash::set_id( mhv, code );

   for(; first != last; ++first )
   {
      koinos::pack::to_binary( e, *first );
      mhv.digests.emplace_back();
      size_t hash_size = e.get_result( mhv.digests.back(), size );

      if( multihash::get_size( mhv ) == 0 )
      {
         if( size )
            KOINOS_ASSERT( size == hash_size, multihash_size_mismatch, "OpenSSL Hash size does not match expected multihash size", () );
         KOINOS_ASSERT( hash_size <= std::numeric_limits< uint16_t >::max(), multihash_size_limit_exceeded, "Multihash size exceeds max", () );

         multihash::set_size( mhv, hash_size );
      }
      else
      {
         KOINOS_ASSERT( hash_size == multihash::get_size( mhv ), multihash_size_mismatch, "OpenSSL Hash size does not match expected multihash size", () );
      }
   }

   return mhv;
};

template< typename T >
bool add_hash( multihash_vector& mhv, T& t )
{
   encoder e( multihash::get_id( mhv ) );
   koinos::pack::to_binary( e, t );
   mhv.digests.emplace_back();
   size_t hash_size = e.get_result( mhv.digests.back() );
   if( hash_size == multihash::get_size( mhv ) )
      return true;

   mhv.digests.pop_back();
   return false;
};

} // koinos::crypto