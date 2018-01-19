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
#ifndef LIBBITCOIN_MINER_HPP
#define LIBBITCOIN_MINER_HPP

#include "object.hpp"
#include "chain_listener.hpp"

#define LOG_MINER "miner"

namespace libbitcoin {
namespace message {

template <class F>
class miner
{
public:
    typedef std::shared_ptr<miner<F>> ptr;
    typedef std::function<void(const bc::code&, object_payload::ptr obj)> result_handler;

    miner(object_payload::ptr obj, chain_sync_state::ptr chain_state);
    ~miner();

    void start_mining(const uint256_t &target, result_handler handler);

private:
    object_payload::ptr obj_;
    chain_sync_state::ptr chain_state_;
};

}
}

#endif