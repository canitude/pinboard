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
#ifndef LIBBITCOIN_MESSAGE_SUBSCRIBER_EX_HPP
#define LIBBITCOIN_MESSAGE_SUBSCRIBER_EX_HPP

#include <altcoin/network/message_subscriber.hpp>

#include "object.hpp"

namespace libbitcoin {
namespace network {

// copy/paste from libbitcoin-network/include/bitcoin/network/message_subscriber.hpp

#define DEFINE_SUBSCRIBER_TYPE(value) \
    typedef resubscriber<code, message::value::const_ptr> \
        value##_subscriber_type

#define DEFINE_SUBSCRIBER_OVERLOAD(value) \
    template <typename Handler> \
    void subscribe(message::value&&, Handler&& handler) \
    { \
        value##_subscriber_->subscribe(std::forward<Handler>(handler), \
            error::channel_stopped, {}); \
    }

#define DECLARE_SUBSCRIBER(value) \
    value##_subscriber_type::ptr value##_subscriber_

class BCT_API message_subscriber_ex : public message_subscriber {
public:
    DEFINE_SUBSCRIBER_TYPE(address);
    DEFINE_SUBSCRIBER_TYPE(alert);
    DEFINE_SUBSCRIBER_TYPE(block);
    DEFINE_SUBSCRIBER_TYPE(block_transactions);
    DEFINE_SUBSCRIBER_TYPE(compact_block);
    DEFINE_SUBSCRIBER_TYPE(fee_filter);
    DEFINE_SUBSCRIBER_TYPE(filter_add);
    DEFINE_SUBSCRIBER_TYPE(filter_clear);
    DEFINE_SUBSCRIBER_TYPE(filter_load);
    DEFINE_SUBSCRIBER_TYPE(get_address);
    DEFINE_SUBSCRIBER_TYPE(get_blocks);
    DEFINE_SUBSCRIBER_TYPE(get_block_transactions);
    DEFINE_SUBSCRIBER_TYPE(get_data);
    DEFINE_SUBSCRIBER_TYPE(get_headers);
    DEFINE_SUBSCRIBER_TYPE(headers);
    DEFINE_SUBSCRIBER_TYPE(inventory);
    DEFINE_SUBSCRIBER_TYPE(memory_pool);
    DEFINE_SUBSCRIBER_TYPE(merkle_block);
    DEFINE_SUBSCRIBER_TYPE(not_found);
    DEFINE_SUBSCRIBER_TYPE(ping);
    DEFINE_SUBSCRIBER_TYPE(pong);
    DEFINE_SUBSCRIBER_TYPE(reject);
    DEFINE_SUBSCRIBER_TYPE(send_compact);
    DEFINE_SUBSCRIBER_TYPE(send_headers);
    DEFINE_SUBSCRIBER_TYPE(transaction);
    DEFINE_SUBSCRIBER_TYPE(verack);
    DEFINE_SUBSCRIBER_TYPE(version);
    DEFINE_SUBSCRIBER_TYPE(object);

    /**
     * Create an instance of this class.
     * @param[in]  pool  The threadpool to use for sending notifications.
     */
    message_subscriber_ex(threadpool& pool);

    /**
     * Subscribe to receive a notification when a message of type is received.
     * The handler is unregistered when the call is made.
     * Subscribing must be immediate, we cannot switch thread contexts.
     * @param[in]  handler  The handler to register.
     */
    template <class Message, typename Handler>
    void subscribe(Handler&& handler)
    {
        subscribe(Message(), std::forward<Handler>(handler));
    }

    /**
     * Broadcast a default message instance with the specified error code.
     * @param[in]  ec  The error code to broadcast.
     */
    virtual void broadcast(const code& ec);

    /*
     * Load a stream of the specified command type.
     * Creates an instance of the indicated message type.
     * Sends the message instance to each subscriber of the type.
     * @param[in]  head     The stream message header.
     * @param[in]  version  The peer protocol version.
     * @param[in]  stream   The stream from which to load the message.
     * @return              Returns error::bad_stream if failed.
     */
    virtual code load(const bc::message::heading& head, uint32_t version,
                      std::istream& stream) const;

