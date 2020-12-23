#include <koinos/net/protocol/jsonrpc/client.hpp>
#include <koinos/net/protocol/jsonrpc/exceptions.hpp>
#include <koinos/net/protocol/jsonrpc/types.hpp>

namespace koinos::net::protocol::jsonrpc {

constexpr const char* json_content_type = "application/json";

uint32_t parse_response( const std::string& msg, call_result& result )
{
   uint32_t id;
   try
   {
      auto response = nlohmann::json::parse( msg );
      if( response.find( field::id ) == response.end() )
         KOINOS_THROW( jsonrpc_exception, "Field 'id' missing from response" );
      if( response.find( field::jsonrpc ) == response.end() )
         KOINOS_THROW( jsonrpc_exception, "Field 'jsonrpc' missing from response" );
      id = response[field::id].get< uint32_t >();

      if( response.find( field::error ) != response.end() )
      {
         const auto& error = response[field::error];

         if( error.find( field::code ) == error.end() )
            KOINOS_THROW( jsonrpc_exception, "Error field 'code' missing from response" );

         std::string message = error.find( field::message ) != error.end() ? error[field::message].get< std::string >() : "jsonrpc error";
         error_code code = error[field::code].get< error_code >();

         switch( code )
         {
            case( error_code::parse_error ):
               KOINOS_THROW( parse_error, message );
            case( error_code::invalid_request ):
               KOINOS_THROW( invalid_request, message );
            case( error_code::method_not_found ):
               KOINOS_THROW( method_not_found, message );
            case( error_code::invalid_params ):
               KOINOS_THROW( invalid_params, message );
            case( error_code::internal_error ):
               KOINOS_THROW( internal_error, message );
            default:
               if( code >= error_code::server_error_lower && code <= error_code::server_error )
                  KOINOS_THROW( server_error, message );
               throw exception( message );
         }
      }

      if( response.find( field::result ) != response.end() )
         result = response[field::result];
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

client::client() :
   _client( parse_response, json_content_type ),
   _next_id( std::make_unique< std::atomic< uint32_t > >() )
{}

client::client( uint32_t timeout ) :
   _client( parse_response, json_content_type, timeout ),
   _next_id( std::make_unique< std::atomic< uint32_t > >() )
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
