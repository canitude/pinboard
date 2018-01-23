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
#ifndef LIBBITCOIN_NODE_SESSION_LITE_MANUAL_HPP
#define LIBBITCOIN_NODE_SESSION_LITE_MANUAL_HPP

#include <memory>
#include <altcoin/network.hpp>

#include "protocol_lite_header_sync.hpp"
#include "lite_session.hpp"
#include "chain_listener.hpp"
#include "pinboard.hpp"

namespace libbitcoin {

namespace network { class message_subscriber_ex; }

namespace node {

/// Manual connections session, thread safe.
class BCT_API session_lite_manual
  : public lite_session<network::session_manual<network::message_subscriber_ex>,
                        network::message_subscriber_ex>,
    track<session_lite_manual>
{
public:
    typedef std::shared_ptr<session_lite_manual> ptr;

    /// Construct an instance.
    session_lite_manual(lite_node& network,
                        chain_sync_state::ptr chain_state,
                        pinboard::ptr pinboard);

protected:
    /// Overridden to attach blockchain protocols.
    void attach_protocols(typename network::channel<network::message_subscriber_ex>::ptr channel) override;

    chain_sync_state::ptr chain_state_;
    pinboard::ptr pinboard_;
};

} // namespace node
} // namespace libbitcoin

#endif
