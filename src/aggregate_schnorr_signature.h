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

class Secp256k1
{
public:
    static secp256k1_context* ContextAllocateIfNeeded(secp256k1_context*& staticPointer);

    static secp256k1_context* ContextForSigning(); //<- use when handling/generating secrets
    static secp256k1_context* ContextForVerification(); //<- use when handling public info
};

class SignatureAggregate;
class PrivateKeyKanban;


//See the remarks before class SignatureSchnorr
//for more info on public keys.
class PublicKeyKanban
{
public:
    secp256k1_pubkey data;
    static const int lengthPublicKeySerializationCompressed = 33;
    static const int lengthPublicKeyData = 64;
    std::string ToHexCompressed() const;
    std::string ToHexUncompressed() const;
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
    bool MakeFromBytesSerializationOffset
    (const std::vector<unsigned char>& serializer, int offset, int length, std::stringstream* commentsOnFailure);
};

//See the remarks before class SignatureSchnorr
//for more info on private keys.
class PrivateKeyKanban
{
public:
    static const unsigned int lengthKey = 32;
    std::vector<unsigned char> data;
    bool flagCorrespondingPublicKeyUsedWithCompressedEncoding;
    const std::vector<unsigned char> getData() const
    {
        return this->data;
    }
    template <typename inputDataType>
    bool MakeFromByteSequenceOffset(const inputDataType& input, int offset, std::stringstream* commentsOnFailure)
    {
        if (offset + PrivateKeyKanban::lengthKey > input.size()) {
            if (commentsOnFailure != 0) {
                *commentsOnFailure
                << "Insufficient bytes at secret input: need " << PrivateKeyKanban::lengthKey
                << " bytes. Input had " << input.size() << " bytes offset at: "
                << offset << ". ";
            }
            return false;
        }
        this->data.resize(PrivateKeyKanban::lengthKey);
        for (unsigned i = 0; i < PrivateKeyKanban::lengthKey; i ++) {
            this->data[i] = input[offset + i];
        }
        return true;
    }
    template <typename inputDataType>
    bool MakeFromByteSequence(const inputDataType& input, std::stringstream* commentsOnFailure)
    { return this->MakeFromByteSequenceOffset(input, 0, commentsOnFailure);
    }
    PrivateKeyKanban();
    bool isInitialized();
    void MakeZero();
    void MakeSmallInt(unsigned int input);
    bool MakeFromBase58WithoutCheck(const std::string& input, std::stringstream *commentsOnFailure);
    bool MakeFromBase58DetectCheck(const std::string& input, std::stringstream *commentsOnFailure);
    bool MakeFromBase58Check(const std::string& input, std::stringstream *commentsOnFailure);
    bool MakeFromUniValueRecognizeFormat(const UniValue& input, std::stringstream* commentsOnFailure);
    std::string ToHex() const;
    std::string ToBase58Check() const;
    std::string ToBase58() const;
    void ToBytesWithPrefixAndSuffix(std::vector<unsigned char>& output) const;
    void operator=(const secp256k1_scalar& other);
    bool operator==(const PrivateKeyKanban& other) const;
    void operator*=(const PrivateKeyKanban& other);
    void operator+=(const PrivateKeyKanban& other);
    void MultiplyMeBy(const PrivateKeyKanban& other, bool otherIsSensitive);
    void AddToMe(const PrivateKeyKanban& other, bool otherIsSensitive);
    bool ComputePublicKey(PublicKeyKanban& output, std::stringstream *commentsOnFailure__SENSITIVE__NULL_SAFE);
    bool SignMessageSchnorr();
    bool GenerateRandomSecurely();
    void reset();
};

//Schnorr signature.
//for documentation, google search file secp256k1_kanban.md file from
//the project presently named kanbanGO.

class SignatureSchnorr
{
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
///The two data structures are sensitive and must not be shared with anyone
///
///
///myNonce_DO_Not_Disclose_To_Aggregator
///myPrivateKey__KEEP_SECRET
///
///All other data structures are not sensitive and can be kept in the open.
class SignatureAggregate
{
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
    Sha3New lockingCoefficientPrecomputedData;
    Sha3New lockingCoefficientComputer;

    //Computed from the publicKeysSigners and the committedSigners.
    //Computed both by the aggregator and the signers.
    //The aggregator can compute this before the signers as the aggregator knows
    //the committedSigners before the signers do.
    //Sent along with the committedSigners array by the aggregator as an error check.
    //Computed as:
    //aggregatePublicKey = \prod_{i \in committedSigners} allPublicKeys[i] ^ lockingCoefficients[i]
    PublicKeyKanban publicKeyAggregate;


    Sha3New challengeDigester;

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

    std::string aggregateSignatureUncompressed;
    std::string aggregateSignatureComplete;

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

    //This function's only goal is to serve
    //an example of using the public interface.
    //
    //Please inspect the functions' code as an example of how to use
    //the public interface of the class.
    //
    //The function does the following:
    //1. Create and initialize 1 aggregator, 6 signers.
    //2. Create private keys for the signers.
    //3. Carry out the aggregate signature protocol twice.
    //3.1. First signature round (will be unsuccessful).
    //In this round, the 6th signer will fail to send a commitment (this is allowed).
    //Later, the 5th singer (who has sent a commitment already)
    //will fail to send a challene solution.
    //
    //As a result, the protocol will be aborted and no aggregate signature will be generated.
    //
    //3.2 Second signature round (will succeed).
    //In this round, the 6th signer will fail to send a commitment.
    //After the failure of the 6th signer,
    //no further problems with the protocol will be created.
    //The aggregate signature will then succeed.
    //4. We verify the successful signature generated in Step 3.
    //5. We tamper 1 bit in the signature bitmap and demonstrate that the
    //signature is not verified.
    //6. We tamper 1 byte in the message and demonstrate that the signature is not verified.
    //7. We replace 1 of the public keys with another valid one and demonstrate that the signature is not verified.
    //8. We tamper 1 byte in the second part of the signature and demonstrate the signature is not verified.
    //9. We replace the challenge part of the signature with a valid public key and demonstrate the signature is not verified.
    //
    //A human-readable ascii summary of the whole procedure will be generated and returned.
    //
    static std::string ExampleRunSampleAggregateSignatureOutputHumanReadable();


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

