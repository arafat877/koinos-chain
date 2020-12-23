#pragma once
#include <exception>
#include <string>

namespace koinos::net {

struct exception : virtual std::exception
{
   std::string msg;

   exception( const std::string& m ) :
      msg( m )
   {}

   exception( const exception& ) = default;
   exception( exception&& ) = default;

   virtual ~exception() {}

   virtual const char* what() const noexcept override
   {
      return msg.c_str();
   }

   exception& operator=( const exception& ) = default;
   exception& operator=( exception&& ) = default;
};

} // koinos::net
