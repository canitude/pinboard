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
#ifndef LIBBITCOIN_NODE_PROTOCOL_PINBOARD_SYNC_HPP
#define LIBBITCOIN_NODE_PROTOCOL_PINBOARD_SYNC_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <altcoin/network.hpp>

#include "chain_listener.hpp"
#include "pinboard.hpp"
#include "object.hpp"

namespace libbitcoin {

namespace network { class message_subscriber_ex; }

namespace node {

class lite_node;

/// Pinboard sync protocol, thread safe.
class BCT_API protocol_pinboard_sync
  : public network::protocol_timer<bc::network::message_subscriber_ex>, public track<protocol_pinboard_sync>
{
public:
    typedef std::shared_ptr<protocol_pinboard_sync> ptr;

    /// Construct a header sync protocol instance.
    protocol_pinboard_sync(lite_node& network,
        typename network::channel<bc::network::message_subscriber_ex>::ptr channel,
        chain_sync_state::ptr chain_state, pinboard::ptr pinboard);

    /// Start the protocol.
    virtual void start(event_handler handler);

    bool send_object(const bc::message::object_payload& op);

private:
    bool handle_receive_object(const code& ec, object_const_ptr message, event_handler complete);
    bool handle_receive_inventory(const code& ec, inventory_const_ptr message);
    void handle_event(const code& ec, event_handler complete);

    void pinboard_complete(const code& ec, event_handler handler);

    chain_sync_state::ptr chain_state_;
    pinboard::ptr pinboard_;

    // -------------------------------------------------------------------------
    mutable bc::upgrade_mutex mutex_;
    std::set<bc::hash_digest> oldest_known_hashes;
    // -------------------------------------------------------------------------
};

} // namespace node
} // namespace libbitcoin

#endif
