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
#ifndef LIBBITCOIN_NODE_LITE_SESSION_HPP
#define LIBBITCOIN_NODE_LITE_SESSION_HPP

#include <utility>
#include <altcoin/network.hpp>

namespace libbitcoin {
namespace node {

class lite_node;

/// Intermediate session base class template.
/// This avoids having to make network::session into a template.
template <class Session, class MessageSubscriber>
class BCT_API lite_session : public Session
{
protected:
    /// Construct an instance.
    lite_session(lite_node& node, bool notify_on_connect)
      : Session(node, notify_on_connect), node_(node)
    {
    }

    /// Attach a protocol to a channel, caller must start the channel.
    template <class Protocol, typename... Args>
    typename Protocol::ptr attach(typename network::channel<MessageSubscriber>::ptr channel,
        Args&&... args)
    {
        return std::make_shared<Protocol>(node_, channel,
            std::forward<Args>(args)...);
    }

private:

    // This is thread safe.
    lite_node& node_;
};

} // namespace node
} // namespace libbitcoin

#endif
