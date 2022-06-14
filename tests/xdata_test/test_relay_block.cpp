#include "gtest/gtest.h"
#include "xdata/xrelay_block.h"
#include "xbase/xmem.h"
#include "xcommon/xeth_address.h"
#include "xdata/xethtransaction.h"
#include "xpbase/base/top_utils.h"
#include "xcrypto/xcrypto_util.h"
#include "xbase/xutl.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xutility/xhash.h"
#include "xmutisig/xmutisig.h"
#include "xcrypto/xckey.h"
#include "xcertauth/xcertauth_face.h"
#include <cinttypes>
#include <trezor-crypto/sha3.h>
#include <secp256k1/secp256k1.h>
#include <secp256k1/secp256k1_recovery.h>
#include<fstream>  


using namespace top;
using namespace top::base;
using namespace top::data;
using namespace top::evm_common;
using namespace top::common;


class test_relay_block : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


//test data
const std::string  test_signature {"1BETTgEv6HFFtxTVCQZBioXc5M2oXb5iPQgoO6qlXlPEzTPK4D2yuz4pAfQqfxwAC"};
const std::string  test_address   {"0xbc9b5f068bc20a5b12030fcb72975d8bddc4e84c"};
const std::string  test_to_address{"0xaaaaaf068bc20a5b12030fcb72975d8bddc4e84c"};
const h256 test_topics1{"4f89ece0f576ba3986204ba19a44d94601604b97cf3baa922b010a758d303842"};
const h256 test_topics2{"000000000000000000000000e22c0e020c99e9aed339618fdcea2871d678ef38"};
const h256 test_topics3{"000000000000000000000000f3b23b373dc8854cc2936f4ab4b8e782011ccf87"};
const h256 test_topics4{"000000000000000000000000f39fd6e51aad88f6f4ce6ab8827279cfffb92266"};

const uint64_t     test_gasUsed{0xccde};
const h2048    test_logsBloom{"00000001000000004000000000000000000000000000000000000000000000000000000000041000000000000000008000000000000080000000000000200000000000000000000000000008000000000000000000008000000000000000000010000000020000000004000100000800000000040000000000000012000000000000000020000000008000000000000000000000000000000000000000000000420000000000000000000000000000000000000000080000000000000000000000000002000000200000000000000000000008002000000000000000000020000010000200000000000000000000000000000000000000000000002000000000"};
const h256     test_public_key_x{"b72d55c76bd8f477f4b251763c33f75e6f5f5dd8af071e711e0cb9b2accc70ea"};
const h256     test_public_key_y{"b72d55c76bd8f477f4b251763c33f75e6f5f5dd8af071e711e0cb9b2accc70ea"};
const uint64_t  test_stake = 0x12345678;
const h256    test_r{"4f89ece0f576ba39123456789123456781604b97cf3baa922b010a758d303842"} ;
const h256    test_s{"4f812345678abcdef1234ba19a44d94601604b97cf3baa922b010a758d303842"} ;
const byte    test_v = 0x1;
const uint8_t            test_version = 0;
const h256    test_inner_hash{"5e173f6ac3c669587538e7727cf19b782a4f2fda07c1eaa662c593e5e85e3051"}; 
const uint64_t            test_height = 123;
const uint64_t            test_epochID = 456;
const uint64_t            test_timestamp = 13579;
const h256    test_elections_hash {"19c2185f4f40634926ebed3af09070ca9e029f2edd5fae6253074896205f5f6c"};
const h256    test_txs_merkle_root{"c45f950382d542169ea207959ee0220ec1491755abe405cd7498d6b16adb6df8"};
const h256    test_receipts_merkle_root{"d25688cf0ab10afa1a0e2dba7853ed5f1e5bf1c631757ed4e103b593ff3f5620"};
const h256    test_state_merkle_root{"f37ec61d84cea03dcc5e8385db93248584e8af4b4d1c832d8c7453c0089687a7"};
const h256    test_block_merkle_root{"e3f407f83fc012470c26a93fdff534100f2c6f736439ce0ca90e9914f7d1c381"};
const h256    test_prev_hash        {"cda1f407f83fc012470c26a93fdff534100f2c6f736439ce0ca9acbde1234567"};
const h256    test_block_hash       {"1234acdeacbfc012470c26a93fdff534100f2c6f736439ce0ca9acbde1234123"};
const u256    test_value{0x45567};
const u256    test_value_gas{0x12346};
const uint64_t test_table_height = 999;

#define  TEST_TOPICS_NUM        (4)
#define  TEST_RECEIPT_LOG_NUM   (2)
#define  TEST_SIGNATURE_NUM     (4)
#define  TEST_ELECTIONS_NUM     (4)
#define  TEST_RECEIPT_NUM       (2)


xevm_log_t  xrelay_evm_log_create() 
{
    xeth_address_t address = xtop_eth_address::build_from(test_address);
    xh256s_t     topics;
    topics.push_back(test_topics1);
    topics.push_back(test_topics2);
    topics.push_back(test_topics3);
    topics.push_back(test_topics4);
    std::string test_str = "000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000a4ba11f3f36b12c71f2aef775583b306a3cf784a";
    std::string log_data = top::HexDecode(test_str);
    xbytes_t  data = bytes(log_data.begin(), log_data.end());

    xevm_log_t _evm_log(address, topics, data); 
    return _evm_log;
}

