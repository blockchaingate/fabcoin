// Copyright (c) 2018 FA Enterprise system
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SCHNORR_NEW_H
#define SCHNORR_NEW_H
#include "crypto/secp256k1_all_in_one.h"
#include <iomanip>
#include <vector>
#include <univalue/include/univalue.h>
#include "crypto/sha3.h"
#include <libdevcrypto/Common.h>

class Secp256k1 {
public:
    static secp256k1_context* ContextAllocateIfNeeded(secp256k1_context*& staticPointer);
    static secp256k1_context* ContextForSigning(); //<- use when handling/generating secrets
    static secp256k1_context* ContextForVerification(); //<- use when handling public info
};

class SignatureAggregate;
class PrivateKeyKanban;

//See the remarks before class SignatureSchnorr
//for more info on public keys.
class PublicKeyKanban {
public:
    secp256k1_pubkey data;
    static const int lengthPublicKeySerializationCompressed = 33;
    static const int lengthPublicKeyData = 64;
    std::string ToHexCompressed() const;
    std::string ToHexUncompressed() const;
    void ToAddressBytesCompressed(std::vector<unsigned char>& output) const;
    std::string ToAddressUncompressedBase58() const;
    std::string ToAddressCompressedBase58() const;
    std::string ToBytesUncompressed() const;
    std::string ToBytesCompressed() const;

    bool Exponentiate(const PrivateKeyKanban& input, std::stringstream *commentsOnFailure);
    bool operator<(const PublicKeyKanban& other)const;
    bool operator==(const PublicKeyKanban& other)const;
    PublicKeyKanban();
    PublicKeyKanban(const PublicKeyKanban& other);
    PublicKeyKanban& operator=(const PublicKeyKanban& other);
    static bool MultiplyMany(const std::vector<PublicKeyKanban>& multiplicands, PublicKeyKanban& output);
    static bool Multiply(const PublicKeyKanban& left, const PublicKeyKanban& right, PublicKeyKanban& output);
    bool operator*=(const PublicKeyKanban& other);
    void SerializeCompressed(std::vector<unsigned char>& output) const;
    void SerializeUncompressed(std::vector<unsigned char>& output) const;
    void MakeTheECGroupGenerator();
    void MakeUnit();
    bool MakeFromUniValueRecognizeFormat(const UniValue& input, std::stringstream* commentsOnFailure);
    bool MakeFromStringRecognizeFormat(const std::string& input, std::stringstream* commentsOnFailure);
    bool MakeFromBytesSerialization(const std::vector<unsigned char>& serializer, std::stringstream* commentsOnFailure);
    bool isInitialized() const;
    bool MakeFromBytesSerializationOffset(
        const std::vector<unsigned char>& serializer, int offset, int length, std::stringstream* commentsOnFailure
    );
};

