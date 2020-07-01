#include <stdint.h>

char prepend[] = "test: ";

void invoke_system_call( uint32_t sid, char* ret_ptr, uint32_t ret_len, char* arg_ptr, uint32_t arg_len );
void invoke_thunk( uint32_t tid, char* ret_ptr, uint32_t ret_len, char* arg_ptr, uint32_t arg_len );

void prints( char* msg )
{
   char args[129];

   int i = 0;

   while( i < sizeof(prepend) )
   {
      args[i+1] = prepend[i];
      i++;
   }

   i = 0;

   while( msg[i] && i < 127 )
   {
      // Purposefully OBO to overwrite 0 at the end of prepend
      args[i + sizeof(prepend)] = msg[i];
      i++;
   }
   args[i] = 0;
   args[0] = (uint8_t)i + sizeof(prepend);

   invoke_thunk( 0, 0, 0, args, i + sizeof(prepend) + 1 );
}

__attribute__( (visibility("default")) )
void apply( uint64_t a, uint64_t b, uint64_t c )
{
   char message[64];
   invoke_system_call( 14, message, 63, 0, 0 );
   prints( message + 1 );
}
