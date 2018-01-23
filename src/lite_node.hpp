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
#ifndef LIBBITCOIN_NODE_LITE_NODE_HPP
#define LIBBITCOIN_NODE_LITE_NODE_HPP

#include <cstdint>
#include <memory>

#include <altcoin/network.hpp>

#include "chain_listener.hpp"
#include "pinboard.hpp"
#include "message_subscriber_ex.hpp"
#include "config.hpp"

#define LOG_NODE "node"

namespace libbitcoin {
namespace node {

/// A node on the *coin P2P network.
class BCT_API lite_node : public network::p2p<bc::network::message_subscriber_ex>
{
public:
    typedef std::shared_ptr<lite_node> ptr;
    typedef message::network_address address;

    /// Construct node.
    lite_node(const network::settings& network_settings,
              chain_sync_state::ptr chain_state,
              pinboard::ptr pinboard);

    /// Ensure all threads are coalesced.
    virtual ~lite_node();

    // Templates (send/receive).
    // ------------------------------------------------------------------------

    /// Send message to all connections.
    template <typename Message>
    void broadcast_to_pb(const Message& message, channel_handler handle_channel,
                   result_handler handle_complete)
    {
        // Safely copy the channel collection.
        const auto channels = pending_close_.collection();

        size_t pb_nodes = connection_count(1u << PINBOARD_SERVICE_BIT);

        // Invoke the completion handler after send complete on all channels.
        const auto join_handler = synchronize(handle_complete, pb_nodes,
                                              "p2p_join", synchronizer_terminate::on_count);

        // No pre-serialize, channels may have different protocol versions.
        for (const auto channel: channels)
            if (channel->peer_version()->services() & (1u << PINBOARD_SERVICE_BIT))
                channel->send(message, std::bind(&p2p::handle_send, this,
                                                 std::placeholders::_1, channel, handle_channel, join_handler));
    }

    // Start/Run sequences.
    // ------------------------------------------------------------------------

    /// Invoke startup and seeding sequence, call from constructing thread.
    void start(result_handler handler) override;

    /// Synchronize the blockchain and then begin long running sessions,
    /// call from start result handler. Call base method to skip sync.
    void run(result_handler handler) override;

    // Hosts collection.
    // ------------------------------------------------------------------------

    /// Get the number of addresses with given services advertised
    virtual size_t address_count(uint64_t services) const;

    /// Get all known addresses with given services advertised
    virtual code fetch_addresses(message::network_address::list& out_addresses, uint64_t services) const;

    /// Store a collection of addresses (asynchronous).
    virtual void store(const address::list& addresses, result_handler handler);

    /// Get a randomly-selected address.
    virtual code fetch_address(address& out_address) const;

    // Pending close collection (open connections).
    // ------------------------------------------------------------------------

    /// Determine if there exists a connection to the address.
    virtual bool connected(const address& address) const;

    /// Count connections with given service bit
    virtual size_t connection_count(uint64_t services) const;

    // Shutdown.
    // ------------------------------------------------------------------------

    /// Idempotent call to signal work stop, start may be reinvoked after.
    /// Returns the result of file save operation.
    bool stop() override;

    /// Blocking call to coalesce all work and then terminate all threads.
    /// Call from thread that constructed this class, or don't call at all.
    /// This calls stop, and start may be reinvoked after calling this.
    bool close() override;

    /// Return the current top block identity.
    virtual config::checkpoint top_block() const;

protected:
    /// Attach a node::session to the network, caller must start the session.
    template <class Session, typename... Args>
    typename Session::ptr attach(Args&&... args)
    {
        return std::make_shared<Session>(*this, std::forward<Args>(args)...);
    }

    /// Override to attach specialized p2p sessions.
    typename network::session_manual<libbitcoin::network::message_subscriber_ex>::ptr attach_manual_session() override;
    typename network::session_inbound<libbitcoin::network::message_subscriber_ex>::ptr attach_inbound_session() override;
    typename network::session_outbound<libbitcoin::network::message_subscriber_ex>::ptr attach_outbound_session() override;

    typedef boost::circular_buffer<address> host_list;
    mutable upgrade_mutex peers_mutex_;
    host_list peers_;

private:
    typedef message::block::ptr_list block_ptr_list;

    void handle_headers_synchronized(const code& ec, result_handler handler);
    void handle_network_stopped(const code& ec, result_handler handler);

    void handle_started(const code& ec, result_handler handler);
    void handle_running(const code& ec, result_handler handler);

    // These are thread safe.
    const uint32_t protocol_maximum_;
    chain_sync_state::ptr chain_state_;
    pinboard::ptr pinboard_;
};

} // namespace node
} // namespace libbitcoin

#endif
