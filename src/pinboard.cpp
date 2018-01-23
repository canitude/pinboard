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
#include <list>

#include <altcoin/network.hpp>

#include "message_subscriber_ex.hpp"
#include "config.hpp"
#include "pinboard.hpp"

using namespace std;
using namespace bc::message;
using namespace bc::chain;

namespace libbitcoin {
namespace node {

pinboard::pinboard(message_broadcaster::ptr broadcaster,
                   chain_sync_state::ptr chain_state,
                   const uint256_t &min_target)
    : broadcaster_(broadcaster), chain_state_(chain_state), min_target_(min_target)
{
}

code pinboard::process(object_const_ptr obj, result_handler handler)
{
    object_payload op(obj->payload());

    if (!op.is_valid())
    {
        LOG_WARNING(LOG_PINBOARD) << "Object payload isn't valid.";
        return error::bad_stream;
    }

    hash_digest id = op.get_id();

    if (op.get_pow_type() != default_pow::type())
    {
        LOG_ERROR(LOG_PINBOARD) << "Incorrect PoW type " << (size_t)op.get_pow_type()
                                  << " in object " << bc::encode_base16(id)
                                  << ". Rejecting.";
        return error::invalid_proof_of_work;
    }


    uint256_t work_done = op.get_work_done();
    size_t size = op.serialized_size(0);

    LOG_INFO(LOG_PINBOARD) << "Incoming object id = " << bc::encode_base16(id)
                             << " size = " << size
                             << " work = " << work_done;

    if (op.get_pow_value() > MIN_TARGET)
    {
        LOG_ERROR(LOG_PINBOARD) << "PoW is below MIN_TARGET for object " << bc::encode_base16(id)
                                  << ". Rejecting.";
        return error::invalid_proof_of_work;
    }

    const hash_digest anchor = op.get_anchor();
    chain::lite_header header;

    if (!chain_state_->get_header_by_id(anchor, header))
    {
        LOG_WARNING(LOG_PINBOARD) << "Anchor with id = " << bc::encode_base16(anchor) << " isn't known";
        return error::unknown;
    }

    uint32_t ttl = calc_ttl(work_done, size);
    uint32_t now = static_cast<uint32_t>(time(nullptr));

    LOG_INFO(LOG_PINBOARD) << "TTL = " << ttl << " sec since " << header.timestamp() << " now = " << now;
    LOG_INFO(LOG_PINBOARD) << "SAVE UNTIL = " << (header.timestamp() + ttl);

    if (now >= (header.timestamp() + ttl))
    {
        LOG_WARNING(LOG_PINBOARD) << "Object " << bc::encode_base16(id) << " is "
                                    << (now - (header.timestamp() + ttl)) << " seconds old. Rejecting.";
        return error::unknown;
    }

    LOG_INFO(LOG_PINBOARD) << "TTL = " << (header.timestamp() + ttl - now) << " seconds more";

    object_details details(move(op), calc_bucket_id(header.timestamp() + ttl), header.timestamp(), ttl);

    LOG_INFO(LOG_PINBOARD) << "BUCKET ID = " << details.bucket_id_ << " " << hex << details.bucket_id_ << dec;

    {
        ///////////////////////////////////////////////////////////////////////////
        // Critical Section.
        bc::unique_lock lock(mutex_);

        const auto i = objects_.find(id);
        if (i != objects_.end()) {
            LOG_INFO(LOG_PINBOARD) << "Object " << bc::encode_base16(id) << " is already known. Doing nothing.";
            return error::success;
        }

        LOG_INFO(LOG_PINBOARD) << "Object " << bc::encode_base16(id) << " accepted.";

        const auto bucket_iter = buckets_.find(details.bucket_id_);
        if (bucket_iter == buckets_.end()) {
            set<bc::hash_digest> s;
            s.insert(id);
            buckets_.emplace(details.bucket_id_, move(s));
        } else {
            buckets_[details.bucket_id_].insert(id);
        }

        objects_.emplace(id, move(details));
        ///////////////////////////////////////////////////////////////////////////
    }

    broadcaster_->broadcast_to_pb(*(obj.get()),
    [](const bc::code &errc, typename network::channel<network::message_subscriber_ex>::ptr channel)
    {
        LOG_INFO(LOG_NETWORK) << "PINBOARD: broadcasted to [" << channel->authority() << "] with code " << errc;
    },
    [](const bc::code &errc)
    {
        LOG_INFO(LOG_NETWORK) << "PINBOARD: broadcasting completed with code " << errc;
    });

    // This is a new object. Let's broadcast it to other nodes.
    handler(error::success, obj);
    return error::success;
}

void pinboard::start(event_handler handler)
{
    threadpool_.join();
    threadpool_.spawn(thread_default(1), thread_priority::normal);

    timer_ = std::make_shared<deadline>(pool(), asio::seconds(60));
    timer_->start([this](const code& ec) {
        handle_timer();
    });

    handler(error::success);
}

void pinboard::handle_timer()
{
    cleanup();
    reset_timer();
}

void pinboard::reset_timer()
{
    timer_->start([this](const code& ec) {
        handle_timer();
    });
}

bool pinboard::stop()
{
    timer_->stop();

    threadpool_.shutdown();

    return true;
}

threadpool& pinboard::pool()
{
    return threadpool_;
}

uint32_t pinboard::calc_ttl(const uint256_t &work_done, size_t size)
{
    uint256_t ttl = default_pow::pow_mul() * work_done / size;
    if (ttl > (60 * 60 * 24))
        return (60 * 60 * 24);
    else
        return static_cast<uint32_t>(ttl);
}

uint32_t pinboard::calc_bucket_id(uint32_t ttl)
{
    return (( ttl >> 8 ) + 1 ) << 8;
}

void pinboard::for_each(object_handler handler)
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    bc::shared_lock lock(mutex_);

