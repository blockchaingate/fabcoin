// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>

#include <hash.h>
#include <tinyformat.h>
#include <utilstrencodings.h>
#include <consensus/consensus.h>
#include <script/interpreter.h>
#include "script/standard.h"

// The following two functions were moved from standard.cpp into this file in
// order to fix a broken build in clang. Should you decide to modify any of this,
// please make sure that your build works both on gcc and on clang.
const char* GetTxnOutputType(txnouttype t)
{
    switch (t)
    {
    case TX_NONSTANDARD: return "nonstandard";
    case TX_PUBKEY: return "pubkey";
    case TX_PUBKEYHASH: return "pubkeyhash";
    case TX_SCRIPTHASH: return "scripthash";
    case TX_MULTISIG: return "multisig";
    case TX_NULL_DATA: return "nulldata";
    case TX_WITNESS_V0_KEYHASH: return "witness_v0_keyhash";
    case TX_WITNESS_V0_SCRIPTHASH: return "witness_v0_scripthash";
    case TX_CREATE: return "create";
    case TX_CALL: return "call";
    case TX_AGGREGATE_SIGNATURE: return "aggregate_signature";
    case TX_CONTRACT_COVERS_FEES: return "contract_covers_fees";
    case TX_PUBLIC_KEY_NO_ANCESTOR: return "public_key_no_ancestor";
    case TX_SCAR_SIGNATURE: return "scar_signature";
    }
    return nullptr;
}

void CScriptTemplate::MakeSCARSignatureTemplate() {
    this->reset();
    this->tx_templateType = TX_AGGREGATE_SIGNATURE;
    this->name = GetTxnOutputType((txnouttype) this->tx_templateType);
    *this << OpcodePattern(OP_DATA, 20, 20) << OpcodePattern(OP_DATA, 20, 20) << OP_DATA << OP_SCARSIGNATURE;
}

void avoidCompilerWarningsDefinedButNotUsedTransaction() {
    (void) FetchSCARShardPublicKeysInternalPointer;
}

std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)", hash.ToString().substr(0,10), n);
}

CTxIn::CTxIn(COutPoint prevoutIn, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

CTxIn::CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = COutPoint(hashPrevTx, nOut);
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

std::string CTxIn::ToString() const
{
    std::string str;
    str += "CTxIn(";
    str += prevout.ToString();
    if (prevout.IsNull())
        str += strprintf(", coinbase %s", HexStr(scriptSig));
    else
        str += strprintf(", scriptSig=%s", HexStr(scriptSig).substr(0, 24));
    if (nSequence != SEQUENCE_FINAL)
        str += strprintf(", nSequence=%u", nSequence);
    str += ")";
    return str;
}

CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
}

std::string CTxOut::ToString() const
{
    return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30));
}

CMutableTransaction::CMutableTransaction() : nVersion(CTransaction::CURRENT_VERSION), nLockTime(0) {}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) : vin(tx.vin), vout(tx.vout), nVersion(tx.nVersion), nLockTime(tx.nLockTime) {}

uint256 CMutableTransaction::GetHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::ComputeHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

bool CTxIn::ToBytesForSignatureWithoutAncestor(std::stringstream& output, std::stringstream* commentsOnFailure) const {
    if (!this->prevout.IsWithoutAncestor()) {
        if (commentsOnFailure != nullptr) {
            *commentsOnFailure
                    << "At the time of writing, ToBytesForSignatureWithoutAncestor requires that the transaction "
                    << "be coin-base like (txin = fff...). This may be changed in the future, but for now "
                    << "an error is returned. ";
        }
        return false;
    }
    ::Serialize(output, this->prevout);
    bool serializeAsEmpty = true;
    if (this->scriptSig.size() > 2) {
        CScriptTemplate scarSignature;
        scarSignature.MakeSCARSignatureTemplateStatic(scarSignature);
        std::vector<std::vector<unsigned char> > scarAddressAndshardId;
        if (this->scriptSig.FitsOpCodePattern(
            scarSignature,
            &scarAddressAndshardId,
            nullptr
        )) {
            CScript lockScript = CScript() << scarAddressAndshardId[0] << scarAddressAndshardId[1];
            ::Serialize(output, lockScript);
            serializeAsEmpty = false;
        }
    }
    if (serializeAsEmpty) {
        ::Serialize(output, CScript());
    }
    ::Serialize(output, this->nSequence);
    return true;
}

