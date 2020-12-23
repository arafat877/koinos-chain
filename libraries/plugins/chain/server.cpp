
#include <thread>
#include <memory>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include <koinos/net/protocol/jsonrpc/request_handler.hpp>
#include <koinos/net/protocol/jsonrpc/types.hpp>
#include <koinos/net/transport/http/router.hpp>
#include <koinos/net/transport/http/server.hpp>

#include <koinos/plugins/chain/server.hpp>

namespace koinos::plugins::chain {

using generic_socket = boost::asio::generic::stream_protocol::socket;
using generic_endpoint = boost::asio::generic::stream_protocol::endpoint;

abstract_jsonrpc_server::abstract_jsonrpc_server() {}
abstract_jsonrpc_server::~abstract_jsonrpc_server() {}

endpoint_factory::endpoint_factory() {}

generic_endpoint endpoint_factory::create_endpoint( const std::string& address )
{
   std::string unix_prefix = "/unix/";

   if( boost::starts_with(address, unix_prefix) )
   {
      boost::filesystem::path unix_socket_path = address.substr( unix_prefix.length() );
      if( unix_socket_path.is_relative() )
         unix_socket_path = _unix_socket_base_path / unix_socket_path;
      boost::asio::local::stream_protocol::endpoint ep( unix_socket_path.string() );

      // Is there a better place to put this code?
      if( boost::filesystem::exists(unix_socket_path) )
      {
         LOG(warning) << "Removed existing socket object " << unix_socket_path.string();
         boost::filesystem::remove(unix_socket_path);
      }

      return ep;
   }

   std::string ip4_prefix = "/ip4/";
   std::string port_sep = "/tcp/";

   if( boost::starts_with(address, ip4_prefix) )
   {
      std::string addr_port = address.substr( ip4_prefix.length() );
      std::string::size_type p = addr_port.find( port_sep );
      std::string addr;
      uint16_t port = 0;
      if( p == std::string::npos )
      {
         addr = addr_port;
         port = _default_port;
      }
      else
      {
         addr = addr_port.substr( 0, p );
         port = boost::lexical_cast< decltype(port) >( addr_port.substr( p+port_sep.length() ) );
      }

      boost::asio::ip::address boost_addr = boost::asio::ip::make_address( addr );
      boost::asio::ip::tcp::endpoint ep( boost_addr, port );
      return ep;
   }

   KOINOS_THROW( malformed_network_address, "Malformed network address" );
}

void endpoint_factory::set_unix_socket_base_path( boost::filesystem::path p )
{
   _unix_socket_base_path = p;
}

void endpoint_factory::set_default_port( uint16_t default_port )
{
   _default_port = default_port;
}

struct jsonrpc_server
   : public abstract_jsonrpc_server
{
   boost::asio::io_context _ioc;

   std::shared_ptr< koinos::net::transport::http::router > _http_router;

   // std::unique_ptr< boost::asio::local::stream_protocol::socket > _socket;
   std::unique_ptr< std::thread > _thread;

   std::string _listen_addr;

   std::shared_ptr< koinos::net::protocol::jsonrpc::request_handler > _jsonrpc_request_handler;

   jsonrpc_server( generic_endpoint ep );
   virtual ~jsonrpc_server() override;

   virtual void add_method_handler( const std::string& method_name, method_handler handler ) override;
};

jsonrpc_server::jsonrpc_server( generic_endpoint ep )
{
   _http_router = std::make_shared< koinos::net::transport::http::router >();

   // Create and launch a listening port
   std::make_shared< koinos::net::transport::http::server >(
      _ioc,
      ep,
      _http_router
   )->run();

   _thread = std::make_unique< std::thread >( [&]{ _ioc.run(); } );

   /*
   _socket = std::make_unique< boost::asio::local::stream_protocol::socket >( _ioc, endpoint.protocol() );
   _socket->connect( unix_socket_path );
   */

   _jsonrpc_request_handler = std::make_shared< koinos::net::protocol::jsonrpc::request_handler >();
   _http_router->handlers[ "application/json" ] = _jsonrpc_request_handler;
}

jsonrpc_server::~jsonrpc_server()
{
   _ioc.stop();
   _thread->join();
   KOINOS_TODO( "Delete endpoint if we had a UNIX socket" );
   // boost::filesystem::remove( unix_socket.string().c_str() );
}

void jsonrpc_server::add_method_handler( const std::string& method_name, method_handler handler )
{
   _jsonrpc_request_handler->add_method_handler( method_name, handler );
}

std::unique_ptr< abstract_jsonrpc_server > create_server( generic_endpoint endpoint )
{
   return std::make_unique< jsonrpc_server >( endpoint );
}

}
