#include <koinos/net/protocol/jsonrpc/types.hpp>

namespace koinos::net::protocol::jsonrpc {

void to_json( json& j, const request& r )
{
   j = json {
      { field::jsonrpc, r.jsonrpc },
      { field::id,      r.id },
      { field::method,  r.method },
      { field::params,  r.params }
   };
}

void from_json( const json& j, request& r )
{
   j.at( field::jsonrpc ).get_to( r.jsonrpc );
   j.at( field::id ).get_to( r.id );
   j.at( field::method ).get_to( r.method );
   j.at( field::params ).get_to( r.params );
}

void to_json( json& j, const error_type& e )
{
   j = json {
      { field::code,    e.code },
      { field::message, e.message }
   };

   if ( e.data.has_value() )
      j[ field::data ] = e.data.value();
}

void from_json( const json& j, error_type& e )
{
   j.at( field::code ).get_to( e.code );
   j.at( field::message ).get_to( e.message );

   if ( j.contains( field::data ) )
      e.data = j[ field::data ];
}

void to_json( json& j, const response& r )
{
   if ( r.result.has_value() )
   {
      j = json {
         { field::jsonrpc, r.jsonrpc },
         { field::id, r.id },
         { field::result, r.result.value() }
      };
   }
   else if ( r.error.has_value() )
   {
      j = json {
         { field::jsonrpc, r.jsonrpc },
         { field::id, r.id },
         { field::error, r.error.value() }
      };
   }
   else
   {
      throw jsonrpc::exception( jsonrpc::error_code::parse_error, "failed to jsonify due to an invalid response object" );
   }
}

void from_json( const json& j, response& r )
{
   j.at( field::id ).get_to( r.id );
   j.at( field::jsonrpc ).get_to( r.jsonrpc );

   if ( j.contains( field::result ) )
   {
      r.error.reset();
      r.result = j[ field::result ];
   }
   else if ( j.contains( field::error ) )
   {
      r.result.reset();
      r.error = j[ field::error ];
   }
   else
   {
      throw jsonrpc::exception( jsonrpc::error_code::parse_error, "failed to dejsonify due to an invalid response object" );
   }
}

} // koinos::net::protocol::jsonrpc