//See the remarks before class SignatureSchnorr
//for more info on private keys.
class PrivateKeyKanban {
    //Data structure used for serialization.
    std::vector<unsigned char> dataBuffeR;
public:
    static const unsigned int lengthKey = 32;
    secp256k1_scalar scalar;
    bool flagCorrespondingPublicKeyUsedWithCompressedEncoding;
    template <typename inputDataType>
    bool MakeFromByteSequenceOffset(const inputDataType& input, int offset, std::stringstream* commentsOnFailure) {
        if (offset + PrivateKeyKanban::lengthKey > input.size()) {
            if (commentsOnFailure != 0) {
                *commentsOnFailure
                << "Insufficient bytes at secret input: need " << PrivateKeyKanban::lengthKey
                << " bytes. Input had " << input.size() << " bytes offset at: "
                << offset << ". ";
            }
            return false;
        }
        this->dataBuffeR.resize(PrivateKeyKanban::lengthKey);
        for (unsigned i = 0; i < PrivateKeyKanban::lengthKey; i ++) {
            this->dataBuffeR[i] = input[offset + i];
        }
        secp256k1_scalar_set_b32(&this->scalar, this->dataBuffeR.data(), 0);
        return true;
    }
    template <typename inputDataType>
    bool MakeFromByteSequence(const inputDataType& input, std::stringstream* commentsOnFailure) {
        return this->MakeFromByteSequenceOffset(input, 0, commentsOnFailure);
    }
    PrivateKeyKanban();
    ~PrivateKeyKanban();
    void MakeZero();
    void MakeSmallInt(unsigned int input);
    bool MakeFromBase58WithoutCheck(const std::string& input, std::stringstream *commentsOnFailure);
    bool MakeFromBase58DetectCheck(const std::string& input, std::stringstream *commentsOnFailure);
    bool MakeFromBase58Check(const std::string& input, std::stringstream *commentsOnFailure);
    bool MakeFromUniValueRecognizeFormat(const UniValue& input, std::stringstream* commentsOnFailure);
    void computeScalarFromData();
    std::vector<unsigned char>& computeDataFromScalar();
    std::string ToHexNonConst();
    std::string ToBase58CheckNonConst();
    std::string ToBase58NonConst();
    std::string ToBase58NonConstFABMainnetPrefix();
    void ToBytesWithPrefixAndSuffixNonConst(
        std::vector<unsigned char>& output, unsigned char *desiredPrefixNullForAuto
    );
    void operator=(const secp256k1_scalar& other);
    void operator=(const PrivateKeyKanban& other);
    bool operator==(const PrivateKeyKanban& other) const;
    void operator*=(const PrivateKeyKanban& other);
    void operator+=(const PrivateKeyKanban& other);
    void MultiplyMeBy(const PrivateKeyKanban& other, bool otherIsSensitive);
    void AddToMe(const PrivateKeyKanban& other, bool otherIsSensitive);
    static bool leftSmallerThanRightByPublicKeyCompressed(const PrivateKeyKanban& left, const PrivateKeyKanban& right);
    bool ComputePublicKey(PublicKeyKanban& output, std::stringstream *commentsOnFailure__SENSITIVE__NULL_SAFE) const;
    bool GenerateRandomSecurely();
    void reset();
};

//ECDSA signature.
//A wrapper around the secp256k1 library.
class SignatureECDSA {
public:
    PrivateKeyKanban challengeR;
    PrivateKeyKanban solutionsS;
    std::vector<unsigned char> messageImplied;
    PrivateKeyKanban messageScalar;
    PublicKeyKanban publicKeyImplied;
    std::vector<unsigned char> signatureBytes;
    bool MakeFromBytesWithSuffixOne(const std::vector<unsigned char>& input, std::stringstream* commentsOnFailure_NULL_for_no_comments);
    bool MakeFromBytes(const std::vector<unsigned char>& input, std::stringstream* commentsOnFailure_NULL_for_no_comments);
    bool Verify(std::stringstream *commentsOnFailure_NULL_for_no_comments);
    bool SignSha256Squared(PrivateKeyKanban& secretKey, std::stringstream *commentsOnFailure_NULL_for_no_comments);
    void GetMessageHash(std::vector<unsigned char>& output);
    bool computeMessageScalar(std::stringstream *commentsOnFailure_NULL_for_no_comments);
};

class AddressKanban {
public:
    static const unsigned int addressSize;
    std::vector<unsigned char> address;
    bool MakeFromBytes(const std::string& input, std::stringstream* commentsOnFailure) {
        std::vector<unsigned char> inputVector;
        inputVector.assign(input.begin(), input.end());
        return this->MakeFromBytes(inputVector, commentsOnFailure);
    }
    bool MakeFromBytes(const std::vector<unsigned char>& input, std::stringstream* commentsOnFailure);
    dev::Address ToEthereumAddress() const;
};

//Schnorr signature.
//for documentation, google search file secp256k1_kanban.md file from
//the project presently named kanbanGO.

