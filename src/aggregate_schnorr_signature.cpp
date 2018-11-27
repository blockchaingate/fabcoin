// Copyright (c) 2018 FA Enterprise system
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "aggregate_schnorr_signature.h"

#include <univalue/include/univalue.h>
#include <random.h> //<- for GetStrongRandBytes
#include <mutex>
#include <iostream>
#include "crypto/sha3.h"
#include "encodings_crypto.h"
#include "utilstrencodings.h"
#include "base58.h"

//Implemented in random.cpp. Include random.h appears to collide with the build system.
//extern void GetRandBytes(unsigned char* buf, int num);
//extern void GetStrongRandBytes(unsigned char* out, int num);

//Implemented in base58.cpp. Include base58.h appears to collide with the build system.
//extern std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn);
//extern std::string EncodeBase58(const std::vector<unsigned char>& vch);
//extern bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet);


bool char2int(char input, int& output, std::stringstream* commentsOnFailure)
{
    if (input >= '0' && input <= '9') {
        output = input - '0';
        return true;
    }
    if (input >= 'A' && input <= 'F') {
        output = input - 'A' + 10;
        return true;
    }
    if (input >= 'a' && input <= 'f') {
        output = input - 'a' + 10;
        return true;
    }
    if (commentsOnFailure != 0)
        *commentsOnFailure << "Failed to interpret character: " << input << " as a hex digit. ";
    return false;
}


unsigned char SignatureSchnorr::prefixSignature = 65;
unsigned char SignatureSchnorr::prefixAggregateSignature = 24;

void SignatureSchnorr::SerializeCompressedChallenge(std::vector<unsigned char>& output, unsigned char prefix)
{
    std::vector<unsigned char> challengeSerialization;
    this->challenge.SerializeCompressed(challengeSerialization);
    const std::vector<unsigned char>& theSolution = this->solution.getData();
    //concatenate: prefix + challenge + solution
    output.resize(1 + challengeSerialization.size() + theSolution.size());
    output[0] = prefix;
    for (unsigned i = 0; i < challengeSerialization.size(); i ++)
        output[i + 1] = challengeSerialization[i];
    unsigned offset = challengeSerialization.size() + 1;
    for (unsigned i = 0; i < theSolution.size(); i ++)
        output[i + offset] = theSolution[i];
}

std::string SignatureSchnorr::ToBytes()
{
    std::vector<unsigned char> serialization;
    this->SerializeCompressedChallenge(serialization, this->prefixSignature);
    return std::string((char*) serialization.data(), serialization.size());
}

std::string SignatureSchnorr::ToBase58Check()
{
    std::vector<unsigned char> serialization;
    this->SerializeCompressedChallenge(serialization, this->prefixSignature);
    return EncodeBase58Check(serialization);
}

std::string SignatureSchnorr::ToBase58CustomPrefix(unsigned char prefix)
{
    if (!this->challenge.isInitialized())
        return "(uninitialized)";
    std::vector<unsigned char> serialization;
    this->SerializeCompressedChallenge(serialization, prefix);
    return EncodeBase58(serialization);
}


std::string SignatureSchnorr::ToBase58()
{
    if (!this->challenge.isInitialized())
        return "(uninitialized)";
    std::vector<unsigned char> serialization;
    this->SerializeCompressedChallenge(serialization, this->prefixSignature);
    return EncodeBase58(serialization);
}

bool SignatureSchnorr::MakeFromBase58Check(const std::string& input, std::stringstream* commentsOnFailure)
{
    std::vector<unsigned char> serialization;
    if (!DecodeBase58Check(input, serialization)) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "Failed to decode from base58 your input: " << input;
        }
        return false;
    }
    return this->MakeFromBytes(serialization, commentsOnFailure);
}

bool SignatureSchnorr::MakeFromBase58WithoutCheck(const std::string& input, std::stringstream* commentsOnFailure)
{
    std::vector<unsigned char> serialization;
    if (!Encodings::Base58ToBytes(input, serialization, commentsOnFailure)) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "Failed to decode base58. Your input: " << input;
        }
        return false;
    }
    return this->MakeFromBytes(serialization, commentsOnFailure);
}

bool SignatureSchnorr::MakeFromBytes(const std::vector<unsigned char>& serialization, std::stringstream *commentsOnFailure)
{
    //std::cout << "DEBUG: signature schnorr to be serialized from " << serialization.size() << " bytes. " << std::endl;
    if (!this->challenge.MakeFromBytesSerializationOffset(
            serialization,
            1,
            PublicKeyKanban::lengthPublicKeySerializationCompressed,
            commentsOnFailure
       )) {
        return false;
    }
    if (!this->solution.MakeFromByteSequenceOffset(serialization, PublicKeyKanban::lengthPublicKeySerializationCompressed + 1, commentsOnFailure)) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "Failed to extract solution from your signature. ";
        }
        return false;
    }
    return true;
}

bool SignatureSchnorr::MakeFromBase58DetectCheck(const std::string& input, std::stringstream* commentsOnFailure)
{
    if (input.size() == 89 || input.size() == 90 || input.size() == 91) {
        //<- didn't do the math what are the possible lengths of correct base58 encodings, so using 90+/-1 (guaranteed to work).
        //To do: do the math, fix the code accordingly, and erase this comment.
        return this->MakeFromBase58WithoutCheck(input, commentsOnFailure);
    }
    if (input.size() == 94 || input.size() == 95 || input.size() == 96) {
        //<- didn't do the math what are the possible lengths of correct base58 encodings, so using 95+/-1 (guaranteed to work).
        //To do: do the math, fix the code accordingly, and erase this comment.
        return this->MakeFromBase58Check(input, commentsOnFailure);
    }
    *commentsOnFailure
    << "Your input had " << input.size()
    << " characters. The expected number of characters is 89, 90 or 91 for base58 encoding and 94, 95 or 96 for base58check. "
    << "Your input was: " << input;
    return false;
}

bool SignatureSchnorr::MakeFromUniValueRecognizeFormat(const UniValue& input, std::stringstream* commentsOnFailure)
{
    if (!input.isStr()) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "At the moment, only string inputs in base58 or base58check are supported. Your input was: " << input.write();
        }
        return false;
    }
    const std::string& inputString = input.get_str();
    return this->MakeFromBase58DetectCheck(inputString, commentsOnFailure);
}

bool SchnorrKanban::Verify(SignatureSchnorr& input, std::stringstream* commentsOnFailure_NULL_for_no_comments)
{
    std::stringstream* commentsOnFailure = commentsOnFailure_NULL_for_no_comments; //keeping it short
    std::string messageToSha3 = input.publicKeyImplied.ToBytesCompressed() + input.challenge.ToBytesCompressed() + input.messageImplied;
    Sha3 theSha;
    std::string messageSha3ed = theSha.computeSha3_256(messageToSha3);
    secp256k1_ge publicKeyEC, challengeEC, solutionProposedComputed, gPowerSolution;
    //std::cout << "DEBUG: got to pubkey loading " << std::endl;
    secp256k1_pubkey_load(Secp256k1::ContextForVerification(), &publicKeyEC, &input.publicKeyImplied.data);
    if (!secp256k1_ge_is_valid_var(&publicKeyEC)) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "The public key appears to not be valid: " << input.publicKeyImplied.ToHexCompressed() << ".";
        }
        return false;
    }
    secp256k1_pubkey_load(Secp256k1::ContextForVerification(), &challengeEC, &input.challenge.data);
    if (!secp256k1_ge_is_valid_var(&challengeEC)) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure
            << "The challenge (first part of the signature) appears to not be valid: "
            << input.challenge.ToHexCompressed() << ".";
        }
        return false;
    }
    //std::cout << "DEBUG: pubkey and challenge loaded " << std::endl;
    secp256k1_gej publicKeyECJacobian, publicKeyPowerMessageJacobian, gPowerSolutionJacobian, solutionProposedComputedJacobian;
    secp256k1_gej_set_ge(&publicKeyECJacobian, &publicKeyEC);
    secp256k1_scalar theMessageExponent, zero, solutionScalar;
    secp256k1_scalar_set_b32(&theMessageExponent, (unsigned char *) messageSha3ed.c_str(), 0);
    secp256k1_scalar_set_b32(&solutionScalar, input.solution.data.data(), 0);
    secp256k1_scalar_set_int(&zero, 0);
    //std::cout << "DEBUG: got to before first multiplication. " << std::endl;
    secp256k1_ecmult(
        &Secp256k1::ContextForVerification()->ecmult_ctx,
        &publicKeyPowerMessageJacobian,
        &publicKeyECJacobian,
        &theMessageExponent,
        &zero
    );
    //std::cout << "DEBUG: one multiplication done. Proceding with the second multiplication " << std::endl;
    secp256k1_ecmult(
        &Secp256k1::ContextForVerification()->ecmult_ctx,
        &gPowerSolutionJacobian,
        &publicKeyECJacobian,
        &zero,
        &solutionScalar
    );
    //std::cout << "DEBUG: multiplications complete. Adding challenge to public key power message jacobian. " << std::endl;
    secp256k1_gej_add_ge(&solutionProposedComputedJacobian, &publicKeyPowerMessageJacobian, &challengeEC);
    //We need to compare gPowerSolution with solutionProposedComputedJacobain
    //To do: since those are in jacobian coordinates, we can do the comparison by clearing denominators.
    //Instead, for now, we simply convert the element back to affine coordinates and compare there.
    secp256k1_ge_set_gej(&solutionProposedComputed, &solutionProposedComputedJacobian);
    secp256k1_ge_set_gej(&gPowerSolution, &gPowerSolutionJacobian);
    if (!secp256k1_fe_equal_var(&solutionProposedComputed.x, &gPowerSolution.x)) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "The solution given does not match the challenge computation: the x coordinates are different. ";
        }
        return false;
    }
    if (!secp256k1_fe_equal_var(&solutionProposedComputed.y, &gPowerSolution.y)) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "The solution give does not match the challenge computation: the x coordinates are different. ";
        }
        return false;
    }
    if (solutionProposedComputed.infinity || gPowerSolution.infinity) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "Either the solution power or the computed challenge is infinity. ";
        }
        return false;
    }
    return true;
}

