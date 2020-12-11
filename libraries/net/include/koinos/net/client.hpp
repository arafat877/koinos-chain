#pragma once

#include <future>
#include <variant>

#include <koinos/net/jsonrpc/client.hpp>
#include <koinos/util.hpp>

namespace koinos::net {

/*
 * An abstract client.
 *
 * Designed to support jsonrpc, binary, and cbor protocols over http and raw tcp connections
 */
struct client
{
   //client() :
   //   client_impl( jsonrpc::jsonrpc_client() )
   //{}

   std::variant< jsonrpc::jsonrpc_client > client_impl;

   template< typename Result, typename Params >
   std::future< Result > call_async( const std::string& method, const Params& params )
   {
      return std::visit( koinos::overloaded {
         [&]( auto&& c )
         {
            return c.template call_async< Result >( method, params );
         }
      }, client_impl );
   }

   template< typename Result, typename Params >
   Result call( const std::string& method, const Params& params )
   {
      return std::visit( koinos::overloaded {
         [&]( auto&& c )
         {
            return c.template call< Result >( method, params );
         }
      }, client_impl );
   }
};

} // koinos::net