xeth_receipt_t xrelay_receipt_create()
{
    xeth_receipt_t _receipt;
    xeth_address_t address = xtop_eth_address::build_from(test_address);
    evm_common::xevm_logs_t  logs;
    for (int i = 0; i < TEST_RECEIPT_LOG_NUM; i++) {
        xevm_log_t log = xrelay_evm_log_create();
        logs.emplace_back(log);
    }
    _receipt.set_tx_status(ethreceipt_status_successful);
    _receipt.set_cumulative_gas_used(test_gasUsed);
    _receipt.set_logs(logs);
    _receipt.create_bloom();

    return _receipt;
}

xeth_transaction_t xrelay_tx_create()
{
    xeth_address_t from_address = xtop_eth_address::build_from(test_address);
    xeth_address_t to_address = xtop_eth_address::build_from(test_to_address);
    std::string test_str = "000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000a4ba11f3f36b12c71f2aef775583b306a3cf784a";
    std::string log_data = top::HexDecode(test_str);
    xbytes_t  data = bytes(log_data.begin(), log_data.end());

    xeth_transaction_t tx(from_address, to_address, data, test_value, test_value_gas, test_value_gas);
    return tx;
}

xrelay_election_node_t xrelay_election_create()
{
    xrelay_election_node_t _election;
    _election.public_key_x = test_public_key_x;
    _election.public_key_y = test_public_key_y;
    _election.stake = test_stake;
    return _election;
}

xrelay_signature_node_t xrelay_signature_node_t_create(bool exist)
{
    xrelay_signature_node_t _signature_node;
    if (exist) {
       _signature_node.exist = true;
        _signature_node.signature.r = test_r;
        _signature_node.signature.s = test_s;
        _signature_node.signature.v = test_v;
    } else {
        _signature_node.exist = false;
    }
    
    return _signature_node;
}


xrelay_signature_node_t xrelay_signature_node_t_string_create(bool exist)
{
    xrelay_signature_node_t _signature_node;
    if (exist) {
       _signature_node = xrelay_signature_node_t(test_signature);
    } else {
        std::string error_signature = "abcdef";
        _signature_node = xrelay_signature_node_t(error_signature);
    }
    
    return _signature_node;
}


xrelay_block_inner_header xrelay_block_inner_header_create()
{
    xrelay_block_inner_header _inner_header;

    _inner_header.set_version(test_version);
    _inner_header.set_block_height(test_height);
    _inner_header.set_epochid(test_epochID);
    _inner_header.set_timestamp(test_timestamp);
    _inner_header.set_txs_merkle_root_hash(test_txs_merkle_root);
    _inner_header.set_receipts_merkle_root_hash(test_receipts_merkle_root);
    _inner_header.set_block_merkle_root_hash(test_block_merkle_root);
    return _inner_header;
}

xrelay_block_header xrelay_block_header_create(bool election_flag = true, bool signature_flag = true)
{
    xrelay_block_header _block_header;
    xrelay_block_inner_header _inner_header = xrelay_block_inner_header_create();
    _block_header.set_inner_header(_inner_header);
    _block_header.set_prev_hash(test_prev_hash);
    
    xrelay_election_group_t election_set;
    election_set.election_epochID = 100;
    for (int i = 0; i< TEST_ELECTIONS_NUM; i++) {
        election_set.elections_vector.push_back(xrelay_election_create());
    }
    if(true == election_flag){
        _block_header.set_elections_next(election_set);
    }

    std::vector<xrelay_signature_node_t> signature_nodes;
    for (int i = 0; i < TEST_SIGNATURE_NUM; i++) {
        signature_nodes.push_back(xrelay_signature_node_t_create(true));
    }
    if(true == signature_flag){
        _block_header.add_signature_nodes(signature_nodes);
    }

    return _block_header;
}

xrelay_block xrelay_block_create()
{
    xrelay_block  _relay_block;
    xrelay_block_header _block_header = xrelay_block_header_create();
    _relay_block.set_header(_block_header);

    std::vector<xeth_receipt_t>   receipts_vector;
    for(int i = 0; i < TEST_RECEIPT_NUM; i ++) {
        receipts_vector.push_back(xrelay_receipt_create());
    }
    _relay_block.set_receipts(receipts_vector);

    return _relay_block;
}


