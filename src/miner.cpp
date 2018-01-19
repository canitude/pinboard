/**
 * Copyright (c) 2017-2018
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <random>
#include <chrono>
#include "miner.hpp"

using namespace std;

namespace libbitcoin {
namespace message {

template <class F>
miner<F>::miner(object_payload::ptr obj, chain_sync_state::ptr chain_state)
    : obj_(obj), chain_state_(chain_state)
{
}

template <class F>
miner<F>::~miner()
{
}

template <class F>
void miner<F>::start_mining(const uint256_t &target, result_handler handler)
{
    obj_->pow_.type_ = F::type();
    obj_->pow_.tag_ = chain_tag::litecoin_main;

    uint64_t &nonce = obj_->pow_.nonce_;
    hash_digest h;

    bc::code ec = bc::error::success;

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    uint64_t start_nonce = dis(gen);

    uint256_t estimated_work = ((~target) / (target + 1)) + 1;
    LOG_INFO(LOG_MINER) << "estimated work = " << estimated_work
                        << ", starting from nonce = " << start_nonce;

    auto start = chrono::steady_clock::now();
    for (nonce = start_nonce; nonce <numeric_limits<uint64_t>::max(); nonce++)
    {
        const std::set<bc::hash_digest> top = chain_state_->get_last_known_block_hash();
        // this is very uneffective due to locks in chain_sync_state

        if (top.empty())
        {
            ec = bc::error::unknown;
            break;
        }
        obj_->pow_.anchor_ = *top.begin();

        data_chunk data = obj_->serialize_id_and_pow();
        h = F::calculate(data);
        if (to_uint256(h) < target)
        {
            auto duration = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start) / 1000;
            LOG_INFO(LOG_MINER) << "Miner: success. nonce = " << nonce;
            LOG_INFO(LOG_MINER) << "t = " << hex << target << dec;
            LOG_INFO(LOG_MINER) << "h = " << hex << to_uint256(h) << dec;
            LOG_INFO(LOG_MINER) << "h = " << bc::encode_base16(h);
            LOG_INFO(LOG_MINER) << "mining time: " << duration.count()
                                << " sec. Attempts done: " << (nonce - start_nonce) << ".";
            double hashrate = (double(nonce - start_nonce) / duration.count());
            LOG_INFO(LOG_MINER) << "Hashrate = " << hashrate << " h/s";
            break;
        }
    }

    LOG_INFO(LOG_MINER) << "ec == " << ec;

    handler(ec, obj_);
}

template class miner<pow_scrypt_14_1_8>;
template class miner<pow_scrypt_10_1_1>;
}
}