#pragma once

#include <any>
#include <atomic>
#include <future>

#include <koinos/net/http_client.hpp>
#include <koinos/pack/rt/json.hpp>
#include <koinos/util.hpp>

namespace koinos::net::jsonrpc {

typedef std::variant<std::any, koinos::exception> call_result;

class jsonrpc_client {
   private:
      struct jsonrpc_response_parser
      {
         static uint64_t parse_response( const std::string&, std::any& ) { return 0; }
      };

      http_client< jsonrpc_response_parser > _client;
      std::atomic< uint32_t >                _next_id;

   public:
      template< typename Result, typename Params >
      std::future< Result > call_async( const std::string& method, const Params& params )
      {
         // TODO: Format jsonrpc request

         return std::async(
            std::launch::async,
            []( std::future< call_result >&& fut )
            {
               Result result;
               fut.wait();

               std::visit( koinos::overloaded {
                  [&result]( const std::any& a )
                  {
                     auto j = std::any_cast< nlohmann::json >( a );
                     koinos::pack::from_json( j, result );
                  },
                  []( const koinos::exception& e )
                  {
                     throw e;
                  }
               }, fut.get() );

               return result;
            },
            _client.send_request( 0, "" )
         );
      }

      template< typename Result, typename Params >
      Result call( const std::string& method, const Params& params )
      {
         auto fut = call_async< Result >( method, params );
         fut.wait();
         return fut.get();
      }
};

} // koinos::net::jsonrpc
