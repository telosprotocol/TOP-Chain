#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace top {
namespace evm {

using bytes = std::vector<uint8_t>;
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

namespace utils {

std::vector<uint8_t> serialize_function_input(std::string const & contract_address, std::string const & contract_function, uint64_t value = 0);
std::vector<uint8_t> to_le_bytes(uint128_t value);
void hex_string_bytes_char(std::string const & input, unsigned char * output);
std::vector<uint8_t> hex_string_to_bytes(std::string const & input);
std::vector<uint8_t> string_to_bytes(std::string const & input);
std::string uint8_vector_to_hex_string(std::vector<uint8_t> const & v);
}  // namespace utils
}  // namespace evm
}  // namespace top