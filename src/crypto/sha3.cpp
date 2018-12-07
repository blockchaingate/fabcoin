//Copyright (c) 2015. Andrey Jivsov. crypto@brainhub.org
//Copyright (c) 2018. FA Enterprise System Inc
//Distributed under the MIT software license, see
//http://www.opensource.org/licenses/mit-license.php.
//
/* -------------------------------------------------------------------------
 * Works when compiled for either 32-bit or 64-bit targets, optimized for
 * 64 bit.
 *
 * Canonical implementation of Init/Update/Finalize for SHA-3 byte input.
 *
 * SHA3-256, SHA3-384, SHA-512 are implemented. SHA-224 can easily be added.
 *
 * Based on code from http://keccak.noekeon.org/ .
 *
 * I place the code that I wrote into public domain, free to use.
 *
 * I would appreciate if you give credits to this work if you used it to
 * write or test * your code.
 *
 * Aug 2015. Andrey Jivsov. crypto@brainhub.org
 * ---------------------------------------------------------------------- */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <iostream>
#include "sha3.h"
#include <assert.h>


Sha3::Sha3()
{
    this->flagUseKeccak = false;
}

// Define SHA3_USE_KECCAK to run "pure" Keccak, as opposed to SHA3.
// The tests that this macro enables use the input and output from [Keccak]
// (see the reference below). The used test vectors aren't correct for SHA3,
// however, they are helpful to verify the implementation.
// SHA3_USE_KECCAK only changes one line of code in Finalize.

static const uint64_t keccakf_rndc[24] = {
    0x0000000000000001UL, 0x0000000000008082UL,
    0x800000000000808aUL, 0x8000000080008000UL,
    0x000000000000808bUL, 0x0000000080000001UL,
    0x8000000080008081UL, 0x8000000000008009UL,
    0x000000000000008aUL, 0x0000000000000088UL,
    0x0000000080008009UL, 0x000000008000000aUL,
    0x000000008000808bUL, 0x800000000000008bUL,
    0x8000000000008089UL, 0x8000000000008003UL,
    0x8000000000008002UL, 0x8000000000000080UL,
    0x000000000000800aUL, 0x800000008000000aUL,
    0x8000000080008081UL, 0x8000000000008080UL,
    0x0000000080000001UL, 0x8000000080008008UL
};

static const unsigned keccakf_rotc[24] = {
    1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14, 27, 41, 56, 8, 25, 43, 62,
    18, 39, 61, 20, 44
};

static const unsigned keccakf_piln[24] ={
    10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4, 15, 23, 19, 13, 12, 2, 20,
    14, 22, 9, 6, 1
};

/* generally called after SHA3_KECCAK_SPONGE_WORDS-ctx->capacityWords words
 * are XORed into the state s
 */
