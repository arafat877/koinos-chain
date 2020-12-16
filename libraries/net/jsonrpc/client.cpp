#include <koinos/net/jsonrpc/client.hpp>

#define JSON_HTML_CONTENT_TYPE "application/json"

namespace koinos::net::jsonrpc {

uint32_t parse_response( const std::string& msg, call_result& result )
{
   uint32_t id;
   try
   {
      auto response = nlohmann::json::parse( msg );
      if( response.find( "id" ) == response.end() ) return 0;
      if( response.find( "jsonrpc" ) == response.end() ) return 0;
      id = response["id"].get< uint32_t >();

      if( response.find( "error" ) != response.end() ) return 0;

      if( response.find( "result" ) != response.end() )
         result = response["result"];
   }
   catch( koinos::exception &e )
   {
      result = e;
   }
   catch( boost::exception& e )
   {
      result = koinos::exception();
   }
   catch( std::exception& e )
   {
      result = koinos::exception( std::string( e.what() ) );
   }

   return id;
}

jsonrpc_client::jsonrpc_client() :
   _client( parse_response, JSON_HTML_CONTENT_TYPE ),
   _next_id( std::make_unique< std::atomic< uint32_t > >() )
{}

jsonrpc_client::jsonrpc_client( uint32_t timeout ) :
   _client( parse_response, JSON_HTML_CONTENT_TYPE, timeout ),
   _next_id( std::make_unique< std::atomic< uint32_t > >() )
{}

void jsonrpc_client::connect( const stream_protocol::endpoint& endpoint )
{
   _client.connect( endpoint );
}

bool jsonrpc_client::is_open() const
{
   return true;
}

void jsonrpc_client::close()
{
   _client.close();
}

} // koinos::net::jsonrpc
