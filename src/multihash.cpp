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
#include <bitcoin/bitcoin/formats/base_58.hpp>

#include "multihash.hpp"

namespace libbitcoin {
namespace message {

multihash multihash::factory_from_data(uint32_t version, const data_chunk& data)
{
    multihash instance;
    instance.from_data(version, data);
    return instance;
}

multihash multihash::factory_from_data(uint32_t version, std::istream& stream)
{
    multihash instance;
    instance.from_data(version, stream);
    return instance;
}

multihash multihash::factory_from_data(uint32_t version, reader& source)
{
    multihash instance;
    instance.from_data(version, source);
    return instance;
}

multihash::multihash()
    : fn_code_(digest_type::identity), digest_()
{
}

multihash::multihash(const multihash& other)
    : fn_code_(other.fn_code_), digest_(other.digest_)
{
}

multihash::multihash(multihash&& other) noexcept
    : fn_code_(other.fn_code_), digest_(std::move(other.digest_))
{
}

multihash::multihash(digest_type fn_code, const data_chunk& digest)
    : fn_code_(fn_code), digest_(digest)
{
}

bool multihash::from_data(uint32_t version, const data_chunk& data)
{
    data_source istream(data);
    return from_data(version, istream);
}

bool multihash::from_data(uint32_t version, std::istream& stream)
{
    istream_reader source(stream);
    return from_data(version, source);
}

bool multihash::from_data(uint32_t version, reader& source)
{
    reset();

    fn_code_ = (digest_type)source.read_size_little_endian();
    const auto dig_size = source.read_size_little_endian();
    digest_ = source.read_bytes(dig_size);

    if (!source)
        reset();

    return source;
}

data_chunk multihash::to_data(uint32_t version) const
{
    data_chunk data;
    const auto size = serialized_size(version);
    data.reserve(size);
    data_sink ostream(data);
    to_data(version, ostream);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    /*if (data.size() != size) {
        std::cerr << "data.size() == " << data.size() << std::endl;
        std::cerr << "     size   == " << size << std::endl;
    }*/
    return data;
}

void multihash::to_data(uint32_t version, std::ostream& stream) const
{
    ostream_writer sink(stream);
    to_data(version, sink);
}

void multihash::to_data(uint32_t version, writer& sink) const
{
    sink.write_size_little_endian(uint64_t(fn_code_));
    sink.write_size_little_endian(digest_.size());
    sink.write_bytes(digest_);
}

bool multihash::is_valid() const
{
    return fn_code_ < digest_type::max_digest_type;
}

void multihash::reset()
{
    fn_code_ = digest_type::identity;
    digest_.clear();
}

size_t multihash::serialized_size(uint32_t version) const
{
    return message::variable_uint_size(uint64_t(fn_code_))
            + message::variable_uint_size(digest_.size())
            + digest_.size();
}

multihash& multihash::operator=(multihash&& other)
{
    fn_code_ = other.fn_code_;
    digest_ = std::move(other.digest_);
    return *this;
}

std::string multihash::to_string() const
{
    std::stringstream stream;
    stream << "{fn_code_=" << uint32_t(fn_code_)
           << " digest_=" << bc::encode_base16(digest_)
           << " base58=" << to_base58()
           << "}";
    return stream.str();
}

std::string multihash::to_base58() const
{
    return bc::encode_base58(to_data(0));
}

}
}