    //Call this function as an aggregator to starts the signing protocol.
    //
    //After calling this function, ensure that the aggregator sends the message
    //to be signed to all signers.
    //The message can be sent over an open network.
    //
    //Aggregators transition from state 1 to state 2.
    //
    //If the function returns false and commentsOnFailure is non-null,
    //it will contain a human-readable explanation of what failed.
    bool TransitionAggregatorState1ToState2
    (const std::string& message,
     std::stringstream* commentsOnFailure);

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
    bool TransitionSignerState1or2ToState3
    (const std::string& message,
     std::stringstream* commentsOnFailure,
     PrivateKeyKanban* desiredNonce__NULL_SAFE__ALL_OTHER_VALUES_CRITICAL_SECURITY_RISK = nullptr);

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
    bool TransitionSignerState2or3ToState4
    (const std::vector<PublicKeyKanban>& inputCommitments,
     const std::vector<bool>& inputCommittedSigners,
     const std::vector<PublicKeyKanban>* publicKeys__null_allowed,
     std::stringstream* commentsOnFailure);

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
    bool TransitionSignerState3or4ToState5
    (const std::vector<bool>& inputcommittedSigners,
     PrivateKeyKanban& inputChallenge,
     PublicKeyKanban& inputAggregateCommitment,
     PublicKeyKanban& inputAggregatePublicKey,
     std::stringstream* commentsOnFailure);

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
    bool TransitionSignerState4or5ToState6
    (const std::vector<PrivateKeyKanban>& inputSolutions,
     std::stringstream* commentsOnFailure);

    bool Verify(std::stringstream* reasonForFailure, bool detailsOnFailure);
    void ComputeCompleteSignature();
    bool ParsePublicKeysAndInitialize(const std::string& publicKeySerialization, std::stringstream* reasonForFailure);
    bool ParseUncompressedSignature(const std::string& uncompressedSignature, std::stringstream* reasonForFailure);

    bool parseCompleteSignature(const std::string& signatureComplete, std::stringstream* reasonForFailure);
    bool parseCompleteSignature(const std::vector<unsigned char>& signatureComplete, std::stringstream* reasonForFailure);
    std::string serializeCommittedSignersBitmap();
    bool deserializeSignersBitmapFromBigEndianBits(const std::string& inputRaw, std::stringstream* reasonForFailure);
    bool VerifyFromSignatureComplete(const std::string& signatureComplete, const std::string& message, std::stringstream* reasonForFailure);
    bool VerifyMessageSignaturePublicKeys(
            std::vector<unsigned char>& message,
            std::vector<unsigned char>& signatureUncompressed,
            std::vector<unsigned char>& publicKeysSerialized,
            std::stringstream* reasonForFailure,
            bool detailsOnFailure = false
    );
    static bool VerifyMessageSignaturePublicKeysStatic(
            std::vector<unsigned char>& message,
            std::vector<unsigned char>& signatureUncompressed,
            std::vector<unsigned char>& publicKeysSerialized,
            std::stringstream* reasonForFailure,
            bool detailsOnFailure = false
    );
};

class SchnorrKanban
{
public:
    bool Sign(PrivateKeyKanban& input,
        const std::string& message,
        SignatureSchnorr& output,
        PrivateKeyKanban *desiredNonce__NULL_SAFE__ALL_OTHER_VALUES_CRITICAL_SECURITY_RISK,
        UniValue* commentsSensitive__NULL_SAFE__SECURITY_RISK_OTHERWISE
    );
    bool Verify(SignatureSchnorr& output, std::stringstream *commentsOnFailure_NULL_for_no_comments);
};

template<typename otherType>
std::string toStringHex(const otherType& other) {
  std::stringstream out;
  for (unsigned i = 0; i < other.size(); i ++)
    out << std::hex << std::setfill('0') << std::setw(2) << ((int) ((unsigned char) other[i]));
  return out.str();
}

template<typename otherType>
std::string toStringHexOffset(const otherType& other, int offset) {
  std::stringstream out;
  for (unsigned i = offset; i < other.size(); i ++)
    out << std::hex << std::setfill('0') << std::setw(2) << ((int) ((unsigned char) other[i]));
  return out.str();
}

bool char2int(char input, int& output, std::stringstream* commentsOnFailure);
template<typename inputType, typename outputType>
bool fromHex(const inputType& input, outputType& result, std::stringstream* commentsOnFailure)
{
    result.clear();
    int currentHigh, currentLow;
    for (unsigned i = 0; i + 1 < input.size(); i += 2) {
        if (!char2int(input[i], currentHigh, commentsOnFailure))
            return false;
        if (!char2int(input[i+1], currentLow, commentsOnFailure))
            return false;
        unsigned char current = ((unsigned char) currentHigh) * 16 + ((unsigned char) currentLow);
        result.push_back(current);
    }
    return true;
}

#endif
