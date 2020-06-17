#include <stdint.h>

void prints( const char* str );
uint32_t read_contract_args( char* memory, uint32_t buffer_size );

__attribute__( (visibility("default")) )
void apply( uint64_t a, uint64_t b, uint64_t c )
{
   char message[32];
   uint32_t size = read_contract_args(message, 31);
   message[size] = 0;
   prints( message );
}
