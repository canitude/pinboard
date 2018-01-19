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

#include "pow_certificate.hpp"

namespace libbitcoin {
namespace message {

pow_certificate pow_certificate::factory_from_data(uint32_t version, const data_chunk& data)
{
    pow_certificate instance;
    instance.from_data(version, data);
    return instance;
}

pow_certificate pow_certificate::factory_from_data(uint32_t version, std::istream& stream)
{
    pow_certificate instance;
    instance.from_data(version, stream);
    return instance;
}

pow_certificate pow_certificate::factory_from_data(uint32_t version, reader& source)
{
    pow_certificate instance;
    instance.from_data(version, source);
    return instance;
}

pow_certificate::pow_certificate()
        : type_(pow_type::plain), tag_(chain_tag::unknown), anchor_(bc::null_hash), nonce_(0)
{
}

pow_certificate::pow_certificate(const pow_certificate& other)
        : type_(other.type_), tag_(other.tag_), anchor_(other.anchor_), nonce_(other.nonce_)
{
}

pow_certificate::pow_certificate(pow_certificate&& other) noexcept
        : type_(other.type_), tag_(other.tag_), anchor_(std::move(other.anchor_)), nonce_(other.nonce_)
{
}

pow_certificate::pow_certificate(pow_type type, chain_tag tag, const hash_digest& anchor, uint64_t nonce)
        : type_(type), tag_(tag), anchor_(anchor), nonce_(nonce)
{
}

bool pow_certificate::from_data(uint32_t version, const data_chunk& data)
{
    data_source istream(data);
    return from_data(version, istream);
}

bool pow_certificate::from_data(uint32_t version, std::istream& stream)
{
    istream_reader source(stream);
    return from_data(version, source);
}

bool pow_certificate::from_data(uint32_t version, reader& source)
{
    reset();

    type_ = (pow_type)source.read_size_little_endian();
    tag_ = (chain_tag)source.read_size_little_endian();
    anchor_ = source.read_hash();
    nonce_ = source.read_8_bytes_little_endian();

    if (!source)
        reset();

    return source;
}

data_chunk pow_certificate::to_data(uint32_t version) const
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

void pow_certificate::to_data(uint32_t version, std::ostream& stream) const
{
    ostream_writer sink(stream);
    to_data(version, sink);
}

void pow_certificate::to_data(uint32_t version, writer& sink) const
{
    sink.write_size_little_endian(uint64_t(type_));
    sink.write_size_little_endian(uint64_t(tag_));
    sink.write_hash(anchor_);
    sink.write_8_bytes_little_endian(nonce_);
}

bool pow_certificate::is_valid() const
{
    return (type_ < pow_type::max_pow_type) && (tag_ < chain_tag::max_chain_tag) && (anchor_ != bc::null_hash);
}

void pow_certificate::reset()
{
    type_ = pow_type::plain;
    tag_ = chain_tag::unknown;
    anchor_ = bc::null_hash;
    nonce_ = 0;
}

size_t pow_certificate::serialized_size(uint32_t version) const
{
    return message::variable_uint_size(uint64_t(type_))
           + message::variable_uint_size(uint64_t(tag_))
           + anchor_.size()
           + 8;
}

pow_certificate& pow_certificate::operator=(pow_certificate&& other)
{
    type_ = other.type_;
    tag_ = other.tag_;
    anchor_ = std::move(other.anchor_);
    nonce_ = other.nonce_;
    return *this;
}

uint256_t pow_certificate::calculate_work_done(const multihash &id)
{
    return calculate_work_done(id.to_data(0));
}

uint256_t pow_certificate::calculate_work_done(const data_chunk &chunk)
{
    uint256_t pow_value = to_uint256(calculate_pow_hash(chunk));
    return ((~pow_value) / (pow_value + 1)) + 1;
}

hash_digest pow_certificate::calculate_pow_hash(const data_chunk &chunk)
{
    return default_pow::calculate(to_pow_blob(chunk));
}

data_chunk pow_certificate::to_pow_blob(const data_chunk &chunk) const
{
    data_chunk data;
    const auto size = chunk.size() + serialized_size(0);
    data.reserve(size);
    data_sink ostream(data);
    to_pow_blob(chunk, ostream);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

void pow_certificate::to_pow_blob(const data_chunk &chunk, std::ostream& stream) const
{
    ostream_writer sink(stream);
    to_pow_blob(chunk, sink);
}

void pow_certificate::to_pow_blob(const data_chunk &chunk, writer& sink) const
{
    sink.write_bytes(chunk);
    to_data(0, sink);
}

std::string pow_certificate::to_string() const
{
    std::stringstream stream;
    stream << "{type_=" << uint32_t(type_)
           << " tag_=" << uint32_t(tag_)
           << " anchor_=" << bc::encode_base16(anchor_)
           << " nonce_=" << nonce_
           << "}";
    return stream.str();
}

}
}