#pragma once

#include <any>
#include <variant>

#include <koinos/exception.hpp>

namespace koinos::net {

typedef std::variant< std::any, koinos::exception > call_result;

} // koinos::net
