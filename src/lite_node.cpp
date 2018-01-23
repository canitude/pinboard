/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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

#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>

#include "lite_node.hpp"
#include "session_lite_inbound.hpp"
#include "session_lite_outbound.hpp"
#include "session_lite_manual.hpp"


namespace libbitcoin {
namespace node {

using namespace bc::config;
using namespace bc::network;
using namespace std::placeholders;

lite_node::lite_node(const network::settings& network_settings,
                     chain_sync_state::ptr chain_state,
                     pinboard::ptr pinboard)
  : p2p<message_subscriber_ex>(network_settings),
    peers_(std::max(network_settings.host_pool_capacity, 1u)),
    protocol_maximum_(network_settings.protocol_maximum),
    chain_state_(chain_state),
    pinboard_(pinboard)
{
}

lite_node::~lite_node()
{
    lite_node::close();
}

// Start.
// ----------------------------------------------------------------------------

void lite_node::start(result_handler handler)
{
    if (!stopped())
    {
        handler(error::operation_failed);
        return;
    }

    // This is invoked on the same thread.
    // Stopped is true and no network threads until after this call.
    p2p<message_subscriber_ex>::start(handler);
}

// Run sequence.
// ----------------------------------------------------------------------------

void lite_node::run(result_handler handler)
{
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    handle_running(error::success, handler);

    return;
}

config::checkpoint lite_node::top_block() const
{
    return chain_state_->get_top_checkpoint();
}

void lite_node::handle_running(const code& ec, result_handler handler)
{
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    // This is invoked on a new thread.
    // This is the end of the derived run startup sequence.
    p2p<message_subscriber_ex>::run(handler);
}

/// Count connections with given service bit
size_t lite_node::connection_count(uint64_t services) const
{
    // Safely copy the channel collection.
    const auto channels = pending_close_.collection();

    size_t nodes = 0;
    for (const auto channel: channels)
        if ((channel->peer_version()->services() & services) == services)
            nodes++;

    return nodes;
}

size_t lite_node::address_count(uint64_t services) const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(peers_mutex_);

    size_t count = 0;
    for (const auto& addr : peers_)
    {
        if ((addr.services() & services) == services)
            count++;
    }

    return count;
    ///////////////////////////////////////////////////////////////////////////
}

boost::circular_buffer<message::network_address>::const_iterator find(const boost::circular_buffer<message::network_address> &list, const message::network_address& host)
{
    const auto found = [&host](const message::network_address& entry)
    {
        return entry.port() == host.port() && entry.ip() == host.ip();
    };

    return std::find_if(list.begin(), list.end(), found);
}

void lite_node::store(const address::list& addresses, result_handler handler)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    peers_mutex_.lock_upgrade();

    if (stopped_)
    {
        peers_mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        handler(error::service_stopped);
        return;
    }

    // Accept between 1 and all of this peer's addresses up to capacity.
    const auto capacity = peers_.capacity();
    const auto usable = std::min(addresses.size(), capacity);
    const auto random = static_cast<size_t>(pseudo_random(1, usable));

    // But always accept at least the amount we are short if available.
    const auto gap = capacity - peers_.size();
    const auto accept = std::max(gap, random);

    // Convert minimum desired to step for iteration, no less than 1.
    const auto step = std::max(usable / accept, size_t(1));
    size_t accepted = 0;

    peers_mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    for (size_t index = 0; index < usable; index = ceiling_add(index, step))
    {
        const auto& host = addresses[index];

        // Do not treat invalid address as an error, just log it.
        if (!host.is_valid())
        {
            LOG_DEBUG(LOG_NETWORK)
                << "Invalid host address from peer.";
            continue;
        }

        // Do not allow duplicates in the host cache.
        if (find(peers_, host) == peers_.end())
        {
            ++accepted;
            peers_.push_back(host);
        }
    }

    peers_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    LOG_DEBUG(LOG_NETWORK)
        << "Accepted (" << accepted << " of " << addresses.size()
        << ") host addresses: "
        << (addresses.size() == 1 ? (authority(addresses[0]).to_string()) : ".");