bool SchnorrKanban::Sign(PrivateKeyKanban& input,
    const std::string& message,
    SignatureSchnorr& output,
    PrivateKeyKanban* desiredNonce__NULL_SAFE__DONT_REUSE_VALUES_SECURITY_RISK,
    UniValue* commentsSensitive__NULL_SAFE__SECURITY_RISK_OTHERWISE)
{
    PrivateKeyKanban nonceIfNotProvidedNoPerformancePenalty;
    if (desiredNonce__NULL_SAFE__DONT_REUSE_VALUES_SECURITY_RISK == nullptr) {
        desiredNonce__NULL_SAFE__DONT_REUSE_VALUES_SECURITY_RISK = & nonceIfNotProvidedNoPerformancePenalty;
        nonceIfNotProvidedNoPerformancePenalty.GenerateRandomSecurely();
    }
    PrivateKeyKanban* nonce = desiredNonce__NULL_SAFE__DONT_REUSE_VALUES_SECURITY_RISK; //<- keeping it short.
    if (!nonce->ComputePublicKey(output.challenge, nullptr)) {
        return false;
    }
    if (commentsSensitive__NULL_SAFE__SECURITY_RISK_OTHERWISE != nullptr) {
        commentsSensitive__NULL_SAFE__SECURITY_RISK_OTHERWISE->pushKV("nonceSchnorrBase58Check", nonce->ToBase58Check());
    }
    if (!input.ComputePublicKey(output.publicKeyImplied, nullptr)) {
        return false;
    }
    output.messageImplied = message;
    std::string messageToSha3 = output.publicKeyImplied.ToBytesCompressed() + output.challenge.ToBytesCompressed() + message;
    Sha3 theSha;
    std::string messageSha3ed = theSha.computeSha3_256(messageToSha3);
    secp256k1_scalar scalarMessage, scalarSecret, scalarNonce, scalarDigestTimesSecret, scalarSolution;
    secp256k1_scalar_set_b32(&scalarMessage, (unsigned char *)  messageSha3ed.c_str(), NULL);
    secp256k1_scalar_set_b32(&scalarNonce, nonce->data.data(), NULL);
    secp256k1_scalar_set_b32(&scalarSecret, input.data.data(), NULL);
    secp256k1_scalar_mul(&scalarDigestTimesSecret, &scalarMessage, &scalarSecret);
    secp256k1_scalar_add(&scalarSolution, &scalarDigestTimesSecret, &scalarNonce);
    output.solution.data.resize(32);
    secp256k1_scalar_get_b32(output.solution.data.data(), &scalarSolution);
    return false;
}

secp256k1_context* Secp256k1::ContextForSigning() {
    static secp256k1_context* resultPointer = nullptr;
    //Static allocation inside the function body guarantees
    //no allocation happens in an arbitrary order.
    //In particular, this should avoid a potential static initialization order fiasco.
    return Secp256k1::ContextAllocateIfNeeded(resultPointer);
}

secp256k1_context* Secp256k1::ContextForVerification()
{
    static secp256k1_context* resultPointer = nullptr;
    //Static allocation inside the function body guarantees
    //no allocation happens in an arbitrary order.
    //In particular, this should avoid a potential static initialization order fiasco.
    return Secp256k1::ContextAllocateIfNeeded(resultPointer);
}

secp256k1_context* Secp256k1::ContextAllocateIfNeeded(secp256k1_context*& staticPointer)
{
    if (staticPointer != nullptr) {
        //<- we don't lock mutexes if context already exists
        return staticPointer;
    }
    static std::mutex contextAllocatorLock;
    std::lock_guard<std::mutex> lockGuard(contextAllocatorLock);
    //std::cout << "DEBUG: generating context" << std::endl;
    //<- if staticPointer was null,
    //only one thread will execute what's next.
    if (staticPointer != nullptr) { //<- this may happen if the two threads simultaneously try to create a contextCreator
        return staticPointer;
    }
    secp256k1_context* tempPointer = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    //<- we do not store the output into staticPointer before it is completed and randomized
    unsigned char randomBytes[32];
    GetRandBytes(randomBytes, 32);
    if (!secp256k1_context_randomize(tempPointer, randomBytes))
    {
        std::cout << "Error initializing secp256k1 context. This should not happen: we terminate immediately" << std::endl;
        assert(false);
    }
    //std::cout << "DEBUG: context almost generated ..." << std::endl;

    staticPointer = tempPointer; //<- this is atomic, which ensures that anyone
    //who checks for staticPointer!=0 at the beginning of this function need not lock
    //any mutexes.
    return staticPointer;
}

PrivateKeyKanban::PrivateKeyKanban()
{
    this->flagCorrespondingPublicKeyUsedWithCompressedEncoding = true;
}

void PrivateKeyKanban::reset()
{
    for (unsigned i = 0; i < this->data.size(); i ++) {
        this->data[i] = 0;
    }
}

bool PrivateKeyKanban::GenerateRandomSecurely()
{
    this->data.resize(this->lengthKey);
    do {
        GetStrongRandBytes(this->data.data(), this->lengthKey);
    } while (!secp256k1_ec_seckey_verify(Secp256k1::ContextForSigning(), this->data.data()));
    return true;
}

bool PrivateKeyKanban::MakeFromBase58DetectCheck(const std::string& input, std::stringstream* commentsOnFailure)
{
    if (input.size() > 47)
        return this->MakeFromBase58Check(input, commentsOnFailure);
    else
        return this->MakeFromBase58WithoutCheck(input, commentsOnFailure);
}

bool PrivateKeyKanban::MakeFromBase58WithoutCheck(const std::string& input, std::stringstream* commentsOnFailure)
{
    std::vector<unsigned char> dataWithPrefix; //<- needs to be wiped after use
    //std::cout << "DEBUG: about to base 58-decode secret: " << input << std::endl;
    if (!Encodings::Base58ToBytes(input, dataWithPrefix, commentsOnFailure))
        return false;
    if (dataWithPrefix.size() < 33) {
        if (commentsOnFailure != nullptr) {
            *commentsOnFailure << "Secret: " << input << " needs to have at least 33 bytes. ";
        }
        return false;
    }
    this->data.resize(this->lengthKey);
    for (unsigned i = 0; i < this->data.size(); i ++)
        this->data[i] = 0;
    int offset = 1;
    this->flagCorrespondingPublicKeyUsedWithCompressedEncoding = false;
    if (dataWithPrefix.size() == 34) {
        if (dataWithPrefix[33] == 0x01) {
            this->flagCorrespondingPublicKeyUsedWithCompressedEncoding = true;
        }
    }
    for (unsigned i = 0; i < this->lengthKey; i ++) {
        this->data[i] = dataWithPrefix[i + offset];
    }
    for (unsigned i = 0; i < dataWithPrefix.size(); i ++)
        dataWithPrefix[i] = 0;
    //std::cout << "DEBUG: datawith prefix had: " << dataWithPrefix.size() << " bytes" << std::endl;
    return true;
}

bool PrivateKeyKanban::MakeFromBase58Check(const std::string& input, std::stringstream* commentsOnFailure)
{
    CFabcoinSecret theSecret;
    //std::cout << "DEBUG: Got to making base 58 check secret; " << std::endl;
    if (!theSecret.SetString(input)) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "Failed to extract base 58 secret from your input: " << input << ". ";
        }
        return false;
    }
    auto otherKey = theSecret.GetKey();
    this->flagCorrespondingPublicKeyUsedWithCompressedEncoding = theSecret.GetKey().IsCompressed();
    this->data.resize(otherKey.end() - otherKey.begin());
    unsigned i = 0;
    for (auto iterator = otherKey.begin(); iterator != otherKey.end(); iterator ++) {
        this->data[i] = *iterator;
        i ++;
    }
    return true;
}

void PrivateKeyKanban::MakeSmallInt(unsigned int input)
{
    this->MakeZero();
    secp256k1_scalar theScalar;
    secp256k1_scalar_set_int(&theScalar, input);
    *this = theScalar;
}

void PrivateKeyKanban::MakeZero()
{
    this->data.resize(this->lengthKey);
    for (unsigned i = 0; i < this->lengthKey; i ++)
        this->data[i] = 0;
}

void PrivateKeyKanban::operator=(const secp256k1_scalar& other)
{
    this->data.resize(this->lengthKey);
    secp256k1_scalar_get_b32(this->data.data(), &other);
}

void PrivateKeyKanban::AddToMe(const PrivateKeyKanban& other, bool otherIsSensitive)
{
    secp256k1_scalar left, right, resultScalar;
    secp256k1_scalar_set_b32(&left, this->data.data(), 0);
    secp256k1_scalar_set_b32(&right, other.data.data(), 0);
    secp256k1_scalar_add(&resultScalar, &left, &right);
    if (otherIsSensitive)
        secp256k1_scalar_clear(&right);
    *this = resultScalar;
}

void PrivateKeyKanban::operator+=(const PrivateKeyKanban& other)
{
    this->AddToMe(other, true);
}

void PrivateKeyKanban::MultiplyMeBy(const PrivateKeyKanban &other, bool otherIsSensitive)
{
    secp256k1_scalar left, right, resultScalar;
    secp256k1_scalar_set_b32(&left, this->data.data(), 0);
    secp256k1_scalar_set_b32(&right, other.data.data(), 0);
    secp256k1_scalar_mul(&resultScalar, &left, &right);
    if (otherIsSensitive)
        secp256k1_scalar_clear(&right);
    *this = resultScalar;
}

void PrivateKeyKanban::operator*=(const PrivateKeyKanban& other)
{
    this->MultiplyMeBy(other, true);
}

