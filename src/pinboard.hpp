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
#ifndef LIBBITCOIN_NODE_PINBOARD_HPP
#define LIBBITCOIN_NODE_PINBOARD_HPP

#include <map>
#include <set>

#include <bitcoin/bitcoin.hpp>

#include "message_broadcaster.hpp"
#include "chain_listener.hpp"
#include "object.hpp"

#define LOG_PINBOARD "pinboard"

namespace libbitcoin {
namespace node {

/**
 * Objects are grouped into buckets by time intervals.
 * Bucket id is the starting timestamp of the interval.
 * Object must be stored in the bucket corresponding to the earliest
 * moment the object has to be deleted.
 */

class pinboard
{
public:
    typedef std::shared_ptr<pinboard> ptr;
    typedef std::map<uint32_t, std::set<bc::hash_digest>> bucket_map;

    struct object_details
    {
        bc::message::object_payload object_;

        uint32_t bucket_id_ = 0;
        uint32_t anchor_timestamp_ = 0;
        uint32_t ttl_ = 0;

        object_details(bc::message::object_payload&& obj, uint32_t bucket_id, uint32_t anchor_timestamp, uint32_t ttl)
                : object_(std::move(obj)), bucket_id_(bucket_id), anchor_timestamp_(anchor_timestamp), ttl_(ttl)
        {
        }

        object_details(const object_details &other)
                : object_(other.object_), bucket_id_(other.bucket_id_),
                  anchor_timestamp_(other.anchor_timestamp_), ttl_(other.ttl_)
        {
        }

        object_details(object_details&& other)
                : object_(std::move(other.object_)), bucket_id_(other.bucket_id_),
                  anchor_timestamp_(other.anchor_timestamp_), ttl_(other.ttl_)
        {
        }
    };

    typedef std::map<bc::hash_digest, object_details> hash_to_object_map;

    typedef std::function<void(const code&)> event_handler;
    typedef std::function<void(const code&, object_const_ptr)> result_handler;
    typedef std::function<void(const bc::message::object_payload&)> object_handler;

    pinboard(message_broadcaster::ptr broadcaster,
             chain_sync_state::ptr chain_state,
             const uint256_t &min_target);

    // Start/Run sequences.
    // ------------------------------------------------------------------------

    virtual void start(event_handler handler);

    // Shutdown.
    // ------------------------------------------------------------------------

    virtual bool stop();

    // Operations
    // ------------------------------------------------------------------------

    virtual code process(object_const_ptr obj, result_handler handler);
    void for_each(object_handler handler);

    /// Get the threadpool.
    virtual threadpool& pool();

    // Debug
    // ------------------------------------------------------------------------

    virtual std::string to_string() const;

protected:
    void reset_timer();
    void handle_timer();

    virtual void cleanup();

    uint32_t calc_ttl(const uint256_t &work_done, size_t size);
    uint32_t calc_bucket_id(uint32_t ttl);

    message_broadcaster::ptr broadcaster_;
    chain_sync_state::ptr chain_state_;
    const uint256_t min_target_;

    deadline::ptr timer_;
    threadpool threadpool_;

    // -------------------------------------------------------------------------
    mutable bc::upgrade_mutex mutex_;
    hash_to_object_map objects_;
    bucket_map buckets_;
    // -------------------------------------------------------------------------
};

}
}

#endif