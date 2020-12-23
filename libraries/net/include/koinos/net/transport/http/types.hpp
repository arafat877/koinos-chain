#pragma once
#include <koinos/net/types.hpp>

namespace koinos::net::transport::http {

struct exception : virtual koinos::net::exception
{
   exception( const std::string& m ) :
      koinos::net::exception( m )
   {}

   virtual ~exception() {}
};

} // koinos::net::transport::http