static void keccakf(uint64_t s[25])
{
    int i, j, round;
    uint64_t t, bc[5];
    for (round = 0; round < Sha3::numberOfKeccakRounds; round ++){ // Theta
        for (i = 0; i < 5; i ++)
            bc[i] = s[i] ^ s[i + 5] ^ s[i + 10] ^ s[i + 15] ^ s[i + 20];
        for (i = 0; i < 5; i ++) {
            t = bc[(i + 4) % 5] ^ Sha3::rotl64(bc[(i + 1) % 5], 1);
            for (j = 0; j < 25; j += 5)
                s[j + i] ^= t;
        }
        // Rho Pi
        t = s[1];
        for(i = 0; i < 24; i ++) {
            j = keccakf_piln[i];
            bc[0] = s[j];
            s[j] = Sha3::rotl64(t, keccakf_rotc[i]);
            t = bc[0];
        }
        // Chi
        for (j = 0; j < 25; j += 5) {
            for(i = 0; i < 5; i ++)
                bc[i] = s[j + i];
            for(i = 0; i < 5; i++)
                s[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }
        // Iota
        s[0] ^= keccakf_rndc[round];
    }
}

// *************************** Public Inteface ************************

// For Init or Reset call these:
void Sha3::sha3_Init256(void *priv)
{
    Sha3 *ctx = (Sha3 *) priv;
    bool flagOldUseKeccak = ctx->flagUseKeccak;
    memset(ctx, 0, sizeof(*ctx));
    ctx->flagUseKeccak = flagOldUseKeccak;
    ctx->capacityWords = 2 * 256 / (8 * sizeof(uint64_t));
}

void sha3_Init384(void *priv)
{
    Sha3 *ctx = (Sha3 *) priv;
    memset(ctx, 0, sizeof(*ctx));
    ctx->capacityWords = 2 * 384 / (8 * sizeof(uint64_t));
}

void sha3_Init512(void *priv)
{
    Sha3 *ctx = (Sha3 *) priv;
    memset(ctx, 0, sizeof(*ctx));
    ctx->capacityWords = 2 * 512 / (8 * sizeof(uint64_t));
}

void Sha3::sha3_Update(void *priv, void const *bufIn, size_t len)
{
    Sha3 *ctx = (Sha3 *) priv;
    /* 0...7 -- how much is needed to have a word */
    unsigned old_tail = (8 - ctx->byteIndex) & 7;
    size_t words;
    unsigned tail;
    size_t i;
    const uint8_t* buf = (const uint8_t *) bufIn;
    if (ctx->byteIndex >= 8) {
        std::cout << "Internal sha3 computation error. ";
        assert(false);
    }
    if (ctx->wordIndex >= (sizeof(ctx->s) / sizeof(ctx->s[0]))) {
        std::cout << "Internal sha3 computation error. ";
        assert(false);
    }
    if (len < old_tail) {
        // Have no complete word or haven't started the word yet.
        // Endian-independent code follows:
        while (len --)
            ctx->saved |= (uint64_t) (*(buf ++)) << ((ctx->byteIndex ++) * 8);
        if (ctx->byteIndex >= 8) {
            std::cout << "Internal sha3 computation error. ";
            assert(false);
        }
        return;
    }
    if (old_tail) {
        // will have one word to process
        // endian-independent code follows:
        len -= old_tail;
        while (old_tail --)
            ctx->saved |= (uint64_t) (*(buf ++)) << ((ctx->byteIndex ++) * 8);
        // now ready to add saved to the sponge
        ctx->s[ctx->wordIndex] ^= ctx->saved;
        if (ctx->byteIndex != 8) {
            std::cout << "Internal sha3 computation error. ";
            assert(false);
        }
        ctx->byteIndex = 0;
        ctx->saved = 0;
        if (++ ctx->wordIndex == (Sha3::numberOfSpongeWords - ctx->capacityWords)) {
            keccakf(ctx->s);
            ctx->wordIndex = 0;
        }
    }
    // now work in full words directly from input
    if (ctx->byteIndex != 0) {
        std::cout << "Internal sha3 computation error. ";
        assert(false);
    }
    words = len / sizeof(uint64_t);
    tail = len - words * sizeof(uint64_t);
    for (i = 0; i < words; i ++, buf += sizeof(uint64_t)) {
        const uint64_t t =
        (uint64_t) (buf[0]) |
        ((uint64_t) (buf[1]) << 8 * 1) |
        ((uint64_t) (buf[2]) << 8 * 2) |
        ((uint64_t) (buf[3]) << 8 * 3) |
        ((uint64_t) (buf[4]) << 8 * 4) |
        ((uint64_t) (buf[5]) << 8 * 5) |
        ((uint64_t) (buf[6]) << 8 * 6) |
        ((uint64_t) (buf[7]) << 8 * 7);
        ctx->s[ctx->wordIndex] ^= t;
        if (++ ctx->wordIndex == (Sha3::numberOfSpongeWords - ctx->capacityWords)) {
            keccakf(ctx->s);
            ctx->wordIndex = 0;
        }
    }
    // finally, save the partial word
    if (!(ctx->byteIndex == 0 && tail < 8)) {
        std::cout << "Internal sha3 computation error. ";
        assert(false);
    }
    while (tail --) {
        ctx->saved |= (uint64_t) (*(buf ++)) << ((ctx->byteIndex ++) * 8);
    }
    if (ctx->byteIndex >= 8) {
        std::cout << "Internal sha3 computation error. ";
        assert(false);
    }
}

// This is simply the 'update' with the padding block.
// The padding block is 0x01 || 0x00* || 0x80. First 0x01 and last 0x80
// bytes are always present, but they can be the same byte.
void const * Sha3::sha3_Finalize(void *priv)
{
    Sha3 *ctx = (Sha3 *) priv;
    // Append 2-bit suffix 01, per SHA-3 spec. Instead of 1 for padding we
    // use 1 << 2 below. The 0x02 below corresponds to the suffix 01.
    // Overall, we feed 0, then 1, and finally 1 to start padding. Without
    // M || 01, we would simply use 1 to start padding.
    if (!ctx->flagUseKeccak) {
        // SHA3 version
        ctx->s[ctx->wordIndex] ^= (ctx->saved ^ ((uint64_t) ((uint64_t) (0x02 | (1 << 2)) << ((ctx->byteIndex) * 8))));
    } else {
        // For testing the "pure" Keccak version
        ctx->s[ctx->wordIndex] ^= (ctx->saved ^ ((uint64_t) ((uint64_t) 1 << (ctx->byteIndex * 8))));
    }
    ctx->s[Sha3::numberOfSpongeWords - ctx->capacityWords - 1] ^= 0x8000000000000000UL;
    keccakf(ctx->s);
      // Return first bytes of the ctx->s. This conversion is not needed for
      // little-endian platforms e.g. wrap with #if !defined(__BYTE_ORDER__)
      // || !defined(__ORDER_LITTLE_ENDIAN__) || __BYTE_ORDER__!=__ORDER_LITTLE_ENDIAN__
      //    ... the conversion below ...
      //
    {
        unsigned i;
        for (i = 0; i < Sha3::numberOfSpongeWords; i ++) {
            const unsigned t1 = (uint32_t) ctx->s[i];
            const unsigned t2 = (uint32_t) ((ctx->s[i] >> 16) >> 16);
            ctx->sb[i * 8 + 0] = (uint8_t) (t1);
            ctx->sb[i * 8 + 1] = (uint8_t) (t1 >> 8);
            ctx->sb[i * 8 + 2] = (uint8_t) (t1 >> 16);
            ctx->sb[i * 8 + 3] = (uint8_t) (t1 >> 24);
            ctx->sb[i * 8 + 4] = (uint8_t) (t2);
            ctx->sb[i * 8 + 5] = (uint8_t) (t2 >> 8);
            ctx->sb[i * 8 + 6] = (uint8_t) (t2 >> 16);
            ctx->sb[i * 8 + 7] = (uint8_t) (t2 >> 24);
        }
    }
    return (ctx->sb);
}

void Sha3::init()
{
    this->sha3_Init256(this);
}

void Sha3::update(const std::string& input)
{
    this->update(input.c_str(), input.size());
}

void Sha3::update(std::vector<unsigned char> &input)
{
    this->update(input.data(), input.size());
}

void Sha3::update(const void* inputBuffer, size_t length)
{
    this->sha3_Update(this, inputBuffer, length);
}

void Sha3::finalize()
{
    this->sha3_Finalize(this);
}

std::string Sha3::computeSha3_256(const std::string& input)
{
    this->flagUseKeccak = false;
    return this->computeKeccakOrSha3(input);
}

std::string Sha3::computeKeccak3_256(const std::string& input)
{
    this->flagUseKeccak = true;
    return this->computeKeccakOrSha3(input);
}

std::string Sha3::computeKeccakOrSha3(const std::string &input)
{
    this->init();
    this->update(input.c_str(), input.size());
    this->finalize();
    return this->getResultString();
}

std::string Sha3::getResultString()
{
    std::string result;
    result.resize(32);
    for (int i = 0; i < 32; i ++) {
        result[i] = this->sb[i];
    }
    return result;
}

void Sha3::getResultVector(std::vector<unsigned char> &output)
{
    output.resize(32);
    for (int i = 0; i < 32; i ++)
        output[i] = this->sb[i];
}

void Sha3::computeKeccak3_256(const std::string& input, std::vector<unsigned char>& output)
{
    Sha3 theHasher;
    theHasher.flagUseKeccak = true;
    theHasher.init();
    theHasher.update(input.c_str(), input.size());
    theHasher.finalize();
    theHasher.getResultVector(output);
}

void Sha3::computeSha3_256(const std::string& input, std::vector<unsigned char>& output)
{
    Sha3 theHasher;
    theHasher.flagUseKeccak = false;
    theHasher.init();
    theHasher.update(input.c_str(), input.size());
    theHasher.finalize();
    theHasher.getResultVector(output);
}