TEST_F(test_relay_block, serialize_election_set_push) {
    std::error_code  ec;
    xrelay_election_group_t election_set_src;
    election_set_src.election_epochID = 100;
    for (int i = 0; i< TEST_ELECTIONS_NUM; i++) {
        election_set_src.elections_vector.push_back(xrelay_election_create());
    }

    RLPStream rlp_election;
    election_set_src.streamRLP(rlp_election);
    bytes  rlp_bytes = rlp_election.out();

    xrelay_election_group_t election_set_dst;
    election_set_dst.decodeRLP(RLP(rlp_bytes), ec);

    EXPECT_EQ(election_set_src.election_epochID, election_set_dst.election_epochID);
    EXPECT_EQ(election_set_src.elections_vector.size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(election_set_dst.elections_vector.size(), TEST_ELECTIONS_NUM);
    for(unsigned i = 0; i < TEST_ELECTIONS_NUM; i++ ) {
        xrelay_election_node_t election_node_src = election_set_src.elections_vector[i];
        xrelay_election_node_t election_node_dst = election_set_dst.elections_vector[i];
        EXPECT_EQ(election_node_src.stake, election_node_dst.stake);
        EXPECT_EQ(election_node_src.public_key_x, election_node_dst.public_key_x);
        EXPECT_EQ(election_node_src.public_key_y, election_node_dst.public_key_y);
    }
}


TEST_F(test_relay_block, serialize_election_set_empty) {
    std::error_code  ec;
    xrelay_election_group_t election_set_src;
    election_set_src.election_epochID = 100;

    RLPStream rlp_election;
    election_set_src.streamRLP(rlp_election);
    bytes  rlp_bytes = rlp_election.out();

    xrelay_election_group_t election_set_dst;
    election_set_dst.decodeRLP(RLP(rlp_bytes), ec);

    EXPECT_EQ(election_set_dst.election_epochID, 0);
    EXPECT_EQ(election_set_src.elections_vector.size(), 0);
    EXPECT_EQ(election_set_dst.elections_vector.size(), 0);
}


TEST_F(test_relay_block, serialize_receipt_log) {
    std::error_code  ec;
    xevm_log_t log_src = xrelay_evm_log_create();

    RLPStream rlp_log;
    log_src.streamRLP(rlp_log);
    bytes  rlp_bytes = rlp_log.out();

    xevm_log_t log_dst;
    RLP rlp_dst = RLP(rlp_bytes);
    log_dst.decodeRLP(rlp_dst, ec);

    EXPECT_EQ(log_dst.data, log_src.data);
    EXPECT_EQ(log_dst.address.to_hex_string(), log_src.address.to_hex_string());
    for(unsigned i = 0; i < TEST_TOPICS_NUM; i++ ) {
        EXPECT_EQ(log_dst.topics[i], log_src.topics[i]);
    }
}

TEST_F(test_relay_block, serialize_tx) {
    eth_error  ec;
    xeth_transaction_t tx_src = xrelay_tx_create();
    xbytes_t tx_data = tx_src.encodeBytes();
    xeth_transaction_t tx_dst;
    tx_dst.decodeBytes(tx_data, ec);

    EXPECT_EQ(tx_dst.get_from().to_hex_string(), tx_dst.get_from().to_hex_string());
    EXPECT_EQ(tx_dst.get_to().to_hex_string(), tx_dst.get_to().to_hex_string());
    EXPECT_EQ(tx_dst.get_data(), tx_dst.get_data());
    EXPECT_EQ(tx_dst.get_value(), tx_dst.get_value());
    EXPECT_EQ(tx_dst.get_gas(), tx_dst.get_gas());
    EXPECT_EQ(tx_dst.get_max_fee_per_gas(), tx_dst.get_max_fee_per_gas());
}



TEST_F(test_relay_block, serialize_receipt) {

    std::error_code  ec;
    xeth_receipt_t receipt_src = xrelay_receipt_create();
    xbytes_t rlp_receipt = receipt_src.encodeBytes();

    xeth_receipt_t receipt_dst;
    receipt_dst.decodeBytes(rlp_receipt, ec);

    EXPECT_EQ(receipt_dst.get_tx_version_type(), receipt_src.get_tx_version_type());
    EXPECT_EQ(receipt_dst.get_tx_status(), receipt_src.get_tx_status());
    EXPECT_EQ(receipt_dst.get_cumulative_gas_used(), receipt_src.get_cumulative_gas_used());
    EXPECT_EQ(receipt_dst.get_logsBloom().get_data(), receipt_src.get_logsBloom().get_data());
    EXPECT_EQ(receipt_src.get_logs().size(), TEST_RECEIPT_LOG_NUM);
    EXPECT_EQ(receipt_dst.get_logs().size(), TEST_RECEIPT_LOG_NUM);
 
    for (unsigned i = 0; i < TEST_RECEIPT_LOG_NUM; i++) {
        auto &log_src = receipt_src.get_logs()[i];
        auto &log_dst = receipt_dst.get_logs()[i];
        EXPECT_EQ(log_dst.data, log_src.data);
        EXPECT_EQ(log_dst.address.to_hex_string(), log_src.address.to_hex_string());
        EXPECT_EQ(log_dst.topics.size(), TEST_TOPICS_NUM);
        EXPECT_EQ(log_src.topics.size(), TEST_TOPICS_NUM);
        for(unsigned i = 0; i< TEST_TOPICS_NUM; i++ ) {
            EXPECT_EQ(log_dst.topics[i], log_src.topics[i]);
        }    
    }
}

TEST_F(test_relay_block, serialize_xrelay_election) {

    xrelay_election_node_t election_src = xrelay_election_create();

    RLPStream rlp_election;
    election_src.streamRLP(rlp_election);
    bytes  rlp_bytes = rlp_election.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_election_node_t election_dst;
    std::error_code  ec;
    election_dst.decodeRLP(rlp_dst, ec);

    EXPECT_EQ(election_dst.public_key_x, election_src.public_key_x);
    EXPECT_EQ(election_dst.public_key_y, election_src.public_key_y);
    EXPECT_EQ(election_dst.stake, election_src.stake);
}


TEST_F(test_relay_block, serialize_xrelay_signature) {

    xrelay_signature_node_t signature_src = xrelay_signature_node_t_create(true);

    RLPStream rlp_signature;
    signature_src.streamRLP(rlp_signature);
    bytes  rlp_bytes = rlp_signature.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_signature_node_t signature_dst;
    std::error_code  ec;
    signature_dst.decodeRLP(rlp_dst, ec);

    EXPECT_EQ(signature_dst.exist, signature_src.exist);
    EXPECT_EQ(signature_dst.signature.r, signature_src.signature.r);
    EXPECT_EQ(signature_dst.signature.s, signature_src.signature.s);
    EXPECT_EQ(signature_dst.signature.v, signature_src.signature.v);
}

TEST_F(test_relay_block, serialize_xrelay_signature_empty) {

    xrelay_signature_node_t signature_src = xrelay_signature_node_t_create(false);

    RLPStream rlp_signature;
    signature_src.streamRLP(rlp_signature);
    bytes  rlp_bytes = rlp_signature.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_signature_node_t signature_dst;
    std::error_code  ec;
    signature_dst.decodeRLP(rlp_dst, ec);

    EXPECT_EQ(signature_dst.exist, signature_src.exist);
}


TEST_F(test_relay_block, serialize_xrelay_signature_string) {

    xrelay_signature_node_t signature_src = xrelay_signature_node_t_string_create(true);

    RLPStream rlp_signature;
    signature_src.streamRLP(rlp_signature);
    bytes  rlp_bytes = rlp_signature.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_signature_node_t signature_dst;
    std::error_code  ec;
    signature_dst.decodeRLP(rlp_dst, ec);

    EXPECT_EQ(signature_dst.exist, signature_src.exist);
    EXPECT_EQ(signature_dst.signature.r, signature_src.signature.r);
    EXPECT_EQ(signature_dst.signature.s, signature_src.signature.s);
    EXPECT_EQ(signature_dst.signature.v, signature_src.signature.v);
}

TEST_F(test_relay_block, serialize_xrelay_signature_string_empty) {

    xrelay_signature_node_t signature_src = xrelay_signature_node_t_create(false);

    RLPStream rlp_signature;
    signature_src.streamRLP(rlp_signature);
    bytes  rlp_bytes = rlp_signature.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_signature_node_t signature_dst;
    std::error_code  ec;
    signature_dst.decodeRLP(rlp_dst, ec);

    EXPECT_EQ(signature_dst.exist, signature_src.exist);
}

TEST_F(test_relay_block, serialize_xrelay_inner_header) {
    std::error_code  ec;
    xrelay_block_inner_header inner_header_src = xrelay_block_inner_header_create();
    xbytes_t rlp_innder_header = inner_header_src.encodeBytes();
    xrelay_block_inner_header inner_header_dst;
    inner_header_dst.decodeBytes(rlp_innder_header , ec);

    EXPECT_EQ(inner_header_dst.get_block_height(), inner_header_src.get_block_height());
    EXPECT_EQ(inner_header_dst.get_epochID(), inner_header_src.get_epochID());
    EXPECT_EQ(inner_header_dst.get_timestamp(), inner_header_src.get_timestamp());
    EXPECT_EQ(inner_header_dst.get_txs_root_hash(), inner_header_src.get_txs_root_hash());
    EXPECT_EQ(inner_header_dst.get_receipts_root_hash(), inner_header_src.get_receipts_root_hash());
    EXPECT_EQ(inner_header_dst.get_block_root_hash(), inner_header_src.get_block_root_hash());
   
  // std::cout << " innder_header " << toHex(rlp_innder_header.out()) << std::endl;
}

TEST_F(test_relay_block, serialize_xrelay_block_header_without_signature) {
    std::error_code  ec;
    xrelay_block_header block_header_src = xrelay_block_header_create();

    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);

    xbytes_t rlp_block_header = block_header_src.encodeBytes();
    xrelay_block_header block_header_dst;
    block_header_dst.decodeBytes(rlp_block_header, ec);

    xrelay_block_inner_header inner_header_dst = block_header_dst.get_inner_header();
    xrelay_block_inner_header inner_header_src = block_header_src.get_inner_header();
 
    EXPECT_EQ(inner_header_dst.get_block_height(), inner_header_src.get_block_height());
    EXPECT_EQ(inner_header_dst.get_epochID(), inner_header_src.get_epochID());
    EXPECT_EQ(inner_header_dst.get_timestamp(), inner_header_src.get_timestamp());
    EXPECT_EQ(inner_header_dst.get_txs_root_hash(), inner_header_src.get_txs_root_hash());
    EXPECT_EQ(inner_header_dst.get_receipts_root_hash(), inner_header_src.get_receipts_root_hash());
    EXPECT_EQ(inner_header_dst.get_block_root_hash(), inner_header_src.get_block_root_hash());

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), block_header_src.get_prev_block_hash());
    EXPECT_EQ(block_header_dst.get_block_hash(), block_header_src.get_block_hash());

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    for (unsigned i = 0; i < TEST_ELECTIONS_NUM; i++) {
        xrelay_election_node_t xrelay_election_dst = block_header_dst.get_elections_sets().elections_vector[i];
        xrelay_election_node_t xrelay_election_src = block_header_src.get_elections_sets().elections_vector[i];
        EXPECT_EQ(xrelay_election_dst.public_key_x, xrelay_election_src.public_key_x);
        EXPECT_EQ(xrelay_election_dst.public_key_y, xrelay_election_src.public_key_y);
        EXPECT_EQ(xrelay_election_dst.stake, xrelay_election_src.stake);
    }

    //std::cout << " block_header serialize_xrelay_block_header_without_signature  " << toHex(rlp_block_header.out()) << std::endl;
}


