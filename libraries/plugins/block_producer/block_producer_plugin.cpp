#include <thread>
#include <chrono>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <boost/thread.hpp>

#include <koinos/pack/rt/json_fwd.hpp>

#include <koinos/pack/classes.hpp>
#include <koinos/plugins/block_producer/block_producer_plugin.hpp>
#include <koinos/pack/rt/binary.hpp>
#include <koinos/pack/rt/json.hpp>
#include <koinos/crypto/multihash.hpp>
#include <koinos/log.hpp>
#include <koinos/util.hpp>

#include <fstream>
#include <iterator>

namespace koinos::plugins::block_producer {

using namespace koinos::types;

using vectorstream = boost::interprocess::basic_vectorstream< std::vector< char > >;

static types::timestamp_type timestamp_now()
{
   auto duration = std::chrono::system_clock::now().time_since_epoch();
   auto ticks = std::chrono::duration_cast< std::chrono::milliseconds >( duration ).count();

   types::timestamp_type t;
   t = ticks;
   return t;
}

std::shared_ptr< protocol::block_header > block_producer_plugin::produce_block()
{
   // Make block header
   auto block = std::make_shared< protocol::block_header >();

   // Make active data, fetch timestamp
   protocol::active_block_data active_data;
   active_data.timestamp = timestamp_now();

   // Get previous block data
   rpc::block_topology topology;
   rpc::query_param_item p = rpc::get_head_info_params();
   vectorstream ostream;
   pack::to_binary( ostream, p );
   crypto::variable_blob query_bytes{ ostream.vector() };
   rpc::query_submission query{ query_bytes };
   auto& controller = appbase::app().get_plugin< chain::chain_plugin >().controller();
   auto r = controller.submit( rpc::submission_item( query ) );
   rpc::query_item_result q;
   try
   {
      auto w = std::get< rpc::query_submission_result >( *(r.get()) );
      vectorstream istream( w.result );
      pack::from_binary( istream, q );
      std::visit( koinos::overloaded {
         [&]( rpc::get_head_info_result& head_info ) {
            active_data.height = head_info.height + 1;
            topology.previous = head_info.id;
            topology.height = active_data.height;
         },
         []( auto& ){}
      }, q );
   }
   catch ( const std::exception &e )
   {
      LOG(error) << e.what();
   }

   // Check if special demo block, call proper function to add transactions
   if( wasm )
   {
      if (topology.height == 1) { demo_create_contract(active_data); }
      else if (topology.height == 2) { demo_call_contract(active_data); }
   }

   // Serialize active data, store it in block header
   vectorstream active_stream;
   pack::to_binary( active_stream, active_data );
   crypto::variable_blob active_data_bytes{ active_stream.vector() };
   block->active_bytes = active_data_bytes;

   // Hash active data and use it to sign block
   protocol::passive_block_data passive_data;
   auto digest = crypto::hash( CRYPTO_SHA2_256_ID, active_data );
   auto signature = block_signing_private_key.sign_compact( digest );
   passive_data.block_signature = signature;

   // Hash passive data
   auto passive_hash = crypto::hash( CRYPTO_SHA2_256_ID, passive_data );
   block->passive_merkle_root = passive_hash;
   block->active_bytes = active_data_bytes;

   // Serialize the header
   vectorstream header_stream;
   pack::to_binary( header_stream, *block );
   crypto::variable_blob block_header_bytes{ header_stream.vector() };

   // Store hash of header as ID
   topology.id = crypto::hash( CRYPTO_SHA2_256_ID, *block );

   // Serialize the passive data
   vectorstream passive_stream;
   pack::to_binary( passive_stream, passive_data );
   crypto::variable_blob passive_data_bytes{ passive_stream.vector() };

   // Create the submit block object
   rpc::block_submission block_submission;
   block_submission.topology = topology;
   block_submission.header_bytes = block_header_bytes;
   block_submission.passives_bytes.push_back( passive_data_bytes );


   // Submit the block
   rpc::submission_item si = block_submission;
   r = controller.submit( si );
   try
   {
      r.get(); // TODO: Probably should do something better here, rather than discarding the result...
   }
   catch ( const std::exception &e )
   {
      LOG(error) << e.what();
   }

   LOG(info) << "produced block: " << topology;

   return block;
}

block_producer_plugin::block_producer_plugin() {}
block_producer_plugin::~block_producer_plugin() {}

void block_producer_plugin::set_program_options( options_description& cli, options_description& cfg )
{
   cfg.add_options()
         ("target-wasm", bpo::value<bfs::path>(),
            "the location of a demo wasm file (absolute path or relative to application data dir)")
         ;
}

void block_producer_plugin::plugin_initialize( const appbase::variables_map& options )
{
   std::string seed = "test seed";

   block_signing_private_key = crypto::private_key::regenerate( crypto::hash_str( CRYPTO_SHA2_256_ID, seed.c_str(), seed.size() ) );

   if( options.count("target-wasm") )
   {
      auto wasm_target = options.at("target-wasm").as<bfs::path>();
      if( !bfs::is_directory( wasm_target ) )
      {
         if( wasm_target.is_relative() )
            wasm = appbase::app().data_dir() / wasm_target;
         else
            wasm = wasm_target;
      }
   }
}

void block_producer_plugin::plugin_startup()
{
   start_block_production();
}

void block_producer_plugin::plugin_shutdown()
{
   stop_block_production();
}

void block_producer_plugin::start_block_production()
{
   producing_blocks = true;

   block_production_thread = std::make_shared< std::thread >( [&]() {
      std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );

      while ( producing_blocks )
      {
         auto block = produce_block();

         // Sleep for the block production time
         std::this_thread::sleep_for( std::chrono::milliseconds( KOINOS_BLOCK_TIME_MS ) );
      }
   } );
}

void block_producer_plugin::stop_block_production()
{
   producing_blocks = false;

   if ( block_production_thread )
      block_production_thread->join();

   block_production_thread.reset();
}

void block_producer_plugin::demo_create_contract( types::protocol::active_block_data& active_data )
{
   LOG(info) << "Creating contract";

   // Create the operation, fill the contract code
   // We will leave extensions and id at default for now
   types::protocol::create_system_contract_operation create_op;
   LOG(info) << wasm->native();
   std::fstream wasm_stream( wasm->native() );
   std::istream_iterator< uint8_t > wasm_itr( wasm_stream );
   std::istream_iterator< uint8_t > end;
   create_op.bytecode.insert( create_op.bytecode.end(), wasm_itr, end );
   LOG(info) << create_op.bytecode.size();

   auto id = crypto::hash( CRYPTO_RIPEMD160_ID, 1 );
   for (int i = 0; i < 20; i++) { create_op.contract_id[i] = id.digest[i]; }

   types::protocol::operation o = create_op;

   types::protocol::transaction_type transaction;
   transaction.operations.push_back(pack::to_variable_blob( o ) );

   active_data.transactions.push_back(pack::to_variable_blob( transaction ) );
}

void block_producer_plugin::demo_call_contract( types::protocol::active_block_data& active_data )
{
   types::protocol::contract_call_operation call_op;

   auto id = crypto::hash( CRYPTO_RIPEMD160_ID, 1 );
   for (int i = 0; i < 20; i++) { call_op.contract_id[i] = id.digest[i]; }

   LOG(info) << "Calling contract";
   types::protocol::operation o = call_op;

   types::protocol::transaction_type transaction;
   transaction.operations.push_back( pack::to_variable_blob( o ) );
   active_data.transactions.push_back( pack::to_variable_blob( transaction ) );
}

} // koinos::plugins::block_producer