bool PrivateKeyKanban::operator ==(const PrivateKeyKanban& other) const
{
    return this->data == other.data;
}

void PrivateKeyKanban::ToBytesWithPrefixAndSuffix(std::vector<unsigned char>& output) const
{
    output.clear();
    output.reserve(this->lengthKey + 2);
    output.push_back(Params().Base58Prefix(CChainParams::SECRET_KEY)[0]);
    //std::cout << "DEBUG: Private key prefix: " << (int) Params().Base58Prefix(CChainParams::SECRET_KEY)[0] << std::endl;
    for (unsigned i = 0; i < this->data.size();  i ++)
        output.push_back(this->data[i]);
    if (this->flagCorrespondingPublicKeyUsedWithCompressedEncoding)
        output.push_back(0x01);
}

bool PrivateKeyKanban::isInitialized()
{ return this->data.size() == this->lengthKey;
}

std::string PrivateKeyKanban::ToHex() const
{
    if (this->data.size() == 0)
        return "(uninitialized)";
    std::vector<unsigned char> prefixedData;
    this->ToBytesWithPrefixAndSuffix(prefixedData);
    return toStringHex(prefixedData);
}

std::string PrivateKeyKanban::ToBase58Check() const
{
    if (this->data.size() == 0)
        return "(uninitialized)";
    std::vector<unsigned char> prefixedData;
    this->ToBytesWithPrefixAndSuffix(prefixedData);
    return EncodeBase58Check(prefixedData);
}

std::string PrivateKeyKanban::ToBase58() const
{
    if (this->data.size() == 0)
        return "(uninitialized)";
    std::vector<unsigned char> prefixedData;
    this->ToBytesWithPrefixAndSuffix(prefixedData);
    return EncodeBase58(prefixedData);
}

bool PrivateKeyKanban::leftSmallerThanRightByPublicKeyCompressed(const PrivateKeyKanban& left, const PrivateKeyKanban& right)
{
    PublicKeyKanban leftPublic, rightPublic;
    left.ComputePublicKey(leftPublic, nullptr);
    right.ComputePublicKey(rightPublic, nullptr);
    return leftPublic < rightPublic;
}

bool PrivateKeyKanban::ComputePublicKey(PublicKeyKanban& output, std::stringstream* commentsOnFailure__SENSITIVE__NULL_SAFE) const
{
    if (this->data.size() != this->lengthKey) {
        std::cout << "Non-initialized private cannot be used to compute a public key. " << std::endl;
        assert(false);
    }
    secp256k1_context* theContext = Secp256k1::ContextForSigning();
    if (!secp256k1_ec_pubkey_create(theContext, &output.data, this->data.data())) {
        if (commentsOnFailure__SENSITIVE__NULL_SAFE != 0) {
            *commentsOnFailure__SENSITIVE__NULL_SAFE
            << "Failed to create public key from private key: "
            << this->ToBase58Check()
            << ". Typical causes: 1) the secret was not specified (hence initialized by zeroes, which is insecure); "
            << "2) very short secret (not enough bytes given). Did you initialize the secret key correctly? ";
        }
        return false;
    }
    //std::cout << "DEBUG:  public key computed. " << std::endl;

    return true;
}

bool PublicKeyKanban::operator<(const PublicKeyKanban& other) const
{
    std::vector<unsigned char> left, right;
    this->SerializeCompressed(left);
    other.SerializeCompressed(right);
    for (unsigned i = 0; i < left.size(); i ++) {
        if (left[i] < right[i]) {
            return true;
        }
        if (left[i] > right[i]) {
            return false;
        }
    }
    return false;
}

bool PublicKeyKanban::operator==(const PublicKeyKanban& other) const
{
    for (unsigned i = 0; i < this->lengthPublicKeyData; i ++)
        if (this->data.data[i] != other.data.data[i])
            return false;
    return true;
}

PublicKeyKanban::PublicKeyKanban()
{
    for (unsigned i = 0; i < this->lengthPublicKeyData; i ++)
        this->data.data[i] = 0;
}

PublicKeyKanban::PublicKeyKanban(const PublicKeyKanban& other)
{
    memcpy(&this->data, &other.data, sizeof(this->data));
}

PublicKeyKanban& PublicKeyKanban::operator=(const PublicKeyKanban& other)
{
    memcpy(&this->data, &other.data, sizeof(this->data));
    return *this;
}

bool PublicKeyKanban::Exponentiate(const PrivateKeyKanban& input, std::stringstream* commentsOnFailure)
{
    secp256k1_ge publicKeyEC, resultEC;
    if (!secp256k1_pubkey_load(Secp256k1::ContextForVerification(), &publicKeyEC, &this->data))
    {   if (commentsOnFailure != 0)
        {
            *commentsOnFailure << "Failed to load public key. ";
        }
        return false;
    }

    secp256k1_gej publicKeyECJacobian, resultJacobian;
    secp256k1_gej_set_ge(&publicKeyECJacobian, &publicKeyEC);
    secp256k1_scalar theExponent, zero;
    secp256k1_scalar_set_b32(&theExponent, input.data.data(), 0);
    secp256k1_scalar_set_int(&zero, 0);
    secp256k1_ecmult(
        &Secp256k1::ContextForVerification()->ecmult_ctx,
        &resultJacobian,
        &publicKeyECJacobian,
        &theExponent,
        &zero
    );
    secp256k1_ge_set_gej(&resultEC, &resultJacobian);
    secp256k1_pubkey_save(&this->data, &resultEC);
    return true;
}

bool PublicKeyKanban::MultiplyMany(const std::vector<PublicKeyKanban>& multiplicands, PublicKeyKanban& output)
{
    std::vector<const secp256k1_pubkey*> multiplicandPointers;
    multiplicandPointers.resize(multiplicands.size());
    for (unsigned i = 0; i < multiplicands.size(); i ++) {
        multiplicandPointers[i] = & multiplicands[i].data;
    }
    if (!secp256k1_ec_pubkey_combine(Secp256k1::ContextForVerification(), &output.data, multiplicandPointers.data(), multiplicands.size()))
        return false;
    return true;
}

bool PublicKeyKanban::operator*=(const PublicKeyKanban& other)
{ return this->Multiply(*this, other, *this);
}

bool PublicKeyKanban::Multiply(const PublicKeyKanban& left, const PublicKeyKanban& right, PublicKeyKanban& output)
{
    //std::cout << "DEBUG: Multiplying pub keys step 0. " << std::endl;
    //std::cout
    //<< "DEBUG: left, right are: "
    //<< left.ToHexCompressedWithLength() << right.ToHexCompressedWithLength() << std::endl;
    secp256k1_gej leftElementECJacobian, resultJacobian;
    secp256k1_ge rightElementEC, leftElementEC, result;

    secp256k1_pubkey_load(Secp256k1::ContextForVerification(), &rightElementEC, &right.data);
    secp256k1_pubkey_load(Secp256k1::ContextForVerification(), &leftElementEC, &left.data);
    //std::cout << "DEBUG: Multiplying pub keys step 1. " << std::endl;
    secp256k1_gej_set_ge(&leftElementECJacobian, &leftElementEC);
    secp256k1_gej_add_ge(&resultJacobian, &leftElementECJacobian, &rightElementEC);
    //std::cout << "DEBUG: Multiplying pub keys step 2. " << std::endl;
    secp256k1_ge_set_gej(&result, &resultJacobian);
    //std::cout << "DEBUG: Multiplying pub keys step 3. " << std::endl;
    secp256k1_pubkey_save(&output.data, &result);
    //std::cout << "DEBUG: Multiplying pub keys step 4. " << std::endl;
    return true;
}

void PublicKeyKanban::SerializeCompressed(std::vector<unsigned char>& output) const
{
    output.resize(33);
    size_t outSize = 33;
    secp256k1_ec_pubkey_serialize(Secp256k1::ContextForVerification(), output.data(), &outSize, &this->data, SECP256K1_EC_COMPRESSED);
    output.resize(outSize); //<- should not be needed
}

void PublicKeyKanban::SerializeUncompressed(std::vector<unsigned char>& output) const
{
    output.resize(65);
    size_t outSize = 65;
    secp256k1_ec_pubkey_serialize(Secp256k1::ContextForVerification(), output.data(), &outSize, &this->data, SECP256K1_EC_UNCOMPRESSED);
}

bool PublicKeyKanban::isInitialized() const
{
    for (int i = 0; i < this->lengthPublicKeyData; i ++)
        if (this->data.data[i] != 0)
            return true;
    return false;
}

bool PublicKeyKanban::MakeFromBytesSerialization(const std::vector<unsigned char>& serializer, std::stringstream *commentsOnFailure)
{
    return this->MakeFromBytesSerializationOffset(serializer, 0, serializer.size(), commentsOnFailure);
}

bool PublicKeyKanban::MakeFromBytesSerializationOffset
(const std::vector<unsigned char>& serializer, int offset, int length, std::stringstream* commentsOnFailure)
{
    if (((unsigned)length + offset) > serializer.size()) {
        std::cout << "Bad serializer length: " << serializer.size() << " with offset, length: "
        << offset << ", " << length << " in function MakeFromBytesSerializationOffset.";
        assert(false);
    }
    if (length != 33) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure
            << "While parsing public key, got " << serializer.size()
            << " bytes instead of the expected 33. ";
        }
        return false;
    }
    const unsigned char* dataStart = &serializer[offset];
    if (!secp256k1_ec_pubkey_parse(Secp256k1::ContextForVerification(), &this->data, dataStart, 33)) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure
            << "Function secp256k1_ec_pubkey_parse failed to parse "
            << (serializer.size() - offset) << " bytes: "
            << toStringHexOffset(serializer, offset) << ". ";
        }
        return false;
    }
    return true;

}

