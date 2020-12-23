#include <koinos/plugins/chain/chain_plugin.hpp>
#include <koinos/plugins/chain/reqhandler.hpp>
#include <koinos/plugins/chain/server.hpp>

#include <koinos/pack/rt/binary.hpp>
#include <koinos/pack/rt/json.hpp>

#include <koinos/log.hpp>
#include <koinos/util.hpp>

#include <mira/database_configuration.hpp>

#include <nlohmann/json.hpp>

#include <memory>

#define MAX_JSON_DEPTH 30

namespace koinos::plugins::chain {

using namespace appbase;

namespace detail {

using json = nlohmann::json;

class chain_plugin_impl
{
   public:
      chain_plugin_impl()  {}
      ~chain_plugin_impl() {}

      void write_default_database_config( bfs::path &p );
      void start_server();
      void stop_server();

      bfs::path            state_dir;
      bfs::path            database_cfg;

      reqhandler           _reqhandler;
      endpoint_factory     _endpoint_factory;
      std::string          _listen_addr;
      std::unique_ptr< abstract_jsonrpc_server >  _server;
};

void chain_plugin_impl::write_default_database_config( bfs::path &p )
{
   LOG(info) << "writing database configuration: " << p.string();
   boost::filesystem::ofstream config_file( p, std::ios::binary );
   config_file << mira::utilities::default_database_configuration();
}

void chain_plugin_impl::start_server()
{
   generic_endpoint ep = _endpoint_factory.create_endpoint( _listen_addr );
   _server = create_server( ep );
   _server->add_method_handler( "call", [&]( const json::object_t& j ) -> json
   {
      LOG(info) << "Got json value:" << json(j).dump();
      koinos::types::rpc::submission_item item;
      json j_params = j;
      koinos::pack::from_json( j, item, 0 );
      std::future< std::shared_ptr< types::rpc::submission_result > > fut = _reqhandler.submit( item );
      std::shared_ptr< types::rpc::submission_result > result = fut.get();
      json j_result;
      koinos::pack::to_json( j_result, *result );
      return j_result;
   } );
}

void chain_plugin_impl::stop_server()
{
   _server.reset();
}

} // detail


chain_plugin::chain_plugin() : my( new detail::chain_plugin_impl() ) {}
chain_plugin::~chain_plugin(){}

bfs::path chain_plugin::state_dir() const
{
   return my->state_dir;
}

void chain_plugin::set_program_options( options_description& cli, options_description& cfg )
{
   cfg.add_options()
         ("state-dir", bpo::value<bfs::path>()->default_value("blockchain"),
            "the location of the blockchain state files (absolute path or relative to application data dir)")
         ("database-config", bpo::value<bfs::path>()->default_value("database.cfg"), "The database configuration file location")
         ("listen", bpo::value<std::string>()->default_value("/unix/koinosd.sock"), "Multiaddress to listen")
         ;
   cli.add_options()
         ("force-open", bpo::bool_switch()->default_value(false), "force open the database, skipping the environment check")
         ;
}

void chain_plugin::plugin_initialize( const variables_map& options )
{
   my->state_dir = app().data_dir() / "blockchain";

   if( options.count("state-dir") )
   {
      auto sfd = options.at("state-dir").as<bfs::path>();
      if(sfd.is_relative())
         my->state_dir = app().data_dir() / sfd;
      else
         my->state_dir = sfd;
   }

   my->database_cfg = options.at( "database-config" ).as< bfs::path >();

   if( my->database_cfg.is_relative() )
      my->database_cfg = app().data_dir() / my->database_cfg;

   if( !bfs::exists( my->database_cfg ) )
   {
      my->write_default_database_config( my->database_cfg );
   }

   my->_endpoint_factory.set_unix_socket_base_path( app().data_dir() );
   my->_endpoint_factory.set_default_port( 9400 );

   my->_listen_addr = options.at("listen").as< std::string >();
   LOG(info) << "Listening on " << my->_listen_addr;
}

void chain_plugin::plugin_startup()
{
   // Check for state directory, and create if necessary
   if ( !bfs::exists( my->state_dir ) ) { bfs::create_directory( my->state_dir ); }

   nlohmann::json database_config;

   try
   {
      boost::filesystem::ifstream config_file( my->database_cfg, std::ios::binary );
      config_file >> database_config;
   }
   catch ( const std::exception& e )
   {
      LOG(error) << "error while parsing database configuration: " << e.what();
      exit( EXIT_FAILURE );
   }

   try
   {
      my->_reqhandler.open( my->state_dir, database_config );
   }
   catch( std::exception& e )
   {
      LOG(error) << "error opening database: " << e.what();
      exit( EXIT_FAILURE );
   }

   my->_reqhandler.start_threads();
   my->start_server();
}

void chain_plugin::plugin_shutdown()
{
   LOG(info) << "stop jsonrpc server";
   my->stop_server();
   LOG(info) << "closing chain database";
   KOINOS_TODO( "We eventually need to call close() from somewhere" )
   //my->db.close();
   my->_reqhandler.stop_threads();
   LOG(info) << "database closed successfully";
}

std::future< std::shared_ptr< koinos::types::rpc::submission_result > > chain_plugin::submit( const koinos::types::rpc::submission_item& item )
{
   return my->_reqhandler.submit( item );
}


} // namespace koinos::plugis::chain
