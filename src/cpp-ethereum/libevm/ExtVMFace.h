/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file ExtVMFace.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <set>
#include <functional>
#include <boost/optional.hpp>
#include <libdevcore/Common.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libevmcore/Instruction.h>
#include <libethcore/Common.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/ChainOperationParams.h>

namespace dev
{
namespace eth
{

/// Reference to a slice of buffer that also owns the buffer.
///
/// This is extension to the concept C++ STL library names as array_view
/// (also known as gsl::span, array_ref, here vector_ref) -- reference to
/// continuous non-modifiable memory. The extension makes the object also owning
/// the referenced buffer.
///
/// This type is used by VMs to return output coming from RETURN instruction.
/// To avoid memory copy, a VM returns its whole memory + the information what
/// part of this memory is actually the output. This simplifies the VM design,
/// because there are multiple options how the output will be used (can be
/// ignored, part of it copied, or all of it copied). The decision what to do
/// with it was moved out of VM interface making VMs "stateless".
///
/// The type is movable, but not copyable. Default constructor available.
class owning_bytes_ref: public vector_ref<byte const>
{
public:
	owning_bytes_ref() = default;

	/// @param _bytes  The buffer.
	/// @param _begin  The index of the first referenced byte.
	/// @param _size   The number of referenced bytes.
	owning_bytes_ref(bytes&& _bytes, size_t _begin, size_t _size):
			m_bytes(std::move(_bytes))
	{
		// Set the reference *after* the buffer is moved to avoid
		// pointer invalidation.
		retarget(&m_bytes[_begin], _size);
	}

	owning_bytes_ref(owning_bytes_ref const&) = delete;
	owning_bytes_ref(owning_bytes_ref&&) = default;
	owning_bytes_ref& operator=(owning_bytes_ref const&) = delete;
	owning_bytes_ref& operator=(owning_bytes_ref&&) = default;

private:
	bytes m_bytes;
};

enum class BlockPolarity
{
	Unknown,
	Dead,
	Live
};

struct LogEntry
{
	LogEntry() {}
	LogEntry(RLP const& _r) { address = (Address)_r[0]; topics = _r[1].toVector<h256>(); data = _r[2].toBytes(); }
	LogEntry(Address const& _address, h256s const& _ts, bytes&& _d): address(_address), topics(_ts), data(std::move(_d)) {}

	void streamRLP(RLPStream& _s) const { _s.appendList(3) << address << topics << data; }

	LogBloom bloom() const
	{
		LogBloom ret;
		ret.shiftBloom<3>(sha3(address.ref()));
		for (auto t: topics)
			ret.shiftBloom<3>(sha3(t.ref()));
		return ret;
	}
    std::string ToString()const;

	Address address;
	h256s topics;
	bytes data;
};

using LogEntries = std::vector<LogEntry>;

struct LocalisedLogEntry: public LogEntry
{
	LocalisedLogEntry() {}
	explicit LocalisedLogEntry(LogEntry const& _le): LogEntry(_le) {}

	explicit LocalisedLogEntry(
		LogEntry const& _le,
		h256 _special
	):
		LogEntry(_le),
		isSpecial(true),
		special(_special)
	{}

	explicit LocalisedLogEntry(
		LogEntry const& _le,
		h256 const& _blockHash,
		BlockNumber _blockNumber,
		h256 const& _transactionHash,
		unsigned _transactionIndex,
		unsigned _logIndex,
		BlockPolarity _polarity = BlockPolarity::Unknown
	):
		LogEntry(_le),
		blockHash(_blockHash),
		blockNumber(_blockNumber),
		transactionHash(_transactionHash),
		transactionIndex(_transactionIndex),
		logIndex(_logIndex),
		polarity(_polarity),
		mined(true)
	{}