TEST_F(test_relay_block, serialize_xrelay_block_header_without_election) {
    std::error_code  ec;
    xrelay_block_header block_header_src = xrelay_block_header_create(false);

    EXPECT_EQ(block_header_src.get_elections_sets().size(), 0);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);

    xbytes_t rlp_block_header = block_header_src.encodeBytes();
    xrelay_block_header block_header_dst;
    block_header_dst.decodeBytes(rlp_block_header, ec);

    xrelay_block_inner_header inner_header_dst = block_header_dst.get_inner_header();
    xrelay_block_inner_header inner_header_src = block_header_src.get_inner_header();
 
    EXPECT_EQ(inner_header_dst.get_block_height(), inner_header_src.get_block_height());
    EXPECT_EQ(inner_header_dst.get_epochID(), inner_header_src.get_epochID());
    EXPECT_EQ(inner_header_dst.get_timestamp(), inner_header_src.get_timestamp());
    EXPECT_EQ(inner_header_dst.get_txs_root_hash(), inner_header_src.get_txs_root_hash());
    EXPECT_EQ(inner_header_dst.get_receipts_root_hash(), inner_header_src.get_receipts_root_hash());
    EXPECT_EQ(inner_header_dst.get_block_root_hash(), inner_header_src.get_block_root_hash());

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), block_header_src.get_prev_block_hash());
    EXPECT_EQ(block_header_dst.get_block_hash(), block_header_src.get_block_hash());

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), 0);
    EXPECT_EQ(block_header_src.get_elections_sets().size(), 0);
   

    //std::cout << " block_header serialize_xrelay_block_header_without_signature  " << toHex(rlp_block_header.out()) << std::endl;
}

