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

#include <iostream>

#include <altcoin/network.hpp>

#include "lite_node.hpp"
#include "protocol_lite_header_sync.hpp"
#include "protocol_pinboard_sync.hpp"
#include "protocol_address.hpp"

#include "session_lite_outbound.hpp"

using namespace std;

namespace libbitcoin {
namespace node {

using namespace bc::message;
using namespace bc::network;
using namespace std::placeholders;

session_lite_outbound::session_lite_outbound(lite_node& network,
                                             chain_sync_state::ptr chain_state,
                                             pinboard::ptr pinboard)
  : lite_session<network::session_outbound<message_subscriber_ex>, message_subscriber_ex>(network, true),
    CONSTRUCT_TRACK(node::session_lite_outbound),
    chain_state_(chain_state),
    pinboard_(pinboard)
{
}

void session_lite_outbound::attach_protocols(typename channel<message_subscriber_ex>::ptr channel)
{
    const auto version = channel->negotiated_version();

    if (version >= version::level::bip31)
        attach<protocol_ping_60001<message_subscriber_ex>>(channel)->start();
    else
        attach<protocol_ping_31402<message_subscriber_ex>>(channel)->start();

    if (version >= message::version::level::bip61)
        attach<protocol_reject_70002<message_subscriber_ex>>(channel)->start();

    attach<protocol_address>(channel)->start();

    attach<protocol_lite_header_sync>(channel, chain_state_)->start([](const bc::code& ec)
    {
        LOG_INFO(LOG_NODE) << "protocol_lite_header_sync completed. ec == " << ec.message();
    });

    if (channel->peer_version()->services() & (1u << PINBOARD_SERVICE_BIT))
        attach<protocol_pinboard_sync>(channel, chain_state_, pinboard_)->start([](const bc::code& ec)
        {
            LOG_INFO(LOG_NODE) << "protocol_pinboard_sync completed. ec == " << ec.message();
        });
}

void session_lite_outbound::attach_handshake_protocols(typename channel<message_subscriber_ex>::ptr channel,
                                result_handler handle_started)
{
    using serve = message::version::service;
    const auto relay = this->settings_.relay_transactions;
    const auto own_version = this->settings_.protocol_maximum;
    const auto own_services = this->settings_.services;
    const auto invalid_services = this->settings_.invalid_services;
    const auto minimum_version = this->settings_.protocol_minimum;

    // Require peer to serve network (and witness if configured on self).
    const auto minimum_services = (own_services & serve::node_witness);

    // Reject messages are not handled until bip61 (70002).
    // The negotiated_version is initialized to the configured maximum.
    if (channel->negotiated_version() >= message::version::level::bip61)
        this->template attach<protocol_version_70002<message_subscriber_ex>>(
                        channel, own_version, own_services,
                        invalid_services, minimum_version, minimum_services, relay)
                ->start(handle_started);
    else
        this->template attach<protocol_version_31402<message_subscriber_ex>>(
                        channel, own_version, own_services,
                        invalid_services, minimum_version, minimum_services)
                ->start(handle_started);
}

// Connect sequence
void session_lite_outbound::new_connect(channel_handler handler)
{
    if (this->stopped())
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Suspended connection.";
        handler(error::channel_stopped, nullptr);
        return;
    }

    network_address address;
    const auto ec = this->fetch_address(address);
    const auto self = network_.network_settings().self;
    if (network_.connected(address)
        || (self.ip() == address.ip() && self.port() == address.port()))
    {
        handler(error::address_in_use, nullptr);
    }
    else
        start_connect(ec, address, handler);
}

} // namespace node
} // namespace libbitcoin