	h256 blockHash;
	BlockNumber blockNumber = 0;
	h256 transactionHash;
	unsigned transactionIndex = 0;
	unsigned logIndex = 0;
	BlockPolarity polarity = BlockPolarity::Unknown;
	bool mined = false;
	bool isSpecial = false;
	h256 special;
};

using LocalisedLogEntries = std::vector<LocalisedLogEntry>;

inline LogBloom bloom(LogEntries const& _logs)
{
	LogBloom ret;
	for (auto const& l: _logs)
		ret |= l.bloom();
	return ret;
}

struct SubState
{
	std::set<Address> suicides;	///< Any accounts that have suicided.
	LogEntries logs;			///< Any logs.
	u256 refunds;				///< Refund counter of SSTORE nonzero->zero.

	SubState& operator+=(SubState const& _s)
	{
		suicides += _s.suicides;
		refunds += _s.refunds;
		logs += _s.logs;
		return *this;
	}

	void clear()
	{
		suicides.clear();
		logs.clear();
		refunds = 0;
	}
};

class ExtVMFace;
class VM;

using LastHashes = std::vector<h256>;

using OnOpFunc = std::function<void(uint64_t /*steps*/, uint64_t /* PC */, Instruction /*instr*/, bigint /*newMemSize*/, bigint /*gasCost*/, bigint /*gas*/, VM*, ExtVMFace const*)>;

struct CallParameters
{
	Address senderAddress;
	Address codeAddress;
	Address receiveAddress;
	u256 valueTransfer;
	u256 apparentValue;
	u256 gas;
	bytesConstRef data;
	OnOpFunc onOp;
};

class EnvInfo
{
public:
	EnvInfo() {}
	EnvInfo(BlockHeader const& _current, LastHashes const& _lh = LastHashes(), u256 const& _gasUsed = u256()):
		m_number(_current.number()),
		m_author(_current.author()),
		m_timestamp(_current.timestamp()),
		m_difficulty(_current.difficulty()),
		// Trim gas limit to int64. convert_to used explicitly instead of
		// static_cast to be noticed when BlockHeader::gasLimit() will be
		// changed to int64 too.
		m_gasLimit(_current.gasLimit().convert_to<int64_t>()),
		m_lastHashes(_lh),
		m_gasUsed(_gasUsed)
	{}

	EnvInfo(BlockHeader const& _current, LastHashes&& _lh, u256 const& _gasUsed = u256()):
		m_number(_current.number()),
		m_author(_current.author()),
		m_timestamp(_current.timestamp()),
		m_difficulty(_current.difficulty()),
		// Trim gas limit to int64. convert_to used explicitly instead of
		// static_cast to be noticed when BlockHeader::gasLimit() will be
		// changed to int64 too.
		m_gasLimit(_current.gasLimit().convert_to<int64_t>()),
		m_lastHashes(_lh),
		m_gasUsed(_gasUsed)
	{}

	u256 const& number() const { return m_number; }
	Address const& author() const { return m_author; }
	u256 const& timestamp() const { return m_timestamp; }
	u256 const& difficulty() const { return m_difficulty; }
	int64_t gasLimit() const { return m_gasLimit; }
	LastHashes const& lastHashes() const { return m_lastHashes; }
	u256 const& gasUsed() const { return m_gasUsed; }

	void setNumber(u256 const& _v) { m_number = _v; }
	void setAuthor(Address const& _v) { m_author = _v; }
	void setTimestamp(u256 const& _v) { m_timestamp = _v; }
	void setDifficulty(u256 const& _v) { m_difficulty = _v; }
	void setGasLimit(int64_t _v) { m_gasLimit = _v; }
	void setLastHashes(LastHashes&& _lh) { m_lastHashes = _lh; }

private:
	u256 m_number;
	Address m_author;
	u256 m_timestamp;
	u256 m_difficulty;
	int64_t m_gasLimit;
	LastHashes m_lastHashes;
	u256 m_gasUsed;
};

/**
 * @brief Interface and null implementation of the class for specifying VM externalities.
 */
class ExtVMFace
{
public:
	/// Null constructor.
	ExtVMFace() = default;

