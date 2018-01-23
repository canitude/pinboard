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

#include <ctime>

#include <bitcoin/bitcoin/log/source.hpp>

#include "chain_listener.hpp"

using namespace std;
using namespace bc;
using namespace bc::node;
using namespace bc::message;

chain_sync_state::chain_sync_state(message_broadcaster::ptr broadcaster, const bc::chain::lite_header& last_checkpoint)
    : broadcaster_(broadcaster),
      starting_height_(last_checkpoint.validation.height)
{
    //chain_.reserve(100000); // commented for testing purpose
    chain_.resize(1);
    chain_[0].insert(last_checkpoint.hash());
    known_blocks_[last_checkpoint.hash()] = last_checkpoint;
    LOG_INFO(LOG_CHAIN_LISTENER) << "chain_sync_state::chain_sync_state completed.";
}

chain_sync_state::~chain_sync_state()
{
    LOG_INFO(LOG_CHAIN_LISTENER) << "-> chain_sync_state::~chain_sync_state";
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    bc::shared_lock lock(mutex_);

    for (size_t h = 0; h < chain_.size(); h++) {
        for (const auto &header_hash : chain_[h])
            LOG_INFO(LOG_CHAIN_LISTENER) << (h + starting_height_) << " " << bc::encode_base16(header_hash);
    }
    ///////////////////////////////////////////////////////////////////////////
}

const set<hash_digest> chain_sync_state::get_last_known_block_hash() const
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    bc::shared_lock lock(mutex_);
    for (size_t i = chain_.size() - 1; i >= 0; i--) {
        set<hash_digest>::const_iterator iter = chain_[i].begin();
        if (iter != chain_[i].end() && *iter != null_hash) {
            return chain_[i];
        }
    }

    // we should know at least last checkpoint. Let's consider this state as an error.
    LOG_ERROR(LOG_CHAIN_LISTENER) << "No known block hashes. At least checkpoint block hash is required to sync.";

    const set<hash_digest> empty_set;
    return empty_set;
    ///////////////////////////////////////////////////////////////////////////
}

const std::set<bc::hash_digest> chain_sync_state::get_known_block_hashes(size_t height) const
{
    const set<hash_digest> empty_set;
    if (height < starting_height_)
    {
        LOG_ERROR(LOG_CHAIN_LISTENER) << "Height " << height
                                      << " is below our earliest checkpoint " << starting_height_;
        return empty_set;
    }

    if (height - starting_height_ >= chain_.size())
    {
        LOG_ERROR(LOG_CHAIN_LISTENER) << "Height " << height
                                      << " is above our oldest known header " << starting_height_ + chain_.size() - 1;
        return empty_set;
    }

    size_t idx = height - starting_height_;

    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    bc::shared_lock lock(mutex_);
    for (size_t i = idx; i >= 0; i--) {
        set<hash_digest>::const_iterator iter = chain_[i].begin();
        if (iter != chain_[i].end() && *iter != null_hash) {
            return chain_[i];
        }
    }

    // we should know at least last checkpoint. Let's consider this state as an error.
    LOG_ERROR(LOG_CHAIN_LISTENER) << "No known block hashes. At least checkpoint block hash is required to sync.";

    return empty_set;
    ///////////////////////////////////////////////////////////////////////////
}

uint32_t chain_sync_state::get_latest_timestamp() const
{
    const set<hash_digest> top = get_last_known_block_hash();

    uint32_t latest_timestamp = 0;
    for (const auto &h : top)
    {
        if (h != null_hash)
        {
            const auto iter = known_blocks_.find(h);
            if (iter->second.timestamp() > latest_timestamp)
                latest_timestamp = iter->second.timestamp();
        }

    }
    return latest_timestamp;
}

size_t chain_sync_state::get_top_height() const
{
    const set<hash_digest> top = get_last_known_block_hash();

    size_t max_height = 0;
    for (const auto &h : top)
    {
        if (h != null_hash)
        {
            const auto iter = known_blocks_.find(h);
            if (iter->second.validation.height > max_height)
                max_height = iter->second.validation.height;
        }

    }

    return max_height;
}

