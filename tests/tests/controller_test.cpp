#include <boost/test/unit_test.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <koinos/chain/controller.hpp>
#include <koinos/chain/exceptions.hpp>
#include <koinos/chain/system_calls.hpp>
#include <koinos/crypto/multihash.hpp>
#include <koinos/crypto/elliptic.hpp>
#include <koinos/pack/rt/binary.hpp>

#include <mira/database_configuration.hpp>

#include <chrono>
#include <filesystem>
#include <sstream>

using namespace std::string_literals;

#define TEST_CHAIN_ID_SEED "koinos"s

struct controller_fixture
{
   controller_fixture()
   {
      auto seed = "test seed"s;
      _block_signing_private_key = koinos::crypto::private_key::regenerate( koinos::crypto::hash_str( CRYPTO_SHA2_256_ID, seed.c_str(), seed.size() ) );

      _state_dir = std::filesystem::temp_directory_path() / boost::filesystem::unique_path().string();
      LOG(info) << "Test temp dir: " << _state_dir.string();
      std::filesystem::create_directory( _state_dir );

      auto database_config = mira::utilities::default_database_configuration();

      koinos::chain::genesis_data genesis_data;
      auto chain_id = koinos::crypto::hash( CRYPTO_SHA2_256_ID, TEST_CHAIN_ID_SEED );
      genesis_data[ KOINOS_STATEDB_CHAIN_ID_KEY ] = koinos::pack::to_variable_blob( chain_id );

      _controller.open( _state_dir, database_config ,genesis_data, false );
   }

   virtual ~controller_fixture()
   {
      std::filesystem::remove_all( _state_dir );
   }

   void set_block_merkle_roots( koinos::protocol::block& block, uint64_t code, uint64_t size = 0 )
   {
      std::vector< koinos::multihash > trx_active_hashes( block.transactions.size() );
      std::vector< koinos::multihash > passive_hashes( 2 * ( block.transactions.size() + 1 ) );

      passive_hashes[0] = koinos::crypto::hash( code, block.passive_data, size );
      passive_hashes[1] = koinos::crypto::empty_hash( code, size );

      // Hash transaction actives, passives, and signatures for merkle roots
      for ( size_t i = 0; i < block.transactions.size(); i++ )
      {
         trx_active_hashes[i]      = koinos::crypto::hash(      code, block.transactions[i].active_data,    size );
         passive_hashes[2*(i+1)]   = koinos::crypto::hash(      code, block.transactions[i].passive_data,   size );
         passive_hashes[2*(i+1)+1] = koinos::crypto::hash_blob( code, block.transactions[i].signature_data, size );
      }

      koinos::crypto::merkle_hash_leaves( trx_active_hashes, code, size );
      koinos::crypto::merkle_hash_leaves( passive_hashes,    code, size );

      block.active_data->transaction_merkle_root  = trx_active_hashes[0];
      block.active_data->passive_data_merkle_root = passive_hashes[0];
   }

   void sign_block( koinos::protocol::block& block, koinos::crypto::private_key& block_signing_key )
   {
      koinos::pack::to_variable_blob(
         block.signature_data,
         block_signing_key.sign_compact(
            koinos::crypto::hash_n( CRYPTO_SHA2_256_ID, block.header, block.active_data )
         )
      );
   }

   koinos::chain::controller   _controller;
   std::filesystem::path       _state_dir;
   koinos::crypto::private_key _block_signing_private_key;
};

BOOST_FIXTURE_TEST_SUITE( controller_tests, controller_fixture )

