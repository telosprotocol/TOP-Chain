#include "xevm_common/xeth/xeth_config.h"

NS_BEG4(top, evm_common, eth, config)

constexpr uint64_t LondonBlock = 12965000;
constexpr uint64_t ArrowGlacierBlock = 13773000;

bool is_london(const bigint num) {
    return (num >= LondonBlock);
}

bool is_arrow_glacier(const bigint num) {
    return (num >= ArrowGlacierBlock);
}

NS_END4
