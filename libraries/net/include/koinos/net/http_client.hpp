#pragma once

#include <any>
#include <chrono>
#include <deque>
#include <future>
#include <mutex>
#include <variant>

#include <boost/asio/connect.hpp>
#include <boost/asio/generic/stream_protocol.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>

#include <boost/thread/sync_bounded_queue.hpp>

#include <koinos/exception.hpp>

#define DEFAULT_REQUEST_TIMEOUT_MS 10000

namespace koinos::net {

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using stream_protocol = net::generic::stream_protocol;

KOINOS_DECLARE_EXCEPTION( http_exception );

/**
 * Call result allows sending of exceptions from parsing of the response
 * back to the caller. std::any should contain a parsed response and
 * koinos::exception an exception that would have been thrown as a result
 * of parsing a bad response.
 */
using call_result = std::variant< std::any, koinos::exception >;

/**
 * Responses are multiplexed by a protocol provided id. (Such as jsonrpc id
 * field) However, the http client does not know how to parse said IDs. The
 * protocol must provide a callback to parse the raw response. In order to
 * avoid parsing the response multiple times, the parsed response should be
 * stored in a call result (either an any or and error) to be returned to
 * the correct calling thread via a promise.
 */
using parse_response_callback_t =
   std::function< uint32_t(const std::string&, call_result&) >;

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
 */
class http_client
{
   private:
      struct request_item
      {
         std::promise< call_result >   result_promise;
         std::future< void >           timeout_future;
      };

      struct remote_endpoint
      {
         std::string                   hostname;
         std::string                   port;
         tcp::resolver::results_type   resolution;
      };

      parse_response_callback_t           _parse_response;

      std::map< uint32_t, request_item >  _request_map;
      std::unique_ptr< std::thread >      _read_thread;
      std::unique_ptr< std::mutex >       _mutex;
      std::deque< uint32_t >              _timeouts;

      std::unique_ptr< net::io_context >  _ioc;
      std::unique_ptr< beast::basic_stream< stream_protocol > > _stream;
      beast::flat_buffer                  _buffer;
      bool                                _is_open = false;

      std::string                         _content_type;
      std::variant<
         tcp::endpoint,
         stream_protocol::endpoint >      _endpoint;

      uint32_t                            _timeout = 0;

      void connect();
      void read_thread_main();

   public:
      http_client( parse_response_callback_t cb, const std::string& http_content_type, uint32_t timeout = DEFAULT_REQUEST_TIMEOUT_MS );
      http_client( const http_client& ) = delete;
      http_client( http_client&& ) = default;

      ~http_client();

      http_client& operator=( const http_client& ) = delete;
      http_client& operator=( http_client&& ) = default;

      template< typename Endpoint >
      void connect( const Endpoint& endpoint )
      {
         if( is_open() ) KOINOS_THROW( http_exception, "http client is already open" );

         _endpoint = endpoint;

         connect();
      }

      bool is_open() const;
      void close();

      std::shared_future< call_result > send_request( uint32_t id, const std::string& bytes );
};

} // koinos::net
