#pragma once

#include <memory>

#include <koinos/exception.hpp>

#include <nlohmann/json.hpp>

namespace koinos::plugins::chain {

KOINOS_DECLARE_EXCEPTION( chain_plugin_exception );

KOINOS_DECLARE_DERIVED_EXCEPTION( malformed_network_address, chain_plugin_exception );

class abstract_jsonrpc_server
{
   public:
      using json = nlohmann::json;
      using method_handler = std::function< json( const json::object_t& ) >;

      abstract_jsonrpc_server();
      virtual ~abstract_jsonrpc_server();
      virtual void add_method_handler( const std::string& method_name, method_handler handler ) = 0;
};

using generic_endpoint = boost::asio::generic::stream_protocol::endpoint;

class endpoint_factory
{
   public:
      endpoint_factory();
      void set_unix_socket_base_path( boost::filesystem::path p );
      void set_default_port( uint16_t default_port );

      generic_endpoint create_endpoint( const std::string& address );

   private:
      boost::filesystem::path _unix_socket_base_path;
      uint16_t _default_port = 8000;
};

std::unique_ptr< abstract_jsonrpc_server > create_server( generic_endpoint endpoint );

} // koinos::plugins::chain
