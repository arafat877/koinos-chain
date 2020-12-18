#pragma once
#include <koinos/exception.hpp>

namespace koinos::net::jsonrpc {

KOINOS_DECLARE_EXCEPTION( jsonrpc_exception );

// jsonrpc Errors
KOINOS_DECLARE_DERIVED_EXCEPTION( parse_error, jsonrpc_exception );
KOINOS_DECLARE_DERIVED_EXCEPTION( invalid_request, jsonrpc_exception );
KOINOS_DECLARE_DERIVED_EXCEPTION( method_not_found, jsonrpc_exception );
KOINOS_DECLARE_DERIVED_EXCEPTION( invalid_params, jsonrpc_exception );
KOINOS_DECLARE_DERIVED_EXCEPTION( internal_error, jsonrpc_exception );
KOINOS_DECLARE_DERIVED_EXCEPTION( server_error, jsonrpc_exception );

} // koinos::net::jsonrpc
