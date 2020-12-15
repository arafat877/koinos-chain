#include <boost/test/unit_test.hpp>

#include <koinos/tests/net_fixture.hpp>

#include <koinos/net/client.hpp>

#include <koinos/exception.hpp>

BOOST_FIXTURE_TEST_SUITE( net_tests, net_fixture )

using json = koinos::net::jsonrpc::json;

BOOST_AUTO_TEST_CASE( net_tests )
{ try {
   socket = std::make_unique< boost::asio::local::stream_protocol::socket >( ioc, endpoint.protocol() );
   socket->connect( unix_socket.string() );

   BOOST_TEST_MESSAGE( "adding method handlers [add, sub, mul, div]" );

   request_handler->add_method_handler( "add", []( const json::object_t& j ) -> json
   {
      json result = j.at( "a" ).get< uint64_t >() + j.at( "b" ).get< uint64_t >();
      return result;
   } );

   request_handler->add_method_handler( "sub", []( const json::object_t& j ) -> json
   {
      json result = j.at( "a" ).get< uint64_t >() - j.at( "b" ).get< uint64_t >();
      return result;
   } );

   request_handler->add_method_handler( "mul", []( const json::object_t& j ) -> json
   {
      json result = j.at( "a" ).get< uint64_t >() * j.at( "b" ).get< uint64_t >();
      return result;
   } );

   request_handler->add_method_handler( "div", []( const json::object_t& j ) -> json
   {
      json result = j.at( "a" ).get< uint64_t >() / j.at( "b" ).get< uint64_t >();
      return result;
   } );

   BOOST_TEST_MESSAGE( "sending 'add' request with params {\"a\":2, \"b\":1}" );

   koinos::net::jsonrpc::request req {
      .jsonrpc = "2.0",
      .id = uint64_t( 1 ),
      .method = "add",
      .params = json::object_t {
         { "a", 2 },
         { "b", 1 }
      }
   };

   write_request( req );

   koinos::net::jsonrpc::response res = read_response();

   BOOST_TEST_MESSAGE( "-> verifying result" );

   BOOST_REQUIRE_EQUAL( res.jsonrpc, "2.0" );
   BOOST_REQUIRE_EQUAL( std::get< uint64_t >( res.id ), 1 );
   BOOST_REQUIRE_EQUAL( res.result->get< uint64_t >(), 3 );

   BOOST_TEST_MESSAGE( "sending 'sub' request with params {\"a\":2, \"b\":1}" );

   req.id = uint64_t( 2 );
   req.method = "sub";

   write_request( req );

   res = read_response();

   BOOST_TEST_MESSAGE( "-> verifying result" );

   BOOST_REQUIRE_EQUAL( res.jsonrpc, "2.0" );
   BOOST_REQUIRE_EQUAL( std::get< uint64_t >( res.id ), 2 );
   BOOST_REQUIRE_EQUAL( res.result->get< uint64_t >(), 1 );

   BOOST_TEST_MESSAGE( "sending 'mul' request with params {\"a\":5, \"b\":6}" );

   req.id = uint64_t( 3 );
   req.method = "mul";
   req.params = json::object_t {
      { "a", 5 },
      { "b", 6 }
   };

   write_request( req );

   res = read_response();

   BOOST_TEST_MESSAGE( "-> verifying result" );

   BOOST_REQUIRE_EQUAL( res.jsonrpc, "2.0" );
   BOOST_REQUIRE_EQUAL( std::get< uint64_t >( res.id ), 3 );
   BOOST_REQUIRE_EQUAL( res.result->get< uint64_t >(), 30 );

   BOOST_TEST_MESSAGE( "sending 'div' request with params {\"a\":100, \"b\":5}" );

   req.id = uint64_t( 4 );
   req.method = "div";
   req.params = json::object_t {
      { "a", 100 },
      { "b", 5 }
   };

   write_request( req );

   res = read_response();

   BOOST_TEST_MESSAGE( "-> verifying result" );

   BOOST_REQUIRE_EQUAL( res.jsonrpc, "2.0" );
   BOOST_REQUIRE_EQUAL( std::get< uint64_t >( res.id ), 4 );
   BOOST_REQUIRE_EQUAL( res.result->get< uint64_t >(), 20 );

} KOINOS_CATCH_LOG_AND_RETHROW(info) }

BOOST_AUTO_TEST_CASE( client_tests )
{ try {
   request_handler->add_method_handler( "add", []( const json::object_t& j ) -> json
   {
      json result = j.at( "a" ).get< uint64_t >() + j.at( "b" ).get< uint64_t >();
      return result;
   } );

   // 100 ms timeout
   //koinos::net::client c( koinos::net::jsonrpc::jsonrpc_client( 100 ) );
   koinos::net::client c;
   c.connect( endpoint );
   c.call< uint32_t >( "add", json{{"a", 1}, {"b", 2}} );

   try
   {
      c.call< uint32_t >( "sub", json{{"a", 1}, {"b", 2}} );
      BOOST_REQUIRE(false);
   } catch( koinos::exception& ) {}

   BOOST_TEST_MESSAGE("foobar");

} KOINOS_CATCH_LOG_AND_RETHROW(info) }

BOOST_AUTO_TEST_SUITE_END()