TEST_F(test_relay_block, serialize_xrelay_block_header_with_signature) {
    std::error_code  ec;
    xrelay_block_header block_header_src = xrelay_block_header_create();
    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);

    xbytes_t rlp_block_header = block_header_src.encodeBytes(true);
    xrelay_block_header block_header_dst;
    block_header_dst.decodeBytes(rlp_block_header, ec, true);

    xrelay_block_inner_header inner_header_dst = block_header_dst.get_inner_header();
    xrelay_block_inner_header inner_header_src = block_header_src.get_inner_header();

    EXPECT_EQ(inner_header_dst.get_block_height(), inner_header_src.get_block_height());
    EXPECT_EQ(inner_header_dst.get_epochID(), inner_header_src.get_epochID());
    EXPECT_EQ(inner_header_dst.get_timestamp(), inner_header_src.get_timestamp());
    EXPECT_EQ(inner_header_dst.get_txs_root_hash(), inner_header_src.get_txs_root_hash());
    EXPECT_EQ(inner_header_dst.get_receipts_root_hash(), inner_header_src.get_receipts_root_hash());
    EXPECT_EQ(inner_header_dst.get_block_root_hash(), inner_header_src.get_block_root_hash());

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), block_header_src.get_prev_block_hash());
    EXPECT_EQ(block_header_dst.get_block_hash(), block_header_src.get_block_hash());

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    for (unsigned i = 0; i < TEST_ELECTIONS_NUM; i++) {
        xrelay_election_node_t xrelay_election_dst = block_header_dst.get_elections_sets().elections_vector[i];
        xrelay_election_node_t xrelay_election_src = block_header_src.get_elections_sets().elections_vector[i];
        EXPECT_EQ(xrelay_election_dst.public_key_x, xrelay_election_src.public_key_x);
        EXPECT_EQ(xrelay_election_dst.public_key_y, xrelay_election_src.public_key_y);
        EXPECT_EQ(xrelay_election_dst.stake, xrelay_election_src.stake);
    }

    EXPECT_EQ(block_header_dst.get_signatures_sets().size(), TEST_SIGNATURE_NUM);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);
    for (unsigned i = 0; i < TEST_SIGNATURE_NUM; i++) {
        xrelay_signature_node_t xrelay_signature_dst = block_header_dst.get_signatures_sets()[i];
        xrelay_signature_node_t xrelay_signature_src = block_header_src.get_signatures_sets()[i];
        EXPECT_EQ(xrelay_signature_dst.exist,  xrelay_signature_src.exist);
        EXPECT_EQ(xrelay_signature_dst.signature.r,  xrelay_signature_src.signature.r);
        EXPECT_EQ(xrelay_signature_dst.signature.s, xrelay_signature_src.signature.s);
        EXPECT_EQ(xrelay_signature_dst.signature.v, xrelay_signature_src.signature.v);
    }

}


