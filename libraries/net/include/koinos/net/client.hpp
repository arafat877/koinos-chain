#pragma once

#include <future>
#include <variant>

#include <koinos/net/jsonrpc/client.hpp>
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
   private:
      std::variant< jsonrpc::jsonrpc_client > _client_impl;

   public:
      template< typename T >
      client( T&& t ) :
         _client_impl( t )
      {}

      template< typename Result, typename Params >
      std::future< Result > call_async( const std::string& method, const Params& params )
      {
         return std::visit( koinos::overloaded {
            [&]( auto&& c )
            {
               return c.template call_async< Result >( method, params );
            }
         }, _client_impl );
      }

      template< typename Result, typename Params >
      Result call( const std::string& method, const Params& params )
      {
         return std::visit( koinos::overloaded {
            [&]( auto&& c )
            {
               return c.template call< Result >( method, params );
            }
         }, _client_impl );
      }

      void connect( const stream_protocol::endpoint& endpoint )
      {
         std::visit( koinos::overloaded {
            [&]( auto&& c )
            {
               c.connect( endpoint );
            }
         }, _client_impl );
      }

      void is_open()
      {
         std::visit( koinos::overloaded {
            [&]( auto&& c )
            {
               return c.is_open();
            }
         }, _client_impl );
      }

      void close()
      {
         std::visit( koinos::overloaded {
            [&]( auto&& c )
            {
               c.close();
            }
         }, _client_impl );
      }
};

} // koinos::net