bool PublicKeyKanban::MakeFromUniValueRecognizeFormat(const UniValue& input, std::stringstream *commentsOnFailure)
{
    if (!input.isStr()) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "At the moment, only string inputs are supported. Instead, I got: " << input.write();
        }
        return false;
    }
    return this->MakeFromStringRecognizeFormat(input.get_str(), commentsOnFailure);
}

bool PublicKeyKanban::MakeFromStringRecognizeFormat(const std::string& input, std::stringstream* commentsOnFailure)
{
    std::vector<unsigned char> serializer;
    unsigned inputSize = input.size();
    if (inputSize == 66 || inputSize == 128 || inputSize == 130) {
        if (!fromHex(input, serializer, commentsOnFailure))
            return false;
    } else {
        serializer.resize(input.size());
        for (unsigned i = 0; i < input.size(); i ++) {
            serializer[i] = input[i];
        }
    }
    if (!this->MakeFromBytesSerialization(serializer, commentsOnFailure)) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "Failed to read public key from input: " << input;
        }
        return false;
    }
    return true;
}

std::string PublicKeyKanban::ToHexCompressed() const
{
    if (!this->isInitialized())
        return "(uninitialized)";
    std::vector<unsigned char> serialized;
    this->SerializeCompressed(serialized);
    return toStringHex(serialized);
}

std::string PublicKeyKanban::ToHexUncompressed() const
{
    if (!this->isInitialized())
        return "(uninitialized)";
    //<- leaving memory traces allowed.
    std::vector<unsigned char> serialized;
    this->SerializeUncompressed(serialized);
    return toStringHex(serialized);
}

std::string PublicKeyKanban::ToAddressUncompressedBase58() const
{
    if (!this->isInitialized())
        return "(uninitialized)";
    //<- leaving memory traces allowed.
//    CHash160().Write(this->data, vch.size()).Finalize(vchHash.data());
    std::cout << "Not implemented yet" << std::endl;
    assert(false);
}

std::string PublicKeyKanban::ToBytesUncompressed() const
{
    return std::string((char *) this->data.data, 64);
}

std::string PublicKeyKanban::ToBytesCompressed() const
{
    std::vector<unsigned char> buffer;
    this->SerializeCompressed(buffer);
    return std::string((char*) buffer.data(), buffer.size());
}

bool PrivateKeyKanban::MakeFromUniValueRecognizeFormat(const UniValue& input, std::stringstream *commentsOnFailure)
{
    if (!input.isStr()) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "I only know how to make secret (private key) from string univalue. The input I got was: " << input.write();
        }
        return false;
    }
    return this->MakeFromBase58DetectCheck(input.get_str(), commentsOnFailure);
}

std::string SignatureAggregate::ExampleRunSampleAggregateSignatureOutputHumanReadable()
{
    std::stringstream out;
    out << "Not implemented yet.";
    return out.str();
}

bool SignatureAggregate::VerifyFromSignatureComplete(const std::string& signatureComplete, const std::string& message, std::stringstream* reasonForFailure)
{
    if (! this->parseCompleteSignature(signatureComplete, reasonForFailure)) {
        return false;
    }
    this->messageImplied = message;
    return this->Verify(reasonForFailure, true);
}

bool SignatureAggregate::Verify(std::stringstream* reasonForFailure, bool detailsOnFailure)
{
    this->currentState = this->stateVerifyingAggregateSignatures;
    //first some basic checks
    if (this->allPublicKeys.size() < 1) {
        if (reasonForFailure != 0)
            *reasonForFailure << "I need at least 1 public key, got: " << this->allPublicKeys.size();
        return false;
    }
    if (this->committedSigners.size() != this->allPublicKeys.size()) {
        if (reasonForFailure != 0)
            *reasonForFailure
            << "I got " << this->allPublicKeys.size() << " public keys, but the committed signers bitmap has "
            << this->committedSigners.size() << " elements. ";
        return false;
    }
    int numSigners = 0;
    for (unsigned i = 0; i < this->committedSigners.size(); i ++)
        if (this->committedSigners[i])
            numSigners ++;
    if (numSigners == 0) {
        if (reasonForFailure != 0)
            *reasonForFailure << "I need at least 1 committed signer. The signer bitmap is: " << this->toStringSignersBitmap();
        return false;
    }
    this->solutionAggregate = this->serializerSignature.solution;
    this->commitmentAggregate = this->serializerSignature.challenge;
    //time to do the real work:
    return this->verifyPartTwo(reasonForFailure, detailsOnFailure);
}

bool SignatureAggregate::verifyPartTwo(std::stringstream* reasonForFailure, bool detailsOnFailure)
{
    //time to do the real work:
    std::sort(this->allPublicKeys.begin(), this->allPublicKeys.end());
    this->computeConcatenatedPublicKeys();
    if (!this->computeLockingCoefficients(reasonForFailure))
        return false;
    if (!this->computeAggregatePublicKey(reasonForFailure))
        return false;
    if (!this->computeMessageDigest(reasonForFailure))
        return false;
    PublicKeyKanban leftHandSide, rightHandSide;
    this->serializerSignature.solution.ComputePublicKey(leftHandSide, nullptr);
    bool found = false;
    for (unsigned i = 0; i < this->allPublicKeys.size(); i ++) {
        if (!this->committedSigners[i])
            continue;
        PublicKeyKanban currentContribution;
        currentContribution = this->allPublicKeys[i];
        if (!currentContribution.Exponentiate(this->allLockingCoefficients[i], reasonForFailure))
            return false;
        if (!found) {
            found = true;
            rightHandSide = currentContribution;
            continue;
        }
        rightHandSide *= currentContribution;
    }
    if (!rightHandSide.Exponentiate(this->digestedMessage, reasonForFailure))
        return false;
    rightHandSide *= this->serializerSignature.challenge;
    if (! (leftHandSide == rightHandSide)) {
        if (reasonForFailure != 0) {
            *reasonForFailure
            << "Signature not verified: all input was valid but the cryptography did not match.  ";
            if (detailsOnFailure) {
                *reasonForFailure
                        << "Message hex: " << HexStr(this->messageImplied) << ". Left hand side: "
                        << leftHandSide.ToHexCompressed() << " does not match the right-hand side: "
                        << rightHandSide.ToHexCompressed() << ". ";
            }
        }
        return false;
    }
    return true;
}

SignatureAggregate::SignatureAggregate()
{
    this->currentState = this->stateUninitialized;
    this->signerIndex = - 1;
    this->flagIsAggregator = false;
    this->flagIsSigner = false;
}

void SignatureAggregate::ResetNoPrivateKeyGeneration(bool isAggregator, bool isSigner)
{
    this->flagIsSigner = isSigner;
    this->flagIsAggregator = isAggregator;
    this->myPrivateKey__KEEP_SECRET.reset();
    this->currentState = this->stateUninitialized;
    this->signerIndex = - 1;
}

bool SignatureAggregate::leftHasSmallerPublicKey(const SignatureAggregate& left, const SignatureAggregate& right)
{
    return left.myPublicKey < right.myPublicKey;
}

std::string SignatureAggregate::toStringMyState()
{
    std::stringstream out;
    out << this->currentState << " (";
    switch (this->currentState) {
    case SignatureAggregate::state1ready:
        out << "ready";
        break;
    case SignatureAggregate::state2MessageSent:
        out << "message sent";
        break;
    case SignatureAggregate::state3MessageReceivedCommitmentsSent:
        out << "committed";
        break;
    case SignatureAggregate::state4CommitmentsReceivedChallengesSent:
        out << "challenges sent";
        break;
    case SignatureAggregate::state5ChallengesReceivedSolutionsSent:
        out << "solutions sent";
        break;
    case SignatureAggregate::state6SolutionsReceivedAggregateSignatureSent:
        out << "aggregated";
        break;
    case SignatureAggregate::stateUninitialized:
        out << "uninitialized";
        break;
    case SignatureAggregate::stateVerifyingAggregateSignatures:
        out << "verifying";
        break;
    default:
        out << "uknown";
        break;
    }
    out << ")";
    return out.str();
}
std::string SignatureAggregate::toStringSignersBitmap()
{
    std::stringstream out;
    for (unsigned i = 0; i < this->committedSigners.size(); i ++) {
        out << this->committedSigners[i];
    }
    return out.str();
}

void SignatureAggregate::toUniValueAppendMessage(UniValue& output)
{
    output.pushKV("message", this->messageImplied);
    output.pushKV("messageHex", HexStr(this->messageImplied));
}

void SignatureAggregate::toUniValueAppendPublicKeys(UniValue& output)
{
    if (this->flagIsSigner) {
        output.pushKV("myPublicKey", this->myPublicKey.ToHexCompressed());
        return;
    }
    if (this->flagIsAggregator || this->currentState == this->stateVerifyingAggregateSignatures) {
        UniValue publicKeys;
        std::vector<std::string> theStrings;
        for (unsigned i = 0; i < this->allPublicKeys.size(); i ++)
            theStrings.push_back(this->allPublicKeys[i].ToHexCompressed());
        this->toUniValueVectorStringsWithIgnoredStatus(theStrings, publicKeys);
        output.pushKV("publicKeys", publicKeys);
    }
}

void SignatureAggregate::toUniValueAppendCommitments(UniValue& output)
{
    if (this->flagIsSigner) {
        output.pushKV("myCommitment", this->myCommitment.ToHexCompressed());
        return;
    }
    if (this->flagIsAggregator) {
        UniValue uniCommitments;
        std::vector<std::string> theStrings;
        for (unsigned i = 0; i < this->allCommitments.size(); i ++) {
            if (!this->committedSigners[i]) {
                theStrings.push_back("not_available");
                continue;
            }
            theStrings.push_back(this->allCommitments[i].ToHexCompressed());
        }
        this->toUniValueVectorStringsWithIgnoredStatus(theStrings, uniCommitments);
        output.pushKV("commitments", uniCommitments);
    }
}