class SignatureSchnorr {
public:
    static unsigned char prefixSignature;
    static unsigned char prefixAggregateSignature;
    PublicKeyKanban challenge;
    PrivateKeyKanban solution;
    //Implied signature members are not
    //``officially'' part of the signature
    //but are implied to be available any time we are
    //verifying a signature.
    //Forging a message and a public key
    //that match a given (challenge,solution) signature
    //is computationally non-feasible.
    //
    //In many circumstances, the message and public
    //key are assumed
    //to be kept in the open.    
    PublicKeyKanban publicKeyImplied;
    std::string messageImplied;
    void SerializeCompressedChallenge(std::vector<unsigned char>& output, unsigned char prefix);
    std::string ToBytes();
    std::string ToBase58Check();
    std::string ToBase58();
    std::string ToBytesCustomPrefix(unsigned char prefix);
    std::string ToBase58CustomPrefix(unsigned char prefix);
    bool MakeFromUniValueRecognizeFormat(const UniValue& input, std::stringstream* commentsOnFailure);
    bool MakeFromBase58DetectCheck(const std::string& input, std::stringstream* commentsOnFailure);
    bool MakeFromBase58Check(const std::string& input, std::stringstream* commentsOnFailure);
    bool MakeFromBase58WithoutCheck(const std::string& input, std::stringstream* commentsOnFailure);
    bool MakeFromBytes(const std::vector<unsigned char>& serialization, std::stringstream* commentsOnFailure);

    bool Sign(
        PrivateKeyKanban& input,
        const std::string& message,
        PrivateKeyKanban *desiredNonce__NULL_SAFE__ALL_OTHER_VALUES_CRITICAL_SECURITY_RISK,
        UniValue* commentsSensitive__NULL_SAFE__SECURITY_RISK_OTHERWISE
    );
    bool Verify(std::stringstream *commentsOnFailure_NULL_for_no_comments);
};

///The elements of the class are given in the order in which they are populated/used.
///Methods that start with Capital letter are considered public.
///Methods with Capital letters should be safe to call even if the
///SignatureAggregate object is not properly initialized.
///
///All methods that start with small letters are considered private.
///Those methods do not come with a promise that they should be safe to call
///independent of the state of the SignatureAggregate object.
///
///Please do not modify variables in the SignatureAggregate object manually.
///Use the Public methods (function names starting with Capital letter)
///to modify the Signature aggregate object.
///
///Read-access to the internals of the SignatureAggregate is allowed.
///
///The following two data structures are sensitive and must not be shared with anyone:
///
///
///myNonce_DO_Not_Disclose_To_Aggregator
///myPrivateKey__KEEP_SECRET
///
///All other data structures are not sensitive and can be kept in the open.
class SignatureAggregate {
public:
    static const unsigned lengthInBytesBitmapSigners;
    static const unsigned lengthInBytesUncompressedSignature;


    bool flagIsAggregator;
    bool flagIsSigner;
    int currentState;
    enum state {
        stateUninitialized,
        state1ready,
        state2MessageSent,
        state3MessageReceivedCommitmentsSent,
        state4CommitmentsReceivedChallengesSent,
        state5ChallengesReceivedSolutionsSent,
        state6SolutionsReceivedAggregateSignatureSent,

        stateVerifyingAggregateSignatures,
    };

    //Private key used to sign.
    PrivateKeyKanban myPrivateKey__KEEP_SECRET;

    //Public key used to sign.
    //Computed as:
    //g^myPrivateKey__KEEP_SECRET
    PublicKeyKanban myPublicKey;

    //Sorted list of the public keys of all signers.
    //This list of public keys is assumed to rarely change.
    //The public keys are assumed registered with an authority
    //(for example, the foundation chain).
    //Every signer is expected to have the same list of public keys.
    //For signers, the list is assumed to contain myPublicKey.
    std::vector<PublicKeyKanban> allPublicKeys;

    //Gives the index of the myPublicKey in allPublicKeys.
    //Set to -1 for aggregators that are not signers.
    int signerIndex;

