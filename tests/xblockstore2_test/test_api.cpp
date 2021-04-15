#include "gtest/gtest.h"
#include "xdata/xblocktool.h"
#include "xstore/xstore.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xdatamock_address.hpp"
#include "tests/mock/xcertauth_util.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xblock_generator.hpp"
#include "test_blockstore_util.hpp"

using namespace top;
using namespace top::base;
using namespace top::mock;

class test_api : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_api, store_genesis_block_1) {
    std::string address = mock::xdatamock_address::make_user_address_random();
    test_blockstore_util blockstore_util;
    base::xvblockstore_t* blockstore = blockstore_util.get_blockstore();

    uint64_t init_balance = 100;
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(address, init_balance);
    xassert(genesis_block != nullptr);
    base::xvaccount_t _vaddr(address);

    auto ret = blockstore->store_block(_vaddr, genesis_block.get());
    xassert(ret);
}

TEST_F(test_api, store_genesis_block_2) {
    std::string address = mock::xdatamock_address::make_user_address_random();
    test_blockstore_util blockstore_util;
    base::xvblockstore_t* blockstore = blockstore_util.get_blockstore();

    uint64_t init_balance = 100;
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(address, init_balance);
    xassert(genesis_block != nullptr);
    base::xvaccount_t _vaddr(address);
    auto ret = blockstore->store_block(_vaddr, genesis_block.get());
    xassert(ret);
    sleep(15);
    xdbg("jimmy========acct_t begin to get");
    base::xauto_ptr<base::xvblock_t> query_block = blockstore->get_latest_committed_block(_vaddr);
    xdbg("jimmy========acct_t end to get");
    xassert(query_block->get_block_class() == base::enum_xvblock_class_light);
    data::xlightunit_block_t* lightunit = dynamic_cast<data::xlightunit_block_t*>(query_block.get());
    xassert(lightunit != nullptr);
    xassert((int64_t)init_balance == lightunit->get_balance_change());
}

TEST_F(test_api, store_empty_block_1) {
    std::string address = mock::xdatamock_address::make_user_address_random();
    test_blockstore_util blockstore_util;
    base::xvblockstore_t* blockstore = blockstore_util.get_blockstore();

    uint64_t init_balance = 100;
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_empty_block(address);
    xassert(genesis_block != nullptr);
    base::xvaccount_t _vaddr(address);

    uint64_t count = 5;
    xblock_ptr_t prev_block = xblock_t::raw_vblock_to_object_ptr(genesis_block.get());
    for (uint64_t i = 0; i < count; i++) {
        base::xauto_ptr<base::xvblock_t> next_block = data::xblocktool_t::create_next_emptyblock(prev_block.get());
        xcertauth_util::instance().do_multi_sign(next_block.get());
        xassert(blockstore->store_block(_vaddr, next_block.get()));
        prev_block = xblock_t::raw_vblock_to_object_ptr(next_block.get());
    }
    base::xauto_ptr<base::xvblock_t> latest_cert_block = blockstore->get_latest_cert_block(_vaddr);
    xassert(latest_cert_block->get_height() == count);
    base::xauto_ptr<base::xvblock_t> latest_locked_block = blockstore->get_latest_locked_block(_vaddr);
    xassert(latest_locked_block->get_height() == count - 1);
    base::xauto_ptr<base::xvblock_t> latest_committed_block = blockstore->get_latest_committed_block(_vaddr);
    xassert(latest_committed_block->get_height() == count - 2);
    base::xauto_ptr<base::xvblock_t> latest_executed_block = blockstore->get_latest_executed_block(_vaddr);
    xassert(latest_executed_block->get_height() == count - 2);
}

TEST_F(test_api, store_unit_block_1) {
    std::string address = mock::xdatamock_address::make_user_address_random();
    test_blockstore_util blockstore_util;
    base::xvblockstore_t* blockstore = blockstore_util.get_blockstore();

    uint64_t max_block_height = 1;
    xdatamock_table mocktable(1, 1);
    mocktable.genrate_table_chain(max_block_height);

    const std::vector<xdatamock_unit> & mockunit = mocktable.get_mock_units();

    base::xvaccount_t unit_vaddr(mockunit[0].get_account());

    const std::vector<xblock_ptr_t> & units = mockunit[0].get_history_units();
    xassert(units.size() == max_block_height+1);
    for (uint64_t i = 0; i <= max_block_height; i++) {
        ASSERT_TRUE(blockstore->store_block(unit_vaddr, units[i].get()));
    }

    base::xauto_ptr<base::xvblock_t> latest_cert_block = blockstore->get_latest_cert_block(unit_vaddr);
    std::cout << "cert=" << latest_cert_block->get_height() << std::endl;
    xassert(latest_cert_block->get_height() == max_block_height);
    base::xauto_ptr<base::xvblock_t> latest_locked_block = blockstore->get_latest_locked_block(unit_vaddr);
    std::cout << "lock=" << latest_locked_block->get_height() << std::endl;
    xassert(latest_locked_block->get_height() == max_block_height - 1);
    base::xauto_ptr<base::xvblock_t> latest_committed_block = blockstore->get_latest_committed_block(unit_vaddr);
    std::cout << "commit=" << latest_committed_block->get_height() << std::endl;
    xassert(latest_committed_block->get_height() == max_block_height - 1);
    base::xauto_ptr<base::xvblock_t> latest_executed_block = blockstore->get_latest_executed_block(unit_vaddr);
    std::cout << "execute=" << latest_executed_block->get_height() << std::endl;
    xassert(latest_executed_block->get_height() == max_block_height - 1);
}