void SignatureAggregate::toUniValueAppendBitmapSigners(UniValue& output)
{
    output.pushKV("committedSignersBitmap", this->toStringSignersBitmap());
}

void SignatureAggregate::toUniValueAppendSignatureSerializationBase58WithoutCheckNoBitmap(UniValue& output)
{
    output.pushKV("signatureNoBitmap", this->serializerSignature.ToBase58CustomPrefix(24));
}

void SignatureAggregate::toUniValueAppendSignatureComplete(UniValue& output)
{
    output.pushKV("signatureComplete", HexStr(this->aggregateSignatureComplete));
    output.pushKV("signatureUncompressed", HexStr(this->aggregateSignatureUncompressed));
}

void SignatureAggregate::toUniValueAppendMessageDigest(UniValue& output)
{
    output.pushKV("messageDigest", this->digestedMessage.ToBase58());
}

void SignatureAggregate::toUniValueConcatenatedPublicKeys(UniValue& output)
{
    std::string converted(this->committedPublicKeysSortedConcatenated.begin(), this->committedPublicKeysSortedConcatenated.end());
    output.pushKV("concatenatedPublicKeys", EncodeBase64(converted));
}

void SignatureAggregate::toUniValueAppendAggregatePublicKey(UniValue& output)
{
    output.pushKV("aggregatePublicKey", this->publicKeyAggregate.ToHexCompressed());
}

void SignatureAggregate::toUniValueAppendAggregateCommitment(UniValue& output)
{
    output.pushKV("aggregateCommitment", this->commitmentAggregate.ToHexCompressed());
}

void SignatureAggregate::toUniValueAppendAggregateCommitmentFromSignature(UniValue& output)
{
    output.pushKV("aggregateCommitmentFromSignature", this->serializerSignature.challenge.ToHexCompressed());
}

void SignatureAggregate::toUniValueAppendMyLockingCoefficient(UniValue& output)
{
    output.pushKV("myLockingCoefficient", this->myLockingCoefficient.ToBase58Check());
}

void SignatureAggregate::toUniValueAppendMySolution(UniValue& output)
{
    output.pushKV("mySolution", this->mySolution.ToBase58Check());
}

void SignatureAggregate::toUniValueAppendAggregateSolution(UniValue& output)
{
    output.pushKV("aggregateSolution", this->solutionAggregate.ToBase58Check());
}

void SignatureAggregate::toUniValueVectorStringsWithIgnoredStatus(const std::vector<std::string>& input, UniValue& output)
{
    output.setArray();
    for (unsigned i = 0; i < input.size(); i ++){
        bool isGood = true;
        if (i < this->committedSigners.size())
            isGood = this->committedSigners[i];
        std::string theCoefficient = input[i];
        if (isGood) {
            output.push_back(theCoefficient);
        } else {
            UniValue oneEntry;
            oneEntry.setObject();
            oneEntry.pushKV("ignored", theCoefficient);
            output.push_back(oneEntry);
        }
    }
}

void SignatureAggregate::toUniValueAppendAllLockingCoefficients(UniValue& output)
{
    UniValue lockingCoefficientsUni;
    std::vector<std::string> theStrings;
    for (unsigned i = 0; i < this->allLockingCoefficients.size(); i ++)
        theStrings.push_back(this->allLockingCoefficients[i].ToBase58());
    this->toUniValueVectorStringsWithIgnoredStatus(theStrings, lockingCoefficientsUni);
    output.pushKV("lockingCoefficients", lockingCoefficientsUni);
}

void SignatureAggregate::toUniValueAppendAllSolutions(UniValue& output)
{
    UniValue solutionsUni;
    std::vector<std::string> theStrings;
    for (unsigned i = 0; i < this->allSolutions.size(); i ++)
        theStrings.push_back(this->allSolutions[i].ToBase58Check());
    this->toUniValueVectorStringsWithIgnoredStatus(theStrings, solutionsUni);
    output.pushKV("solutions", solutionsUni);
}

UniValue SignatureAggregate::toUniValueTransitionState__SENSITIVE()
{
    UniValue result;
    result.setObject();
    //std::cout << "DEBUG: assembling univalue ... Signer index: " << this->signerIndex << std::endl;
    if (!this->flagIsSigner && !this->flagIsAggregator) {
        result.pushKV("role", "undefined");
    }
    if (this->flagIsSigner && !this->flagIsAggregator) {
        result.pushKV("role", "signer");
    }
    if (!this->flagIsSigner && this->flagIsAggregator) {
        result.pushKV("role", "aggregator");
    }
    if (this->flagIsSigner && this->flagIsAggregator) {
        result.pushKV("role", "signer+aggregator");
    }
    if (this->allPublicKeys.size() > 0) {
        result.pushKV("knownPublicKeys", (int64_t) this->allPublicKeys.size());
    }
    result.pushKV("state", this->toStringMyState());
    if (this->currentState == this->state1ready) {
        if (this->flagIsSigner) {
            result.pushKV("privateKeyBase58", this->myPrivateKey__KEEP_SECRET.ToBase58());
        }
        this->toUniValueAppendPublicKeys(result);
        return result;
    }
    if (this->currentState == this->state2MessageSent) {
        this->toUniValueAppendMessage(result);
        return result;
    }
    if (this->currentState == this->state3MessageReceivedCommitmentsSent) {
        this->toUniValueAppendMessage(result);
        result.pushKV("myNonceBase58", this->myNonce_DO_Not_Disclose_To_Aggregator.ToBase58());
        result.pushKV("commitmentHexCompressed", this->myCommitment.ToHexCompressed());
        this->toUniValueAppendPublicKeys(result);
        return result;
    }
    if (this->currentState == this->state4CommitmentsReceivedChallengesSent) {
        this->toUniValueAppendPublicKeys(result);
        this->toUniValueAppendCommitments(result);
        this->toUniValueAppendBitmapSigners(result);
        this->toUniValueAppendAggregatePublicKey(result);
        this->toUniValueAppendAggregateCommitment(result);
        this->toUniValueAppendMessageDigest(result);
        this->toUniValueAppendAllLockingCoefficients(result);
    }
    if (this->currentState == this->state5ChallengesReceivedSolutionsSent) {
        this->toUniValueAppendMyLockingCoefficient(result);
        this->toUniValueAppendMySolution(result);
    }
    if (this->currentState == this->state6SolutionsReceivedAggregateSignatureSent) {
        this->toUniValueAppendMessage(result);
        this->toUniValueAppendAllLockingCoefficients(result);
        //this->ToUniValueAppendAllSolutions(result);
        this->toUniValueAppendMessageDigest(result);
        this->toUniValueAppendAggregatePublicKey(result);
        this->toUniValueAppendPublicKeys(result);
        this->toUniValueAppendAggregateSolution(result);
        this->toUniValueAppendAggregateCommitment(result);
        this->toUniValueAppendAggregateCommitmentFromSignature(result);
        this->toUniValueAppendSignatureSerializationBase58WithoutCheckNoBitmap(result);
        this->toUniValueAppendSignatureComplete(result);
        this->toUniValueAppendBitmapSigners(result);
    }
    if (this->currentState == this->stateVerifyingAggregateSignatures) {
        this->toUniValueAppendMessage(result);
        this->toUniValueAppendAllLockingCoefficients(result);
        this->toUniValueConcatenatedPublicKeys(result);
        this->toUniValueAppendMessageDigest(result);
        this->toUniValueAppendAggregatePublicKey(result);
        this->toUniValueAppendPublicKeys(result);
        this->toUniValueAppendAggregateSolution(result);
        this->toUniValueAppendAggregateCommitment(result);
        this->toUniValueAppendAggregateCommitmentFromSignature(result);
        this->toUniValueAppendSignatureSerializationBase58WithoutCheckNoBitmap(result);
        this->toUniValueAppendSignatureComplete(result);
        this->toUniValueAppendBitmapSigners(result);
    }
    return result;
}

bool SignatureAggregate::TransitionAggregatorState1ToState2(const std::string& message, std::stringstream* commentsOnFailure)
{
    if (!this->flagIsAggregator) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "Transition from state 1 (ready) to state 2 "
            << "(message sent by aggregator) requires that the node be an aggregator.";
        }
        return false;
    }
    if (this->currentState != this->state1ready) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "Transition from state 1 (ready) to state 2 "
            << "requires that the current state be state 1. Current state: "
            << this->toStringMyState() << ". ";
        }
        return false;
    }
    ///Actual work starts here.
    this->messageImplied = message;
    this->currentState = this->state2MessageSent;
    return true;
    ///After returning, please dispatch the message to the signers.
}

bool SignatureAggregate::TransitionSignerState1or2ToState3
(const std::string& message,
 std::stringstream* commentsOnFailure,
 PrivateKeyKanban* desiredNonce__NULL_SAFE__ALL_OTHER_VALUES_CRITICAL_SECURITY_RISK)
{
    if (!this->flagIsSigner) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "Transition to state 3 "
            << "(message received by signer, commitment sent) "
            << "requires that the node be a signer without being an aggregator.";
        }
        return false;
    }
    if (this->currentState != this->state1ready && this->currentState != this->state2MessageSent) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "Transition from state 1 (ready) or 2 (message sent) to state 3 "
            << "(message received by signer) requires that the current state be state 1 or 2. Current state: "
            << this->toStringMyState();
        }
        return false;
    }
    ///Actual work starts here.
    this->messageImplied = message;
    if (desiredNonce__NULL_SAFE__ALL_OTHER_VALUES_CRITICAL_SECURITY_RISK != 0)
        this->myNonce_DO_Not_Disclose_To_Aggregator = *desiredNonce__NULL_SAFE__ALL_OTHER_VALUES_CRITICAL_SECURITY_RISK;
    else
        this->myNonce_DO_Not_Disclose_To_Aggregator.GenerateRandomSecurely();
    if (!this->myNonce_DO_Not_Disclose_To_Aggregator.ComputePublicKey(this->myCommitment, nullptr))
    {
        if (commentsOnFailure != 0)
        {
            if (desiredNonce__NULL_SAFE__ALL_OTHER_VALUES_CRITICAL_SECURITY_RISK !=0)
                *commentsOnFailure << "The nonce you specified is bad (does not properly generate a public key). ";
            else
                *commentsOnFailure << "This shouldn't happen: I generated a bad nonce. This is likely a programming error. ";
        }
        return false;
    }
    this->currentState = this->state3MessageReceivedCommitmentsSent;
    return true;
    ///After returning, please dispatch the commitment to the aggregator.
}