TEST_F(test_relay_block, serialize_xrelay_block_header_with_contract) {
    std::error_code  ec;
    xrelay_block_header block_header_src = xrelay_block_header_create();

    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);

    xbytes_t rlp_block_header = block_header_src.encodeBytes(true);
    xrelay_block_header block_header_dst;
    block_header_dst.decodeBytes(rlp_block_header, ec, true);

    xrelay_block_inner_header inner_header_dst = block_header_dst.get_inner_header();
    xrelay_block_inner_header inner_header_src = block_header_src.get_inner_header();

    EXPECT_EQ(inner_header_dst.get_block_height(), inner_header_src.get_block_height());
    EXPECT_EQ(inner_header_dst.get_epochID(), inner_header_src.get_epochID());
    EXPECT_EQ(inner_header_dst.get_timestamp(), inner_header_src.get_timestamp());
    EXPECT_EQ(inner_header_dst.get_txs_root_hash(), inner_header_src.get_txs_root_hash());
    EXPECT_EQ(inner_header_dst.get_receipts_root_hash(), inner_header_src.get_receipts_root_hash());
    EXPECT_EQ(inner_header_dst.get_block_root_hash(), inner_header_src.get_block_root_hash());

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), block_header_src.get_prev_block_hash());

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    for (unsigned i = 0; i < TEST_ELECTIONS_NUM; i++) {
        xrelay_election_node_t xrelay_election_dst = block_header_dst.get_elections_sets().elections_vector[i];
        xrelay_election_node_t xrelay_election_src = block_header_src.get_elections_sets().elections_vector[i];
        EXPECT_EQ(xrelay_election_dst.public_key_x, xrelay_election_src.public_key_x);
        EXPECT_EQ(xrelay_election_dst.public_key_y, xrelay_election_src.public_key_y);
        EXPECT_EQ(xrelay_election_dst.stake, xrelay_election_src.stake);
    }

    EXPECT_EQ(block_header_dst.get_signatures_sets().size(), TEST_SIGNATURE_NUM);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);
    for (unsigned i = 0; i < TEST_SIGNATURE_NUM; i++) {
        xrelay_signature_node_t xrelay_signature_dst = block_header_dst.get_signatures_sets()[i];
        xrelay_signature_node_t xrelay_signature_src = block_header_src.get_signatures_sets()[i];
        EXPECT_EQ(xrelay_signature_dst.exist,  xrelay_signature_src.exist);
        EXPECT_EQ(xrelay_signature_dst.signature.r,  xrelay_signature_src.signature.r);
        EXPECT_EQ(xrelay_signature_dst.signature.s, xrelay_signature_src.signature.s);
        EXPECT_EQ(xrelay_signature_dst.signature.v, xrelay_signature_src.signature.v);
    }

    xbytes_t rlp_block_header_data =  block_header_src.streamRLP_header_to_contract();
    std::cout << " streamRLP_to_contract   " << toHex(rlp_block_header_data) << std::endl;
    std::ofstream fin("block_index_0.bin", std::ios::binary);
    for (auto c :  rlp_block_header_data) {
        fin.write((char*)&c, sizeof(uint8_t));
    }
    fin.close();
}

TEST_F(test_relay_block, serialize_xrelay_block_without_signature) {
    std::error_code  ec;
    xrelay_block  block_src = xrelay_block_create();

    xbytes_t rlp_block =  block_src.encodeBytes();
    xrelay_block  block_dst;
    block_dst.decodeBytes(rlp_block, ec);

    xrelay_block_header block_header_dst = block_dst.get_header();
    xrelay_block_header block_header_src = block_src.get_header();

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), block_header_src.get_prev_block_hash());
    EXPECT_EQ(block_header_dst.get_block_hash(), block_header_src.get_block_hash());

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    for(unsigned i = 0; i < TEST_ELECTIONS_NUM; i++) {
        xrelay_election_node_t xrelay_election_dst = block_header_dst.get_elections_sets().elections_vector[i];
        xrelay_election_node_t xrelay_election_src = block_header_src.get_elections_sets().elections_vector[i];
        EXPECT_EQ(xrelay_election_dst.public_key_x, xrelay_election_src.public_key_x);
        EXPECT_EQ(xrelay_election_dst.public_key_y, xrelay_election_src.public_key_y);
        EXPECT_EQ(xrelay_election_dst.stake, xrelay_election_src.stake);
    }

    xrelay_block_inner_header inner_header_dst = block_dst.get_inner_header();
    xrelay_block_inner_header inner_header_src = block_src.get_inner_header();

    EXPECT_EQ(inner_header_dst.get_block_height(), inner_header_src.get_block_height());
    EXPECT_EQ(inner_header_dst.get_epochID(), inner_header_src.get_epochID());
    EXPECT_EQ(inner_header_dst.get_timestamp(), inner_header_src.get_timestamp());
    EXPECT_EQ(inner_header_dst.get_txs_root_hash(), inner_header_src.get_txs_root_hash());
    EXPECT_EQ(inner_header_dst.get_receipts_root_hash(), inner_header_src.get_receipts_root_hash());
    EXPECT_EQ(inner_header_dst.get_block_root_hash(), inner_header_src.get_block_root_hash());

    EXPECT_EQ(block_dst.get_all_receipts().size(), TEST_RECEIPT_NUM);
    EXPECT_EQ(block_src.get_all_receipts().size(), TEST_RECEIPT_NUM);
    for (unsigned i = 0; i < TEST_RECEIPT_NUM; i++) {
        auto receipt_src = block_src.get_all_receipts()[i];
        auto receipt_dst = block_dst.get_all_receipts()[i];

        EXPECT_EQ(receipt_dst.get_tx_version_type(), receipt_src.get_tx_version_type());
        EXPECT_EQ(receipt_dst.get_tx_status(), receipt_src.get_tx_status());
        EXPECT_EQ(receipt_dst.get_cumulative_gas_used(), receipt_src.get_cumulative_gas_used());
        EXPECT_EQ(receipt_dst.get_logsBloom().get_data(), receipt_src.get_logsBloom().get_data());

        EXPECT_EQ(receipt_src.get_logs().size(), TEST_RECEIPT_LOG_NUM);
        EXPECT_EQ(receipt_dst.get_logs().size(), TEST_RECEIPT_LOG_NUM);
        
        for (unsigned i = 0; i < TEST_RECEIPT_LOG_NUM; i++) {
            auto log_src = receipt_src.get_logs()[i];
            auto log_dst = receipt_dst.get_logs()[i];

            EXPECT_EQ(log_dst.data, log_src.data);
            EXPECT_EQ(log_dst.address.to_hex_string(), log_src.address.to_hex_string());
            EXPECT_EQ(log_dst.topics.size(), TEST_TOPICS_NUM);
            EXPECT_EQ(log_src.topics.size(), TEST_TOPICS_NUM);
            for(unsigned i = 0; i< TEST_TOPICS_NUM; i++ ) {
                EXPECT_EQ(log_dst.topics[i], log_src.topics[i]);
            }         
        }
    }
}

