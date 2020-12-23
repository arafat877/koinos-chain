#pragma once

#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include <koinos/net/protocol/jsonrpc/types.hpp>
#include <koinos/net/transport/http/abstract_request_handler.hpp>

namespace koinos::net::protocol::jsonrpc {

class request_handler :
   public std::enable_shared_from_this< request_handler >,
   public koinos::net::transport::http::abstract_request_handler
{
public:
   using method_handler = std::function< json( const json::object_t& ) >;

   jsonrpc::request parse_request( const std::string& payload ) const
   {
      LOG(info) << payload;
      jsonrpc::request req;

      try
      {
         req = json::parse( payload );
      }
      catch( const jsonrpc::exception& e )
      {
         LOG(info) << e.what();
         throw;
      }
      catch ( const std::exception& e )
      {
         LOG(info) << e.what();
         throw jsonrpc::parse_error( "unable to parse request", e.what(), nullptr );
      }

      return req;
   }

   jsonrpc::response call_handler( const jsonrpc::id_type& id, const method_handler& h, const json::object_t& j ) const
   {
      jsonrpc::response resp;

      try
      {
         resp = jsonrpc::response {
            .jsonrpc = "2.0",
            .id = id,
            .error = {},
            .result = h( j )
         };
      }
      catch ( const jsonrpc::exception& e )
      {
         throw jsonrpc::exception( e.what(), e.code, e.data, id );
      }
      catch ( const std::exception& e )
      {
         throw jsonrpc::server_error( "a server error has occurred", e.what(), id );
      }

      return resp;
   }

   std::string handle( const std::string& payload ) override
   {
      jsonrpc::response resp;
      id_type id = nullptr;

      try
      {
         jsonrpc::request req = parse_request( payload );
         req.validate();
         id = req.id;

         auto h = get_method_handler( req.method );

         if ( !h.has_value() )
            throw jsonrpc::method_not_found( "method not found: " + req.method, {}, req.id );

         resp = call_handler( req.id, h.value(), req.params );
      }
      catch ( const jsonrpc::exception& e )
      {
         resp = jsonrpc::response {
            .jsonrpc = "2.0",
            .id = id,
            .error = jsonrpc::error_type {
               .code = e.code,
               .message = e.what(),
               .data = e.data
            }
         };
      }
      catch ( const std::exception& e )
      {
         resp = jsonrpc::response {
            .jsonrpc = "2.0",
            .id = id,
            .error = jsonrpc::error_type {
               .code = jsonrpc::error_code::internal_error,
               .message = "an internal error has ocurred",
               .data = e.what()
            }
         };
      }

      json j = resp;

      return j.dump();
   }

   void add_method_handler( const std::string& method_name, method_handler handler )
   {
      if ( method_handlers.find( method_name ) != method_handlers.end() )
         throw std::runtime_error( "unable to override method handler" );

      method_handlers.insert( { method_name, handler } );
   }

   std::optional< method_handler > get_method_handler( const std::string& method_name )
   {
      auto it = method_handlers.find( method_name );
      if ( it != method_handlers.end() )
         return it->second;
      return {};
   }

private:
   std::unordered_map< std::string, method_handler > method_handlers;
};

} // koinos::net::protocol::jsonrpc