bool SignatureAggregate::checkCorrectCommittedSigners(std::stringstream* commentsOnFailure)
{
    //Check that the committed signers bitmap has the correct size
    if (this->committedSigners.size() != this->allPublicKeys.size()) {
        if (commentsOnFailure != nullptr) {
            *commentsOnFailure
            << "The committed signers bitmap has " << this->allCommitments.size()
            << " entries instead of the desired " << this->allPublicKeys.size() << ". ";
        }
        return false;
    }
    //If fewer commitments than required are given,
    //beef up the commitments by empty public keys.
    if (this->allCommitments.size() < this->allPublicKeys.size()) {
        int numCommitmentsReceived = this->allCommitments.size();
        this->allCommitments.resize(this->allPublicKeys.size());
        for (unsigned i = numCommitmentsReceived; i < this->allCommitments.size(); i ++) {
            this->committedSigners[i] = false; //<- ensure the missing commitments are set to false
        }
    }
    if (this->allCommitments.size() > this->allPublicKeys.size()) {
        if (commentsOnFailure != nullptr) {
            *commentsOnFailure
            << "I received more commitments: " << this->allCommitments.size()
            << " than public keys I have on record: " << this->allPublicKeys.size() << ". ";
        }
        return false;
    }
    bool found = false;
    for (unsigned i = 0; i < this->committedSigners.size(); i ++) {
        if (!this->committedSigners[i])
            continue;
        found = true;
        if (i >= this->allCommitments.size()) {
            if (commentsOnFailure != 0) {
                *commentsOnFailure << "The committed signers bitmap claims signer index " << i << " is committed to sign, but "
                << " I received only " << this->allCommitments.size() << " commitments. ";
            }
            return false;
        }
    }
    if (!found)
        if (commentsOnFailure != 0)
            *commentsOnFailure << "I could not find any committed signers";
    return found;
}

bool SignatureAggregate::TransitionSignerState2or3ToState4
(const std::vector<PublicKeyKanban>& inputCommitments,
 const std::vector<bool>& inputCommittedSigners,
 const std::vector<PublicKeyKanban>* publicKeys__null_allowed,
 std::stringstream* commentsOnFailure)
{
    //This part initializes and sanitizes inputs, does sanity checks, etc.
    if (!this->flagIsAggregator) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "This step is to be carried out by an aggregator. ";
        }
        return false;
    }
    if (this->currentState != this->state2MessageSent && this->currentState != this->state3MessageReceivedCommitmentsSent) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "Commitment step is to be carried out on a signature protocol "
            << "in state 2 (message sent) or state 3 (commitment sent). Current state: "
            << this->toStringMyState() << ". ";
        }
        return false;
    }
    this->committedSigners = inputCommittedSigners;
    this->allCommitments = inputCommitments;
    if (!this->checkCorrectCommittedSigners(commentsOnFailure))
        return false;
    //the real work is done in the next part:
    return this->transitionSignerState2or3ToState4PartTwo(commentsOnFailure);
}

bool SignatureAggregate::transitionSignerState2or3ToState4PartTwo(std::stringstream* commentsOnFailure)
{
    //The real work starts here.
    if (!this->computeAggregateCommitment(commentsOnFailure))
        return false;
    this->computeConcatenatedPublicKeys();
    if (!this->computeLockingCoefficients(commentsOnFailure))
        return false;
    if (!this->computeAggregatePublicKey(commentsOnFailure))
        return false;
    if (!this->computeMessageDigest(commentsOnFailure))
        return false;
    this->currentState = this->state4CommitmentsReceivedChallengesSent;
    return true;
    ///After returning, please dispatch the challenge to the signers.
}

bool SignatureAggregate::computeErrorChecksChallenge(std::stringstream* commentsOnFailure)
{
    this->computeConcatenatedPublicKeys();
    if (!this->computeLockingCoefficients(commentsOnFailure)) {
        //<-To do: we only need to compute the locking coefficient of the current node (this->signerIndex).
        return false;
    }
    PublicKeyKanban oldAggregatePublicKey = this->publicKeyAggregate;
    if (!this->computeAggregatePublicKey(commentsOnFailure))
        return false;
    if (!(oldAggregatePublicKey == this->publicKeyAggregate)) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure
            << "The received aggregate public key: " << oldAggregatePublicKey.ToHexCompressed()
            << " does not match the computed aggregate public key: "
            << this->publicKeyAggregate.ToHexCompressed() << ". ";
        }
        return false;
    }
    PrivateKeyKanban oldDigest = this->digestedMessage;
    if (!this->computeMessageDigest(commentsOnFailure))
        return false;
    if (!(oldDigest == this->digestedMessage)) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure
            << "The received challenge: " << oldDigest.ToBase58Check()
            << " does not match the computed challenge: "
            << this->digestedMessage.ToBase58Check() << ". ";
        }
        return false;
    }
    return true;
}

bool SignatureAggregate::computeMySolution(std::stringstream* commentsOnFailure)
{
    //std::cout << "DEBUG:: about to compute solution. challenge: " << this->challenge.ToHex() << std::endl;
    this->mySolution = this->digestedMessage;
    //std::cout << "DEBUG:: computing solution. mylocking coeff: " << this->myLockingCoefficient.ToHex() << std::endl;
    if (!this->myLockingCoefficient.isInitialized()) {
        if (commentsOnFailure != nullptr) {
            *commentsOnFailure << "My locking coefficient not initialized. ";
        }
        return false;
    }
    this->mySolution.MultiplyMeBy(this->myLockingCoefficient, false);
    //std::cout << "DEBUG:: computing solution. myPrivateKey__KEEP_SECRET: " << this->myPrivateKey__KEEP_SECRET.ToHex() << std::endl;
    this->mySolution.MultiplyMeBy(this->myPrivateKey__KEEP_SECRET, true);
    //std::cout << "DEBUG:: computing solution. myNonce_DO_Not_Disclose_To_Aggregator: " << this->myNonce_DO_Not_Disclose_To_Aggregator.ToHex() << std::endl;
    this->mySolution.AddToMe(this->myNonce_DO_Not_Disclose_To_Aggregator, true);
    return true;
}

bool SignatureAggregate::TransitionSignerState4or5ToState6(const std::vector<PrivateKeyKanban>& inputSolutions, std::stringstream* commentsOnFailure)
{
    if (this->currentState != this->state5ChallengesReceivedSolutionsSent &&
        this->currentState != this->state4CommitmentsReceivedChallengesSent
    ) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "Final signature aggregation requires that the present state be 4 (aggregator has sent challenges) or 5 "
            << "(signer has computed solutions).";
        }
        return false;
    }
    if (inputSolutions.size() > this->allPublicKeys.size()) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure
            << "Too many solutions: " << inputSolutions.size() << ". There are only: "
            << this->allPublicKeys.size() << " public keys. ";
        }
        return false;
    }
    if (inputSolutions.size() < 1) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "At least one solution required (0 provided). ";
        }
        return false;
    }
    this->allSolutions = inputSolutions;
    this->solutionAggregate.MakeZero();
    for (unsigned i = 0; i < this->allCommitments.size(); i ++) {
        if (!this->committedSigners[i])
            continue;
        this->solutionAggregate += this->allSolutions[i];
    }
    this->serializerSignature.challenge = this->commitmentAggregate;
    this->serializerSignature.solution = this->solutionAggregate;
    this->currentState = this->state6SolutionsReceivedAggregateSignatureSent;
    return true;
    ///After returning, the aggregate solution is complete.
}

bool SignatureAggregate::transitionSignerState3or4ToState5PartTwo(std::stringstream* commentsOnFailure)
{
    //The real work starts here.
    //std::cout << "DEBUG: got to before computeErrorChecksChallenge" << std::endl;
    if (!this->computeErrorChecksChallenge(commentsOnFailure))
        return false;
    //std::cout << "DEBUG: got to before computeSolution" << std::endl;
    if (!this->computeMySolution(commentsOnFailure))
        return false;
    //std::cout << "DEBUG: got to end" << std::endl;
    ///Here we need to dispatch the solution to the aggregator.
    /// ... not implemented yet ...
    this->currentState = this->state5ChallengesReceivedSolutionsSent;
    return true;
}

bool SignatureAggregate::TransitionSignerState3or4ToState5
(const std::vector<bool>& inputcommittedSigners,
 PrivateKeyKanban& inputChallenge,
 PublicKeyKanban& inputAggregateCommitment,
 PublicKeyKanban& inputAggregatePublicKey,
 std::stringstream* commentsOnFailure)
{
    //This part initializes and sanitizes inputs, does sanity checks, etc.
    if (this->currentState != this->state3MessageReceivedCommitmentsSent &&
        this->currentState != this->state4CommitmentsReceivedChallengesSent) {
        if (commentsOnFailure != 0)
            *commentsOnFailure << "Solutions computation allowed only when the node is a signer in state 3 (commitments sent) or 4 (challenges sent)";
        return false;
    }
    this->committedSigners = inputcommittedSigners;
    this->digestedMessage = inputChallenge;
    this->commitmentAggregate = inputAggregateCommitment;
    this->publicKeyAggregate = inputAggregatePublicKey;
    if (this->committedSigners.size() != this->allPublicKeys.size()) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure
            << "committed signers bitmap is required to have " << this->allPublicKeys.size() << " elements but it has "
            << this->committedSigners.size() << ". ";
        }
        return false;
    }
    if (this->signerIndex < 0 || this->signerIndex >= (signed) this->committedSigners.size()) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure
            << "My signer index: " << this->signerIndex << " is not valid. There are: "
            << this->committedSigners.size() << " total signers. ";
        }
        return false;
    }
    if (!this->committedSigners[this->signerIndex]) {
        std::cout << "Unusual behavior: I am asked to compute a solution although I am not a committed signer. My signer index: "
        << this->signerIndex << ". " << std::endl;
        this->currentState = this->state5ChallengesReceivedSolutionsSent;
        return true;
    }
    return this->transitionSignerState3or4ToState5PartTwo(commentsOnFailure);
}