BOOST_AUTO_TEST_CASE( submission_tests )
{ try {
   using namespace koinos;

   BOOST_TEST_MESSAGE( "Test submit transaction" );

   auto key = crypto::private_key::generate_from_seed( crypto::hash( CRYPTO_SHA2_256_ID, "foobar"s ) );
   rpc::chain::submit_transaction_request trx_req;
   trx_req.transaction.active_data.make_mutable();
   trx_req.transaction.active_data->operations.push_back( protocol::nop_operation() );
   trx_req.transaction.active_data->resource_limit = 20;
   trx_req.transaction.id = crypto::hash( CRYPTO_SHA2_256_ID, trx_req.transaction.active_data );
   auto signature = key.sign_compact( trx_req.transaction.id );
   trx_req.transaction.signature_data = variable_blob( signature.begin(), signature.end() );

   _controller.submit_transaction( trx_req );

   trx_req.transaction.active_data.make_mutable();
   trx_req.transaction.active_data->operations.push_back( protocol::reserved_operation() );
   trx_req.transaction.active_data->resource_limit = 10;
   trx_req.transaction.id = crypto::hash( CRYPTO_SHA2_256_ID, trx_req.transaction.active_data );
   signature = key.sign_compact( trx_req.transaction.id );
   trx_req.transaction.signature_data = variable_blob( signature.begin(), signature.end() );

   BOOST_CHECK_THROW( _controller.submit_transaction( trx_req ), chain::reserved_operation_exception );

   BOOST_TEST_MESSAGE( "Test submit block" );
   BOOST_TEST_MESSAGE( "Error when first block does not have height of 1" );

   rpc::chain::submit_block_request block_req;
   block_req.verify_passive_data = true;
   block_req.verify_block_signature = true;
   block_req.verify_transaction_signatures = true;

   auto duration = std::chrono::system_clock::now().time_since_epoch();
   block_req.block.header.timestamp = std::chrono::duration_cast< std::chrono::milliseconds >( duration ).count();
   block_req.block.header.height = 2;
   block_req.block.header.previous = crypto::zero_hash( CRYPTO_SHA2_256_ID );

   set_block_merkle_roots( block_req.block, CRYPTO_SHA2_256_ID );
   sign_block( block_req.block, _block_signing_private_key );

   block_req.block.id = crypto::hash_n( CRYPTO_SHA2_256_ID, block_req.block.header, block_req.block.active_data );

   BOOST_CHECK_THROW( _controller.submit_block( block_req ), chain::root_height_mismatch );


   BOOST_TEST_MESSAGE( "Error when signature does not match" );

   block_req.block.active_data.make_mutable();
   block_req.block.active_data->signer_address = crypto::hash( CRYPTO_SHA2_256_ID, std::string( "random" ) );
   block_req.block.header.height = 1;
   block_req.block.id = crypto::hash_n( CRYPTO_SHA2_256_ID, block_req.block.header, block_req.block.active_data );

   BOOST_CHECK_THROW( _controller.submit_block( block_req ), chain::invalid_block_signature );


   BOOST_TEST_MESSAGE( "Error when previous block does not match" );

   block_req.block.header.previous = crypto::empty_hash( CRYPTO_SHA2_256_ID );
   block_req.block.active_data.make_mutable();

   set_block_merkle_roots( block_req.block, CRYPTO_SHA2_256_ID );
   sign_block( block_req.block, _block_signing_private_key );

   BOOST_CHECK_THROW( _controller.submit_block( block_req ), chain::unknown_previous_block );


   BOOST_TEST_MESSAGE( "Test succesful block" );

   block_req.block.header.previous = crypto::zero_hash( CRYPTO_SHA2_256_ID );
   block_req.block.active_data.make_mutable();

   set_block_merkle_roots( block_req.block, CRYPTO_SHA2_256_ID );
   sign_block( block_req.block, _block_signing_private_key );

   _controller.submit_block( block_req );


   BOOST_TEST_MESSAGE( "Test chain ID retrieval" );

   BOOST_CHECK_EQUAL( _controller.get_chain_id().chain_id, koinos::crypto::hash( CRYPTO_SHA2_256_ID, TEST_CHAIN_ID_SEED ) );

} KOINOS_CATCH_LOG_AND_RETHROW(info) }

BOOST_AUTO_TEST_CASE( block_irreversibility )
{ try {
   using namespace koinos;

   rpc::chain::submit_block_request block_req;
   block_req.verify_passive_data = true;
   block_req.verify_block_signature = true;
   block_req.verify_transaction_signatures = true;

   auto head_info_res = _controller.get_head_info();

   for( int i = 1; i <= 6; i++ )
   {
      auto duration = std::chrono::system_clock::now().time_since_epoch();
      block_req.block.active_data.make_mutable();
      block_req.block.header.timestamp = std::chrono::duration_cast< std::chrono::milliseconds >( duration ).count();
      block_req.block.header.height    = head_info_res.head_topology.height + 1;
      block_req.block.header.previous  = head_info_res.head_topology.id;

      set_block_merkle_roots( block_req.block, CRYPTO_SHA2_256_ID );
      sign_block( block_req.block, _block_signing_private_key );

      block_req.block.id = crypto::hash_n( CRYPTO_SHA2_256_ID, block_req.block.header, block_req.block.active_data );

      _controller.submit_block( block_req );

      head_info_res = _controller.get_head_info();

      BOOST_REQUIRE( head_info_res.last_irreversible_height == 0 );
   }

   for( int i = 7; i <= 10; i++ )
   {
      auto duration = std::chrono::system_clock::now().time_since_epoch();
      block_req.block.active_data.make_mutable();
      block_req.block.header.timestamp = std::chrono::duration_cast< std::chrono::milliseconds >( duration ).count();
      block_req.block.header.height    = head_info_res.head_topology.height + 1;
      block_req.block.header.previous  = head_info_res.head_topology.id;

      set_block_merkle_roots( block_req.block, CRYPTO_SHA2_256_ID );
      sign_block( block_req.block, _block_signing_private_key );

      block_req.block.id = crypto::hash_n( CRYPTO_SHA2_256_ID, block_req.block.header, block_req.block.active_data );

      _controller.submit_block( block_req );

      head_info_res = _controller.get_head_info();

      BOOST_REQUIRE( head_info_res.last_irreversible_height == i - 6 );
   }

} KOINOS_CATCH_LOG_AND_RETHROW(info) }

