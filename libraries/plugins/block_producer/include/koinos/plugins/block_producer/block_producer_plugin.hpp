#pragma once

#include <appbase/application.hpp>
#include <koinos/plugins/chain/chain_plugin.hpp>
#include <koinos/pack/classes.hpp>
#include <koinos/crypto/elliptic.hpp>

#define KOINOS_BLOCK_PRODUCER_PLUGIN_NAME "block_producer"
#define KOINOS_BLOCK_TIME_MS              3000

const std::vector< uint8_t > DEMO_CONTRACT {
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x13, 0x03, 0x60,
  0x05, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x60,
  0x03, 0x7e, 0x7e, 0x7e, 0x00, 0x02, 0x1a, 0x01, 0x03, 0x65, 0x6e, 0x76,
  0x12, 0x69, 0x6e, 0x76, 0x6f, 0x6b, 0x65, 0x5f, 0x73, 0x79, 0x73, 0x74,
  0x65, 0x6d, 0x5f, 0x63, 0x61, 0x6c, 0x6c, 0x00, 0x00, 0x03, 0x03, 0x02,
  0x01, 0x02, 0x04, 0x05, 0x01, 0x70, 0x01, 0x01, 0x01, 0x05, 0x03, 0x01,
  0x00, 0x02, 0x06, 0x08, 0x01, 0x7f, 0x01, 0x41, 0x80, 0x88, 0x04, 0x0b,
  0x07, 0x12, 0x02, 0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x02, 0x00,
  0x05, 0x61, 0x70, 0x70, 0x6c, 0x79, 0x00, 0x02, 0x0a, 0xeb, 0x03, 0x02,
  0xea, 0x02, 0x01, 0x2d, 0x7f, 0x23, 0x80, 0x80, 0x80, 0x80, 0x00, 0x21,
  0x01, 0x41, 0xa0, 0x01, 0x21, 0x02, 0x20, 0x01, 0x20, 0x02, 0x6b, 0x21,
  0x03, 0x20, 0x03, 0x24, 0x80, 0x80, 0x80, 0x80, 0x00, 0x41, 0x00, 0x21,
  0x04, 0x20, 0x03, 0x20, 0x00, 0x36, 0x02, 0x9c, 0x01, 0x20, 0x03, 0x20,
  0x04, 0x36, 0x02, 0x0c, 0x03, 0x40, 0x41, 0x00, 0x21, 0x05, 0x20, 0x03,
  0x28, 0x02, 0x9c, 0x01, 0x21, 0x06, 0x20, 0x03, 0x28, 0x02, 0x0c, 0x21,
  0x07, 0x20, 0x06, 0x20, 0x07, 0x6a, 0x21, 0x08, 0x20, 0x08, 0x2d, 0x00,
  0x00, 0x21, 0x09, 0x41, 0x18, 0x21, 0x0a, 0x20, 0x09, 0x20, 0x0a, 0x74,
  0x21, 0x0b, 0x20, 0x0b, 0x20, 0x0a, 0x75, 0x21, 0x0c, 0x20, 0x05, 0x21,
  0x0d, 0x02, 0x40, 0x20, 0x0c, 0x45, 0x0d, 0x00, 0x41, 0xff, 0x00, 0x21,
  0x0e, 0x20, 0x03, 0x28, 0x02, 0x0c, 0x21, 0x0f, 0x20, 0x0f, 0x21, 0x10,
  0x20, 0x0e, 0x21, 0x11, 0x20, 0x10, 0x20, 0x11, 0x48, 0x21, 0x12, 0x20,
  0x12, 0x21, 0x0d, 0x0b, 0x20, 0x0d, 0x21, 0x13, 0x41, 0x01, 0x21, 0x14,
  0x20, 0x13, 0x20, 0x14, 0x71, 0x21, 0x15, 0x02, 0x40, 0x20, 0x15, 0x45,
  0x0d, 0x00, 0x41, 0x10, 0x21, 0x16, 0x20, 0x03, 0x20, 0x16, 0x6a, 0x21,
  0x17, 0x20, 0x17, 0x21, 0x18, 0x20, 0x03, 0x28, 0x02, 0x9c, 0x01, 0x21,
  0x19, 0x20, 0x03, 0x28, 0x02, 0x0c, 0x21, 0x1a, 0x20, 0x19, 0x20, 0x1a,
  0x6a, 0x21, 0x1b, 0x20, 0x1b, 0x2d, 0x00, 0x00, 0x21, 0x1c, 0x20, 0x03,
  0x28, 0x02, 0x0c, 0x21, 0x1d, 0x41, 0x01, 0x21, 0x1e, 0x20, 0x1d, 0x20,
  0x1e, 0x6a, 0x21, 0x1f, 0x20, 0x18, 0x20, 0x1f, 0x6a, 0x21, 0x20, 0x20,
  0x20, 0x20, 0x1c, 0x3a, 0x00, 0x00, 0x20, 0x03, 0x28, 0x02, 0x0c, 0x21,
  0x21, 0x41, 0x01, 0x21, 0x22, 0x20, 0x21, 0x20, 0x22, 0x6a, 0x21, 0x23,
  0x20, 0x03, 0x20, 0x23, 0x36, 0x02, 0x0c, 0x0c, 0x01, 0x0b, 0x0b, 0x41,
  0x00, 0x21, 0x24, 0x41, 0x10, 0x21, 0x25, 0x20, 0x03, 0x20, 0x25, 0x6a,
  0x21, 0x26, 0x20, 0x26, 0x21, 0x27, 0x20, 0x03, 0x28, 0x02, 0x0c, 0x21,
  0x28, 0x20, 0x03, 0x20, 0x28, 0x3a, 0x00, 0x10, 0x20, 0x03, 0x28, 0x02,
  0x0c, 0x21, 0x29, 0x41, 0x01, 0x21, 0x2a, 0x20, 0x29, 0x20, 0x2a, 0x6a,
  0x21, 0x2b, 0x20, 0x24, 0x20, 0x24, 0x20, 0x24, 0x20, 0x27, 0x20, 0x2b,
  0x10, 0x80, 0x80, 0x80, 0x80, 0x00, 0x41, 0xa0, 0x01, 0x21, 0x2c, 0x20,
  0x03, 0x20, 0x2c, 0x6a, 0x21, 0x2d, 0x20, 0x2d, 0x24, 0x80, 0x80, 0x80,
  0x80, 0x00, 0x0f, 0x0b, 0x7d, 0x01, 0x0b, 0x7f, 0x23, 0x80, 0x80, 0x80,
  0x80, 0x00, 0x21, 0x03, 0x41, 0xe0, 0x00, 0x21, 0x04, 0x20, 0x03, 0x20,
  0x04, 0x6b, 0x21, 0x05, 0x20, 0x05, 0x24, 0x80, 0x80, 0x80, 0x80, 0x00,
  0x20, 0x05, 0x21, 0x06, 0x41, 0x0b, 0x21, 0x07, 0x41, 0x3f, 0x21, 0x08,
  0x41, 0x00, 0x21, 0x09, 0x20, 0x05, 0x20, 0x00, 0x37, 0x03, 0x58, 0x20,
  0x05, 0x20, 0x01, 0x37, 0x03, 0x50, 0x20, 0x05, 0x20, 0x02, 0x37, 0x03,
  0x48, 0x20, 0x07, 0x20, 0x06, 0x20, 0x08, 0x20, 0x09, 0x20, 0x09, 0x10,
  0x80, 0x80, 0x80, 0x80, 0x00, 0x41, 0x01, 0x21, 0x0a, 0x20, 0x06, 0x20,
  0x0a, 0x6a, 0x21, 0x0b, 0x20, 0x0b, 0x10, 0x81, 0x80, 0x80, 0x80, 0x00,
  0x41, 0xe0, 0x00, 0x21, 0x0c, 0x20, 0x05, 0x20, 0x0c, 0x6a, 0x21, 0x0d,
  0x20, 0x0d, 0x24, 0x80, 0x80, 0x80, 0x80, 0x00, 0x0f, 0x0b, 0x00, 0x2b,
  0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x24, 0x03, 0x00, 0x12, 0x69, 0x6e,
  0x76, 0x6f, 0x6b, 0x65, 0x5f, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x5f,
  0x63, 0x61, 0x6c, 0x6c, 0x01, 0x06, 0x70, 0x72, 0x69, 0x6e, 0x74, 0x73,
  0x02, 0x05, 0x61, 0x70, 0x70, 0x6c, 0x79, 0x00, 0x26, 0x09, 0x70, 0x72,
  0x6f, 0x64, 0x75, 0x63, 0x65, 0x72, 0x73, 0x01, 0x0c, 0x70, 0x72, 0x6f,
  0x63, 0x65, 0x73, 0x73, 0x65, 0x64, 0x2d, 0x62, 0x79, 0x01, 0x05, 0x63,
  0x6c, 0x61, 0x6e, 0x67, 0x06, 0x31, 0x30, 0x2e, 0x30, 0x2e, 0x30
};

using namespace appbase;

namespace koinos::plugins::block_producer {

    class block_producer_plugin : public appbase::plugin< block_producer_plugin >
    {
       public:
          block_producer_plugin();
          virtual ~block_producer_plugin();

          APPBASE_PLUGIN_REQUIRES( (plugins::chain::chain_plugin) );

          static const std::string& name() { static std::string name = KOINOS_BLOCK_PRODUCER_PLUGIN_NAME; return name; }

          std::shared_ptr<protocol::block_header> produce_block();

          virtual void set_program_options( options_description&, options_description& ) override {}

          virtual void plugin_initialize( const variables_map& options ) override;
          virtual void plugin_startup() override;
          virtual void plugin_shutdown() override;

          void demo_create_contract(protocol::active_block_data& active_data);
          void demo_call_contract(protocol::active_block_data& active_data, uint32_t cycle);

          void start_block_production();
          void stop_block_production();

          // Whether or not we should be producing blocks
          // stop_block_production uses this to shut down the production thread
          bool producing_blocks = false;
          crypto::private_key block_signing_private_key;

          std::shared_ptr< std::thread > block_production_thread;
    };
}