TEST_F(test_relay_block, serialize_xrelay_block_with_signature) {
    std::error_code  ec;
    xrelay_block  block_src = xrelay_block_create();

    xbytes_t rlp_block = block_src.encodeBytes(true);
    xrelay_block  block_dst;
    block_dst.decodeBytes(rlp_block, ec, true);

    xrelay_block_header block_header_dst = block_dst.get_header();
    xrelay_block_header block_header_src = block_src.get_header();

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), block_header_src.get_prev_block_hash());
    EXPECT_EQ(block_header_dst.get_block_hash(), block_header_src.get_block_hash());

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    for(unsigned i = 0; i < TEST_ELECTIONS_NUM; i++) {
        auto xrelay_election_dst = block_header_dst.get_elections_sets().elections_vector[i];
        auto xrelay_election_src = block_header_src.get_elections_sets().elections_vector[i];
        EXPECT_EQ(xrelay_election_dst.public_key_x, xrelay_election_src.public_key_x);
        EXPECT_EQ(xrelay_election_dst.public_key_y, xrelay_election_src.public_key_y);
        EXPECT_EQ(xrelay_election_dst.stake, xrelay_election_src.stake);
    }

    EXPECT_EQ(block_header_dst.get_signatures_sets().size(), TEST_SIGNATURE_NUM);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);
    for (unsigned i = 0; i < TEST_SIGNATURE_NUM; i++) {
        xrelay_signature_node_t xrelay_signature_dst = block_header_dst.get_signatures_sets()[i];
        xrelay_signature_node_t xrelay_signature_src = block_header_src.get_signatures_sets()[i];
        EXPECT_EQ(xrelay_signature_dst.exist,  xrelay_signature_src.exist);
        EXPECT_EQ(xrelay_signature_dst.signature.r,  xrelay_signature_src.signature.r);
        EXPECT_EQ(xrelay_signature_dst.signature.s, xrelay_signature_src.signature.s);
        EXPECT_EQ(xrelay_signature_dst.signature.v, xrelay_signature_src.signature.v);
    }

    xrelay_block_inner_header inner_header_dst = block_dst.get_inner_header();
    xrelay_block_inner_header inner_header_src = block_src.get_inner_header();

    EXPECT_EQ(inner_header_dst.get_block_height(), inner_header_src.get_block_height());
    EXPECT_EQ(inner_header_dst.get_epochID(), inner_header_src.get_epochID());
    EXPECT_EQ(inner_header_dst.get_timestamp(), inner_header_src.get_timestamp());
    EXPECT_EQ(inner_header_dst.get_txs_root_hash(), inner_header_src.get_txs_root_hash());
    EXPECT_EQ(inner_header_dst.get_receipts_root_hash(), inner_header_src.get_receipts_root_hash());
    EXPECT_EQ(inner_header_dst.get_block_root_hash(), inner_header_src.get_block_root_hash());

    EXPECT_EQ(block_dst.get_all_receipts().size(), TEST_RECEIPT_NUM);
    EXPECT_EQ(block_src.get_all_receipts().size(), TEST_RECEIPT_NUM);
     for (unsigned i = 0; i < TEST_RECEIPT_NUM; i++) {
        auto receipt_src = block_src.get_all_receipts()[i];
        auto receipt_dst = block_dst.get_all_receipts()[i];

        EXPECT_EQ(receipt_dst.get_tx_version_type(), receipt_src.get_tx_version_type());
        EXPECT_EQ(receipt_dst.get_tx_status(), receipt_src.get_tx_status());
        EXPECT_EQ(receipt_dst.get_cumulative_gas_used(), receipt_src.get_cumulative_gas_used());
        EXPECT_EQ(receipt_dst.get_logsBloom().get_data(), receipt_src.get_logsBloom().get_data());

        EXPECT_EQ(receipt_src.get_logs().size(), TEST_RECEIPT_LOG_NUM);
        EXPECT_EQ(receipt_dst.get_logs().size(), TEST_RECEIPT_LOG_NUM);
        
        for (unsigned i = 0; i < TEST_RECEIPT_LOG_NUM; i++) {
            auto log_src = receipt_src.get_logs()[i];
            auto log_dst = receipt_dst.get_logs()[i];

            EXPECT_EQ(log_dst.data, log_src.data);
            EXPECT_EQ(log_dst.address.to_hex_string(), log_src.address.to_hex_string());
            EXPECT_EQ(log_dst.topics.size(), TEST_TOPICS_NUM);
            EXPECT_EQ(log_src.topics.size(), TEST_TOPICS_NUM);
            for(unsigned i = 0; i< TEST_TOPICS_NUM; i++ ) {
                EXPECT_EQ(log_dst.topics[i], log_src.topics[i]);
            }         
        }
    }

    block_dst.build_finish();  
    xrelay_block_header relayer_header_dst_build = block_dst.get_header();
   // std::cout << " header inner hash   " <<  block_dst.get_inner_header_hash() << std::endl;
  //  std::cout << " block  hash   " <<  block_dst.get_block_hash() << std::endl;

    xbytes_t rlp_block_header_data =  block_dst.streamRLP_header_to_contract();
    std::cout << " streamRLP_to_contract   " << toHex(rlp_block_header_data) << std::endl;
    std::ofstream fin("block_index_0.bin", std::ios::binary);
    for (auto c :  rlp_block_header_data) {
        fin.write((char*)&c, sizeof(uint8_t));
    }
    fin.close();

}

