#include <cstdint>

#include <koinos/pack/classes.hpp>

class pow_algorithm
{
public:
   pow_algorithm() { };
   virtual ~pow_algorithm() { };
   virtual std::optional< uint64_t > generate_pow(const koinos::pack::multihash_type multihash, uint32_t difficulty, uint64_t start, uint64_t end);
};

class timed_block_generation : public pow_algorithm
{
public:
   timed_block_generation(const uint32_t time_ms);
   std::optional< uint64_t > generate_pow(const koinos::pack::multihash_type multihash, uint32_t difficulty, uint64_t start, uint64_t end) override;

protected:
   uint32_t time_ms;
};

class sha256_pow : public pow_algorithm
{
public:
   sha256_pow();
   std::optional< uint64_t > generate_pow(const koinos::pack::multihash_type multihash, uint32_t difficulty, uint64_t start, uint64_t end) override;
};
