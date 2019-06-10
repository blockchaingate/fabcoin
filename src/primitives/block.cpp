// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>
#include <utilstrencodings.h>
#include <crypto/common.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <util.h>


uint256 CBlockHeader::GetHash(const Consensus::Params& params) const
{
    return SerializeHash(*this);

    /*int version;

    if ((uint32_t) -1 == (uint32_t)params.FABHeight ) {
        version = PROTOCOL_VERSION | SERIALIZE_BLOCK_LEGACY;
    }
    else if (nHeight >= (uint32_t)params.FABHeight) {
        version = PROTOCOL_VERSION;
    } else {
        version = PROTOCOL_VERSION | SERIALIZE_BLOCK_LEGACY;
    }
    CHashWriter writer(SER_GETHASH, version);

    ::Serialize(writer, *this);
    return writer.GetHash();
    */
}

uint256 CBlockHeader::GetHash() const
{
    //const Consensus::Params& consensusParams = Params().GetConsensus();
    //return GetHash(consensusParams);

    return SerializeHash(*this);
<<<<<<< HEAD
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
=======
>>>>>>> origin/aggregate-signature
}

uint256 CBlockHeader::GetHashWithoutSign() const
{
    return SerializeHash(*this, SER_GETHASH | SER_WITHOUT_SIGNATURE);
}

std::string CBlockHeader::ToString() const
{
    std::stringstream s;
<<<<<<< HEAD
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s,  hashStateRoot=%s, hashUTXORoot=%s, nHeight=%u, nTime=%u, nBits=%08x, nNonce=%s, vtx=%u)\n",
=======
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s,  hashStateRoot=%s, hashUTXORoot=%s, nHeight=%u, nTime=%u, nBits=%08x, nNonce=%s)\n",
>>>>>>> origin/aggregate-signature
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        hashStateRoot.ToString(), // fasc
        hashUTXORoot.ToString(), // fasc
<<<<<<< HEAD
        nHeight, nTime, nBits, nNonce.GetHex(),
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
=======
        nHeight, nTime, nBits, nNonce.GetHex());

    return s.str();
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << "CBlock(";
    s << "hash=" << GetHash().ToString() << ", ";
    s << "ver=" << this->nVersion << ", ";
    s << "hashPrevBlock=" << this->hashPrevBlock.ToString() << ", ";
    s << "hashMerkleRoot=" << this->hashMerkleRoot.ToString() << ", ";
    s << "hashStateRoot=" << this->hashStateRoot.ToString() << ", ";
    s << "hashUTXORoot=" << this->hashUTXORoot.ToString() << ", ";
    s << "height=" << this->nHeight << ", ";
    s << "time=" << this->nTime << ", ";
    s << "bits=" << this->nBits << ", ";
    s << "nonce=0x" << this->nNonce.ToString() << ", ";
    s << "\nnumberOfTransactions=" << this->vtx.size() << ")";
    int transactionCount = 0;
    for (const auto& tx : this->vtx) {
        if (tx == nullptr) {
            s << "\n(non-initialized)";
            continue;
        }
        s << "\nTx " << transactionCount << ": " << tx->ToString();
        transactionCount ++;
>>>>>>> origin/aggregate-signature
    }

    return s.str();
}

bool _IsSupportContract(int nVersion, int nHeight)
{
    //genesis block
    if ( nHeight == 0 ) {
       if ( nVersion == 5 )
           return true;
       else
           return false;
    }

    bool fRegTest =  (Params().NetworkIDString() == CBaseChainParams::REGTEST ) ||  (Params().NetworkIDString() == CBaseChainParams::REGTEST );
    if ( fRegTest )
       return true;

    return ((uint32_t) nHeight >= (uint32_t) Params().GetConsensus().ContractHeight );

}

bool _IsLegacyFormat( int nHeight)
{
    if ( nHeight == 0 )
       return false;
    //regtest legacyformat
    return  (Params().NetworkIDString() == CBaseChainParams::REGTEST ) ||  (Params().NetworkIDString() == CBaseChainParams::UNITTEST );
}