	/// Full constructor.
	ExtVMFace(EnvInfo const& _envInfo, Address _myAddress, Address _caller, Address _origin, u256 _value, u256 _gasPrice, bytesConstRef _data, bytes _code, h256 const& _codeHash, unsigned _depth);

	virtual ~ExtVMFace() = default;

	ExtVMFace(ExtVMFace const&) = delete;
	ExtVMFace& operator=(ExtVMFace const&) = delete;

	/// Read storage location.
	virtual u256 store(u256) { return 0; }

	/// Write a value in storage.
	virtual void setStore(u256, u256) {}

	/// Read address's balance.
	virtual u256 balance(Address) { return 0; }

	/// Read address's code.
	virtual bytes const& codeAt(Address) { return NullBytes; }

	/// @returns the size of the code in bytes at the given address.
	virtual size_t codeSizeAt(Address) { return 0; }

	/// Does the account exist?
	virtual bool exists(Address) { return false; }

	/// Suicide the associated contract and give proceeds to the given address.
	virtual void suicide(Address) { sub.suicides.insert(myAddress); }

	/// Create a new (contract) account.
	virtual h160 create(u256, u256&, bytesConstRef, OnOpFunc const&) { return h160(); }

	/// Make a new message call.
	virtual boost::optional<owning_bytes_ref> call(CallParameters&) = 0;

	/// Revert any changes made (by any of the other calls).
    virtual void
    log(h256s&& _topics, bytesConstRef _data) {
        LogEntry incoming = LogEntry(myAddress, std::move(_topics), _data.toBytes());
        this->sub.logs.push_back(incoming);
        this->logsGenerated.push_back(incoming);
    }

	/// Hash of a block if within the last 256 blocks, or h256() otherwise.
	h256 blockHash(u256 _number) { return _number < envInfo().number() && _number >= (std::max<u256>(256, envInfo().number()) - 256) ? envInfo().lastHashes()[(unsigned)(envInfo().number() - 1 - _number)] : h256(); }

	/// Get the execution environment information.
	EnvInfo const& envInfo() const { return m_envInfo; }

	/// Return the EVM gas-price schedule for this execution context.
	virtual EVMSchedule const& evmSchedule() const { return DefaultSchedule; }

private:
	EnvInfo const& m_envInfo;

public:
    //At present, logs generated during EVM execution are
    //recorded in SubState sub and discarded if
    //an exception is raised after the logging.
    //A typical example when this happens is logs generated before solidity's require(false).
    //
    //The reason for discarding the logs is that
    //logs are considered part of the state of the blockchain,
    //so if an operation is reverted, so should its log messages.
    //
    //At the same time, this is not the behavior expected by the programmer,
    //and it makes it very hard to trace smart contract failure.
    //
    //The problem here lies in the fact that two different use cases for "logs" are
    //conflated together.
    //The first use of logs is their use as "side-effects"
    //to extract output from the smart contract.
    //The second very different use is for debugging and
    //diagnostics.
    //
    //An obvious solution to this issue is what we do here. Every time a
    //log is executed, we create two copies: one which is to become part of the blockchain state,
    //and a second, temporary one which is stored for the sole purpose of displaying
    //it to the programmer who has initiated the call.
	// TODO: make private
    LogEntries logsGenerated;   ///< Generated logs. If the VM raised an exception (for example, via solidity require(false)), then the logs will be reverted.
    Address myAddress;			///< Address associated with executing code (a contract, or contract-to-be).
	Address caller;				///< Address which sent the message (either equal to origin or a contract).
	Address origin;				///< Original transactor.
	u256 value;					///< Value (in Wei) that was passed to this address.
	u256 gasPrice;				///< Price of gas (that we already paid).
	bytesConstRef data;			///< Current input data.
	bytes code;					///< Current code that is executing.
	h256 codeHash;				///< SHA3 hash of the executing code
	SubState sub;				///< Sub-band VM state (suicides, refund counter, logs).
	unsigned depth = 0;			///< Depth of the present call.
};

}
}
