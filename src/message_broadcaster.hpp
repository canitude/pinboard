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
#ifndef LIBBITCOIN_NODE_MESSAGE_BROADCASTER_HPP
#define LIBBITCOIN_NODE_MESSAGE_BROADCASTER_HPP

#include <altcoin/network.hpp>

#include "message_subscriber_ex.hpp"

namespace libbitcoin {
namespace node {

class lite_node;

class BCT_API message_broadcaster
{
public:
    typedef std::shared_ptr<message_broadcaster> ptr;
    typedef std::shared_ptr<lite_node> lite_node_ptr;
    typedef network::p2p<network::message_subscriber_ex>::result_handler result_handler;
    typedef network::p2p<network::message_subscriber_ex>::channel_handler channel_handler;

    message_broadcaster()
    : network_()
    {
    }

    void link_to_node(lite_node_ptr network)
    {
        network_ = network;
    }

    template <typename Message>
    void broadcast_to_pb(const Message& message,
                         channel_handler handle_channel,
                         result_handler handle_complete);

protected:
    lite_node_ptr network_;
};

} // namespace node
} // namespace libbitcoin

#endif