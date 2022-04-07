#pragma once
#include "xbasic/xmemory.hpp"
#include "xevm_contract_runtime/xevm_type.h"
#include "xevm_runner/evm_storage_face.h"
#include "xevm_runner/evm_util.h"

#include <cstdint>
#include <map>
#include <memory>

namespace top {
namespace evm {
class xtop_evm_logic {
public:
    xtop_evm_logic(std::shared_ptr<xevm_storage_face_t> storage_ptr, observer_ptr<evm_runtime::xevm_context_t> const & context);
    xtop_evm_logic(xtop_evm_logic const &) = delete;
    xtop_evm_logic & operator=(xtop_evm_logic const &) = delete;
    xtop_evm_logic(xtop_evm_logic &&) = default;
    xtop_evm_logic & operator=(xtop_evm_logic &&) = default;
    ~xtop_evm_logic() = default;

private:
    std::shared_ptr<xevm_storage_face_t> m_storage_ptr;
    observer_ptr<evm_runtime::xevm_context_t> m_context;
    std::map<uint64_t, bytes> m_registers;
    bytes m_return_data_value;

public:
    std::shared_ptr<xevm_storage_face_t> ext_ref();
    observer_ptr<evm_runtime::xevm_context_t> context_ref();
    bytes return_value();

public:
    // interface to evm_import_instance:

    // register:
    void read_register(uint64_t register_id, uint64_t ptr);
    uint64_t register_len(uint64_t register_id);

    // context:
    // void current_account_id(uint64_t register_id);
    // void signer_account_id(uint64_t register_id);
    void sender_address(uint64_t register_id);
    void input(uint64_t register_id);

    // math:
    void random_seed(uint64_t register_id);
    void sha256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id);
    void keccak256(uint64_t value_len, uint64_t value_ptr, uint64_t register_id);
    void ripemd160(uint64_t value_len, uint64_t value_ptr, uint64_t register_id);

    // others:
    void value_return(uint64_t value_len, uint64_t value_ptr);
    void log_utf8(uint64_t len, uint64_t ptr);

    // storage:
    uint64_t storage_write(uint64_t key_len, uint64_t key_ptr, uint64_t value_len, uint64_t value_ptr, uint64_t register_id);
    uint64_t storage_read(uint64_t key_len, uint64_t key_ptr, uint64_t register_id);
    uint64_t storage_remove(uint64_t key_len, uint64_t key_ptr, uint64_t register_id);

private:
    // inner api
    std::string get_utf8_string(uint64_t len, uint64_t ptr);
    void internal_write_register(uint64_t register_id, std::vector<uint8_t> const & context_input);
    std::vector<uint8_t> get_vec_from_memory_or_register(uint64_t offset, uint64_t len);
    void memory_set_slice(uint64_t offset, std::vector<uint8_t> buf);
    std::vector<uint8_t> memory_get_vec(uint64_t offset, uint64_t len);
    std::vector<uint8_t> internal_read_register(uint64_t register_id);
};
using xevm_logic_t = xtop_evm_logic;
}  // namespace evm
}  // namespace top