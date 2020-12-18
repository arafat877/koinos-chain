#include <koinos/net/protocol/jsonrpc/client.hpp>
#include <koinos/net/protocol/jsonrpc/exceptions.hpp>

#define JSON_HTML_CONTENT_TYPE "application/json"

#define JSONRPC_PARSE_ERROR         -32700
#define JSONRPC_INVALID_REQUEST     -32600
#define JSONRPC_METHOD_NOT_FOUND    -32601
#define JSONRPC_INVALID_PARAMS      -32602
#define JSONRPC_INTERNAL_ERROR      -32603
#define JSONRPC_SERVER_ERROR_LOWER  -32000
#define JSONRPC_SERVER_ERROR_UPPER  -32099

namespace koinos::net::jsonrpc {

uint32_t parse_response( const std::string& msg, call_result& result )
{
   uint32_t id;
   try
   {
      auto response = nlohmann::json::parse( msg );
      if( response.find( "id" ) == response.end() )
         KOINOS_THROW( jsonrpc_exception, "Field 'id' missing from response" );
      if( response.find( "jsonrpc" ) == response.end() )
         KOINOS_THROW( jsonrpc_exception, "Field 'jsonrpc' missing from response" );
      id = response["id"].get< uint32_t >();

      if( response.find( "error" ) != response.end() )
      {
         const auto& error = response["error"];

         if( error.find( "code" ) == error.end() )
            KOINOS_THROW( jsonrpc_exception, "Error field 'code' missing from response" );

         std::string message = error.find( "message" ) != error.end() ? error["message"].get< std::string >() : "jsonrpc error";
         int32_t code = error["code"].get< int32_t >();

         switch( code )
         {
            case( JSONRPC_PARSE_ERROR ):
               KOINOS_THROW( parse_error, message );
            case( JSONRPC_INVALID_REQUEST ):
               KOINOS_THROW( invalid_request, message );
            case( JSONRPC_METHOD_NOT_FOUND ):
               KOINOS_THROW( method_not_found, message );
            case( JSONRPC_INVALID_PARAMS ):
               KOINOS_THROW( invalid_params, message );
            case( JSONRPC_INTERNAL_ERROR ):
               KOINOS_THROW( internal_error, message );
            default:
               if( code >= JSONRPC_SERVER_ERROR_LOWER && code <= JSONRPC_SERVER_ERROR_UPPER )
                  KOINOS_THROW( server_error, message );
         }
      }

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

bool jsonrpc_client::is_open() const
{
   return _client.is_open();
}

void jsonrpc_client::close()
{
   _client.close();
}

} // koinos::net::jsonrpc
