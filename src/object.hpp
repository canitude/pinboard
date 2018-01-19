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
#ifndef LIBBITCOIN_MESSAGE_OBJECT_HPP
#define LIBBITCOIN_MESSAGE_OBJECT_HPP

#include <istream>
#include <memory>
#include <string>
#include <bitcoin/bitcoin/define.hpp>
#include <bitcoin/bitcoin/utility/data.hpp>
#include <bitcoin/bitcoin/utility/reader.hpp>
#include <bitcoin/bitcoin/utility/writer.hpp>

#include "multihash.hpp"
#include "pow_certificate.hpp"

namespace libbitcoin {
namespace message {

class BC_API object_payload {
public:
    typedef std::shared_ptr<object_payload> ptr;
    typedef std::shared_ptr<const object_payload> const_ptr;

    template <class F>
    friend class miner;

    // THIS IS FOR LIBRARY USE ONLY, DO NOT CREATE A DEPENDENCY ON IT.
    struct validation
    {
        uint256_t work_done = 0;
        uint256_t pow_value = 0;
        hash_digest pow_hash = bc::null_hash; // hash for PoW
        hash_digest id = bc::null_hash;   // id for network protocol
    };

    static object_payload factory_from_data(uint32_t version, const data_chunk& data);
    static object_payload factory_from_data(uint32_t version, std::istream& stream);
    static object_payload factory_from_data(uint32_t version, reader& source);

    object_payload();
    object_payload(const object_payload& other);
    object_payload(object_payload&& other) noexcept;

    object_payload(const std::string &str);

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
    object_payload& operator=(object_payload&& other);
    void operator=(const object_payload&) = delete;

    bool operator==(const object_payload& other) const;
    bool operator!=(const object_payload& other) const;

    inline uint64_t get_nonce() const
    {
        return pow_.get_nonce();
    }

    inline pow_type get_pow_type() const
    {
        return pow_.get_pow_type();
    }

    inline hash_digest get_anchor() const
    {
        return pow_.get_anchor();
    }

    data_chunk serialize_id_and_pow();
    uint256_t get_work_done();
    uint256_t get_pow_value();
    multihash get_body_id();
    hash_digest get_id() const;

    std::string to_string() const;

    // THIS IS FOR LIBRARY USE ONLY, DO NOT CREATE A DEPENDENCY ON IT.
    mutable validation validation;

protected:

private:
    data_chunk body_;     // empty in case of pure PoW (empty pin)
    multihash body_id_;        // empty in wire format in case !body_.empty()

    pow_certificate pow_;
};

class BC_API object // : public object_payload
{
public:
    typedef std::shared_ptr<object> ptr;
    typedef std::shared_ptr<const object> const_ptr;

    static object factory_from_data(uint32_t version, const data_chunk& data);
    static object factory_from_data(uint32_t version, std::istream& stream);
    static object factory_from_data(uint32_t version, reader& source);

    object();
    object(const object_payload& payload);
    object(object_payload&& payload);
    object(const object& other);
    object(object&& other) noexcept;

    /*data_chunk& payload();
    const data_chunk& payload() const;
    void set_payload(const data_chunk& value);
    void set_payload(data_chunk&& value);*/
    const object_payload& payload() const;

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
    object& operator=(object&& other);
    void operator=(const object&) = delete;

    bool operator==(const object& other) const;
    bool operator!=(const object& other) const;

    static const std::string command;
    static const uint32_t version_minimum;
    static const uint32_t version_maximum;

private:
    object_payload payload_;
};

} // namespace message

#define DECLARE_MESSAGE_POINTER_TYPES(type) \
typedef message::type::ptr type##_ptr; \
typedef message::type::const_ptr type##_const_ptr

DECLARE_MESSAGE_POINTER_TYPES(object);

#undef DECLARE_MESSAGE_POINTER_TYPE

} // namespace libbitcoin

#endif
