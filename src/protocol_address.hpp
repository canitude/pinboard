/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_ADDRESS_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_ADDRESS_HPP

#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/protocols/protocol_timer.hpp>

namespace libbitcoin {

namespace network { class message_subscriber_ex; }

namespace node {

class lite_node;

/**
 * Address protocol.
 * Attach this to a channel immediately following handshake completion.
 */
class BCT_API protocol_address
  : public network::protocol_timer<network::message_subscriber_ex>,
    public track<protocol_address>
{
public:
    typedef std::shared_ptr<protocol_address> ptr;

    /**
     * Construct an address protocol instance.
     * @param[in]  network   The network interface.
     * @param[in]  channel   The channel on which to start the protocol.
     */
    protocol_address(lite_node& network,
                     typename network::channel<network::message_subscriber_ex>::ptr channel);

    /**
     * Start the protocol.
     */
    virtual void start();

protected:
    virtual void handle_stop(const code& ec);
    virtual void handle_store_addresses(const code& ec);

    virtual bool handle_receive_address(const code& ec,
                                        address_const_ptr address);
    virtual bool handle_receive_get_address(const code& ec,
                                            get_address_const_ptr message);

    void handle_event(const code& ec);

    lite_node& network_;
    const message::address self_;
};

} // namespace network
} // namespace libbitcoin

#endif
