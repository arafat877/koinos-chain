
#include <future>
#include <mutex>
#include <queue>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/thread/sync_bounded_queue.hpp>

#include <koinos/net/client_fwd.hpp>

namespace koinos::net {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

/**
 * http_client is designed to manage sending http GET requests asynchronously.
 *
 * New requests are made asynchronously and added to the request map.
 * When responses are handled, their ID is looked up via the request map
 * and returned to the correct calling thread via a promise.
 *
 * Request runtimes are monitored and timed out, returning an error to the
 * calling thread.
 *
 * If enough requests time out, a new connection will be established
 * and all outstanding requests will be cancelled.
 *
 * This class needs to be templated on class providing information about
 * wrapping protocols. Specifically, providing the http content type,
 * and response parsing that returns the response ID.
 *
 * The template class must provide the following interfaces:
 *
 * static constexpr std::string http_content_type "";
 *
 * // Returns response ID and a parsed version of the response as a
 * // std::any to return to the caller.
 * static uint64_t parse_response( const std::string&, std::any& );
 */
template< typename Protocol >
class http_client
{
   private:
      using protocol_type = Protocol;
      static constexpr std::size_t MAX_QUEUE_SIZE = 1024;

      struct request
      {
         uint64_t id;
         std::vector< uint8_t > payload;
      };

      struct request_time_info
      {
         uint32_t send_time;
         uint64_t id;
      };

      std::map<
         uint64_t,
         std::promise< call_result > >                            _request_map;
      //std::priority_queue<
      //   request_time_info,
      //   std::greater< typename request_time_info::send_time > >  _timeout_queue;
      std::thread                                                 _timeout_thread;
      std::thread                                                 _read_thread;
      std::mutex                                                  _mutex;
      std::function<uint64_t(const std::any&)>                    _get_response_id;

      std::string _content_type;

      net::io_context    _ioc;
      beast::tcp_stream  _stream;
      beast::flat_buffer _buffer;

      std::string _host;

      void read_thread_main()
      {
         while (true)
         {
            http::response< http::string_body > res;
            http::read(_stream, _buffer, res);

            LOG(info) << res;

            call_result parsed_res;
            uint64_t id = protocol_type::parse_response( res.body(), parsed_res );

            auto req = _request_map.find( id );
            if (id != _request_map.end())
            {
               req->second.set_value( parsed_res );
               req->remove();
            }

            // TODO: Check for timeouts
         }
      }

   public:
      /**
       * Parse the response body, placing it in a call_result object, and return the id associated
       * with the response.
       */
      //virtual uint64_t parse_response( const http::string_body& body, call_result& res ) const = 0;
      //virtual const std::string& get_content_type() const = 0;

      http_client() :
         _stream( _ioc )
      {}

      std::future< call_result > send_request( uint64_t id, const std::string& bytes )
      {
         if (_request_map.find(id) != _request_map.end())
         {
            // TODO: Throw koinos exception
            throw std::exception();
         }

         http::request< http::string_body > req;
         //{ http::verb::get, "1.1", 11 };
         req.version( 11 );
         req.method( http::verb::get );
         //req.body( bytes );
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
};

} // koinos::net