bool CTransaction::ToBytesForSignatureWithoutAncestor(std::vector<unsigned char>& output, std::stringstream* commentsOnFailure) const {
    std::stringstream outputStream;
    if (!this->ToBytesForSignatureWithoutAncestor(outputStream, commentsOnFailure)) {
        return false;
    }
    std::string outputString = outputStream.str();
    output = std::vector<unsigned char>(outputString.begin(), outputString.end());
    return true;
}

bool CTransaction::ToBytesForSignatureWithoutAncestor(std::stringstream& output, std::stringstream* commentsOnFailure) const {
    // write version:
    ::Serialize(output, this->nVersion);
    unsigned int nInputs = this->vin.size();
    if (nInputs != 1) {
        if (commentsOnFailure != nullptr) {
            *commentsOnFailure
            << "At the moment, ToBytesForSignatureWithoutAncestor is supported "
            << "for transactions with single input only. ";
        }
        return false;
    }
    ::WriteCompactSize(output, nInputs);
    for (unsigned int nInput = 0; nInput < nInputs; nInput ++) {
        if (!this->vin[nInput].ToBytesForSignatureWithoutAncestor(output, commentsOnFailure)) {
            return false;
        }
    }
    // Serialize vout
    unsigned int nOutputs = this->vout.size();
    ::WriteCompactSize(output, nOutputs);
    for (unsigned int nOutput = 0; nOutput < nOutputs; nOutput ++) {
        ::Serialize(output, this->vout[nOutput]);
    }
    ::Serialize(output, (uint32_t) 0); //<-time lock
    ::Serialize(output, (uint32_t) SIGHASH_ALL);
    return true;
}

uint256 CTransaction::GetWitnessHash() const {
    if (!HasWitness()) {
        return GetHash();
    }
    return SerializeHash(*this, SER_GETHASH, 0);
}

/* For backward compatibility, the hash is initialized to 0. TODO: remove the need for this default constructor entirely. */
CTransaction::CTransaction() : vin(), vout(), nVersion(CTransaction::CURRENT_VERSION), nLockTime(0), hash() {}
CTransaction::CTransaction(const CMutableTransaction &tx) : vin(tx.vin), vout(tx.vout), nVersion(tx.nVersion), nLockTime(tx.nLockTime), hash(ComputeHash()) {}
CTransaction::CTransaction(CMutableTransaction &&tx) : vin(std::move(tx.vin)), vout(std::move(tx.vout)), nVersion(tx.nVersion), nLockTime(tx.nLockTime), hash(ComputeHash()) {}

CAmount CTransaction::GetValueOut() const
{
    CAmount nValueOut = 0;
    for (const auto& tx_out : vout) {
        nValueOut += tx_out.nValue;
        if (!MoneyRange(tx_out.nValue) || !MoneyRange(nValueOut))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nValueOut;
}

unsigned int CTransaction::GetTotalSize() const
{
    return ::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION);
}

std::string CTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%d, vin.size=%u, vout.size=%u, nLockTime=%u)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        vin.size(),
        vout.size(),
        nLockTime);
    for (const auto& tx_in : vin)
        str += "    " + tx_in.ToString() + "\n";
    for (const auto& tx_in : vin)
        str += "    " + tx_in.scriptWitness.ToString() + "\n";
    for (const auto& tx_out : vout)
        str += "    " + tx_out.ToString() + "\n";
    return str;
}
int64_t GetTransactionWeight(const CTransaction& tx)
{
    return ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR -1) + ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
}
///////////////////////////////////////////////////////////// fabcoin

bool CTransaction::HasCreateOrCallInOutputs() const
{
    for (const CTxOut& v : vout) {
        if (v.scriptPubKey.HasOpCreate() || v.scriptPubKey.HasOpCall()) {
            return true;
        }
    }
    return false;
}

bool CTransaction::HasNonFeeCallOrCreateInOutputs() const
{
    for (const CTxOut& v : vout) {
        if (v.scriptPubKey.HasOpCreate() || v.scriptPubKey.HasOpCall()) {
            if (v.scriptPubKey.HasOpContractCoversFees()) {
                continue;
            }
            return true;
        }
    }
    return false;
}

bool CTransaction::HasOpSpend() const
{
    for (const CTxIn& i : vin) {
        if (i.scriptSig.HasOpSpend()) {
            return true;
        }
    }
    return false;
}
/////////////////////////////////////////////////////////////

