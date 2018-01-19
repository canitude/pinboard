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

#ifndef LIBBITCOIN_MESSAGE_POW_CERTIFICATE_HPP
#define LIBBITCOIN_MESSAGE_POW_CERTIFICATE_HPP

#include <istream>
#include <memory>
#include <string>
#include <bitcoin/bitcoin/define.hpp>
#include <bitcoin/bitcoin/utility/data.hpp>
#include <bitcoin/bitcoin/utility/reader.hpp>
#include <bitcoin/bitcoin/utility/writer.hpp>

#include "multihash.hpp"

namespace libbitcoin {
namespace message {

enum class pow_type: uint32_t {
    plain           = 0x00,   // no PoW
    scrypt_14_1_8   = 0x01,   // SCrypt with recommended parameters N = 2^14 = 16384, r = 8, p = 1
    scrypt_10_1_1   = 0x02,   // Litecoin's parameters

    max_pow_type
};

template<pow_type PT, uint32_t MUL, size_t SIZE, size_t N, size_t P, size_t R>
class pow_scrypt
{
public:
    static pow_type type()
    {
        return PT;
    }

    static size_t digest_size()
    {
        return SIZE;
    }

    static hash_digest calculate(const data_chunk &data)
    {
        return scrypt<SIZE>(data, data, N, P, R);
    }

    static uint32_t pow_mul()
    {
        return MUL;
    }
};

typedef pow_scrypt<pow_type::scrypt_14_1_8, 30, 32, 16384, 1, 8> pow_scrypt_14_1_8;
typedef pow_scrypt<pow_type::scrypt_10_1_1, 10, 32, 1024, 1, 1> pow_scrypt_10_1_1;

//typedef pow_scrypt_10_1_1 default_pow;
typedef pow_scrypt_14_1_8 default_pow;

enum chain_tag : uint32_t {
    unknown         = 0,
    bitcoin_main    = 1,
    bitcoin_test3   = 2,
    litecoin_main   = 10,
    litecoin_test4  = 11,

    max_chain_tag
};

class pow_certificate {
public:
    typedef std::shared_ptr<pow_certificate> ptr;
    typedef std::shared_ptr<const pow_certificate> const_ptr;

    template <class F>
    friend class miner;

    static pow_certificate factory_from_data(uint32_t version, const data_chunk& data);
    static pow_certificate factory_from_data(uint32_t version, std::istream& stream);
    static pow_certificate factory_from_data(uint32_t version, reader& source);

    pow_certificate();
    pow_certificate(const pow_certificate& other);
    pow_certificate(pow_certificate&& other) noexcept;

    pow_certificate(pow_type type, chain_tag tag, const hash_digest& anchor, uint64_t nonce);

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
    pow_certificate& operator=(pow_certificate&& other);
    void operator=(const pow_certificate&) = delete;

    inline bool operator==(const pow_certificate& other) const
    {
        return (type_ == other.type_) && (tag_ == other.tag_)
            && (anchor_ == other.anchor_) && (nonce_ == other.nonce_);
    }

    inline bool operator!=(const pow_certificate& other) const
    {
        return !(*this == other);
    }

    inline uint64_t get_nonce() const
    {
        return nonce_;
    }

    data_chunk to_pow_blob(const data_chunk &chunk) const;
    void to_pow_blob(const data_chunk &chunk, std::ostream& stream) const;
    void to_pow_blob(const data_chunk &chunk, writer& sink) const;

    uint256_t calculate_work_done(const multihash &id);
    uint256_t calculate_work_done(const data_chunk &chunk);
    hash_digest calculate_pow_hash(const data_chunk &chunk);

    inline hash_digest get_anchor() const
    {
        return anchor_;
    }

    inline pow_type get_pow_type() const
    {
        return type_;
    }

    std::string to_string() const;

private:
    pow_type type_;
    chain_tag tag_;
    hash_digest anchor_;
    uint64_t nonce_;
};

}
}

#endif
