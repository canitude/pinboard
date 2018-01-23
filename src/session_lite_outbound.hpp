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
#ifndef LIBBITCOIN_NODE_SESSION_LITE_OUTBOUND_HPP
#define LIBBITCOIN_NODE_SESSION_LITE_OUTBOUND_HPP

#include <memory>
#include <altcoin/network.hpp>

#include "lite_session.hpp"
#include "pinboard.hpp"

namespace libbitcoin {

namespace network { class message_subscriber_ex; }

namespace node {

/// Outbound connections session, thread safe.
class BCT_API session_lite_outbound
  : public lite_session<network::session_outbound<network::message_subscriber_ex>,
                        network::message_subscriber_ex>,
    track<session_lite_outbound>
{
public:
    typedef std::shared_ptr<session_lite_outbound> ptr;

    /// Construct an instance.
    session_lite_outbound(lite_node& network,
                          chain_sync_state::ptr chain_state,
                          pinboard::ptr pinboard);

protected:
    /// Overridden to attach pinboard protocols.
    void attach_protocols(typename network::channel<network::message_subscriber_ex>::ptr channel) override;

    /// Overridden to change version negotiation protocols.
    virtual void attach_handshake_protocols(typename network::channel<network::message_subscriber_ex>::ptr channel,
                                            result_handler handle_started) override;

    // Connect sequence
    virtual void new_connect(channel_handler handler);

    chain_sync_state::ptr chain_state_;
    pinboard::ptr pinboard_;
};

} // namespace node
} // namespace libbitcoin

#endif
