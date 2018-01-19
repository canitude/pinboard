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

#include "message_subscriber_ex.hpp"

#include <altcoin/src/network/p2p.cpp>
template class bc::network::p2p<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/connector.cpp>
template class bc::network::connector<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/acceptor.cpp>
template class bc::network::acceptor<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/channel.cpp>
template class bc::network::channel<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/proxy.cpp>
template class bc::network::proxy<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/protocols/protocol.cpp>
template class bc::network::protocol<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/protocols/protocol_address_31402.cpp>
template class bc::network::protocol_address_31402<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/protocols/protocol_events.cpp>
template class bc::network::protocol_events<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/protocols/protocol_ping_31402.cpp>
template class bc::network::protocol_ping_31402<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/protocols/protocol_ping_60001.cpp>
template class bc::network::protocol_ping_60001<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/protocols/protocol_reject_70002.cpp>
template class bc::network::protocol_reject_70002<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/protocols/protocol_seed_31402.cpp>
template class bc::network::protocol_seed_31402<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/protocols/protocol_timer.cpp>
template class bc::network::protocol_timer<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/protocols/protocol_version_70002.cpp>
template class bc::network::protocol_version_70002<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/protocols/protocol_version_31402.cpp>
template class bc::network::protocol_version_31402<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/sessions/session.cpp>
template class bc::network::session<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/sessions/session_batch.cpp>
template class bc::network::session_batch<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/sessions/session_inbound.cpp>
template class bc::network::session_inbound<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/sessions/session_outbound.cpp>
template class bc::network::session_outbound<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/sessions/session_manual.cpp>
template class bc::network::session_manual<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME

#include <altcoin/src/network/sessions/session_seed.cpp>
template class bc::network::session_seed<bc::network::message_subscriber_ex>;
#undef CLASS
#undef NAME
