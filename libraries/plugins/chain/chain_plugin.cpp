#include <koinos/plugins/chain/chain_plugin.hpp>
#include <koinos/plugins/chain/reqhandler.hpp>

#include <koinos/mq/message_broker.hpp>

#include <koinos/log.hpp>
#include <koinos/util.hpp>

#include <mira/database_configuration.hpp>
#include <atomic>
#include <thread>

namespace koinos::plugins::chain {

using namespace appbase;

namespace detail {

class chain_plugin_impl
{
   public:
      chain_plugin_impl()  {}
      ~chain_plugin_impl() {}

      void write_default_database_config( bfs::path &p );

      bfs::path                  state_dir;
      bfs::path                  database_cfg;

      reqhandler                 _reqhandler;
//      koinos::mq::message_broker _message_broker;
      std::atomic< bool >            _process_messages = true;
      std::unique_ptr< std::thread > _message_thread;
};

void chain_plugin_impl::write_default_database_config( bfs::path &p )
{
   LOG(info) << "writing database configuration: " << p.string();
   boost::filesystem::ofstream config_file( p, std::ios::binary );
   config_file << mira::utilities::default_database_configuration();
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

   my->_process_messages = true;
   my->_message_thread = std::make_unique< std::thread >( [&]
   {
      koinos::mq::message_broker broker;
      broker.connect( "localhost", 5672 );
      broker.queue_declare( "my_cool_queue" );
      broker.queue_bind( "my_cool_queue", koinos::mq::exchange::event, "my_cool_binding_key" );
      for (;;)
      {
         if ( !my->_process_messages )
            break;

         auto result = broker.consume();

         if ( result.first == koinos::mq::error_code::time_out )
            continue;

         if ( result.first != koinos::mq::error_code::success )
         {
            LOG(error) << "something bad has happened";
         }
         else
         {
            LOG(info) << "message: " << result.second.value().data;
         }
      }
   } );
}

void chain_plugin::plugin_shutdown()
{
   LOG(info) << "closing chain database";
   KOINOS_TODO( "We eventually need to call close() from somewhere" )
   //my->db.close();
   my->_process_messages = false;
   my->_message_thread->join();
   my->_reqhandler.stop_threads();
   LOG(info) << "database closed successfully";
}

std::future< std::shared_ptr< koinos::types::rpc::submission_result > > chain_plugin::submit( const koinos::types::rpc::submission_item& item )
{
   return my->_reqhandler.submit( item );
}


} // namespace koinos::plugis::chain