    //Sent by the aggregator to the signers to initiate the protocol.
    std::string messageImplied;

    //One-time use random number.
    //Signers generate this and keep it secret.
    //Must not be reused on protocol restarts.
    //If signers disclose this to the aggregator, the aggregator can derive the private key
    //of the signer.
    //The nonce is to be used as an exponent of g (the generator of the elliptic curve group).
    PrivateKeyKanban myNonce_DO_Not_Disclose_To_Aggregator;

    //A component of needed to start the aggregate signature.
    //Sending this indicates a commitment to the aggregate signature protocol.
    //Sent from each signer to the aggregator.
    //Computed as:
    //g^myNonce
    PublicKeyKanban myCommitment;

    //A list of all commitments.
    //The aggregator stores here each commitment received by the signers.
    //Signers who fail to commit have undefined commitment (no need to wipe previous entries).
    std::vector<PublicKeyKanban> allCommitments;

    //A bitmap that has 1 for each public key that participates in the signature,
    //0 if the public key does not. The order of public keys is dictated by the
    //allPublicKeys data structure.
    //Signers receive this from the aggregator.
    std::vector<bool> committedSigners;

    //Aggregators generate this.
    //Used to compute the challenge.
    //Sent to the signers as an error check.
    //Used as the first part of the signature
    //Computed as:
    //\prod_{i \in committedSigners} allCommitments[i]
    PublicKeyKanban commitmentAggregate;

    //A concatenation of a sorted list of all committed public keys
    std::vector<unsigned char> committedPublicKeysSortedConcatenated;

    //Contained in allLockingCoefficients. Stored
    //separately for convenience, see below.
    PrivateKeyKanban myLockingCoefficient;

    //Computed from the committedSigners and public keys.
    //Does not need to be recomputed if the committedSigners bitmap has not changed.
    //The values of the locking coefficients can be cached for different
    //values of the committedSigners bitmap.
    //
    //The term "locking coefficient" stands for the coefficients a_i defined
    //on page 10, Simple Schnorr Multi-Signatures with Application to Bitcoin.
    //
    //Please see pages 10, 11 and 12 of the same paper for rationale for
    //the locking coefficients.
    //
    //It is possible that the attacks described on page 12 are mitigated by
    //requiring that public keys be registered in advance
    //with a signature proving that each signer owns the corresponding secret.
    //This requirement is made by our system for reasons independent of cryptographic security.
    //However, whether indeed the registration process mitigates the attacks described in
    //the aforementioned paper remains to be carefully investigated.
    //
    //We therefore choose to assume the worst case scenario and
    //use the locking coefficients as described on pages 10-12 in the article.
    //
    //Computed as:
    //lockingCoefficients[i] = Sha3_256( concatenate(committedPublicKeysSortedConcatenated, allPublicKeys[i]) )
    std::vector<PrivateKeyKanban> allLockingCoefficients;

    //Hashers needed to compute the locking coefficients.
    Sha3 lockingCoefficientPrecomputedData;
    Sha3 lockingCoefficientComputer;

    //Computed from the publicKeysSigners and the committedSigners.
    //Computed both by the aggregator and the signers.
    //The aggregator can compute this before the signers as the aggregator knows
    //the committedSigners before the signers do.
    //Sent along with the committedSigners array by the aggregator as an error check.
    //Computed as:
    //aggregatePublicKey = \prod_{i \in committedSigners} allPublicKeys[i] ^ lockingCoefficients[i]
    PublicKeyKanban publicKeyAggregate;


    Sha3 challengeDigester;

    //Aggregators generate this and send it to the signers.
    //Computed as:
    //Sha3_256(concatenate( publicKeyAggregate, commitmentAggregate, messageImplied) )
    PrivateKeyKanban digestedMessage;

    //Signers generate this and send it to the aggregator.
    //Computed as:
    //myNonce + challenge * myLockingCoefficient * myPrivateKey__KEEP_SECRET
    PrivateKeyKanban mySolution;

