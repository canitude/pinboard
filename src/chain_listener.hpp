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
#ifndef CHAIN_LISTENER_HPP
#define CHAIN_LISTENER_HPP

#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/bitcoin/utility/thread.hpp>
#include <bitcoin/bitcoin/math/hash.hpp>
#include <bitcoin/bitcoin/message/messages.hpp>
#include <bitcoin/bitcoin/config/checkpoint.hpp>

#include "lite_header.hpp"
#include "message_broadcaster.hpp"

#define LOG_CHAIN_LISTENER "chain_listener"

class chain_sync_state
{
public:
    typedef std::shared_ptr<chain_sync_state> ptr;

    typedef std::map<bc::hash_digest, bc::chain::lite_header> hash_to_header_map;

    explicit chain_sync_state(bc::node::message_broadcaster::ptr broadcaster, const bc::chain::lite_header& last_checkpoint);
    virtual ~chain_sync_state();

    bc::code merge(bc::headers_const_ptr message);

    bool try_to_connect_orphans() { return false; }

    const std::set<bc::hash_digest> get_last_known_block_hash() const;
    const std::set<bc::hash_digest> get_known_block_hashes(size_t height) const;
    uint32_t get_latest_timestamp() const;
    size_t get_top_height() const;
    bc::config::checkpoint get_top_checkpoint() const;
    bool get_header_by_id(const bc::hash_digest &id, bc::chain::lite_header &header);
    bool get_height_by_id(const bc::hash_digest &id, size_t &height);
    bool get_prev_hash_by_id(const bc::hash_digest &id, bc::hash_digest &prev_hash);
    bool is_synchronized() const;

protected:

    bc::node::message_broadcaster::ptr broadcaster_;

    // -------------------------------------------------------------------------
    mutable bc::upgrade_mutex mutex_;
    size_t starting_height_;
    std::vector<std::set<bc::hash_digest>> chain_;
    hash_to_header_map known_blocks_;
    hash_to_header_map orphans_;
    // -------------------------------------------------------------------------
};

#endif
