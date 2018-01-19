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

#include <sstream>

#include <bitcoin/bitcoin/math/limits.hpp>
#include <bitcoin/bitcoin/message/messages.hpp>
#include <bitcoin/bitcoin/message/version.hpp>
#include <bitcoin/bitcoin/utility/assert.hpp>
#include <bitcoin/bitcoin/utility/container_sink.hpp>
#include <bitcoin/bitcoin/utility/container_source.hpp>
#include <bitcoin/bitcoin/utility/istream_reader.hpp>
#include <bitcoin/bitcoin/utility/ostream_writer.hpp>

#include "object.hpp"

namespace libbitcoin {
namespace message {

object_payload object_payload::factory_from_data(uint32_t version, const data_chunk& data)
{
    object_payload instance;
    instance.from_data(version, data);
    return instance;
}

object_payload object_payload::factory_from_data(uint32_t version, std::istream& stream)
{
    object_payload instance;
    instance.from_data(version, stream);
    return instance;
}

object_payload object_payload::factory_from_data(uint32_t version, reader& source)
{
    object_payload instance;
    instance.from_data(version, source);
    return instance;
}

object_payload::object_payload()
    : validation{}, body_(), body_id_(), pow_()
{
}

object_payload::object_payload(const object_payload& other)
    : validation{}, body_(other.body_), body_id_(other.body_id_), pow_(other.pow_)
{
}

object_payload::object_payload(object_payload&& other) noexcept
    : validation{}, body_(std::move(other.body_)), body_id_(std::move(other.body_id_)), pow_(std::move(other.pow_))
{
}

object_payload::object_payload(const std::string &str)
        : body_(str.cbegin(), str.cend()), body_id_(), pow_()
{
}

void object_payload::reset()
{
    body_.clear();
    body_id_.reset();
    pow_.reset();
}

bool object_payload::from_data(uint32_t version, const data_chunk& data)
{
    data_source istream(data);
    return from_data(version, istream);
}

bool object_payload::from_data(uint32_t version, std::istream& stream)
{
    istream_reader source(stream);
    return from_data(version, source);
}

bool object_payload::from_data(uint32_t version, reader& source)
{
    reset();

    const auto body_size = source.read_size_little_endian();
    if (body_size > 0)
        body_ = source.read_bytes(body_size);
    else
        body_id_.from_data(version, source);

    pow_.from_data(version, source);

    if (!source)
        reset();

    return source;
}

data_chunk object_payload::to_data(uint32_t version) const
{
    data_chunk data;
    const auto size = serialized_size(version);
    data.reserve(size);
    data_sink ostream(data);
    to_data(version, ostream);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

void object_payload::to_data(uint32_t version, std::ostream& stream) const
{
    ostream_writer sink(stream);
    to_data(version, sink);
}

void object_payload::to_data(uint32_t version, writer& sink) const
{
    sink.write_size_little_endian(body_.size());

    if (!body_.empty())
        sink.write_bytes(body_);
    else
        body_id_.to_data(version, sink);

    pow_.to_data(version, sink);
}

bool object_payload::is_valid() const
{
    return ((body_.empty() && !body_id_.empty()) || !body_.empty()) && body_id_.is_valid() && pow_.is_valid();
}

size_t object_payload::serialized_size(uint32_t version) const
{
    return message::variable_uint_size(body_.size()) + body_.size()
            + (body_.empty() ? body_id_.serialized_size(version) : 0)
            + pow_.serialized_size(version);
}

object_payload& object_payload::operator=(object_payload&& other)
{
    body_ = std::move(other.body_);
    body_id_ = std::move(other.body_id_);
    pow_ = std::move(other.pow_);
    return *this;
}

bool object_payload::operator==(const object_payload& other) const
{
    return (body_ == other.body_) && (body_id_ == other.body_id_) && (pow_ == other.pow_);
}

bool object_payload::operator!=(const object_payload& other) const
{
    return !(*this == other);
}

hash_digest object_payload::get_id() const
{
    BITCOIN_ASSERT(is_valid());
    if (validation.id == bc::null_hash)
    {
        validation.id = sha256_hash(to_data(0));
    }

    return validation.id;
}


multihash object_payload::get_body_id()
{
    BITCOIN_ASSERT(is_valid());
    if (body_id_.empty())
    {
        multihash mh(digest_type::sha2_256, sha256_hash_chunk(body_));
        body_id_ = std::move(mh);
    }

    return body_id_;
}

data_chunk object_payload::serialize_id_and_pow()
{
    get_body_id();
    data_chunk data;
    const auto size = body_id_.serialized_size(0) + pow_.serialized_size(0);
    data.reserve(size);
    data_sink ostream(data);
    body_id_.to_data(0, ostream);
    pow_.to_data(0, ostream);
    ostream.flush();
    return data;
}

uint256_t object_payload::get_work_done()
{
    if (validation.work_done == 0)
    {
        get_pow_value();
        validation.work_done = ((~validation.pow_value) / (validation.pow_value + 1)) + 1;
    }

    return validation.work_done;
}

uint256_t object_payload::get_pow_value()
{
    if (validation.pow_value == 0)
    {
        validation.pow_hash = pow_.calculate_pow_hash(get_body_id().to_data(0));
        validation.pow_value = to_uint256(validation.pow_hash);
    }

    return validation.pow_value;
}

std::string object_payload::to_string() const
{
    std::stringstream stream;
    stream << "{body_=" << bc::encode_base16(body_)
           << " id_=" << body_id_.to_string()
           << " pow_=" << pow_.to_string()
           << "}";
    return stream.str();
}

const std::string object::command = "object";
const uint32_t object::version_minimum = version::level::minimum;
const uint32_t object::version_maximum = version::level::maximum;

object object::factory_from_data(uint32_t version, const data_chunk& data)
{
    object instance;
    instance.from_data(version, data);
    return instance;
}

object object::factory_from_data(uint32_t version, std::istream& stream)
{
    object instance;
    instance.from_data(version, stream);
    return instance;
}

object object::factory_from_data(uint32_t version, reader& source)
{
    object instance;
    instance.from_data(version, source);
    return instance;
}

object::object()
  : payload_()
{
}

object::object(const object_payload& payload)
  : payload_(payload)
{
}

object::object(object_payload&& payload)
  : payload_(std::move(payload))
{
}

object::object(const object& other)
  : payload_(other.payload_)
{
}

object::object(object&& other) noexcept
  : payload_(std::move(other.payload_))
{
}

const object_payload& object::payload() const
{
    return payload_;
}

bool object::is_valid() const
{
    return payload_.is_valid();
}

void object::reset()
{
    payload_.reset();
}

bool object::from_data(uint32_t version, const data_chunk& data)
{
    data_source istream(data);
    return from_data(version, istream);
}

bool object::from_data(uint32_t version, std::istream& stream)
{
    istream_reader source(stream);
    return from_data(version, source);
}

bool object::from_data(uint32_t version, reader& source)
{
    reset();

    payload_.from_data(version, source);

    if (!source)
        reset();

    return source;
}

data_chunk object::to_data(uint32_t version) const
{
    data_chunk data;
    const auto size = serialized_size(version);
    data.reserve(size);
    data_sink ostream(data);
    to_data(version, ostream);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

void object::to_data(uint32_t version, std::ostream& stream) const
{
    ostream_writer sink(stream);
    to_data(version, sink);
}

void object::to_data(uint32_t version, writer& sink) const
{
    payload_.to_data(version, sink);
}

size_t object::serialized_size(uint32_t version) const
{
    return payload_.serialized_size(version);
}

object& object::operator=(object&& other)
{
    payload_ = std::move(other.payload_);
    return *this;
}

bool object::operator==(const object& other) const
{
    return (payload_ == other.payload_);
}

bool object::operator!=(const object& other) const
{
    return !(*this == other);
}

} // namespace message
} // namespace libbitcoin
