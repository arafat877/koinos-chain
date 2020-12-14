#pragma once

#include <any>
#include <future>
#include <mutex>
#include <queue>
#include <variant>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/thread/sync_bounded_queue.hpp>

#include <koinos/exception.hpp>

namespace koinos::net {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

/**
 * Call result allows sending of exceptions from parsing of the response
 * back to the caller. std::any should contain a parsed response and
 * koinos::exception an exception that would have been thrown as a result
 * of parsing a bad response.
 */
using call_result = std::variant< std::any, koinos::exception >;

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
      static constexpr std::size_t MAX_QUEUE_SIZE = 1024;

      struct request
      {
         uint32_t id;
         std::vector< uint8_t > payload;
      };

      struct request_time_info
      {
         uint32_t send_time;
         uint32_t id;
      };

      std::map<
         uint32_t,
         std::promise< call_result > >                            _request_map;
      //std::priority_queue<
      //   request_time_info,
      //   std::greater< typename request_time_info::send_time > >  _timeout_queue;
      std::thread                                                 _read_thread;
      std::mutex                                                  _mutex;

      std::string _content_type;
      using parse_response_callback_t =
         std::function< uint32_t(const std::string&, call_result&) >;
      parse_response_callback_t                                   _parse_response;

      net::io_context    _ioc;
      beast::tcp_stream  _stream;
      beast::flat_buffer _buffer;
      bool _is_open = false;

      std::string _host;
      uint32_t    _port = 0;

      http_client();

      void read_thread_main();

   public:
      http_client( parse_response_callback_t cb, const std::string& http_content_type );
      ~http_client();

      void connect( const std::string& host, uint32_t port );
      bool is_open() const;
      void close();

      std::future< call_result > send_request( uint32_t id, const std::string& bytes );
};

} // koinos::net