    /**
     * Start all subscribers so that they accept subscription.
     */
    virtual void start();

    /**
     * Stop all subscribers so that they no longer accept subscription.
     */
    virtual void stop();

private:
    DEFINE_SUBSCRIBER_OVERLOAD(address);
    DEFINE_SUBSCRIBER_OVERLOAD(alert);
    DEFINE_SUBSCRIBER_OVERLOAD(block);
    DEFINE_SUBSCRIBER_OVERLOAD(block_transactions);
    DEFINE_SUBSCRIBER_OVERLOAD(compact_block);
    DEFINE_SUBSCRIBER_OVERLOAD(fee_filter);
    DEFINE_SUBSCRIBER_OVERLOAD(filter_add);
    DEFINE_SUBSCRIBER_OVERLOAD(filter_clear);
    DEFINE_SUBSCRIBER_OVERLOAD(filter_load);
    DEFINE_SUBSCRIBER_OVERLOAD(get_address);
    DEFINE_SUBSCRIBER_OVERLOAD(get_blocks);
    DEFINE_SUBSCRIBER_OVERLOAD(get_block_transactions);
    DEFINE_SUBSCRIBER_OVERLOAD(get_data);
    DEFINE_SUBSCRIBER_OVERLOAD(get_headers);
    DEFINE_SUBSCRIBER_OVERLOAD(headers);
    DEFINE_SUBSCRIBER_OVERLOAD(inventory);
    DEFINE_SUBSCRIBER_OVERLOAD(memory_pool);
    DEFINE_SUBSCRIBER_OVERLOAD(merkle_block);
    DEFINE_SUBSCRIBER_OVERLOAD(not_found);
    DEFINE_SUBSCRIBER_OVERLOAD(ping);
    DEFINE_SUBSCRIBER_OVERLOAD(pong);
    DEFINE_SUBSCRIBER_OVERLOAD(reject);
    DEFINE_SUBSCRIBER_OVERLOAD(send_compact);
    DEFINE_SUBSCRIBER_OVERLOAD(send_headers);
    DEFINE_SUBSCRIBER_OVERLOAD(transaction);
    DEFINE_SUBSCRIBER_OVERLOAD(verack);
    DEFINE_SUBSCRIBER_OVERLOAD(version);
    DEFINE_SUBSCRIBER_OVERLOAD(object);

    DECLARE_SUBSCRIBER(address);
    DECLARE_SUBSCRIBER(alert);
    DECLARE_SUBSCRIBER(block);
    DECLARE_SUBSCRIBER(block_transactions);
    DECLARE_SUBSCRIBER(compact_block);
    DECLARE_SUBSCRIBER(fee_filter);
    DECLARE_SUBSCRIBER(filter_add);
    DECLARE_SUBSCRIBER(filter_clear);
    DECLARE_SUBSCRIBER(filter_load);
    DECLARE_SUBSCRIBER(get_address);
    DECLARE_SUBSCRIBER(get_blocks);
    DECLARE_SUBSCRIBER(get_block_transactions);
    DECLARE_SUBSCRIBER(get_data);
    DECLARE_SUBSCRIBER(get_headers);
    DECLARE_SUBSCRIBER(headers);
    DECLARE_SUBSCRIBER(inventory);
    DECLARE_SUBSCRIBER(memory_pool);
    DECLARE_SUBSCRIBER(merkle_block);
    DECLARE_SUBSCRIBER(not_found);
    DECLARE_SUBSCRIBER(ping);
    DECLARE_SUBSCRIBER(pong);
    DECLARE_SUBSCRIBER(reject);
    DECLARE_SUBSCRIBER(send_compact);
    DECLARE_SUBSCRIBER(send_headers);
    DECLARE_SUBSCRIBER(transaction);
    DECLARE_SUBSCRIBER(verack);
    DECLARE_SUBSCRIBER(version);
    DECLARE_SUBSCRIBER(object);
};

#undef DEFINE_SUBSCRIBER_TYPE
#undef DEFINE_SUBSCRIBER_OVERLOAD
#undef DECLARE_SUBSCRIBER

}
}

#endif