#include <koinos/net/protocol/jsonrpc/client.hpp>

namespace koinos::net::protocol::jsonrpc {

constexpr const char* json_content_type = "application/json";

uint32_t parse_response( const std::string& msg, call_result& result )
{
   uint32_t id = 0;
   try
   {
      auto response = nlohmann::json::parse( msg );
      if( response.find( field::id ) == response.end() )
      {
         result = exception( "Field 'id' missing from response" );
         return id;
      }

      id = response[field::id].get< uint32_t >();

      if( response.find( field::jsonrpc ) == response.end() )
      {
         result = exception( "Field 'jsonrpc' missing from response" );
      }
      else if( response.find( field::error ) != response.end() )
      {
         const auto& error = response[field::error];

         if( error.find( field::code ) == error.end() )
         {
            result = exception( "Error field 'code' missing from response" );
         }
         else
         {
            std::string message = error.find( field::message ) != error.end() ? error[field::message].get< std::string >() : "jsonrpc error";

            result = exception( message );
         }
      }
      else if( response.find( field::result ) != response.end() )
      {
         result = response[field::result];
      }
      else
      {
         result = exception( "JSONRPC response MUST include either field 'error' or 'response'" );
      }
   }
   catch( const std::exception& e )
   {
      result = exception( e.what() );
   }
   catch( ... )
   {
      result = exception( "Unexpected error parsing response" );
   }

   return id;
}

client::client() :
   _client( parse_response, json_content_type ),
   _next_id( std::make_unique< std::atomic< uint32_t > >(1) )
{}

client::client( uint32_t timeout ) :
   _client( parse_response, json_content_type, timeout ),
   _next_id( std::make_unique< std::atomic< uint32_t > >(1) )
{}

bool client::is_open() const
{
   return _client.is_open();
}

void client::close()
{
   _client.close();
}

} // koinos::net::jsonrpc
