#include <koinos/net/jsonrpc/client.hpp>

#define JSON_HTML_CONTENT_TYPE "application/json"

namespace koinos::net::jsonrpc {

uint32_t parse_response( const std::string&, call_result& ) { return 0; }

jsonrpc_client::jsonrpc_client() :
   _client( parse_response, JSON_HTML_CONTENT_TYPE )
{}

void jsonrpc_client::connect( const std::string& host, uint32_t port )
{
   _client.connect( host, port );
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
