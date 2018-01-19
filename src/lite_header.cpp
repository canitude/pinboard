/**
 * Copyright (c) 2011-2017 libbitcoin developers
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
#include "lite_header.hpp"

#include <chrono>
#include <cstddef>
#include <utility>
#include <bitcoin/bitcoin/chain/chain_state.hpp>
#include <bitcoin/bitcoin/chain/compact.hpp>
#include <bitcoin/bitcoin/constants.hpp>
#include <bitcoin/bitcoin/error.hpp>
#include <bitcoin/bitcoin/math/hash.hpp>
#include <bitcoin/bitcoin/utility/container_sink.hpp>
#include <bitcoin/bitcoin/utility/container_source.hpp>
#include <bitcoin/bitcoin/utility/istream_reader.hpp>
#include <bitcoin/bitcoin/utility/ostream_writer.hpp>
#include <bitcoin/bitcoin/chain/header.hpp>

namespace libbitcoin {
namespace chain {

// Use system clock because we require accurate time of day.
using wall_clock = std::chrono::system_clock;

// Constructors.
//-----------------------------------------------------------------------------

lite_header::lite_header()
  : lite_header(0, null_hash, null_hash, 0, 0, 0)
{
}

lite_header::lite_header(lite_header&& other)
  : lite_header(other.version_, std::move(other.previous_block_hash_),
      std::move(other.merkle_), other.timestamp_, other.bits_, other.nonce_)
{
    // TODO: implement safe private accessor for conditional cache transfer.
    validation = std::move(other.validation);
}

lite_header::lite_header(const lite_header& other)
  : lite_header(other.version_, other.previous_block_hash_, other.merkle_,
        other.timestamp_, other.bits_, other.nonce_)
{
    // TODO: implement safe private accessor for conditional cache transfer.
    validation = other.validation;
}

lite_header::lite_header(const header& other)
  : validation{}
{
    data_chunk temp = other.to_data();
    from_data(temp);
}

lite_header::lite_header(lite_header&& other, hash_digest&& hash)
  : lite_header(other.version_, std::move(other.previous_block_hash_),
      std::move(other.merkle_), other.timestamp_, other.bits_, other.nonce_)
{
    hash_ = std::make_shared<hash_digest>(std::move(hash));
    validation = std::move(other.validation);
}

lite_header::lite_header(const lite_header& other, const hash_digest& hash)
  : lite_header(other.version_, other.previous_block_hash_, other.merkle_,
        other.timestamp_, other.bits_, other.nonce_)
{
    hash_ = std::make_shared<hash_digest>(hash);
    validation = other.validation;
}

lite_header::lite_header(uint32_t version, hash_digest&& previous_block_hash,
    hash_digest&& merkle, uint32_t timestamp, uint32_t bits, uint32_t nonce)
  : validation{},
    version_(version),
    previous_block_hash_(std::move(previous_block_hash)),
    merkle_(std::move(merkle)),
    timestamp_(timestamp),
    bits_(bits),
    nonce_(nonce)
{
}

lite_header::lite_header(uint32_t version, const hash_digest& previous_block_hash,
    const hash_digest& merkle, uint32_t timestamp, uint32_t bits,
    uint32_t nonce)
  : validation{},
    version_(version),
    previous_block_hash_(previous_block_hash),
    merkle_(merkle),
    timestamp_(timestamp),
    bits_(bits),
    nonce_(nonce)
{
}

// Operators.
//-----------------------------------------------------------------------------

lite_header& lite_header::operator=(lite_header&& other)
{
    // TODO: implement safe private accessor for conditional cache transfer.
    version_ = other.version_;
    previous_block_hash_ = std::move(other.previous_block_hash_);
    merkle_ = std::move(other.merkle_);
    timestamp_ = other.timestamp_;
    bits_ = other.bits_;
    nonce_ = other.nonce_;
    validation = std::move(other.validation);
    return *this;
}

lite_header& lite_header::operator=(const lite_header& other)
{
    // TODO: implement safe private accessor for conditional cache transfer.
    version_ = other.version_;
    previous_block_hash_ = other.previous_block_hash_;
    merkle_ = other.merkle_;
    timestamp_ = other.timestamp_;
    bits_ = other.bits_;
    nonce_ = other.nonce_;
    validation = other.validation;
    return *this;
}

bool lite_header::operator==(const lite_header& other) const
{
    return (version_ == other.version_)
        && (previous_block_hash_ == other.previous_block_hash_)
        && (merkle_ == other.merkle_)
        && (timestamp_ == other.timestamp_)
        && (bits_ == other.bits_)
        && (nonce_ == other.nonce_);
}

bool lite_header::operator!=(const lite_header& other) const
{
    return !(*this == other);
}

// Deserialization.
//-----------------------------------------------------------------------------

// static
lite_header lite_header::factory_from_data(const data_chunk& data, bool wire)
{
    lite_header instance;
    instance.from_data(data, wire);
    return instance;
}

// static
lite_header lite_header::factory_from_data(std::istream& stream, bool wire)
{
    lite_header instance;
    instance.from_data(stream, wire);
    return instance;
}

// static
lite_header lite_header::factory_from_data(reader& source, bool wire)
{
    lite_header instance;
    instance.from_data(source, wire);
    return instance;
}

bool lite_header::from_data(const data_chunk& data, bool wire)
{
    data_source istream(data);
    return from_data(istream, wire);
}

bool lite_header::from_data(std::istream& stream, bool wire)
{
    istream_reader source(stream);
    return from_data(source, wire);
}

bool lite_header::from_data(reader& source, bool wire)
{
    version_ = source.read_4_bytes_little_endian();
    previous_block_hash_ = source.read_hash();
    merkle_ = source.read_hash();
    timestamp_ = source.read_4_bytes_little_endian();
    bits_ = source.read_4_bytes_little_endian();
    nonce_ = source.read_4_bytes_little_endian();

    if (!wire)
        validation.median_time_past = source.read_4_bytes_little_endian();

    if (!source)
        reset();

    return source;
}

// protected
void lite_header::reset()
{
    version_ = 0;
    previous_block_hash_.fill(0);
    merkle_.fill(0);
    timestamp_ = 0;
    bits_ = 0;
    nonce_ = 0;
    invalidate_cache();
}

bool lite_header::is_valid() const
{
    return (version_ != 0) ||
        (previous_block_hash_ != null_hash) ||
        (merkle_ != null_hash) ||
        (timestamp_ != 0) ||
        (bits_ != 0) ||
        (nonce_ != 0);
}

// Serialization.
//-----------------------------------------------------------------------------

data_chunk lite_header::to_data(bool wire) const
{
    data_chunk data;
    const auto size = serialized_size(wire);
    data.reserve(size);
    data_sink ostream(data);
    to_data(ostream, wire);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

void lite_header::to_data(std::ostream& stream, bool wire) const
{
    ostream_writer sink(stream);
    to_data(sink, wire);
}

void lite_header::to_data(writer& sink, bool wire) const
{
    sink.write_4_bytes_little_endian(version_);
    sink.write_hash(previous_block_hash_);
    sink.write_hash(merkle_);
    sink.write_4_bytes_little_endian(timestamp_);
    sink.write_4_bytes_little_endian(bits_);
    sink.write_4_bytes_little_endian(nonce_);

    if (!wire)
        sink.write_4_bytes_little_endian(validation.median_time_past);
}

// Size.
//-----------------------------------------------------------------------------

// static
size_t lite_header::satoshi_fixed_size()
{
    return sizeof(version_)
        + hash_size
        + hash_size
        + sizeof(timestamp_)
        + sizeof(bits_)
        + sizeof(nonce_);
}

size_t lite_header::serialized_size(bool wire) const
{
    return satoshi_fixed_size() + (wire ? 0 : sizeof(uint32_t));
}

// Accessors.
//-----------------------------------------------------------------------------

uint32_t lite_header::version() const
{
    return version_;
}

void lite_header::set_version(uint32_t value)
{
    version_ = value;
    invalidate_cache();
}

hash_digest& lite_header::previous_block_hash()
{
    return previous_block_hash_;
}

const hash_digest& lite_header::previous_block_hash() const
{
    return previous_block_hash_;
}

void lite_header::set_previous_block_hash(const hash_digest& value)
{
    previous_block_hash_ = value;
    invalidate_cache();
}

void lite_header::set_previous_block_hash(hash_digest&& value)
{
    previous_block_hash_ = std::move(value);
    invalidate_cache();
}

hash_digest& lite_header::merkle()
{
    return merkle_;
}

const hash_digest& lite_header::merkle() const
{
    return merkle_;
}

void lite_header::set_merkle(const hash_digest& value)
{
    merkle_ = value;
    invalidate_cache();
}

void lite_header::set_merkle(hash_digest&& value)
{
    merkle_ = std::move(value);
    invalidate_cache();
}

uint32_t lite_header::timestamp() const
{
    return timestamp_;
}

void lite_header::set_timestamp(uint32_t value)
{
    timestamp_ = value;
    invalidate_cache();
}

uint32_t lite_header::bits() const
{
    return bits_;
}

void lite_header::set_bits(uint32_t value)
{
    bits_ = value;
    invalidate_cache();
}

uint32_t lite_header::nonce() const
{
    return nonce_;
}

void lite_header::set_nonce(uint32_t value)
{
    nonce_ = value;
    invalidate_cache();
}

// Cache.
//-----------------------------------------------------------------------------

// protected
void lite_header::invalidate_cache() const
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    mutex_.lock_upgrade();

    if (hash_)
    {
        mutex_.unlock_upgrade_and_lock();
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        hash_.reset();
        //---------------------------------------------------------------------
        mutex_.unlock_and_lock_upgrade();
    }

    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////
}

hash_digest lite_header::hash() const
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    mutex_.lock_upgrade();

    if (!hash_)
    {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        mutex_.unlock_upgrade_and_lock();
        hash_ = std::make_shared<hash_digest>(bitcoin_hash(to_data()));
        mutex_.unlock_and_lock_upgrade();
        //---------------------------------------------------------------------
    }

    const auto hash = *hash_;
    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////

    return hash;
}

hash_digest lite_header::pow_hash() const
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    mutex_.lock_upgrade();

    if (!pow_hash_)
    {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        mutex_.unlock_upgrade_and_lock();
        const data_chunk data = to_data();
        pow_hash_ = std::make_shared<hash_digest>(scrypt<32>(data, data, 1024, 1, 1));
        mutex_.unlock_and_lock_upgrade();
        //---------------------------------------------------------------------
    }

    const auto hash = *pow_hash_;
    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////

    return hash;
}

// Validation helpers.
//-----------------------------------------------------------------------------

/// BUGBUG: bitcoin 32bit unix time: en.wikipedia.org/wiki/Year_2038_problem
bool lite_header::is_valid_timestamp() const
{
    using namespace std::chrono;
    static const auto two_hours = seconds(timestamp_future_seconds);
    const auto time = wall_clock::from_time_t(timestamp_);
    const auto future = wall_clock::now() + two_hours;
    return time <= future;
}

// [CheckProofOfWork]
bool lite_header::is_valid_proof_of_work(bool retarget) const
{
    const auto bits = compact(bits_);
    static const uint256_t pow_limit(compact{ work_limit(retarget) });

    if (bits.is_overflowed())
        return false;

    uint256_t target(bits);

    // Ensure claimed work is within limits.
    if (target < 1 || target > pow_limit)
        return false;

    // Ensure actual work is at least claimed amount (smaller is more work).
    return to_uint256(pow_hash()) <= target;
}

// Validation.
//-----------------------------------------------------------------------------

code lite_header::check(bool retarget) const
{
    if (!is_valid_proof_of_work(retarget))
        return error::invalid_proof_of_work;

    else if (!is_valid_timestamp())
        return error::futuristic_timestamp;

    else
        return error::success;
}

code lite_header::accept(const chain_state& state) const
{
    if (bits_ != state.work_required())
        return error::incorrect_proof_of_work;

    else if (state.is_checkpoint_conflict(hash()))
        return error::checkpoints_failed;

    else if (state.is_under_checkpoint())
        return error::success;

    else if (version_ < state.minimum_version())
        return error::old_version_block;

    else if (timestamp_ <= state.median_time_past())
        return error::timestamp_too_early;

    else
        return error::success;
}

} // namespace chain
} // namespace libbitcoin