bc::config::checkpoint chain_sync_state::get_top_checkpoint() const
{
    const set<hash_digest> top = get_last_known_block_hash();

    size_t max_height = 0;
    hash_digest id_at_max_height = null_hash;
    for (const auto &h : top)
    {
        if (h != null_hash)
        {
            const auto iter = known_blocks_.find(h);
            if (iter->second.validation.height > max_height)
            {
                max_height = iter->second.validation.height;
                id_at_max_height = iter->second.hash();
            }
        }
    }

    config::checkpoint ch(id_at_max_height, max_height);

    return ch;
}

bc::code chain_sync_state::merge(headers_const_ptr message)
{
    LOG_INFO(LOG_CHAIN_LISTENER) << "-> chain_sync_state::merge";

    size_t count = 0;
    hash_digest latest_header_id = null_hash;

    for (const auto &h : message->elements())
    {
        chain::lite_header lh(h);

        bc::code ec = lh.check(true);
        if (ec != error::success)
        {
            LOG_WARNING(LOG_CHAIN_LISTENER) << "Bad PoW in header with hash " << bc::encode_base16(lh.hash());
            return ec;
        }

        ///////////////////////////////////////////////////////////////////////////
        // Critical Section.
        bc::unique_lock lock(mutex_);

        if (known_blocks_.find(lh.hash()) != known_blocks_.end())
        {
            LOG_INFO(LOG_CHAIN_LISTENER) << "Header with hash " << bc::encode_base16(lh.hash()) << " is already known";
            continue;
        }

        hash_to_header_map::const_iterator iter = known_blocks_.find(lh.previous_block_hash());
        if (known_blocks_.end() == iter)
        {
            // TODO: add everything to orphan map
        }
        else
        {
            lh.validation.height = iter->second.validation.height + 1;
            if (chain_.size() <= lh.validation.height)
                chain_.resize(lh.validation.height - starting_height_ + 1);
            known_blocks_[lh.hash()] = lh;
            chain_[lh.validation.height - starting_height_].insert(lh.hash());
            count++;
            latest_header_id = lh.hash();
        }
        ///////////////////////////////////////////////////////////////////////////
    }

    if (count > 0)
    {
        inventory inv;
        inv.inventories().emplace_back(move(inventory_vector(inventory_vector::type_id::block, latest_header_id)));
        broadcaster_->broadcast_to_pb(inv,
            [](const bc::code &errc, typename network::channel<network::message_subscriber_ex>::ptr channel)
            {
              LOG_INFO(LOG_CHAIN_LISTENER) << "Broadcasted inv to [" << channel->authority() << "] with code " << errc;
            },
            [](const bc::code &errc)
            {
              LOG_INFO(LOG_CHAIN_LISTENER) << "Broadcasting inv completed with code " << errc;
            });
    }

    return error::success;
}

bool chain_sync_state::get_header_by_id(const hash_digest &id, chain::lite_header &header)
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    bc::shared_lock lock(mutex_);
    auto it = known_blocks_.find(id);
    if (it == known_blocks_.end())
        return false;
    header = it->second;
    return true;
    ///////////////////////////////////////////////////////////////////////////
}

bool chain_sync_state::get_height_by_id(const bc::hash_digest &id, size_t &height)
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    bc::shared_lock lock(mutex_);
    auto it = known_blocks_.find(id);
    if (it == known_blocks_.end())
        return false;
    height = it->second.validation.height;
    return true;
    ///////////////////////////////////////////////////////////////////////////
}

bool chain_sync_state::get_prev_hash_by_id(const bc::hash_digest &id, hash_digest &prev_hash)
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    bc::shared_lock lock(mutex_);
    auto it = known_blocks_.find(id);
    if (it == known_blocks_.end())
        return false;
    prev_hash = it->second.previous_block_hash();
    return true;
    ///////////////////////////////////////////////////////////////////////////
}

bool chain_sync_state::is_synchronized() const
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    bc::shared_lock lock(mutex_);

    if (chain_.size() == 0)
        return false;

    uint32_t now = static_cast<uint32_t>(time(nullptr));

    for (size_t pos = chain_.size() - 1; pos > 0; pos--)
    {
        if (!chain_[pos].empty())
        {
            for (const auto &id : chain_[pos])
            {
                const auto iter = known_blocks_.find(id);
                if (iter != known_blocks_.end())
                {
                    if (now > iter->second.timestamp() && (now - 600) < iter->second.timestamp())
                        return true;
                }
            }

            return false;
        }
    }

    return false;
    ///////////////////////////////////////////////////////////////////////////
}
