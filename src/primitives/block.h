// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FABCOIN_PRIMITIVES_BLOCK_H
#define FABCOIN_PRIMITIVES_BLOCK_H

#include <arith_uint256.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>
#include <version.h>
#include <util.h>
#include <string.h>

static const int SER_WITHOUT_SIGNATURE = 1 << 3;
namespace Consensus {
    struct Params;
};

static const int SERIALIZE_BLOCK_LEGACY = 0x04000000;
static const int SERIALIZE_BLOCK_NO_CONTRACT = 0x08000000;
bool _IsSupportContract(int nVersion, int nHeight);
bool _IsLegacyFormat(int nHeight);

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */

class CBlockHeader
{
public:

    static const size_t HEADER_SIZE = 4+32+32+4+28+4+4+32;  // Excluding Equihash solution
    static const size_t HEADER_NEWSIZE = 4+32+32+4+28+4+4+32+32+32;  // Excluding Equihash solution

    // header
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nHeight;        //Equihash 
    uint32_t nReserved[7];   //Equihash 
    uint32_t nTime;
    uint32_t nBits;
    uint256 hashStateRoot; // fasc
    uint256 hashUTXORoot; // fasc
  
    uint256 nNonce;
    std::vector<unsigned char> nSolution;  // Equihash solution.

    CBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {

        READWRITE(this->nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);

        if ( !( s.GetVersion() & SERIALIZE_BLOCK_LEGACY ) )
           READWRITE(nHeight);

        bool equihash_format =  ! ( (s.GetVersion() & SERIALIZE_BLOCK_LEGACY) || _IsLegacyFormat(nHeight) );

        if (equihash_format) {
            for(size_t i = 0; i < (sizeof(nReserved) / sizeof(nReserved[0])); i++) {
                READWRITE(nReserved[i]);
            }
        }
        READWRITE(nTime);
        READWRITE(nBits);
      
        if ( _IsSupportContract( this->nVersion, nHeight ) ) {
            READWRITE(hashStateRoot); // fasc
            READWRITE(hashUTXORoot); // fasc
        }

        // put nonce in the end , but before nSolution 
        if (equihash_format) {
            READWRITE(nNonce);
            READWRITE(nSolution);
        } else {
            uint32_t legacy_nonce = (uint32_t)nNonce.GetUint64(0);
            READWRITE(legacy_nonce);
            nNonce = ArithToUint256(arith_uint256(legacy_nonce));
        }

    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        //hashStateRoot.SetNull(); // fasc
        //hashUTXORoot.SetNull(); // fasc
        hashStateRoot= uint256S("9514771014c9ae803d8cea2731b2063e83de44802b40dce2d06acd02d0ff65e9");
        hashUTXORoot = uint256S("21b463e3b52f6201c0ad6c991be0485b6ef8c092e64583ffa655cc1b171fe856");
        nHeight = 0;
        memset(nReserved, 0, sizeof(nReserved));
        nTime = 0;
        nBits = 0;
        nNonce.SetNull();
        nSolution.clear();
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    uint256 GetHash() const;
    uint256 GetHash(const Consensus::Params& params) const;
    uint256 GetHashWithoutSign() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
    CBlockHeader& operator=(const CBlockHeader& other) //fasc
    {
        if (this != &other)
        {
            this->nVersion       = other.nVersion;
            this->hashPrevBlock  = other.hashPrevBlock;
            this->hashMerkleRoot = other.hashMerkleRoot;
            this->nHeight        = other.nHeight;
            memcpy(this->nReserved, other.nReserved, sizeof(other.nReserved));
            this->nTime          = other.nTime;
            this->nBits          = other.nBits;
            this->hashStateRoot  = other.hashStateRoot;
            this->hashUTXORoot   = other.hashUTXORoot;
            this->nNonce         = other.nNonce;
            this->nSolution      = other.nSolution;

        }
        return *this;
    }

    std::string ToString() const;
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nHeight        = nHeight;                               //equihash
        memcpy(block.nReserved, nReserved, sizeof(block.nReserved));  //equihash
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.hashStateRoot  = hashStateRoot; // fasc
        block.hashUTXORoot   = hashUTXORoot;  // fasc
        block.nNonce         = nNonce;
        block.nSolution      = nSolution;

        return block;
    }

    std::string ToString() const;
};

/**
 * Custom serializer for CBlockHeader that omits the nonce and solution, for use
 * as input to Equihash.
 */
class CEquihashInput : private CBlockHeader
{
public:
    CEquihashInput(const CBlockHeader &header)
    {
        CBlockHeader::SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;


    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {

        READWRITE(this->nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nHeight);
        for (size_t i = 0; i < (sizeof(nReserved) / sizeof(nReserved[0])); i++) {
            READWRITE(nReserved[i]);
        }
        READWRITE(nTime);
        READWRITE(nBits);

        if (_IsSupportContract( this->nVersion, nHeight ) ){
            READWRITE(hashStateRoot); // fasc
            READWRITE(hashUTXORoot); // fasc
        }
    }
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    explicit CBlockLocator(const std::vector<uint256>& vHaveIn) : vHave(vHaveIn) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};


#endif // FABCOIN_PRIMITIVES_BLOCK_H