    //Aggregators receive this from the senders as computed in the preceding step.
    std::vector<PrivateKeyKanban> allSolutions;

    //Aggregate solution, second part of the signature.
    //Computed as
    //\sum_{i \in committedSigners} allSolutions[i]
    PrivateKeyKanban solutionAggregate;

    SignatureSchnorr serializerSignature;

    std::vector<unsigned char> aggregateSignatureUncompressed;
    std::vector<unsigned char> aggregateSignatureComplete;

    SignatureAggregate();

    bool computeMySolution(std::stringstream *commentsOnFailure);
    bool computeErrorChecksChallenge(std::stringstream* commentsOnFailure);
    bool computeAggregateCommitment(std::stringstream* commentsOnFailure);
    bool computeOneLockingCoefficient(int indexCoefficient, std::stringstream* commentsOnFailure);
    bool computeLockingCoefficients(std::stringstream* commentsOnFailure);
    bool computeLockingCoefficientsMultiple(std::stringstream* commentsOnFailure);
    bool computeLockingCoefficientsOne(std::stringstream* commentsOnFailure);
    void computeConcatenatedPublicKeys();
    bool computeAggregatePublicKey(std::stringstream* commentsOnFailure);
    bool computeMessageDigest(std::stringstream* commentsOnFailure);
    bool checkCorrectCommittedSigners(std::stringstream* commentsOnFailure);
    void toUniValueAppendMessage(UniValue& output);
    void toUniValueAppendPublicKeys(UniValue& output);
    void toUniValueAppendCommitments(UniValue& output);
    void toUniValueAppendBitmapSigners(UniValue& output);
    void toUniValueConcatenatedPublicKeys(UniValue& output);
    void toUniValueAppendAggregatePublicKey(UniValue& output);
    void toUniValueAppendAggregateCommitment(UniValue& output);
    void toUniValueAppendAllLockingCoefficients(UniValue& output);
    void toUniValueAppendAllSolutions(UniValue& output);
    void toUniValueVectorStringsWithIgnoredStatus(const std::vector<std::string>& input, UniValue& output);
    void toUniValueAppendMyLockingCoefficient(UniValue& output);
    void toUniValueAppendMySolution(UniValue& output);
    void toUniValueAppendAggregateSolution(UniValue& output);
    void toUniValueAppendMessageDigest(UniValue& output);
    void toUniValueAppendAggregateCommitmentFromSignature(UniValue& output);
    void toUniValueAppendSignatureSerializationBase58WithoutCheckNoBitmap(UniValue& output);
    void toUniValueAppendSignatureComplete(UniValue& output);
    static bool leftHasSmallerPublicKey(const SignatureAggregate& left, const SignatureAggregate& right);
    std::string toStringMyState();
    std::string toStringSignersBitmap();
    //Prepares a human-readable JSON (UniValue) with
    //internal information.
    //Reveals secrets, do not display this information in the open.
    UniValue toUniValueTransitionState__SENSITIVE();

    bool verifyPartTwo(std::stringstream* reasonForFailure, bool detailsOnFailure);
    bool transitionSignerState2or3ToState4PartTwo(std::stringstream* commentsOnFailure);
    bool transitionSignerState3or4ToState5PartTwo(std::stringstream* commentsOnFailure);


    ///Public interface. Please do not modify the
    ///variables of this class directly, but instead
    ///use the function calls below.

    //Resets everything.
    void ResetNoPrivateKeyGeneration(bool isAggregator, bool isSigner);

    //Resets everything and generates a (secret) private key to sign with.
    //The private key is needed by a node that is a signer or a signer+aggregator.
    //Function should always return true.
    bool ResetGeneratePrivateKey(bool isAggregator, bool isSigner);

    //Initializes the signers and aggregators.
    //Initializes signer public keys and sorts them
    //Sets the state to state1ready.
    //Must be done by anyone who wishes to do signature verification.
    //
    //If the function returns false and commentsOnFailure is non-null,
    //it will contain a human-readable explanation of what failed.
    bool InitializePublicKeys(const std::vector<PublicKeyKanban>& registeredSigners, std::stringstream* commentsOnFailure);

