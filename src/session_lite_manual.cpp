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

#include "session_lite_manual.hpp"

using namespace std;

namespace libbitcoin {
namespace node {

using namespace bc::message;
using namespace bc::network;
using namespace std::placeholders;

session_lite_manual::session_lite_manual(lite_node& network,
                                         chain_sync_state::ptr chain_state,
                                         pinboard::ptr pinboard)
  : lite_session<session_manual<message_subscriber_ex>, message_subscriber_ex>(network, true),
    CONSTRUCT_TRACK(session_lite_manual),
    chain_state_(chain_state),
    pinboard_(pinboard)
{
}

void session_lite_manual::attach_protocols(typename channel<message_subscriber_ex>::ptr channel)
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

} // namespace node
} // namespace libbitcoin