    for (const auto &bucket : buckets_)
    {
        for (const auto &id : bucket.second) {
            const auto iter = objects_.find(id);
            if (iter != objects_.end())
            {
                handler(iter->second.object_);
            }
            else
            {
                LOG_ERROR(LOG_NETWORK) << "Object with id " << bc::encode_base16(iter->first) << " isn't found";
            }
        }
    }
    ///////////////////////////////////////////////////////////////////////////
}

string pinboard::to_string() const
{
    stringstream s;

    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    bc::shared_lock lock(mutex_);

    for (const auto &bucket : buckets_)
    {
        s << bucket.first << endl;

        for (const auto &id : bucket.second) {
            s << "\t" << bc::encode_base16(id) << "\t";
            const auto iter = objects_.find(id);
            if (iter != objects_.end())
            {
                object_payload op(iter->second.object_);
                s << op.get_body_id().to_base58();
            }
            else
            {
                s << "ERROR: obj not found";
            }

            s << endl;
        }
    }

    return s.str();
    ///////////////////////////////////////////////////////////////////////////
}

void pinboard::cleanup()
{
    LOG_INFO(LOG_NETWORK) << "-> cleanup";
    uint32_t now = static_cast<uint32_t>(time(nullptr));

    ///////////////////////////////////////////////////////////////////////////
    // Critical Section.
    bc::unique_lock lock(mutex_);

    list<uint32_t> buckets_to_remove;

    for (const auto &bucket : buckets_)
    {
        if (bucket.first > now)
            continue;

        LOG_INFO(LOG_NETWORK) << "Deleting bucket " << bucket.first;

        buckets_to_remove.push_back(bucket.first);

        for (const auto &id : bucket.second)
        {
            const auto iter = objects_.find(id);
            if (iter != objects_.end())
            {
                LOG_INFO(LOG_NETWORK) << "Deleting object with id "
                                      << bc::encode_base16(id) << " from bucket " << bucket.first;
                objects_.erase(iter);
            }
            else
            {
                LOG_ERROR(LOG_NETWORK) << "Object with id "
                                       << bc::encode_base16(id) << " isn't found";
            }
        }

    }

    for (const auto &bucket_id : buckets_to_remove)
    {
        buckets_.erase(bucket_id);
    }
    ///////////////////////////////////////////////////////////////////////////
}

}
}