    //Call this function as an aggregator to start the signing protocol.
    //
    //After calling this function, ensure that the aggregator sends the message
    //to be signed to all signers.
    //The message can be sent over an open network.
    //
    //Aggregators transition from state 1 to state 2.
    //
    //If the function returns false and commentsOnFailure is non-null,
    //it will contain a human-readable explanation of what failed.
    bool TransitionAggregatorState1ToState2(
        const std::string& message,
        std::stringstream* commentsOnFailure
    );

    int GetNumberOfCommittedSigners();
    //Call this function as signer to commit to the signing protocol.
    //In this function, the signers generate a cryptographic commitment
    //this->myCommitment
    //to the rest of the signing protocol.
    //It is allowed for signers to bail out at this step and not generate
    //a commitment. If that is to happen, they will be excluded from the
    //committedSigners bitmap.
    //
    //After calling this function, please ensure that each signer sends
    //its commitment to the aggregator.
    //The commitments may be sent over an open network.
    //Network failure here is NOT considered
    //a punishable offence (unless it happens often).
    //
    //
    //Signers that are not aggregators transition from state 1 to state 3 directly.
    //Signers that are also aggregators transition from state 2 to state 3.
    //
    //If the function returns false and commentsOnFailure is non-null,
    //it will contain a human-readable explanation of what failed.
    //
    //Please do not use the last argument unless you know what you are doing.
    //The last argument is used for debugging to specify particular nonces for the signers.
    bool TransitionSignerState1or2ToState3(
        const std::string& message,
        std::stringstream* commentsOnFailure,
        PrivateKeyKanban* desiredNonce__NULL_SAFE__ALL_OTHER_VALUES_CRITICAL_SECURITY_RISK = nullptr
    );

    //Call this function to inform the aggregator
    //of the received commitments by the signers.
    //The aggregator then uses the received commitments to generate
    //a challenge.
    //
    //After calling this function, please ensure that the aggregator
    //sends this->challenge, this->commitmentAggregate and this->committedSigners
    //to each of the signer nodes.
    //The challenge may be sent over an open network.
    //Network failure here is considered
    //a punishable offence.
    //
    //
    //Aggregator that is not a signer transitions fom state 2 (message sent) to state 4 directly.
    //Aggregator that is also a signer transitions fom state 3 to state 4.
    //
    //If the function returns false and commentsOnFailure is non-null,
    //it will contain a human-readable explanation of what failed.
    //
    //Please do not use the last argument unless you know what you are doing.
    //The last argument is used for debugging to specify particular nonces for the signers.
    bool TransitionSignerState2or3ToState4(
        const std::vector<PublicKeyKanban>& inputCommitments,
        const std::vector<bool>& inputCommittedSigners,
        const std::vector<PublicKeyKanban>* publicKeys__null_allowed,
        std::stringstream* commentsOnFailure
    );

    //Call this function to inform the signers
    //of the agregator's challenge.
    //
    //After calling this function, please ensure that each signer
    //sends this->mySolution
    //to the aggregator over the network.
    //The network may be open.
    //Network failure here is considered
    //a punishable offence.
    //
    //Signers that are not aggregators transition from state 3 to state 5 directly.
    //Signers that are also aggregators transition from state 4 to state 5.
    //
    //If the function returns false and commentsOnFailure is non-null,
    //it will contain a human-readable explanation of what failed.
    bool TransitionSignerState3or4ToState5(
        const std::vector<bool>& inputcommittedSigners,
        PrivateKeyKanban& inputChallenge,
        PublicKeyKanban& inputAggregateCommitment,
        PublicKeyKanban& inputAggregatePublicKey,
        std::stringstream* commentsOnFailure
    );

