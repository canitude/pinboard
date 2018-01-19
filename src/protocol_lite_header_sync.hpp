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
#ifndef LIBBITCOIN_NODE_PROTOCOL_LITE_HEADER_SYNC_HPP
#define LIBBITCOIN_NODE_PROTOCOL_LITE_HEADER_SYNC_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <altcoin/network.hpp>

#include "chain_listener.hpp"

#define LOG_PROTO_HEADER_SYNC "proto_header_sync"

namespace libbitcoin {

namespace network { class message_subscriber_ex; }

namespace node {

class lite_node;

/// Headers sync protocol, thread safe.
class BCT_API protocol_lite_header_sync
  : public network::protocol_timer<bc::network::message_subscriber_ex>, public track<protocol_lite_header_sync>
{
public:
    typedef std::shared_ptr<protocol_lite_header_sync> ptr;

    /// Construct a header sync protocol instance.
    protocol_lite_header_sync(lite_node& network,
        typename network::channel<bc::network::message_subscriber_ex>::ptr channel,
        chain_sync_state::ptr chain_state);

    /// Start the protocol.
    virtual void start(event_handler handler);

private:
    void handle_event(const code& ec, event_handler complete);
    void headers_complete(const code& ec, event_handler handler);

    bool handle_receive_headers(const code& ec, headers_const_ptr message, event_handler complete);
    bool handle_receive_get_headers(const code& ec, get_headers_const_ptr message, event_handler complete);
    bool handle_receive_inventory(const code& ec, inventory_const_ptr message, event_handler complete);

    bool request_missing_headers(const bc::hash_digest &last);

    chain_sync_state::ptr chain_state_;
};

} // namespace node
} // namespace libbitcoin

#endif
