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
#include "util.h"

uint256 CBlockHeader::GetHash(const Consensus::Params& params) const
{
    int version;

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

std::string CBlockHeader::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s,  hashStateRoot=%s, hashUTXORoot=%s, nHeight=%u, nTime=%u, nBits=%08x, nNonce=%s)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        hashStateRoot.ToString(), // fasc
        hashUTXORoot.ToString(), // fasc
        nHeight, nTime, nBits, nNonce.GetHex());

    return s.str();
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

bool _IsSupportContract(int nVersion, int nHeight)
{
    if ( nVersion == 5 )
       return true;

    if ( nHeight == 0 )
       return false;

    bool fRegTest =  (Params().NetworkIDString() == CBaseChainParams::REGTEST ) ||  (Params().NetworkIDString() == CBaseChainParams::REGTEST );
    if ( fRegTest )
       return true;


    return (nHeight >= (uint32_t)Params().GetConsensus().ContractHeight );
}

bool _IsLegacyFormat( int nHeight)
{
    if ( nHeight == 0 )
       return false;

    return  (Params().NetworkIDString() == CBaseChainParams::REGTEST ) ||  (Params().NetworkIDString() == CBaseChainParams::REGTEST );
}
