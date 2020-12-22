#pragma once

#include <thread>
#include <memory>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include <koinos/net/protocol/jsonrpc/request_handler.hpp>
#include <koinos/net/protocol/jsonrpc/types.hpp>
#include <koinos/net/transport/http/router.hpp>
#include <koinos/net/transport/http/server.hpp>

namespace koinos::plugins::chain {

struct jsonrpc_server
{
   boost::asio::io_context _ioc;

   std::shared_ptr< koinos::net::transport::http::router > _http_router;

   std::unique_ptr< boost::asio::local::stream_protocol::socket > _socket;
   std::unique_ptr< std::thread > _thread;

   std::string _listen_addr;

   std::shared_ptr< koinos::net::protocol::jsonrpc::request_handler > _jsonrpc_request_handler;

   jsonrpc_server( const std::string& listen_addr )
      : _listen_addr( listen_addr )
   {
      _http_router = std::make_shared< koinos::net::transport::http::router >();

      // For now, listen_addr is always a UNIX socket
      std::string unix_socket_path = listen_addr;

      boost::asio::local::stream_protocol::endpoint endpoint( unix_socket_path );

      // Create and launch a listening port
      std::make_shared< koinos::net::transport::http::server >(
         _ioc,
         endpoint,
         _http_router
      )->run();

      _thread = std::make_unique< std::thread >( [&]{ _ioc.run(); } );

      _socket = std::make_unique< boost::asio::local::stream_protocol::socket >( _ioc, endpoint.protocol() );
      _socket->connect( unix_socket_path );

      _jsonrpc_request_handler = std::make_shared< koinos::net::protocol::jsonrpc::request_handler >();
      _http_router->handlers[ "application/json" ] = _jsonrpc_request_handler;
   }

   ~jsonrpc_server()
   {
      _ioc.stop();
      _thread->join();
   }

   void add_method_handler( const std::string& method_name, koinos::net::protocol::jsonrpc::method_handler handler )
   {
      _jsonrpc_request_handler->add_method_handler( method_name, handler );
   }
};

} // koinos::plugins::chain
