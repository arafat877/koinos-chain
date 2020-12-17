#include <koinos/plugins/chain/chain_plugin.hpp>

#include <koinos/log.hpp>
#include <koinos/util.hpp>
#include <koinos/net/client.hpp>

#include <mira/database_configuration.hpp>

namespace koinos::plugins::chain {

using namespace appbase;

namespace detail {

class chain_plugin_impl
{
   public:
      chain_plugin_impl()  {}
      ~chain_plugin_impl() {}

      void write_default_database_config( bfs::path &p );
      void connect_to_api_endpoint( const std::string& config_key, const std::string& endpoint_str, std::shared_ptr< net::client >& api );

      bfs::path                        state_dir;
      bfs::path                        database_cfg;

      koinos::chain::controller        controller;
      std::shared_ptr< net::client >   blockstore_api;
};

void chain_plugin_impl::write_default_database_config( bfs::path &p )
{
   LOG(info) << "writing database configuration: " << p.string();
   boost::filesystem::ofstream config_file( p, std::ios::binary );
   config_file << mira::utilities::default_database_configuration();
}

void chain_plugin_impl::connect_to_api_endpoint( const std::string& api_name, const std::string& endpoint_str, std::shared_ptr< net::client >& api )
{
   try {
      size_t port_loc = endpoint_str.find( ":" );

      boost::system::error_code ec;

      if( port_loc != std::string::npos )
      {
         std::string address_str = endpoint_str.substr( 0, port_loc );
         int32_t port = std::atoi( endpoint_str.c_str() + port_loc + 1 );

         if( port < 0 || port > 0xFFFF )
         {
            LOG(error) << api_name << " port is out of range";
            exit( EXIT_FAILURE );
         }

         auto address = boost::asio::ip::make_address( address_str, ec );
         if( ec )
         {
            LOG(error) << api_name << " address error";
            LOG(error) << ec.message();
            exit( EXIT_FAILURE );
         }

         boost::asio::ip::tcp::endpoint endpoint( address, uint16_t(port) );
         api->connect( endpoint );
      }
      else
      {
         bfs::path socket_path( endpoint_str );
         if( socket_path.is_relative() )
            socket_path = app().data_dir() / socket_path;

         bfs::create_directories( socket_path.parent_path(), ec );
         if( ec )
         {
            LOG(error) << "Error creating " << api_name << "endpoint";
            LOG(error) << ec.message();
            exit( EXIT_FAILURE );
         }

         boost::asio::local::stream_protocol::endpoint endpoint( socket_path.string() );
         api->connect( endpoint );
      }
   }
   KOINOS_CATCH_LOG_AND_RETHROW(error)
   catch( boost::exception& e )
   {
      LOG(error) << "Exception when connecting to " << api_name << ":\n" << boost::diagnostic_information( e );
      exit( EXIT_FAILURE );
   }
   catch( std::exception& e )
   {
      LOG(error) << "Exception when connecting to " << api_name << ": " << e.what();
      exit( EXIT_FAILURE );
   }
}

} // detail


chain_plugin::chain_plugin() : my( new detail::chain_plugin_impl() ) {}
chain_plugin::~chain_plugin(){}

koinos::chain::controller& chain_plugin::controller() { return my->controller; }
const koinos::chain::controller& chain_plugin::controller() const { return my->controller; }

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
         ("blockstore-endpoint", bpo::value<std::string>()->default_value("sockets/block_store"), "The block store endpoint to connect to")
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

   if( options.count( "blockstore-endpoint" ) )
   {
      my->blockstore_api = std::make_shared< net::client >( net::jsonrpc::jsonrpc_client() );
      auto bs_str = options.at( "blockstore-endpoint" ).as< std::string >();
      my->connect_to_api_endpoint( "blockstore", bs_str, my->blockstore_api );
   }
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
      my->controller.open( my->state_dir, database_config );
   }
   catch( std::exception& e )
   {
      LOG(error) << "error opening database: " << e.what();
      exit( EXIT_FAILURE );
   }

   my->controller.start_threads();
}

void chain_plugin::plugin_shutdown()
{
   LOG(info) << "closing chain database";
   KOINOS_TODO( "We eventually need to call close() from somewhere" )
   //my->db.close();
   my->controller.stop_threads();
   LOG(info) << "database closed successfully";
}

} // namespace koinos::plugis::chain
