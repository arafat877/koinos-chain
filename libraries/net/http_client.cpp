#include <koinos/net/http_client.hpp>

namespace koinos::net {

http_client::http_client() :
   _stream( _ioc )
{}

http_client::http_client( parse_response_callback_t cb, const std::string& http_content_type ) :
   _stream( _ioc ),
   _parse_response( cb ),
   _content_type( http_content_type )
{}

http_client::~http_client()
{
   if( is_open() ) close();
}

void http_client::connect( const std::string& host, uint32_t port )
{
   if( is_open() ) return; // TODO: Throw

   _host = host;
   _port = port;

   tcp::resolver resolver( _ioc );
   auto results = resolver.resolve( _host, std::to_string(_port) );
   _stream.connect( results );
   _is_open = true;
}

bool http_client::is_open() const
{
   return _is_open;
}

void http_client::close()
{
   if( !is_open() ) return;

   _stream.close();
   _is_open = false;
}

std::future< call_result > http_client::send_request( uint32_t id, const std::string& bytes )
{
   if (_request_map.find(id) != _request_map.end())
   {
      // TODO: Throw koinos exception
      throw std::exception();
   }

   http::request< http::string_body > req;
   req.version( 11 );
   req.method( http::verb::get );
   req.set( http::field::host, _host );
   req.set( http::field::user_agent, BOOST_BEAST_VERSION_STRING );
   req.set( http::field::content_type, _content_type );
   req.set( http::field::body, bytes );

   http::write( _stream, req );

   std::promise< call_result > prom;
   auto fut = prom.get_future();

   _request_map[id] = std::move(prom);

   // TODO: Add request to timeout map

   return fut;
}

void http_client::read_thread_main()
{
   while (true)
   {
      http::response< http::string_body > res;
      http::read( _stream, _buffer, res );

      LOG(info) << res;

      call_result parsed_res;
      uint32_t id = _parse_response( res.body(), parsed_res );

      auto req = _request_map.find( id );
      if (req != _request_map.end())
      {
         req->second.set_value( parsed_res );
         _request_map.erase( req );
      }

      // TODO: Check for timeouts
   }
}

} // koinos::net
