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
#include <cstddef>
#include <functional>
#include <list>
#include <altcoin/network.hpp>

#include "lite_node.hpp"
#include "protocol_lite_header_sync.hpp"

namespace libbitcoin {
namespace node {

#define NAME "header_sync"
#define CLASS protocol_lite_header_sync

using namespace bc::config;
using namespace bc::message;
using namespace bc::network;
using namespace std::placeholders;

static const asio::seconds expiry_interval(60);

// This class requires protocol version 31800.
protocol_lite_header_sync::protocol_lite_header_sync(lite_node& network,
    typename channel<message_subscriber_ex>::ptr channel,
    chain_sync_state::ptr chain_state)
  : protocol_timer<message_subscriber_ex>(network, channel, true, NAME),
    CONSTRUCT_TRACK(protocol_lite_header_sync),
    chain_state_(chain_state)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void protocol_lite_header_sync::start(event_handler handler)
{
    const auto complete = synchronize<event_handler>(
        BIND2(headers_complete, _1, handler), 1, NAME);

    protocol_timer::start(expiry_interval, BIND2(handle_event, _1, complete));

    SUBSCRIBE3(headers, handle_receive_headers, _1, _2, complete);
    SUBSCRIBE3(get_headers, handle_receive_get_headers, _1, _2, complete);
    SUBSCRIBE3(inventory, handle_receive_inventory, _1, _2, complete);

    // This is the end of the start sequence.
    request_missing_headers(bc::null_hash);
}

// Header sync sequence.
// ----------------------------------------------------------------------------

bool protocol_lite_header_sync::handle_receive_inventory(const code& ec,
    inventory_const_ptr message, event_handler complete)
{
    if (stopped(ec))
        return false;

    if (message->inventories().size() == 0)
        return true;

    for (const auto &i : message->inventories())
    {
        LOG_INFO(LOG_PROTO_HEADER_SYNC) << "handle_receive_inventory:\n\tnew hash " << bc::encode_base16(i.hash());
        if (i.type() == bc::message::inventory_vector::type_id::block)
            if (!request_missing_headers(i.hash()))
            {
                return true;
            }
    }

    complete(error::success);
    return true;
}

bool protocol_lite_header_sync::request_missing_headers(const bc::hash_digest &last)
{
    const std::set<bc::hash_digest> known_headers = chain_state_->get_last_known_block_hash();
    LOG_INFO(LOG_PROTO_HEADER_SYNC) << "Found known headers: " << known_headers.size() << " pieces";

    for (const auto &first : known_headers)
    {
        if (first != last)
        {
            const get_headers request
                    {
                            {first},
                            last
                    };

            SEND2(request, handle_send, _1, request.command);
        } else
            return false;
    }

    return true;
}

bool protocol_lite_header_sync::handle_receive_headers(const code& ec,
    headers_const_ptr message, event_handler complete)
{
    if (stopped(ec))
        return false;

    if (!chain_state_->merge(message))
    {
        LOG_WARNING(LOG_PROTO_HEADER_SYNC)
            << "Failure merging headers from [" << authority() << "]";
        complete(error::invalid_previous_block);
    }

    if (message->elements().size() == max_get_headers) // Better way to verify is needed
        request_missing_headers(bc::null_hash);
    else
        complete(error::success);

    return true;
}

bool protocol_lite_header_sync::handle_receive_get_headers(const code& ec, get_headers_const_ptr message, event_handler complete)
{
    if (stopped(ec))
        return false;

    const hash_list& start_list = message->start_hashes();
    hash_digest stop = message->stop_hash();
    bc::chain::lite_header lh;

    if (!chain_state_->get_header_by_id(stop, lh))
    {
        LOG_INFO(LOG_NETWORK) << "Can't find stop header by id " << bc::encode_base16(stop);
        LOG_INFO(LOG_NETWORK) << "Assuming current chain top.";
        const std::set<hash_digest> top = chain_state_->get_last_known_block_hash();
        if (top.empty())
        {
            LOG_WARNING(LOG_NETWORK) << "Our chain top is empty. Strange :/";
        }

        for (const auto &h : top)
        {
            if (h != null_hash)
            {
                stop = h;
                break;
                // Warning: this is possibly wrong assumption.
            }
        }

        if (stop == null_hash)
        {
            LOG_WARNING(LOG_NETWORK) << "Still can't find not null chain top. Very strange :/";
            return true;
        }
    }

    std::list<hash_digest> missing_headers;
    missing_headers.push_front(stop);

    hash_digest known_start = null_hash;
    hash_list::const_iterator iter = start_list.begin();
    size_t max_height = 0;
    while (iter != start_list.end())
    {
        if (!chain_state_->get_header_by_id(*iter, lh))
        {
            LOG_INFO(LOG_NETWORK) << "Can't find start header by id " << bc::encode_base16(*iter);
            iter++;

            continue;
        }

        if (lh.validation.height > max_height)
        {
            max_height = lh.validation.height;
            known_start = lh.hash();
        }

        iter++;
    }

    if (known_start == null_hash)
    {
        LOG_WARNING(LOG_NETWORK) << "Don't know all of requested start headers.";
        return true;
    }

    while (missing_headers.front() != known_start)
    {
        hash_digest prev = null_hash;

        if (!chain_state_->get_prev_hash_by_id(missing_headers.front(), prev))
        {
            LOG_ERROR(LOG_NETWORK) << "Can't find prev hash for known header "
                                   << bc::encode_base16(missing_headers.front());
            return true;
        }

        if (prev == known_start)
            break;

        missing_headers.push_front(prev);
    }

    headers new_msg;

    while (!missing_headers.empty())
    {
        const auto id = missing_headers.front();
        missing_headers.pop_front();
        bc::chain::lite_header lh;
        if (!chain_state_->get_header_by_id(id, lh))
        {
            LOG_ERROR(LOG_NETWORK) << "Can't find header by id " << bc::encode_base16(id);
            return true;
        }

        data_chunk d = lh.to_data();
        new_msg.elements().emplace_back(std::move(chain::header::factory_from_data(d)));
        if (new_msg.elements().size() == max_get_headers)
            break;
    }

    if (!new_msg.elements().empty())
        SEND2(new_msg, handle_send, _1, new_msg.command);

    return true;
}

// This is fired by the base timer and stop handler.
void protocol_lite_header_sync::handle_event(const code& ec, event_handler complete)
{
    if (stopped(ec))
        return;

    if (ec && ec != error::channel_timeout)
    {
        LOG_WARNING(LOG_PROTO_HEADER_SYNC)
            << "Failure in header sync timer for [" << authority() << "] "
            << ec.message();
        complete(ec);
        return;
    }
}

void protocol_lite_header_sync::headers_complete(const code& ec,
    event_handler handler)
{
    handler(ec);

    // this protocol never stops
}

} // namespace node
} // namespace libbitcoin