TEST_F(test_api, execute_block_1) {
    std::string address = mock::xdatamock_address::make_user_address_random();
    base::xvaccount_t _vaddr(address);
    test_blockstore_util blockstore_util;
    base::xvblockstore_t* blockstore = blockstore_util.get_blockstore();

    uint64_t max_block_height = 8;
    xblock_generator bgenerator(address);
    const std::vector<xblock_ptr_t> & blocks = bgenerator.generate_all_empty_blocks(max_block_height);
    for (uint64_t i = 5; i <= max_block_height; i++) {
        ASSERT_TRUE(blockstore->store_block(_vaddr, blocks[i].get()));
    }
    for (uint64_t i = 4; i >= 1; i--) {
        ASSERT_TRUE(blockstore->store_block(_vaddr, blocks[i].get()));
    }

    base::xauto_ptr<base::xvblock_t> latest_cert_block = blockstore->get_latest_cert_block(_vaddr);
    xassert(latest_cert_block->get_height() == max_block_height);
    base::xauto_ptr<base::xvblock_t> latest_locked_block = blockstore->get_latest_locked_block(_vaddr);
    xassert(latest_locked_block->get_height() == max_block_height - 1);
    base::xauto_ptr<base::xvblock_t> latest_committed_block = blockstore->get_latest_committed_block(_vaddr);
    xassert(latest_committed_block->get_height() == max_block_height - 2);
    base::xauto_ptr<base::xvblock_t> latest_executed_block = blockstore->get_latest_executed_block(_vaddr);
    xassert(latest_executed_block->get_height() == max_block_height - 2);
}

TEST_F(test_api, store_table_block_1) {
    std::string address = mock::xdatamock_address::make_user_address_random();
    test_blockstore_util blockstore_util;
    base::xvblockstore_t* blockstore = blockstore_util.get_blockstore();

    uint64_t max_block_height = 1;
    xdatamock_table mocktable;
    mocktable.genrate_table_chain(max_block_height);

    base::xvaccount_t table_vaddr(mocktable.get_account());

    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == max_block_height+1);
    for (uint64_t i = 0; i <= max_block_height; i++) {
        ASSERT_TRUE(blockstore->store_block(table_vaddr, tables[i].get()));
    }

    base::xauto_ptr<base::xvblock_t> latest_cert_block = blockstore->get_latest_cert_block(table_vaddr);
    std::cout << "cert=" << latest_cert_block->get_height() << std::endl;
    // xassert(latest_cert_block->get_height() == count);
    base::xauto_ptr<base::xvblock_t> latest_locked_block = blockstore->get_latest_locked_block(table_vaddr);
    std::cout << "lock=" << latest_locked_block->get_height() << std::endl;
    // xassert(latest_locked_block->get_height() == count - 1);
    base::xauto_ptr<base::xvblock_t> latest_committed_block = blockstore->get_latest_committed_block(table_vaddr);
    std::cout << "commit=" << latest_committed_block->get_height() << std::endl;
    // xassert(latest_committed_block->get_height() == count - 2);
    base::xauto_ptr<base::xvblock_t> latest_executed_block = blockstore->get_latest_executed_block(table_vaddr);
    std::cout << "execute=" << latest_executed_block->get_height() << std::endl;
    // xassert(latest_executed_block->get_height() == count - 2);
}