bool SignatureAggregate::computeMessageDigest(std::stringstream* commentsOnFailure)
{
    this->challengeDigester.init();
    std::vector<unsigned char> serializationAggregateKey, serializationCommitment, challengeDigestResult;
    this->publicKeyAggregate.SerializeCompressed(serializationAggregateKey);
    this->challengeDigester.update(serializationAggregateKey);
    this->commitmentAggregate.SerializeCompressed(serializationCommitment);
    this->challengeDigester.update(serializationCommitment);
    this->challengeDigester.update(this->messageImplied);
    this->challengeDigester.finalize();
    this->challengeDigester.getResultVector(challengeDigestResult);
    bool mustBeTrue = this->digestedMessage.MakeFromByteSequence(challengeDigestResult, commentsOnFailure);
    if (!mustBeTrue)  {
        std::cout << "Failed to construct challenge. This should not happen.";
        assert(false);
    }
    return true;
}

bool SignatureAggregate::computeAggregatePublicKey(std::stringstream* commentsOnFailure)
{
    bool found = false;
    PublicKeyKanban currentContribution;
    for (unsigned i = 0; i < this->committedSigners.size(); i ++) {
        if (!this->committedSigners[i])
            continue;
        currentContribution = this->allPublicKeys[i];
        bool shouldBeTrue = currentContribution.Exponentiate(this->allLockingCoefficients[i], commentsOnFailure);
        if (!shouldBeTrue) {
            if (commentsOnFailure != 0) {
                *commentsOnFailure << "Failed to exponentiate public key index " << i << ".";
            }
            return false;
        }
        if (!found) {
            found = true;
            this->publicKeyAggregate = currentContribution;
            continue;
        }
        shouldBeTrue = (this->publicKeyAggregate*=(currentContribution));
        if (!shouldBeTrue) {
            if (commentsOnFailure != 0) {
                *commentsOnFailure << "Failed to aggregate public key index " << i << ".";
            }
            return false;
        }
    }
    if (!found) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure << "This should not happen at this point of code: did not find committed a committed signer. ";
        }
        assert(false);
        return false;
    }
    return found;
}

void SignatureAggregate::computeConcatenatedPublicKeys()
{
    this->committedPublicKeysSortedConcatenated.reserve(this->allPublicKeys.size() * PublicKeyKanban::lengthPublicKeyData);
    this->committedPublicKeysSortedConcatenated.clear();
    std::vector<unsigned char> serialization;
    for (unsigned i = 0; i < this->committedSigners.size(); i ++) {
        if (! this->committedSigners[i])
            continue;
        this->allPublicKeys[i].SerializeCompressed(serialization);
        for (unsigned k = 0; k < serialization.size(); k ++)
            this->committedPublicKeysSortedConcatenated.push_back(serialization[k]);
    }
}

bool SignatureAggregate::computeLockingCoefficientsOne(std::stringstream *commentsOnFailure)
{
    for (unsigned i = 0; i < this->committedSigners.size(); i ++) {
        if (!this->committedSigners[i]) {
            continue;
        }
        this->allLockingCoefficients[i].MakeSmallInt(1);
        return true;
    }
    if (commentsOnFailure != 0) {
        *commentsOnFailure << "Fatal error: failed to find a committed signer. ";
    }
    return false;
}

bool SignatureAggregate::computeLockingCoefficientsMultiple(std::stringstream *commentsOnFailure)
{
    if (this->allLockingCoefficients.size() <= 1) {
        std::cout << "Fatal error: this function should be called only for two or more public keys." << std::endl;
        assert(false);
    }
    this->lockingCoefficientPrecomputedData.init();
    this->lockingCoefficientPrecomputedData.update(this->committedPublicKeysSortedConcatenated);
    std::vector<unsigned char> publicKeySerialization, sha3Result;
    for (unsigned i = 0; i < this->committedSigners.size(); i ++) {
        if (!this->committedSigners[i])
            continue;
        memcpy(&this->lockingCoefficientComputer, &this->lockingCoefficientPrecomputedData, sizeof(this->lockingCoefficientPrecomputedData));
        this->allPublicKeys[i].SerializeCompressed(publicKeySerialization);
        this->lockingCoefficientComputer.update(publicKeySerialization);
        this->lockingCoefficientComputer.finalize();
        this->lockingCoefficientComputer.getResultVector(sha3Result);
        bool shouldBeTrue =
        this->allLockingCoefficients[i].MakeFromByteSequence(sha3Result, commentsOnFailure);
        if (!shouldBeTrue) {
            std::cout << "Programming error: failed to make locking coefficient. " << std::endl;
            assert(false);
        }
    }
    return true;
}

int SignatureAggregate::GetNumberOfCommittedSigners()
{
    int result = 0;
    for (unsigned i = 0; i < this->committedSigners.size(); i ++) {
        if (this->committedSigners[i]) {
            result ++;
        }
    }
    return result;
}

bool SignatureAggregate::computeLockingCoefficients(std::stringstream* commentsOnFailure)
{
    this->allLockingCoefficients.resize(this->allPublicKeys.size());
    bool isGood = true;
    if (this->GetNumberOfCommittedSigners() == 1)
        isGood = this->computeLockingCoefficientsOne(commentsOnFailure);
    else
        isGood = this->computeLockingCoefficientsMultiple(commentsOnFailure);
    if (!isGood)
        return false;
    if (this->signerIndex >= 0 && this->signerIndex < (signed) this->allLockingCoefficients.size()) {
        this->myLockingCoefficient = this->allLockingCoefficients[this->signerIndex];
    }
    return true;
}

bool SignatureAggregate::computeAggregateCommitment(std::stringstream* commentsOnFailure)
{
    bool found = false;
    if (this->allCommitments.size() != this->committedSigners.size()) {
        std::cout
        << "Size of all commitments: " << this->allCommitments.size()
        << " should equal the size of the committed signers bitmap: "
        << this->committedSigners.size() << ". " << std::endl;
        assert(false);
    }
    for (unsigned i = 0; i < this->allCommitments.size(); i ++) {
        if (!this->committedSigners[i])
            continue;
        if (!found) {
            this->commitmentAggregate = this->allCommitments[i];
            found = true;
            continue;
        }
        if (!(this->commitmentAggregate *= this->allCommitments[i])) {
            if (commentsOnFailure != 0) {
                *commentsOnFailure << "Aggregation of commitments failed. ";
            }
            return false;
        }
    }
    return true;
}

bool SignatureAggregate::InitializePublicKeys(const std::vector<PublicKeyKanban>& registeredSigners, std::stringstream* commentsOnFailure)
{
    this->allPublicKeys = registeredSigners;
    std::sort(this->allPublicKeys.begin(), this->allPublicKeys.end());
    this->currentState = this->state1ready;
    this->signerIndex = - 1;
    if (this->flagIsSigner) {
        for (unsigned i = 0; i < this->allPublicKeys.size(); i ++) {
            if (this->myPublicKey == this->allPublicKeys[i]) {
                this->signerIndex = i;
                break;
            }
        }
        if (this->signerIndex == - 1) {
            if (commentsOnFailure != 0)
                *commentsOnFailure << "Signer node could not find its public key in the list of all public keys. ";
            return false;
        }
    }
    return true;
}

bool SignatureAggregate::ResetGeneratePrivateKey(bool isAggregator, bool isSigner)
{
    this->ResetNoPrivateKeyGeneration(isAggregator, isSigner);
    while (this->flagIsSigner) {
        this->myPrivateKey__KEEP_SECRET.GenerateRandomSecurely(); //<- should always return true
        if (this->myPrivateKey__KEEP_SECRET.ComputePublicKey(this->myPublicKey, nullptr))
            break;
    }
    return true;
}

bool SignatureAggregate::parseCompleteSignature(const std::vector<unsigned char>& signatureComplete, std::stringstream* reasonForFailure)
{
    std::string stringCopy((const char *) signatureComplete.data(), signatureComplete.size());
    return this->parseCompleteSignature(stringCopy, reasonForFailure);
}