BOOST_AUTO_TEST_CASE( fork_heads )
{ try {
   using namespace koinos;

   BOOST_TEST_MESSAGE( "Setting up forks and checking heads" );

   rpc::chain::submit_block_request block_req;
   block_req.verify_passive_data = true;
   block_req.verify_block_signature = true;
   block_req.verify_transaction_signatures = true;

   uint64_t test_timestamp = 1609459200;

   auto root_head_info = _controller.get_head_info();
   auto head_info = root_head_info;

   for( int i = 1; i <= 6; i++ )
   {
      block_req.block.active_data.make_mutable();
      block_req.block.header.timestamp = test_timestamp + i;
      block_req.block.header.height    = head_info.head_topology.height + 1;
      block_req.block.header.previous  = head_info.head_topology.id;

      set_block_merkle_roots( block_req.block, CRYPTO_SHA2_256_ID );
      sign_block( block_req.block, _block_signing_private_key );

      block_req.block.id = crypto::hash_n( CRYPTO_SHA2_256_ID, block_req.block.header, block_req.block.active_data );

      _controller.submit_block( block_req );

      head_info.head_topology.height++;
      head_info.head_topology.previous = head_info.head_topology.id;
      head_info.head_topology.id = block_req.block.id;
   }

   auto fork_head_info = head_info;
   head_info = root_head_info;

   for( int i = 1; i <= 6; i++ )
   {
      block_req.block.active_data.make_mutable();
      block_req.block.header.timestamp = test_timestamp + i + 6;
      block_req.block.header.height    = head_info.head_topology.height + 1;
      block_req.block.header.previous  = head_info.head_topology.id;

      set_block_merkle_roots( block_req.block, CRYPTO_SHA2_256_ID );
      sign_block( block_req.block, _block_signing_private_key );

      block_req.block.id = crypto::hash_n( CRYPTO_SHA2_256_ID, block_req.block.header, block_req.block.active_data );

      _controller.submit_block( block_req );

      head_info.head_topology.height++;
      head_info.head_topology.previous = head_info.head_topology.id;
      head_info.head_topology.id = block_req.block.id;

      auto fork_heads = _controller.get_fork_heads();

      BOOST_REQUIRE_EQUAL( fork_heads.fork_heads.size(), 2 );
      auto topo0 = fork_heads.fork_heads[0];
      auto topo1 = fork_heads.fork_heads[1];

      BOOST_CHECK( (    topo0.height   == fork_head_info.head_topology.height
                     && topo0.previous == fork_head_info.head_topology.previous
                     && topo0.id       == fork_head_info.head_topology.id
                     && topo1.height   == head_info.head_topology.height
                     && topo1.previous == head_info.head_topology.previous
                     && topo1.id       == head_info.head_topology.id )
                   || ( topo1.height   == fork_head_info.head_topology.height
                     && topo1.previous == fork_head_info.head_topology.previous
                     && topo1.id       == fork_head_info.head_topology.id
                     && topo0.height   == head_info.head_topology.height
                     && topo0.previous == head_info.head_topology.previous
                     && topo0.id       == head_info.head_topology.id ) );
   }

   block_req.block.active_data.make_mutable();
   block_req.block.header.timestamp = test_timestamp + 13;
   block_req.block.header.height    = head_info.head_topology.height + 1;
   block_req.block.header.previous  = head_info.head_topology.id;

   set_block_merkle_roots( block_req.block, CRYPTO_SHA2_256_ID );
   sign_block( block_req.block, _block_signing_private_key );

   block_req.block.id = crypto::hash_n( CRYPTO_SHA2_256_ID, block_req.block.header, block_req.block.active_data );

   _controller.submit_block( block_req );

   head_info.head_topology.height++;
   head_info.head_topology.previous = head_info.head_topology.id;
   head_info.head_topology.id = block_req.block.id;

   auto fork_heads = _controller.get_fork_heads();

   BOOST_REQUIRE_EQUAL( fork_heads.fork_heads.size(), 1 );
   BOOST_CHECK_EQUAL( fork_heads.fork_heads[0].height, head_info.head_topology.height );
   BOOST_CHECK_EQUAL( fork_heads.fork_heads[0].previous, head_info.head_topology.previous );
   BOOST_CHECK_EQUAL( fork_heads.fork_heads[0].id, head_info.head_topology.id );


} KOINOS_CATCH_LOG_AND_RETHROW(info) }

BOOST_AUTO_TEST_SUITE_END()