    // we still use default host store
    p2p<message_subscriber_ex>::store(addresses, handler);
}

code lite_node::fetch_address(address& out_address) const
{
    const auto dice = static_cast<size_t>(pseudo_random(0, 3));

    if (connection_count(1u << PINBOARD_SERVICE_BIT) < (settings_.outbound_connections / 2)
                && (dice + connection_count(bc::message::version::service::node_network))
                   > (settings_.outbound_connections / 2))
    {
        std::vector <address> temp;

        // Critical Section
        ///////////////////////////////////////////////////////////////////////////
        shared_lock lock(peers_mutex_);

        for (const auto& addr : peers_)
        {
            if (addr.services() & (1u << PINBOARD_SERVICE_BIT))
                temp.push_back(addr);
        };

        if (temp.size() > 0) {
            LOG_INFO(LOG_NODE) << temp.size() << " pinboard nodes found in hosts_ list.";
            for (const auto n : temp)
            {
                config::authority auth(n);
                LOG_INFO(LOG_NODE) << "\t" << auth;
            }
            const auto random = pseudo_random(0, temp.size() - 1);
            const auto index = static_cast<size_t>(random);
            out_address = temp[index];
            LOG_INFO(LOG_NODE) << "Trying pinboard node " << config::authority(out_address);
            return error::success;
        }
        ///////////////////////////////////////////////////////////////////////////
    }

    LOG_INFO(LOG_NODE) << "Trying litecoin node";
    return p2p<bc::network::message_subscriber_ex>::fetch_address(out_address);
}

/// Get all known addresses with given services advertised
code lite_node::fetch_addresses(message::network_address::list& out_addresses, uint64_t services) const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(peers_mutex_);

    for (const auto& addr : peers_)
    {
        if ((addr.services() & services) == services)
            out_addresses.push_back(addr);
    }

    return error::success;
    ///////////////////////////////////////////////////////////////////////////
}

// Pending close collection (open connections).
// ----------------------------------------------------------------------------
bool lite_node::connected(const address& address) const
{
    const auto match = [&address](const typename channel<message_subscriber_ex>::ptr& element)
    {
        return (element->authority().ip() == address.ip())
               && (element->authority().port() == address.port());
    };

    return pending_close_.exists(match);
}

// Specializations.
// ----------------------------------------------------------------------------
// Create derived sessions and override these to inject from derived node.

// Must not connect until running, otherwise imports may conflict with sync.
// But we establish the session in network so caller doesn't need to run.
typename network::session_manual<message_subscriber_ex>::ptr lite_node::attach_manual_session()
{
    return attach<node::session_lite_manual>(chain_state_, pinboard_);
}

typename network::session_inbound<message_subscriber_ex>::ptr lite_node::attach_inbound_session()
{
    return attach<node::session_lite_inbound>(chain_state_, pinboard_);
}

typename network::session_outbound<message_subscriber_ex>::ptr lite_node::attach_outbound_session()
{
    return attach<node::session_lite_outbound>(chain_state_, pinboard_);
}

// Shutdown
// ----------------------------------------------------------------------------
bool lite_node::stop()
{
    if (stopped())
    {
        LOG_ERROR(LOG_NODE)
            << "An attempt to stop lite_node that is already stopped.";
        return true;
    }

    // Suspend new work last so we can use work to clear subscribers.
    const auto p2p_stop = p2p<message_subscriber_ex>::stop();

    if (!p2p_stop)
        LOG_ERROR(LOG_NODE)
            << "Failed to stop network.";

    return p2p_stop;
}

// This must be called from the thread that constructed this class (see join).
bool lite_node::close()
{
    // Invoke own stop to signal work suspension.
    if (!lite_node::stop())
        return false;

    const auto p2p_close = p2p<message_subscriber_ex>::close();

    if (!p2p_close)
        LOG_ERROR(LOG_NODE)
            << "Failed to close network.";

    return p2p_close;
}

} // namespace node
} // namespace libbitcoin