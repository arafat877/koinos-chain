#include <thread>
#include <chrono>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <boost/thread.hpp>

#include <koinos/pack/rt/string_fwd.hpp>
#include <koinos/pack/rt/json_fwd.hpp>

#include <koinos/pack/classes.hpp>
#include <koinos/plugins/block_producer/block_producer_plugin.hpp>
#include <koinos/pack/rt/binary.hpp>
#include <koinos/pack/rt/json.hpp>
#include <koinos/pack/rt/string.hpp>
#include <koinos/crypto/multihash.hpp>
#include <koinos/log.hpp>
#include <koinos/util.hpp>

namespace koinos::plugins::block_producer {

typedef boost::interprocess::basic_vectorstream< std::vector<char> > vectorstream;

static protocol::timestamp_type timestamp_now()
{
   auto duration = std::chrono::system_clock::now().time_since_epoch();
   auto ticks = std::chrono::duration_cast< std::chrono::milliseconds >( duration ).count();

   protocol::timestamp_type t;
   t.timestamp = ticks;
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
   chain_control::block_topology topology;
   chain_control::query_param_item p = chain_control::get_head_info_params();
   vectorstream ostream;
   pack::to_binary(ostream, p);
   crypto::vl_blob query_bytes{ostream.vector()};
   chain_control::submit_query query{query_bytes};
   auto& controller = appbase::app().get_plugin< chain::chain_plugin >().controller();
   auto r = controller.submit(chain_control::submit_item(query));
   chain_control::query_result_item q;
   try
   {
      auto w = std::get<chain_control::submit_return_query>(*(r.get()));
      vectorstream istream(w.result.data);
      pack::from_binary(istream, q);
      std::visit(koinos::overloaded{
         [&](chain_control::get_head_info_return& head_info) {
            active_data.height.height = head_info.height.height+1;
            topology.previous = head_info.id;
            topology.block_num = active_data.height;
         },
         []( auto& ){}
      },q);
   }
   catch (const std::exception &e)
   {
      LOG(error) << e.what();
   }

   // Check if special demo block, call proper function
   if (topology.block_num.height == 1) { this->demo_create_contract(active_data); }
   else if (topology.block_num.height == 3) { this->demo_call_contract(active_data); }

   // Serialize active data, store it in block header
   vectorstream active_stream;
   pack::to_binary(active_stream, active_data);
   crypto::vl_blob active_data_bytes{active_stream.vector()};
   block->active_bytes = active_data_bytes;

   // Hash active data and use it to sign block
   protocol::passive_block_data passive_data;
   auto digest = crypto::hash(CRYPTO_SHA2_256_ID, active_data);
   auto signature = block_signing_private_key.sign_compact(digest);
   passive_data.block_signature = signature;

   // Hash passive data
   auto passive_hash = crypto::hash(CRYPTO_SHA2_256_ID, passive_data);
   block->passive_merkle_root = passive_hash;
   block->active_bytes = active_data_bytes;

   // Serialize the header
   vectorstream header_stream;
   pack::to_binary(header_stream, *block);
   crypto::vl_blob block_header_bytes{header_stream.vector()};

   // Store hash of header as ID
   topology.id = crypto::hash(CRYPTO_SHA2_256_ID, *block);

   // Serialize the passive data
   vectorstream passive_stream;
   pack::to_binary(passive_stream, passive_data);
   crypto::vl_blob passive_data_bytes{passive_stream.vector()};

   // Create the submit block object
   chain_control::submit_block block_submission;
   block_submission.block_topo = topology;
   block_submission.block_header_bytes = block_header_bytes;
   block_submission.block_passives_bytes.push_back(passive_data_bytes);

   // Submit the block
   chain_control::submit_item si = block_submission;
   r = controller.submit(si);
   try
   {
      r.get(); // TODO: Probably should do something better here, rather than discarding the result...
   }
   catch (const std::exception &e)
   {
      LOG(error) << e.what();
   }

   LOG(info) << "produced block: " << topology;

   return block;
}

block_producer_plugin::block_producer_plugin() {}
block_producer_plugin::~block_producer_plugin() {}

void block_producer_plugin::demo_create_contract(protocol::active_block_data& active_data)
{
   LOG(info) << "Creating contract";
}

void block_producer_plugin::demo_call_contract(protocol::active_block_data& active_data)
{
   LOG(info) << "Calling contract";
}

void block_producer_plugin::plugin_initialize( const variables_map& options )
{
   std::string seed = "test seed";

   block_signing_private_key = crypto::private_key::regenerate(crypto::hash_str(CRYPTO_SHA2_256_ID, seed.c_str(), seed.size()));
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

   block_production_thread = std::make_shared< std::thread >( [&]()
                                                             {
      std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );

      while ( producing_blocks )
      {
         auto block = produce_block();

         // Sleep for the block production time
         std::this_thread::sleep_for( std::chrono::milliseconds( KOINOS_BLOCK_TIME_MS ) );
      }
   });
}

void block_producer_plugin::stop_block_production()
{
   producing_blocks = false;

   if ( block_production_thread )
      block_production_thread->join();

   block_production_thread.reset();
}

} // koinos::plugins::block_producer
