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
#ifndef LIBBITCOIN_MESSAGE_MULTIHASH_HPP
#define LIBBITCOIN_MESSAGE_MULTIHASH_HPP

#include <istream>
#include <memory>
#include <string>
#include <bitcoin/bitcoin/define.hpp>
#include <bitcoin/bitcoin/utility/data.hpp>
#include <bitcoin/bitcoin/utility/reader.hpp>
#include <bitcoin/bitcoin/utility/writer.hpp>

namespace libbitcoin {
namespace message {

// Values taken from https://github.com/multiformats/multihash/blob/master/hashtable.csv
enum class digest_type : uint32_t {
    identity         = 0x00,

    sha1             = 0x11,
    sha2_256         = 0x12,
    sha2_512         = 0x13,

    sha3_224         = 0x17,
    sha3_256         = 0x16,
    sha3_384         = 0x15,
    sha3_512         = 0x14,

    dbl_sha2_256     = 0x56,

    md4              = 0xd4,
    md5              = 0xd5,
    // TODO: more constants must be added

    max_digest_type
};

class multihash {
public:
    typedef std::shared_ptr<multihash> ptr;
    typedef std::shared_ptr<const multihash> const_ptr;

    static multihash factory_from_data(uint32_t version, const data_chunk& data);
    static multihash factory_from_data(uint32_t version, std::istream& stream);
    static multihash factory_from_data(uint32_t version, reader& source);

    multihash();
    multihash(const multihash& other);
    multihash(multihash&& other) noexcept;

    multihash(digest_type fn_code, const data_chunk& digest);

    bool from_data(uint32_t version, const data_chunk& data);
    bool from_data(uint32_t version, std::istream& stream);
    bool from_data(uint32_t version, reader& source);
    data_chunk to_data(uint32_t version) const;
    void to_data(uint32_t version, std::ostream& stream) const;
    void to_data(uint32_t version, writer& sink) const;
    bool is_valid() const;
    void reset();
    size_t serialized_size(uint32_t version) const;

    /// This class is move assignable but not copy assignable.
    multihash& operator=(multihash&& other);
    void operator=(const multihash&) = delete;

    inline bool operator==(const multihash& other) const
    {
        return (fn_code_ == other.fn_code_) && (digest_ == other.digest_);
    }

    inline bool operator!=(const multihash& other) const
    {
        return !(*this == other);
    }

    bool empty() const
    {
        return digest_.empty();
    }

    std::string to_string() const;
    std::string to_base58() const;

private:
    digest_type fn_code_;
    data_chunk digest_;
};
}
}

#endif