// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"

#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "chainparams.h"
#include "consensus/params.h"
#include "crypto/common.h"

uint256 CBlockHeader::GetHash(const Consensus::Params& params) const
{
    int version;

    if ((uint32_t) -1 == (uint32_t)params.FABHeight ) {
        version = PROTOCOL_VERSION | SERIALIZE_BLOCK_LEGACY;
    }
    else if (nHeight >= (uint32_t)params.FABHeight) {
        version = PROTOCOL_VERSION;
    } else {
        version = PROTOCOL_VERSION | SERIALIZE_BLOCK_LEGACY;
    }

    if ( (uint32_t) -1 == (uint32_t)params.ContractHeight || nHeight < (uint32_t)params.ContractHeight) 
    {
        version |= SERIALIZE_BLOCK_NO_CONTRACT;
    }

    CHashWriter writer(SER_GETHASH, version);
    ::Serialize(writer, *this);
    return writer.GetHash();
}

uint256 CBlockHeader::GetHash() const
{
    const Consensus::Params& consensusParams = Params().GetConsensus();
    return GetHash(consensusParams);
}

uint256 CBlockHeader::GetHashWithoutSign() const
{
    return SerializeHash(*this, SER_GETHASH | SER_WITHOUT_SIGNATURE);
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s,  hashStateRoot=%s, hashUTXORoot=%s, nHeight=%u, nTime=%u, nBits=%08x, nNonce=%s, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        hashStateRoot.ToString(), // fasc
        hashUTXORoot.ToString(), // fasc
        nHeight, nTime, nBits, nNonce.GetHex(),
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    
    return s.str();
}