TEST_F(test_api, store_table_block_2) {
    std::string address = mock::xdatamock_address::make_user_address_random();
    test_blockstore_util blockstore_util;
    base::xvblockstore_t* blockstore = blockstore_util.get_blockstore();

    uint64_t max_block_height = 10;
    xdatamock_table mocktable;
    mocktable.genrate_table_chain(max_block_height);

    base::xvaccount_t table_vaddr(mocktable.get_account());

    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == max_block_height+1);
    for (uint64_t i = 0; i <= max_block_height; i++) {
        ASSERT_TRUE(blockstore->store_block(table_vaddr, tables[i].get()));
    }

    base::xauto_ptr<base::xvblock_t> latest_cert_block = blockstore->get_latest_cert_block(table_vaddr);
    std::cout << "cert=" << latest_cert_block->get_height() << std::endl;
    xassert(latest_cert_block->get_height() == max_block_height);
    base::xauto_ptr<base::xvblock_t> latest_locked_block = blockstore->get_latest_locked_block(table_vaddr);
    std::cout << "lock=" << latest_locked_block->get_height() << std::endl;
    xassert(latest_locked_block->get_height() == max_block_height - 1);
    base::xauto_ptr<base::xvblock_t> latest_committed_block = blockstore->get_latest_committed_block(table_vaddr);
    std::cout << "commit=" << latest_committed_block->get_height() << std::endl;
    xassert(latest_committed_block->get_height() == max_block_height - 2);
    base::xauto_ptr<base::xvblock_t> latest_executed_block = blockstore->get_latest_executed_block(table_vaddr);
    std::cout << "execute=" << latest_executed_block->get_height() << std::endl;
    xassert(latest_executed_block->get_height() == max_block_height - 2);
}

TEST_F(test_api, store_table_block_3) {
    std::string address = mock::xdatamock_address::make_user_address_random();
    test_blockstore_util blockstore_util;
    base::xvblockstore_t* blockstore = blockstore_util.get_blockstore();

    uint64_t max_block_height = 10;
    xdatamock_table mocktable;
    mocktable.genrate_table_chain(max_block_height);

    base::xvaccount_t table_vaddr(mocktable.get_account());

    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == max_block_height+1);
    for (uint64_t i = 5; i <= max_block_height; i++) {
        ASSERT_TRUE(blockstore->store_block(table_vaddr, tables[i].get()));
    }
    for (uint64_t i = 4; i >= 1; i--) {
        ASSERT_TRUE(blockstore->store_block(table_vaddr, tables[i].get()));
    }

    base::xauto_ptr<base::xvblock_t> latest_cert_block = blockstore->get_latest_cert_block(table_vaddr);
    std::cout << "cert=" << latest_cert_block->get_height() << std::endl;
    xassert(latest_cert_block->get_height() == max_block_height);
    base::xauto_ptr<base::xvblock_t> latest_locked_block = blockstore->get_latest_locked_block(table_vaddr);
    std::cout << "lock=" << latest_locked_block->get_height() << std::endl;
    xassert(latest_locked_block->get_height() == max_block_height - 1);
    base::xauto_ptr<base::xvblock_t> latest_committed_block = blockstore->get_latest_committed_block(table_vaddr);
    std::cout << "commit=" << latest_committed_block->get_height() << std::endl;
    xassert(latest_committed_block->get_height() == max_block_height - 2);
    base::xauto_ptr<base::xvblock_t> latest_executed_block = blockstore->get_latest_executed_block(table_vaddr);
    std::cout << "execute=" << latest_executed_block->get_height() << std::endl;
    xassert(latest_executed_block->get_height() == max_block_height - 2);
}

// TEST_F(test_api, store_tx_1) {
//     std::string address = mock::xdatamock_address::make_user_address_random();
//     test_blockstore_util blockstore_util;
//     base::xvblockstore_t* blockstore = blockstore_util.get_blockstore();

//     uint64_t init_balance = 100;
//     base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
//     xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
//     tx->set_digest();
//     tx->set_len();
//     xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(tx.get());
//     xlightunit_block_para_t para1;
//     para1.set_one_input_tx(tx);
//     base::xauto_ptr<base::xvblock_t> lightunit1 = xblocktool_t::create_next_lightunit(para1, genesis_block.get());

//     xassert(genesis_block != nullptr);
//     base::xvaccount_t _vaddr(address);
//     auto ret = blockstore->store_block(_vaddr, lightunit1.get());
//     xassert(ret);

//     base::xauto_ptr<base::xvblock_t> query_block = blockstore->get_latest_cert_block(_vaddr);
//     xassert(query_block->get_block_class() == base::enum_xvblock_class_light);
//     data::xlightunit_block_t* lightunit = dynamic_cast<data::xlightunit_block_t*>(query_block.get());
//     xassert(lightunit != nullptr);
//     xassert(lightunit->get_height() == 1);

//     xvtransaction_store_ptr_t txstore = blockstore->query_tx(tx->get_digest_str(), enum_transaction_subtype_all);
//     xassert(txstore != nullptr);
// }