TEST_F(test_relay_block, serialize_xrelay_block_without_election) {


    xrelay_block  _relay_block;
    xrelay_block_header _block_header = xrelay_block_header_create(false);
    _relay_block.set_header(_block_header);

    std::vector<xeth_receipt_t>   receipts_vector;
    for(int i = 0; i < TEST_RECEIPT_NUM; i ++) {
        receipts_vector.push_back(xrelay_receipt_create());
    }
    _relay_block.set_receipts(receipts_vector);

    EXPECT_EQ(_block_header.get_elections_sets().size(), 0);
    EXPECT_EQ(_block_header.get_signatures_sets().size(), TEST_SIGNATURE_NUM);

    _relay_block.build_finish();

    xrelay_block_header relayer_header_dst_build = _relay_block.get_header();
   // std::cout << " header inner hash   " <<  block_dst.get_inner_header_hash() << std::endl;
  //  std::cout << " block  hash   " <<  block_dst.get_block_hash() << std::endl;

    xbytes_t rlp_block_header_data =  _relay_block.streamRLP_header_to_contract();
    std::cout << " streamRLP_to_contract   " << toHex(rlp_block_header_data) << std::endl;
    std::ofstream fin("block_index_no_election.bin", std::ios::binary);
    for (auto c :  rlp_block_header_data) {
        fin.write((char*)&c, sizeof(uint8_t));
    }
    fin.close();
}


TEST_F(test_relay_block, serialize_xrelay_block_sign) {

    utl::xecprikey_t  random_data; //using xecprikey_t generate random data
    uint256_t msg_digest((uint8_t*)random_data.data());
    
    utl::xecprikey_t raw_pri_key_obj;
    utl::xecpubkey_t raw_pub_key_obj = raw_pri_key_obj.get_public_key();
    const std::string uncompressed_pub_key_data((const char*)raw_pub_key_obj.data(),raw_pub_key_obj.size());
    const std::string compressed_pub_key_data = raw_pri_key_obj.get_compress_public_key();
    const std::string account_addr_from_raw_pri_key = raw_pri_key_obj.to_account_address(enum_vaccount_addr_type_secp256k1_user_account, 0);
    //std::cout << " account_addr_from_raw_pri_key from pri key:" << account_addr_from_raw_pri_key << std::endl;
    utl::xecdsasig_t signature_obj = raw_pri_key_obj.sign(msg_digest);
    const std::string signature = utl::xcrypto_util::digest_sign(msg_digest,raw_pri_key_obj.data());
    xassert(utl::xcrypto_util::verify_sign(msg_digest,signature,account_addr_from_raw_pri_key));
    
 
    const std::string account_addr_from_raw_pub_key = raw_pub_key_obj.to_address(enum_vaccount_addr_type_secp256k1_user_account, 0);
    //std::cout << " account_addr_from_raw_pub_key from  raw_pub_key_obj: " << account_addr_from_raw_pub_key << std::endl;
    xassert(account_addr_from_raw_pri_key == account_addr_from_raw_pub_key);
    xassert(raw_pub_key_obj.verify_signature(signature_obj, msg_digest));
    

    std::string pub_addr_new;
    uint8_t  out_publickey_data[65] = {0};
    if(utl::xsecp256k1_t::get_publickey_from_signature(signature_obj, msg_digest, out_publickey_data))
    {
        utl::xecpubkey_t raw_pub_key_obj_new = utl::xecpubkey_t(out_publickey_data);
        pub_addr_new = raw_pub_key_obj_new.to_address(enum_vaccount_addr_type_secp256k1_user_account, 0);
        //std::cout << " pub_addr_new from raw_pub_key_obj_new: " << pub_addr_new << std::endl;
    }
    EXPECT_EQ(account_addr_from_raw_pri_key, account_addr_from_raw_pub_key);
    EXPECT_EQ(pub_addr_new, account_addr_from_raw_pub_key);
    
}