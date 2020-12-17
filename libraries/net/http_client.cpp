#include <koinos/net/http_client.hpp>

#include <koinos/util.hpp>

#include <boost/beast/http.hpp>

#define LOCALHOST "127.0.0.1"

namespace koinos::net {

namespace http = beast::http;

http_client::http_client( parse_response_callback_t cb, const std::string& http_content_type, uint32_t timeout ) :
   _ioc( std::make_unique< net::io_context >() ),
   _stream( std::make_unique< beast::basic_stream< stream_protocol > >( *_ioc ) ),
   _parse_response( cb ),
   _content_type( http_content_type ),
   _timeout( timeout )
{}

http_client::~http_client()
{
   if( _ioc )
      close();
}

void http_client::connect()
{
   std::visit( koinos::overloaded{
      [&]( auto&& e )
      {
         _stream->connect( e );
      }},
      _endpoint );

   _is_open = true;
   _read_thread = std::make_unique< std::thread >( [this](){ read_thread_main(); } );
}

bool http_client::is_open() const
{
   return _is_open;
}

void http_client::close()
{
   if( !is_open() ) return;
   _is_open = false;
   _stream->close();
   _read_thread->join();
   _read_thread.reset();
}

std::shared_future< call_result > http_client::send_request( uint32_t id, const std::string& bytes )
{
   if (_request_map.find(id) != _request_map.end())
   {
      KOINOS_THROW( http_exception, "Request ID conflict" );
   }

   http::request< http::string_body > req { http::verb::get, "/", 11 };

   std::visit( koinos::overloaded{
      [&]( tcp::endpoint& e )
      {
         req.set( http::field::host, e.address() );
      },
      [&]( stream_protocol::endpoint& )
      {
         req.set( http::field::host, LOCALHOST );
      }},
      _endpoint );

   req.set( http::field::user_agent, BOOST_BEAST_VERSION_STRING );
   req.set( http::field::content_type, _content_type );
   req.body() = bytes;
   req.keep_alive( true );
   req.prepare_payload();

   http::write( *_stream, req );

   request_item item;
   std::shared_future fut( item.result_promise.get_future() );

   item.timeout_future = std::async( std::launch::async, [fut,id,this]()
   {
      auto status = fut.wait_for( std::chrono::milliseconds( _timeout ) );
      if( status == std::future_status::timeout )
      {
         // TODO: Guard with lock
         _request_map[id].result_promise.set_value( koinos::exception( "Request timeout" ) );
         _timeouts.push_back( id );
      }
   } );

   // TODO: Guard with lock
   _request_map[id] = std::move( item );

   return fut;
}

void http_client::read_thread_main()
{
   while( is_open() )
   { try {
      http::response< http::string_body > res;
      http::read( *_stream, _buffer, res );

      call_result parsed_res;
      uint32_t id = _parse_response( res.body(), parsed_res );

      auto req = _request_map.find( id );
      if (req != _request_map.end())
      {
         req->second.result_promise.set_value( parsed_res );
         _request_map.erase( req );
      }

      while( _timeouts.size() )
      {
         _request_map.erase( _timeouts.front() );
         _timeouts.pop_front();
      }
   } catch( boost::exception& ) {} }
}

} // koinos::net
