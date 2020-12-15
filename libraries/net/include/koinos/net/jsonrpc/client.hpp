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
      http_client _client;
      std::atomic< uint32_t >                _next_id = 0;

      template< typename Params >
      typename std::enable_if_t< koinos::pack::jsonifiable< Params >::value, nlohmann::json >
      create_request( const std::string& method, const Params& params )
      {
         static_assert(!koinos::pack::jsonifiable<Params>::value);
         LOG(info) << params;
         nlohmann::json j;
         koinos::pack::to_json( j, params );

         return nlohmann::json{
            {"id", _next_id++},
            {"jsonrpc", "2.0"},
            {"method", method},
            {"params", std::move( j )}
         };
      }

      template< typename Params >
      typename std::enable_if_t< !koinos::pack::jsonifiable< Params >::value, nlohmann::json >
      create_request( const std::string& method, const Params& params )
      {
         return nlohmann::json{
            {"id", _next_id++},
            {"jsonrpc", "2.0"},
            {"method", method},
            {"params", params}
         };
      }

   public:
      jsonrpc_client();
      jsonrpc_client( uint32_t timeout );

      void connect( const stream_protocol::endpoint& );
      bool is_open() const;
      void close();

      template< typename Result, typename Params >
      std::future< Result > call_async( const std::string& method, const Params& params )
      {
         auto j = create_request( method, params );

         return std::async(
            std::launch::async,
            []( std::shared_future< call_result >&& fut )
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
            _client.send_request( j["id"].template get< uint32_t >(), j.dump() )
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
