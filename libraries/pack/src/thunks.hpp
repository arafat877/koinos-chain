
namespace koinos { namespace chain { namespace thunk {

struct hello_thunk_args
{
   protocol::uint64 a;
   protocol::uint64 b;
};

struct hello_thunk_ret
{
   protocol::uint64 c;
   protocol::uint64 d;
};

struct verify_block_header_ret
{
   protocol::uint8 header_ok;
};

struct verify_block_header_args
{
   protocol::signature_type sig;
   protocol::multihash_type digest;
};

struct apply_transaction_ret
{
};

struct apply_transaction_args
{
   protocol::transaction_type transaction;
};

struct apply_block_ret
{
};

struct apply_block_args
{
   protocol::active_block_data b;
};

struct apply_upload_contract_operation_ret
{
};

struct apply_upload_contract_operation_args
{
   protocol::create_system_contract_operation op;
};

struct apply_execute_contract_operation_ret
{
};

struct apply_execute_contract_operation_args
{
   protocol::contract_call_operation op;
};

typedef protocol::uint256 object_space;
typedef protocol::uint256 object_key;

struct db_put_object_args
{
   object_space           space;
   object_key             key;
   protocol::vl_blob      vl_object;
};

struct db_put_object_ret
{
   protocol::uint8        object_existed;
};

struct db_get_object_ret
{
   protocol::vl_blob      vl_object;
};

struct db_get_object_args
{
   object_space           space;
   object_key             key;
   protocol::int32        object_size_hint;
};

struct db_get_next_object_ret
{
   protocol::vl_blob      vl_object;
};

struct db_get_next_object_args
{
   object_space space;
   object_key   key;
   protocol::int32        object_size_hint;
};

struct db_get_prev_object_ret
{
   protocol::vl_blob      vl_object;
};

struct db_get_prev_object_args
{
   object_space           space;
   object_key             key;
   protocol::int32        object_size_hint;
};

} } }
