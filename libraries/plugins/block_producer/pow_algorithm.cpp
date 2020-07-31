#include <thread>
#include <chrono>

#include <koinos/crypto/multihash.hpp>
#include <koinos/pack/rt/binary.hpp>

#include <koinos/plugins/block_producer/pow_algorithm.hpp>

timed_block_generation::timed_block_generation(uint32_t time_ms)
{
   this->time_ms = time_ms;
}

std::optional< uint64_t > timed_block_generation::generate_pow(const koinos::pack::multihash& mh,
                                                               uint32_t difficulty, uint64_t start, uint64_t end)
{
   std::this_thread::sleep_for( std::chrono::milliseconds( time_ms ) );

   return 0;
}

std::optional< uint64_t > sha256_pow::generate_pow(const koinos::pack::multihash& digest,
                                                               uint32_t difficulty, uint64_t start, uint64_t end)
{
   bool done = false;
   uint32_t count = 0;
   std::optional< uint64_t > ret;

   koinos::pack::variable_blob h(digest.digest);
   uint32_t osize = h.size();

   auto start_time = std::chrono::steady_clock::now();

   // Loop through each nonce in the given range
   for ( uint64_t nonce = start; nonce <= end; nonce++ )
   {
      // Reset values for this nonce
      done = false;
      count = 0;

      // Set the vector to multihash+nonce
      h.resize( osize );
      koinos::pack::to_variable_blob( h, nonce, true );

      // Now hash it
      auto hash = koinos::crypto::hash_blob( CRYPTO_SHA2_256_ID, h );

      for ( size_t mi = 0; mi < hash.digest.size(); mi++ )
      {
         char c = hash.digest[mi];
         for (size_t ci = 0; ci < 8; ci++)
         {
            if (c & 0x80)
            {
               done = true;
               break;
            }

            count++;
            c <<= 1;
         }

         // If done is set, we stop counting zeroes in this char
         if (done) { break; }
      }

      // If we have found a working nonce, then set ret and stop the loop
      if (count >= difficulty)
      {
         ret = nonce;
         auto duration = std::chrono::steady_clock::now() - start_time;
         LOG(info) << "Current hashrate: " << ( ( nonce * 1000000000 ) / duration.count() ) << " H/s";
         break;
      }
   }

   return ret;
}