bool SignatureAggregate::ParsePublicKeysAndInitialize(const std::string& publicKeySerialization, std::stringstream *reasonForFailure)
{
    if (publicKeySerialization.size() < 2) {
        if (reasonForFailure != nullptr) {
            *reasonForFailure << "Public key serialization needs to have at least 35 bytes, instead I got: "
                              << publicKeySerialization.size() << ". ";
        }
        return false;
    }
    unsigned lengthCompressedPublicKey = 33;
    std::string publicKeyBytes = publicKeySerialization.substr(2);
    unsigned expectedNumberOfPublicKeys = unsigned(publicKeySerialization[0]) * 256 + unsigned(publicKeySerialization[1]);
    unsigned expectedPublicKeyLength = lengthCompressedPublicKey * expectedNumberOfPublicKeys;
    if (expectedPublicKeyLength != publicKeyBytes.size()) {
        if (reasonForFailure != 0) {
            *reasonForFailure << "PubKeys first two bytes: "
                              << expectedNumberOfPublicKeys << " pub keys expected, total "
                              << expectedNumberOfPublicKeys << "*33 = "
                              << " bytes; got "
                              << publicKeyBytes.size()
                              <<   " bytes instead.";
        }
        return false;
    }
    unsigned offset = 0;
    std::vector<PublicKeyKanban> thePublicKeys;
    thePublicKeys.resize(expectedNumberOfPublicKeys);
    for (unsigned i = 0; i < expectedNumberOfPublicKeys; i ++) {
        std::string currentKeyString = publicKeyBytes.substr(offset, lengthCompressedPublicKey);
        if (!thePublicKeys[i].MakeFromStringRecognizeFormat(currentKeyString, reasonForFailure)) {
            if (reasonForFailure != 0) {
                *reasonForFailure << "Failed to read public key index "
                                  << i
                                  << " ("
                                  << i + 1
                                  << " out of "
                                  << expectedNumberOfPublicKeys
                                  << "). ";
            }
            return false;
        }
        offset += lengthCompressedPublicKey;
    }
    if (!this->InitializePublicKeys(thePublicKeys, reasonForFailure)) {
        if (reasonForFailure != nullptr) {
            *reasonForFailure << "While parsing aggregate signature: failed to initialize the public keys. ";
        }
        return false;
    }
    return true;
}

bool SignatureAggregate::ParseUncompressedSignature(const std::string& uncompressedSignature, std::stringstream* reasonForFailure)
{
    unsigned lengthSchnorr = 66;
    unsigned expectedSize = lengthSchnorr + this->lengthInBytesBitmapSigners;
    if (uncompressedSignature.size() < expectedSize && uncompressedSignature.size() != lengthSchnorr) {
        if (reasonForFailure != nullptr) {
            *reasonForFailure << "Uncompressed aggregate signature is expected to have at least " << expectedSize
                              << " bytes, but instead it has: " << uncompressedSignature.size() << ". ";
        }
        return false;
    }
    std::string signatureUncompressedString = uncompressedSignature.substr(0, lengthSchnorr);
    std::vector<unsigned char> signatureUncompressedBytes;
    signatureUncompressedBytes.resize(signatureUncompressedString.size());
    for (unsigned i = 0; i < signatureUncompressedString.size(); i ++) {
        signatureUncompressedBytes[i] = signatureUncompressedString[i];
    }
    if (!this->serializerSignature.MakeFromBytes(signatureUncompressedBytes, reasonForFailure)) {
        if (reasonForFailure != nullptr) {
            *reasonForFailure << "Failed to parse the (challenge, solution) part of the aggregate signature. ";
        }
        return false;
    }
    if (uncompressedSignature.size() == lengthSchnorr) {
        this->committedSigners.resize(this->allPublicKeys.size());
        for (unsigned i = 0; i < this->allPublicKeys.size(); i ++) {
            this->committedSigners[i] = true;
        }
    } else {
        std::string signersBitmap = uncompressedSignature.substr(lengthSchnorr, this->lengthInBytesBitmapSigners);
        //if (reasonForFailure != 0) {
        //   *reasonForFailure << "DEBUG: signers bitmap: " << HexStr(signersBitmap) << ". ";
        //}
        if (!this->deserializeSignersBitmapFromBigEndianBits(signersBitmap, reasonForFailure)) {
            return false;
        }
        //if (reasonForFailure != 0) {
        //   *reasonForFailure << "DEBUG: after deserialize: signers bitmap: " << this->toStringSignersBitmap() << ". ";
        //}
    }
    return true;
}

bool SignatureAggregate::parseCompleteSignature(const std::string& signatureComplete, std::stringstream* reasonForFailure)
{
    unsigned lengthCompressedPublicKey = 33;
    unsigned lengthSchnorr = 66;
    unsigned lengthRawSignatureUncompressed = lengthSchnorr + this->lengthInBytesBitmapSigners;
    unsigned minimumAllowedLength = lengthRawSignatureUncompressed + 2 + lengthCompressedPublicKey;
    unsigned maxAllowedLength = lengthRawSignatureUncompressed + 2 + lengthCompressedPublicKey * 256;
    if (signatureComplete.size() < minimumAllowedLength || signatureComplete.size() > maxAllowedLength) {
        if (reasonForFailure != 0) {
            *reasonForFailure << "Complete signature needs to have at least "
                              << minimumAllowedLength
                              << ", at most "
                              << maxAllowedLength
                              << " bytes, but the one given has "
                              << signatureComplete.size()
                              << ". ";
        }
        return false;
    }
    std::string publicKeySerialization = signatureComplete.substr(lengthRawSignatureUncompressed);
    if (!this->ParsePublicKeysAndInitialize(publicKeySerialization, reasonForFailure)) {
        return false;
    }
    std::string uncompressedSignatureSerialization = signatureComplete.substr(0, lengthRawSignatureUncompressed);
    if (!this->ParseUncompressedSignature(uncompressedSignatureSerialization, reasonForFailure)) {
        return false;
    }
    return true;
}

bool SignatureAggregate::deserializeSignersBitmapFromBigEndianBits(const std::string& inputRaw, std::stringstream* reasonForFailure)
{
    if (inputRaw.size() != this->lengthInBytesBitmapSigners) {
        if (reasonForFailure != 0) {
            *reasonForFailure << "Signer bitmap has: " << inputRaw.size()
                              << " bytes, instead of the expected: " << this->lengthInBytesBitmapSigners << ". ";
        }
        return false;
    }
    unsigned numberOfSigners = this->allPublicKeys.size();
    if (this->committedSigners.size() != numberOfSigners) {
        this->committedSigners.resize(numberOfSigners);
    }
    for (unsigned i = 0; i < numberOfSigners; i ++) {
        unsigned byteIndex = i / 8;
        unsigned bitOffset = i % 8;
        unsigned char currentByte = inputRaw[byteIndex];
        unsigned char shiftedLeft = currentByte << bitOffset;
        unsigned char bitOfInterestInLastPosition = shiftedLeft >> 7;
        this->committedSigners[i] = (bitOfInterestInLastPosition == 1);
        //if (reasonForFailure != 0) {
        //    std::string temp;
        //    temp = currentByte;
        //    *reasonForFailure << "DEBUG: byte current: " << HexStr(temp)
        //                      << ", shiftedLeft: "
        //                      << shiftedLeft
        //                      << " and (inputRaw[byteIndex] << bitOffset) >> 7: "
        //                      << bitOfInterestInLastPosition
        //                      << ", finally, boolean: "
        //                      << this->committedSigners[i];
        //}
    }
    return true;
}

const unsigned SignatureAggregate::lengthInBytesBitmapSigners = 32;
const unsigned SignatureAggregate::lengthInBytesUncompressedSignature = 1 + 33 + 32 + SignatureAggregate::lengthInBytesBitmapSigners;

std::string SignatureAggregate::serializeCommittedSignersBitmap()
{
    int numberOfSigners = this->allPublicKeys.size();
    std::string result;
    result.resize(this->lengthInBytesBitmapSigners);
    unsigned char currentByte = 0;
    for (int i = 0; i < numberOfSigners; i ++ ) {
        int bitOffset = i % 8;
        if (this->committedSigners[i]) {
            currentByte |= 1 << (7 - bitOffset);
        }
        if (bitOffset == 7 || i == numberOfSigners - 1) {
            result[i / 8] = currentByte;
            currentByte = 0;
        }
    }
    return result;
}

void SignatureAggregate::ComputeCompleteSignature()
{
    std::stringstream resultStream;
    std::vector<unsigned char> signatureSerialization;
    this->serializerSignature.SerializeCompressedChallenge(signatureSerialization, SignatureSchnorr::prefixAggregateSignature);
    for (unsigned i = 0; i < signatureSerialization.size(); i ++) {
        resultStream << signatureSerialization[i];
    }
    resultStream << this->serializeCommittedSignersBitmap();
    this->aggregateSignatureUncompressed = resultStream.str();
    unsigned numberOfSigners = this->allPublicKeys.size();
    unsigned char topByte = numberOfSigners / 256;
    unsigned char bottomByte = numberOfSigners % 256;
    resultStream << topByte << bottomByte;
    for (unsigned i = 0; i < this->allPublicKeys.size(); i ++) {
        resultStream << this->allPublicKeys[i].ToBytesCompressed();
    }
    this->aggregateSignatureComplete = resultStream.str();
}

bool SignatureAggregate::VerifyMessageSignaturePublicKeysStatic(std::vector<unsigned char>& message,
        std::vector<unsigned char>& signatureUncompressed,
        std::vector<unsigned char>& publicKeysSerialized,
        std::stringstream *reasonForFailure,
        bool detailsOnFailure
)
{
    SignatureAggregate verifier;
    return verifier.VerifyMessageSignaturePublicKeys(message, signatureUncompressed, publicKeysSerialized, reasonForFailure, detailsOnFailure);
}

// The order of arguments is the order in which they appear on the stack.
// This does not coicide with the order in which they are used.
bool SignatureAggregate::VerifyMessageSignaturePublicKeys(
        std::vector<unsigned char>& message,
        std::vector<unsigned char>& signatureUncompressed,
        std::vector<unsigned char>& publicKeysSerialized,
        std::stringstream *reasonForFailure,
        bool detailsOnFailure
)
{
    SignatureAggregate verifier;
    std::string publicKeysSerializedString((const char *) publicKeysSerialized.data(), publicKeysSerialized.size());
    if (!verifier.ParsePublicKeysAndInitialize(publicKeysSerializedString, reasonForFailure)) {
        return false;
    }
    std::string signatureUncompressedString((const char *) signatureUncompressed.data(), signatureUncompressed.size());
    if (!verifier.ParseUncompressedSignature(signatureUncompressedString, reasonForFailure)) {
        return false;
    }
    verifier.messageImplied = std::string((const char*) message.data(), message.size());
    return verifier.Verify(reasonForFailure, detailsOnFailure);
}
