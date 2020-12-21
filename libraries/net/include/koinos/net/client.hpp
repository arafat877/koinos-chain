#pragma once

#include <future>
#include <variant>

#include <koinos/net/protocol/jsonrpc/client.hpp>
#include <koinos/util.hpp>

namespace koinos::net {

/*
 * An rpc client interface.
 *
 * Designed to wrap jsonrpc, binary, and cbor protocols over http and raw
 * tcp/unix socket connections
 */
struct client
{
   using client_variant = std::variant< protocol::jsonrpc::jsonrpc_client >;

   private:
      client_variant _client_impl;

   public:
      client() = default;
      client( const client& ) = delete;
      client( client&& ) = default;

      template< typename T >
      client( T&& t ) :
         _client_impl( std::forward< T >( t ) )
      {}

      template< typename T >
      client& operator=( T&& t )
      {
         _client_impl = std::move( client_variant( std::forward< T >( t ) ) );
         return *this;
      }

      client& operator=( const client& ) = delete;
      client& operator=( client&& ) = default;

      template< typename Result, typename Params >
      std::future< Result > call_async( const std::string& method, const Params& params )
      {
         return std::visit( koinos::overloaded {
            [&]( auto&& c )
            {
               return c.template call_async< Result >( method, params );
            }},
            _client_impl );
      }

      template< typename Result, typename Params >
      Result call( const std::string& method, const Params& params )
      {
         return std::visit( koinos::overloaded {
            [&]( auto&& c )
            {
               return c.template call< Result >( method, params );
            }},
            _client_impl );
      }

      template< typename Endpoint >
      void connect( const Endpoint& e )
      {
         std::visit( koinos::overloaded {
            [&]( auto&& c )
            {
               c.connect( e );
            }},
            _client_impl );
      }

      bool is_open()
      {
         return std::visit( koinos::overloaded {
            [&]( auto&& c )
            {
               return c.is_open();
            }},
            _client_impl );
      }

      void close()
      {
         std::visit( koinos::overloaded {
            [&]( auto&& c )
            {
               c.close();
            }},
            _client_impl );
      }
};

} // koinos::net
