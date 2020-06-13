#pragma once

#include <koinos/pack/classes.hpp>

namespace koinos { namespace chain {
class apply_context;

namespace thunk {

void hello( apply_context& ctx, hello_thunk_ret& ret, const hello_thunk_args& arg );
void verify_block_header( apply_context& ctx, verify_block_header_ret& ret, const verify_block_header_args& arg );
void apply_block( apply_context& ctx, apply_block_ret& ret, const apply_block_args& arg );
void apply_transaction( apply_context& ctx, apply_transaction_ret& ret, const apply_transaction_args& args );
void apply_upload_contract_operation( apply_context& ctx, apply_upload_contract_operation_ret& ret, const apply_upload_contract_operation_args& args );
void apply_execute_contract_operation( apply_context& ctx, apply_execute_contract_operation_ret& ret, const apply_execute_contract_operation_args& args );
void db_put_object( apply_context& ctx, db_put_object_ret& ret, const db_put_object_args& args );
void db_get_object( apply_context& ctx, db_get_object_ret& ret, const db_get_object_args& args );
void db_get_next_object( apply_context& ctx, db_get_next_object_ret& ret, const db_get_next_object_args& args );
void db_get_prev_object( apply_context& ctx, db_get_prev_object_ret& ret, const db_get_prev_object_args& args );

} } }
