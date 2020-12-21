#include <koinos/net/transport/http/http_client.hpp>

#include <koinos/util.hpp>

#include <boost/beast/http.hpp>

#define LOCALHOST "127.0.0.1"
#define TIMEOUT_THRESHOLD 5
#define RECONNECT_SLEEP_MS_MIN uint32_t(100)
#define RECONNECT_SLEEP_MS_MAX uint32_t(10000)


namespace koinos::net {

namespace http = beast::http;

http_client::http_client( parse_response_callback_t cb, const std::string& http_content_type, uint32_t timeout ) :
   _mutex( std::make_unique< std::mutex >() ),
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
         const std::lock_guard< std::mutex > lock( *_mutex );
         if( !_request_map[id].is_result_set )
         {
            _request_map[id].result_promise.set_value( koinos::exception( "Request timeout" ) );
            _request_map[id].is_result_set = true;
         }

         _requests_to_remove.push_back( id );
         _consecutive_timeouts++;

         if( _consecutive_timeouts >= TIMEOUT_THRESHOLD )
            _stream->close();
      }
   } );

   const std::lock_guard< std::mutex > lock( *_mutex );
   _request_map[id] = std::move( item );

   return fut;
}

void http_client::read_thread_main()
{
   uint32_t reconnect_sleep_ms = RECONNECT_SLEEP_MS_MIN;

   while( is_open() )
   { try {
      http::response< http::string_body > res;
      http::read( *_stream, _buffer, res );

      call_result parsed_res;
      uint32_t id = _parse_response( res.body(), parsed_res );

      const std::lock_guard< std::mutex > lock( *_mutex );

      auto req = _request_map.find( id );
      if (req != _request_map.end())
      {
         req->second.result_promise.set_value( parsed_res );
         _request_map.erase( req );
      }

      while( _requests_to_remove.size() )
      {
         _request_map.erase( _requests_to_remove.front() );
         _requests_to_remove.pop_front();
      }
   } catch( boost::exception& e )
   {
      if( !is_open() ) break;

      // An error occurred, let's reconnect and cancel pending requests
      LOG(warning) << "Connection error: " << boost::diagnostic_information( e );
      LOG(warning) << "Attempting to reconnect http client...";

      {
         const std::lock_guard< std::mutex > lock( *_mutex );

         for( auto& req : _request_map )
         {
            // There is a race condition between this thread and the timeout thread
            // A lock solves concurrent access, but we cannot set the state again
            if( !req.second.is_result_set )
               req.second.result_promise.set_value( koinos::exception( "Connection error" ) );
         }

         _consecutive_timeouts = 0;
      }

      try
      {
         std::visit( koinos::overloaded{
            [&]( auto&& e )
            {
               _stream->connect( e );
            }},
            _endpoint );
         reconnect_sleep_ms = RECONNECT_SLEEP_MS_MIN;

         LOG(warning) << "Success!";
      } catch( boost::exception& )
      {
         LOG(warning) << "Failed. Sleeping for " << reconnect_sleep_ms << "ms";
         std::this_thread::sleep_for( std::chrono::milliseconds( reconnect_sleep_ms ) );
         reconnect_sleep_ms = std::min( reconnect_sleep_ms << 2, RECONNECT_SLEEP_MS_MAX );
      }
   } }
}

} // koinos::net