    //Call this function to inform the aggregator of
    //the solutions sent by the signers.
    //
    //The signature will be complete after this step.
    //
    //Aggregators that are not signers transition from state 4 to state 6 directly.
    //Aggregators that are also signers transition from state 5 to state 6.
    //
    //If the function returns false and commentsOnFailure is non-null,
    //it will contain a human-readable explanation of what failed.
    bool TransitionSignerState4or5ToState6(
        const std::vector<PrivateKeyKanban>& inputSolutions,
        std::stringstream* commentsOnFailure
    );

    bool Verify(std::stringstream* reasonForFailure, bool detailsOnFailure);
    void ComputeCompleteSignature();
    std::vector<unsigned char> ToBytesSignatureUncompressed() {
        this->ComputeCompleteSignature();
        return this->aggregateSignatureUncompressed;
    }
    std::vector<unsigned char> ToBytesSignatureComplete() {
        this->ComputeCompleteSignature();
        return this->aggregateSignatureComplete;
    }
    bool ParsePublicKeysFromVectorAndInitialize(const std::vector<unsigned char>& publicKeySerialization, std::stringstream* reasonForFailure);
    bool ParsePublicKeysAndInitialize(const std::vector<std::vector<unsigned char> >& publicKeysSerialized, std::stringstream* reasonForFailure);
    bool ParseUncompressedSignature(const std::string& uncompressedSignature, std::stringstream* reasonForFailure);

    bool parseCompleteSignature(const std::string& signatureComplete, std::stringstream* reasonForFailure);
    bool parseCompleteSignature(const std::vector<unsigned char>& signatureComplete, std::stringstream* reasonForFailure);
    std::string serializeCommittedSignersBitmap();
    bool deserializeSignersBitmapFromBigEndianBits(const std::string& inputRaw, std::stringstream* reasonForFailure);


    // There are 3 different aggregate signature serializations:
    // compressed, uncompressed and complete.
    //
    //   **Attention** In many contexts, it is important to have unique
    //   signature encoding. If that is not the case, there are potential replay attacks
    //   in which an attacker takes a correct transaction, changes the signature encoding,
    //   and re-submits the transaction. Since re-encoding changes the transaction id,
    //   the transaction will be new. This poses no problem for classical bitcoin transaction since
    //   there, spending a transaction consumes a previous transaction and that prevents a replay.
    //   However, for the purpose of coordinating Kanban with the foundation chain,
    //   we need to have ancestor-less transactions. Those do not consume previous transactions and
    //   thus hold a risk of the decode-recode-in-different-format-replay attack.
    //
    //   To reduce that risk, we ensure that our transactions use only the uncompressed signature encoding.
    //   For more details, see the aggregate signature documentation in file
    //   secp256k1_kanban_schnorr_signature.md
    //   in the kanbanGo repository.

    bool VerifyFromSignatureComplete(
        const std::vector<unsigned char>& signatureComplete,
        const std::string& message,
        std::stringstream* reasonForFailure
    );
    bool VerifyMessageSignatureUncompressedPublicKeysSerialized(
        std::vector<unsigned char>& message,
        std::vector<unsigned char>& signatureUncompressed,
        std::vector<unsigned char>& publicKeysSerialized,
        std::stringstream* reasonForFailure,
        bool detailsOnFailure = false
    );
    static bool VerifyMessageSignatureUncompressedPublicKeysDeserialized(
        const std::vector<unsigned char>& message,
        const std::vector<unsigned char>& signatureUncompressed,
        const std::vector<std::vector<unsigned char> >& publicKeysSerialized,
        std::stringstream* reasonForFailure,
        bool detailsOnFailure = false
    );
    static bool VerifyMessageSignatureUncompressedPublicKeysSerializedStatic(
        std::vector<unsigned char>& message,
        std::vector<unsigned char>& signatureUncompressed,
        std::vector<unsigned char>& publicKeysSerialized,
        std::stringstream* reasonForFailure,
        bool detailsOnFailure = false
    );
};

#endif
