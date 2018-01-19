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

#include <algorithm>
#include <list>
#include <cstddef>
#include <functional>
#include <altcoin/network.hpp>

#include "lite_node.hpp"
#include "protocol_pinboard_sync.hpp"
#include "config.hpp"

namespace libbitcoin {
namespace node {

#define NAME "pinboard_sync"
#define CLASS protocol_pinboard_sync

using namespace bc::config;
using namespace bc::message;
using namespace bc::network;
using namespace std::placeholders;

static const asio::seconds expiry_interval(600);

// This class requires protocol version 31800.
protocol_pinboard_sync::protocol_pinboard_sync(lite_node& network,
    typename channel<message_subscriber_ex>::ptr channel,
    chain_sync_state::ptr chain_state, pinboard::ptr pinboard)
  : protocol_timer<message_subscriber_ex>(network, channel, true, NAME),
    CONSTRUCT_TRACK(protocol_pinboard_sync),
    chain_state_(chain_state),
    pinboard_(pinboard)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void protocol_pinboard_sync::start(event_handler handler)
{
    const auto complete = synchronize<event_handler>(
        BIND2(pinboard_complete, _1, handler), 1, NAME);

    LOG_INFO(LOG_NETWORK) << "PINBOARD: peer_version.services == "
                          << peer_version()->services()
                          << " on [" << authority() << "]";

    if (peer_version()->services() & (1u << PINBOARD_SERVICE_BIT))
    {
        LOG_INFO(LOG_NETWORK) << "PINBOARD: service bit detected on [" << authority() << "]";
        protocol_timer::start(expiry_interval, BIND2(handle_event, _1, complete));
        SUBSCRIBE3(object, handle_receive_object, _1, _2, complete);
        SUBSCRIBE2(inventory, handle_receive_inventory, _1, _2);

        auto peer_start_height = peer_version()->start_height();
        const std::set<bc::hash_digest> hashes = chain_state_->get_known_block_hashes(peer_start_height);
        if (!hashes.empty() && (*hashes.begin()) != bc::null_hash)
        {
            ///////////////////////////////////////////////////////////////////////////
            // Critical Section.
            bc::unique_lock lock(mutex_);

            oldest_known_hashes = hashes;

            LOG_INFO(LOG_NETWORK) << "PINBOARD: updated [" << authority()
                                  << "] sync state to height " << peer_start_height;
            ///////////////////////////////////////////////////////////////////////////
        }
    }
    else
    {
        LOG_INFO(LOG_NETWORK) << "PINBOARD: no pinboard service bit detected on [" << authority() << "]";
        pinboard_complete(error::success, handler);
    }
}

void protocol_pinboard_sync::pinboard_complete(const code& ec, event_handler handler)
{
    // Pinboard is synchronized now
    LOG_INFO(LOG_NETWORK) << "PINBOARD: sync completed with [" << authority() << "]";
    handler(ec);
}

// Protocol.
// ----------------------------------------------------------------------------

bool protocol_pinboard_sync::handle_receive_inventory(const code& ec, inventory_const_ptr message)
{
    if (stopped(ec))
    {
        LOG_INFO(LOG_NETWORK) << "PINBOARD: handle_receive_inventory ec = " << ec << ". Protocol stopped";
        return false;
    }

    LOG_INFO(LOG_NETWORK) << "PINBOARD: handle_receive_inventory from [" << authority() << "]";
    if (message->inventories().empty())
        return true;

    for (size_t i = message->inventories().size() - 1; i >= 0; i--)
    {
        if (message->inventories()[i].type() == inventory_vector::type_id::block)
        {
            size_t max_old_height = 0;
            for (const auto& h : oldest_known_hashes)
            {
                if (h == bc::null_hash)
                    continue;
                size_t height = 0;
                if (chain_state_->get_height_by_id(h, height))
                {
                    if (height > max_old_height)
                        max_old_height = height;
                }
            }

            bc::hash_digest new_hash = message->inventories()[i].hash();
            size_t new_height = 0;
            if (chain_state_->get_height_by_id(new_hash, new_height))
            {
                if (new_height > max_old_height)
                {
                    oldest_known_hashes.empty();
                    oldest_known_hashes.insert(new_hash);
                    LOG_INFO(LOG_NETWORK) << "PINBOARD: updated [" << authority()
                                          << "] sync state to height " << new_height;

                    pinboard_->for_each([this, max_old_height, new_height](const object_payload& op)
                    {
                        const auto& anchor = op.get_anchor();
                        size_t anchor_height = 0;
                        if (chain_state_->get_height_by_id(anchor, anchor_height))
                        {
                            if (anchor_height > max_old_height && anchor_height <= new_height)
                            {
                                const object obj(op);
                                SEND2(obj, handle_send, _1, obj.command);
                            }
                        }
                    });
                }
            }
            else
            {
                // Remote peer reported id of unknown block header.
                // This is handled by header_sync protocol.
            }

            break;
        }
    }

    return true;
}

bool protocol_pinboard_sync::handle_receive_object(const code& ec,
                                                     object_const_ptr message, event_handler complete)
{
    if (stopped(ec))
    {
        LOG_INFO(LOG_NETWORK) << "PINBOARD: handle_receive_object ec = " << ec << ". Protocol stopped";
        return false;
    }

    LOG_INFO(LOG_NETWORK) << "PINBOARD: handle_receive_object from [" << authority() << "]";

    code error = pinboard_->process(message, [](const code& wb_ec, object_const_ptr message){});

    if (error == error::invalid_proof_of_work || error == error::bad_stream)
    {
        LOG_WARNING(LOG_NETWORK) << "PINBOARD: incorrect object received from [" << authority() << "]. Disconnecting.";
        complete(error);
        stop(error);
        return false;
    }

    return true;
}

bool protocol_pinboard_sync::send_object(const object_payload& op)
{
    LOG_INFO(LOG_NETWORK) << "PINBOARD: send_object to [" << authority() << "]";

    const auto& anchor = op.get_anchor();
    std::list<hash_digest> missing_headers;
    size_t anchor_height = 0;
    if (!chain_state_->get_height_by_id(anchor, anchor_height))
    {
        LOG_ERROR(LOG_NETWORK) << "PINBOARD: can't find height for known anchor " << bc::encode_base16(anchor);
        return false;
    }

    hash_digest h = anchor;
    while (oldest_known_hashes.find(h) == oldest_known_hashes.end())
    {
        missing_headers.push_front(h);
        if (!chain_state_->get_prev_hash_by_id(missing_headers.front(), h))
        {
            LOG_ERROR(LOG_NETWORK) << "PINBOARD: can't find prev hash for known header " << bc::encode_base16(h);
            return false;
        }
    }

    if (!missing_headers.empty())
    {
        while (!missing_headers.empty())
        {
            headers new_msg;

            while (!missing_headers.empty())
            {
                const auto id = missing_headers.back();
                missing_headers.pop_back();
                bc::chain::lite_header lh;
                if (!chain_state_->get_header_by_id(id, lh))
                {
                    LOG_ERROR(LOG_NETWORK) << "PINBOARD: can't find header by id " << bc::encode_base16(id);
                    return false;
                }
                data_chunk d = lh.to_data();
                new_msg.elements().emplace_back(std::move(chain::header::factory_from_data(d)));
                if (new_msg.elements().size() == 2000)
                {
                    SEND2(new_msg, handle_send, _1, new_msg.command);
                    oldest_known_hashes.clear();
                    oldest_known_hashes.insert(id);
                    new_msg.elements().clear();
                    break;
                }
            }

            if (!new_msg.elements().empty())
            {
                SEND2(new_msg, handle_send, _1, new_msg.command);
                oldest_known_hashes.clear();
                oldest_known_hashes.insert(new_msg.elements()[new_msg.elements().size() - 1].hash());
            }
        }
    }

    const object obj(op);
    SEND2(obj, handle_send, _1, obj.command);

    return true;
}

// This is fired by the base timer and stop handler.
void protocol_pinboard_sync::handle_event(const code& ec, event_handler complete)
{
    if (stopped(ec))
        return;

    if (ec && ec != error::channel_timeout)
    {
        LOG_WARNING(LOG_NETWORK)
            << "Failure in pinboard sync timer for [" << authority() << "] "
            << ec.message();
        complete(ec);
        return;
    }
}

} // namespace node
} // namespace libbitcoin
