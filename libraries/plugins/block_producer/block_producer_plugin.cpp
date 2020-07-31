#include <thread>
#include <chrono>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <boost/thread.hpp>

#include <koinos/pack/classes.hpp>
#include <koinos/pack/system_call_ids.hpp>
#include <koinos/plugins/block_producer/block_producer_plugin.hpp>
#include <koinos/plugins/block_producer/util/block_util.hpp>
#include <koinos/crypto/multihash.hpp>
#include <koinos/log.hpp>
#include <koinos/util.hpp>

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

std::shared_ptr< protocol::block > block_producer_plugin::produce_block()
{
   // Make block header
   auto block = std::make_shared< protocol::block >();

   // Make active data, fetch timestamp
   protocol::active_block_data active_data;
   active_data.timestamp = timestamp_now();
   active_data.header_hashes.digests.resize( size_t(types::protocol::header_hash_index::NUM_HEADER_HASHES) );

   // Get previous block data
   rpc::block_topology topology;

   auto& controller = appbase::app().get_plugin< chain::chain_plugin >().controller();
   auto r = controller.submit( rpc::query_submission( rpc::get_head_info_params() ) );

   try
   {
      auto res = std::get< rpc::query_submission_result >( *r.get() );
      res.unbox();
      std::visit( koinos::overloaded {
         [&]( const rpc::get_head_info_result& head_info ) {
            active_data.height = head_info.height + 1;
            active_data.header_hashes.digests[(uint32_t)types::protocol::header_hash_index::previous_block_hash_index] = head_info.id.digest;
            topology.previous = head_info.id;
            topology.height = active_data.height;
         },
         []( const auto& ){}
      }, res.get_const_native() );
   }
   catch ( const std::exception &e )
   {
      LOG(error) << e.what();
      return block;
   }

   if( topology.height == 8 )
   {
      demo_do_pow( *block );
   }
   else if( topology.height == 9 )
   {
      pow = std::make_shared< sha256_pow >();
   }

   // TODO: Add transactions from the mempool

   // Add passive data
   block->passive_data = protocol::passive_block_data();

   // Serialize active data, store it in block header
   block->active_data = std::move( active_data );

   util::set_block_merkle_roots( *block, CRYPTO_SHA2_256_ID );

   // Store hash of header as ID
   topology.id = crypto::hash( CRYPTO_SHA2_256_ID, block->active_data );

   auto nonce = pow->generate_pow( topology.id, 23, 1, ~0 );
   if ( !nonce.has_value() ) { return std::shared_ptr< protocol::block >(); }

   if (!*nonce)
   {
      util::sign_block( *block, block_signing_private_key );
   }
   else
   {
      pack::to_variable_blob( block->signature_data, *nonce );
   }

   // Submit the block
   r = controller.submit( rpc::block_submission{
      .topology = topology,
      .block = *block,
      .verify_passive_data = true,
      .verify_block_signature = true,
      .verify_transaction_signatures = true } );
   try
   {
      r.get(); // TODO: Probably should do something better here, rather than discarding the result...
   }
   catch ( const std::exception &e )
   {
      LOG(error) << e.what();
      block.reset();
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
   this->pow = std::make_shared< timed_block_generation >( KOINOS_BLOCK_TIME_MS );
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
         // std::this_thread::sleep_for( std::chrono::milliseconds( KOINOS_BLOCK_TIME_MS ) );
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

void block_producer_plugin::demo_do_pow( types::protocol::block& block )
{
   LOG(info) << "Uploading and ovewriting consensus to PoW";

   // Create the operation, fill the contract code
   // We will leave extensions and id at default for now
   types::protocol::create_system_contract_operation create_op;
   LOG(info) << wasm->native();
   std::fstream wasm_stream( wasm->native() );
   std::istreambuf_iterator< char > wasm_itr( wasm_stream );
   std::istreambuf_iterator< char > end;
   create_op.bytecode.insert( create_op.bytecode.end(), wasm_itr, end );
   LOG(info) << create_op.bytecode.size();

   auto id = crypto::hash( CRYPTO_RIPEMD160_ID, 1 );
   for (int i = 0; i < 20; i++) { create_op.contract_id[i] = id.digest[i]; }

   types::protocol::operation o = create_op;

   types::protocol::transaction transaction;
   transaction.operations.emplace_back( std::move( create_op ) );

   types::protocol::set_system_call_operation set_op;
   set_op.call_id = (uint32_t)types::system::system_call_id::verify_block_sig;
   types::system::contract_call_bundle call_bundle;
   for (int i = 0; i < 20; i++) { call_bundle.contract_id[i] = id.digest[i]; }
   call_bundle.entry_point = 0;
   set_op.target = call_bundle;

   transaction.operations.emplace_back( std::move( set_op ) );

   block.transactions.emplace_back( std::move( transaction ) );
}


} // koinos::plugins::block_producer
