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

#include <ctime>
#include <functional>
#include <bitcoin/bitcoin.hpp>
#include <altcoin/network/channel.hpp>
#include <altcoin/network/define.hpp>
#include <altcoin/network/p2p.hpp>

#include "lite_node.hpp"
#include "protocol_address.hpp"
#include "config.hpp"


namespace libbitcoin {
namespace node {

#define NAME "address"
#define CLASS protocol_address

using namespace bc::message;
using namespace bc::network;
using namespace std::placeholders;

static const asio::seconds expiry_interval(60);

static message::address configured_self(const network::settings& settings)
{
    if (settings.self.port() == 0)
        return address{};

    network_address netaddr = settings.self.to_network_address();
    netaddr.set_timestamp(static_cast<uint32_t>(time(nullptr)));
    netaddr.set_services(settings.services);

    network_address::list l;
    l.push_back(netaddr);

    return message::address(l);
}

protocol_address::protocol_address(lite_node& network, typename channel<message_subscriber_ex>::ptr channel)
  : protocol_timer<message_subscriber_ex>(network, channel, true, NAME),
    CONSTRUCT_TRACK(protocol_address),
    network_(network), self_(configured_self(network_.network_settings()))
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void protocol_address::start()
{
    const auto& settings = network_.network_settings();

    // Must have a handler to capture a shared self pointer in stop subscriber.
    protocol_events<bc::network::message_subscriber_ex>::start(BIND1(handle_stop, _1));

    if (!self_.addresses().empty())
    {
        SEND2(self_, handle_send, _1, self_.command);
    }

    // If we can't store addresses we don't ask for or handle them.
    if (settings.host_pool_capacity == 0)
        return;

    protocol_timer::start(expiry_interval, BIND1(handle_event, _1));

    SUBSCRIBE2(address, handle_receive_address, _1, _2);
    SUBSCRIBE2(get_address, handle_receive_get_address, _1, _2);
    SEND2(get_address{}, handle_send, _1, get_address::command);
}

// Protocol.
// ----------------------------------------------------------------------------

bool protocol_address::handle_receive_address(const code& ec, address_const_ptr message)
{
    if (this->stopped(ec))
        return false;

    LOG_DEBUG(LOG_NETWORK)
        << "Storing addresses from [" << this->authority() << "] ("
        << message->addresses().size() << ")";

    // TODO: manage timestamps (active channels are connected < 3 hours ago).
    network_.store(message->addresses(), BIND1(handle_store_addresses, _1));

    // RESUBSCRIBE
    return true;
}

bool protocol_address::handle_receive_get_address(const code& ec, get_address_const_ptr message)
{
    if (this->stopped(ec))
        return false;

    // TODO: allowing repeated queries can allow a channel to map our history.
    // TODO: pull active hosts from host cache (currently just resending self).
    // TODO: need to distort for privacy, don't send currently-connected peers.
    // TODO: response size limit is max_address (1000).

    network_address::list l;
    network_.fetch_addresses(l, 1u << PINBOARD_SERVICE_BIT);

    for (const auto &addr : self_.addresses())
    {
        network_address netaddr = addr;
        netaddr.set_timestamp(static_cast<uint32_t>(time(nullptr)));

        l.push_back(netaddr);
    }

    if (l.empty())
        return true; // Nothing to send. Let's try again later.

    LOG_DEBUG(LOG_NETWORK)
        << "Sending addresses to [" << this->authority() << "] ("
        << l.size() << ")";

    message::address addr(l);
    SEND2(addr, handle_send, _1, addr.command);


    // RESUBSCRIBE
    return true;
}

void protocol_address::handle_store_addresses(const code& ec)
{
    if (this->stopped(ec))
        return;

    if (ec)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Failure storing addresses from [" << this->authority() << "] "
            << ec.message();
        this->stop(ec);
    }
}

void protocol_address::handle_stop(const code&)
{
}

void protocol_address::handle_event(const code& ec)
{
    if (stopped(ec))
        return;

    if (ec && ec != error::channel_timeout)
    {
        LOG_WARNING(LOG_NETWORK)
            << "Failure in protocol_address timer for [" << authority() << "] "
            << ec.message();
        return;
    }

    /*if (self_.addresses().size() > 0)
    {
        LOG_INFO(LOG_NETWORK)
            << "Rebroadcasting our address for [" << authority() << "]";

        network_address::list l;
        for (const auto &addr : self_.addresses())
        {
            network_address netaddr = addr;
            netaddr.set_timestamp(static_cast<uint32_t>(time(nullptr)));

            l.push_back(netaddr);
        }

        message::address addr(l);
        SEND2(addr, handle_send, _1, addr.command);
    }*/

    if (network_.address_count(1u << PINBOARD_SERVICE_BIT) < network_.network_settings().outbound_connections)
    {
        LOG_INFO(LOG_NETWORK)
            << "Not enougth pinboard addresses known. Requesting more from [" << authority() << "]";

        SEND2(get_address{}, handle_send, _1, get_address::command);
    }
}

} // namespace network
} // namespace libbitcoin
