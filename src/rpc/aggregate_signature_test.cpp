// Copyright (c) 2018 FA Enterprise system
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/server.h"

#include "crypto/sha3.h"
#include "../aggregate_schnorr_signature.h"
#include "utilstrencodings.h"
#include "encodings_crypto.h"
#include <primitives/transaction.h>
#include <core_io.h>
#include <coins.h>
#include <txmempool.h>
#include <validation.h>


#include <univalue.h>
#include <iostream>
#include <mutex>

void avoidCompilerWarningsDefinedButNotUsedAggregateSignature() {
    (void) FetchSCARShardPublicKeysInternalPointer;
}

extern CTxMemPool mempool;


std::vector<SignatureAggregate> currentSigners;
SignatureAggregate currentAggregator;
static std::mutex aggregateSignatureLock;

extern UniValue getlogfile(const JSONRPCRequest& request);

void splitString(const std::string& input, const std::string& delimiters, std::vector<std::string>& output) {
    output.clear();
    std::string current;
    for (unsigned i = 0; i < input.size(); i ++) {
        char currentChar = input[i];
        if (delimiters.find(currentChar) != std::string::npos) {
            if (current.size() > 0) {
                output.push_back(current);
            }
            current.clear();
            continue;
        }
        current.push_back(currentChar);
    }
    if (current.size() > 0) {
        output.push_back(current);
    }
}

bool getVectorOfStringsFromBase64List(
    const std::string& input, std::vector<std::string>& output, std::stringstream* commentsOnFailure
) {
    std::string decoded = DecodeBase64(input);
    splitString(decoded, ", ", output);
    return true;
}

//Attempts to recognize the input encoding (comma delimited string vs base64 encoded comma delimited string).
bool getVectorOfStrings(const UniValue& input, std::vector<std::string>& output, std::stringstream* commentsOnFailure)
{
    output.clear();
    if (!input.isStr()) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure
            << "Failed to extract base64 encoded vectors: input univalue is not a string. Input: "
            << input.write() << ". ";
        }
        return false;
    }
    std::string inputString = input.get_str();
    if (inputString == "")
        return true;
    std::vector<std::string> candidateOutputNoBase64;
    std::vector<std::string> candidateOutputBase64;
    bool goodBase64 = getVectorOfStringsFromBase64List(inputString, candidateOutputBase64, commentsOnFailure);
    splitString(inputString, ", ", candidateOutputNoBase64);
    if (!goodBase64) {
        output = candidateOutputNoBase64;
        return true;
    }
    //Both the base64 and regular encoding are valid. Return whichever resulted in more entries.
    if (candidateOutputBase64.size() >= candidateOutputNoBase64.size()) {
        output = candidateOutputBase64;
        return true;
    }
    output = candidateOutputNoBase64;
    return true;
}

//Expected input: comma/space separated list of hex. To do: auto recognize & decode other encodings.
bool getVectorsUnsigned(
    const UniValue& input,
    std::vector<std::vector<unsigned char> >& output,
    std::stringstream* commentsOnFailure
) {
    output.clear();
    if (!input.isStr()) {
        if (commentsOnFailure != 0) {
            *commentsOnFailure
            << "Failed to extract base64 encoded vectors: input univalue is not a string. Input: "
            << input.write() << ". ";
        }
        return false;
    }
    std::string inputString = input.get_str();
    if (inputString == "") {
        return true;
    }
    std::vector<std::string> outputStrings;
    splitString(inputString, ", ", outputStrings);
    output.resize(outputStrings.size());
    for (unsigned i = 0; i < outputStrings.size(); i ++) {
        if (!Encodings::fromHex(outputStrings[i], output[i], commentsOnFailure)) {
            if (commentsOnFailure != nullptr) {
                *commentsOnFailure << "Failed to extract vector of unsigned chars from index " << i << ".\n";
            }
            return false;
        }
    }
    return true;
}


void appendSignerStateToUniValue(UniValue& output) {
    UniValue signerKeyPairs;
    signerKeyPairs.setArray();

    for (unsigned i = 0; i < currentSigners.size(); i ++) {
        //std::cout << "DEBUG: Computing uni value for signer: " << i << std::endl;
        signerKeyPairs.push_back(currentSigners[i].toUniValueTransitionState__SENSITIVE());
    }
    output.pushKV("signers", signerKeyPairs);
}

std::string toStringVector(const std::vector<std::string>& input)
{
    std::stringstream out;
    out << "(";
    for (unsigned i = 0; i < input.size(); i ++) {
        out << input[i];
        if (i + 1 != input.size())
            out << ", ";
    }
    out << ")";
    return out.str();
}

bool getBitmap(const UniValue input, std::vector<bool>& output, std::stringstream* commentsOnFailure)
{
    if (!input.isStr()) {
        if (commentsOnFailure != 0)
            *commentsOnFailure << "Failed to read bitmap: I expected string as input, got instead: " << input.write();
        return false;
    }
    const std::string& inputString = input.get_str();
    //std::cout << "DEBUG: reading bitmap from: " << inputString;
    output.resize(inputString.size());
    for (unsigned i = 0; i < inputString.size(); i ++) {
        output[i] = false;
        if (inputString[i] == '1')
            output[i] = true;
    }
    return true;
}

bool getVectorOfEllipticCurveElements(const UniValue& input, std::vector<PublicKeyKanban>& output, std::stringstream* commentsOnFailure)
{
    std::vector<std::string> inputVector;
    if (! getVectorOfStrings(input, inputVector, commentsOnFailure)) {
        return false;
    }
    output.resize(inputVector.size());
    for (unsigned i = 0; i < inputVector.size(); i ++) {
        if (inputVector[i] == "" || inputVector[i] == "(uninitialized)") {
            continue;
        }
        if (!output[i].MakeFromStringRecognizeFormat(inputVector[i], commentsOnFailure)) {
            if (commentsOnFailure != 0) {
                *commentsOnFailure
                << "Failed to convert element index "  << i
                << " (" << i + 1 << " out of " << inputVector.size()
                << ") to elliptic curve element (used for public keys, commitments, ...). Input vector: "
                << toStringVector(inputVector)
                << ". ";
            }
            return false;
        }
    }
    return true;
}

bool getVectorOfSecp256k1Scalars(const UniValue& input, std::vector<PrivateKeyKanban>& output, std::stringstream* commentsOnFailure)
{
    std::vector<std::string> inputVector;
    if (!getVectorOfStrings(input, inputVector, commentsOnFailure)) {
        return false;
    }
    //std::cout << "DEBUG: HIGHLIGHT: got to here in gettting secp256k1 vectors. " << std::endl;
    output.resize(inputVector.size());
    for (unsigned i = 0; i < inputVector.size(); i ++) {
        if (!output[i].MakeFromBase58DetectCheck(inputVector[i], commentsOnFailure)) {
            if (inputVector[i] == "" || inputVector[i] == "(uninitialized)") {
                continue;
            }
            if (commentsOnFailure != 0) {
                *commentsOnFailure
                << "Failed to convert element index "  << i
                << " (" << i + 1 << " out of " << inputVector.size()
                << ") to secp256k1 scalar (used for private keys, nonces, ...). Input vector: "
                << toStringVector(inputVector)
                << ". ";
            }
            return false;
        }
    }
    return true;
}

UniValue testaggregatesignaturegeneratepublic(UniValue& result, std::vector<PrivateKeyKanban>& desiredPrivateKeys)
{
    std::lock_guard<std::mutex> lockGuard1(aggregateSignatureLock);
    std::stringstream errorStream;
    currentSigners.resize(desiredPrivateKeys.size());
    for (unsigned i = 0; i < currentSigners.size(); i ++) {
        currentSigners[i].ResetNoPrivateKeyGeneration(false, true);
        currentSigners[i].myPrivateKey__KEEP_SECRET = desiredPrivateKeys[i];
        if (!currentSigners[i].myPrivateKey__KEEP_SECRET.ComputePublicKey(currentSigners[i].myPublicKey, &errorStream)) {
            errorStream
            << "Faled to generate public key index " << i << " (" << i + 1 << " out of " << currentSigners.size()
            << "). The private key seed was: " << currentSigners[i].myPrivateKey__KEEP_SECRET.ToHexNonConst();
            result.pushKV("error", errorStream.str());
            return result;
        }
    }
    std::sort(currentSigners.begin(), currentSigners.end(), SignatureAggregate::leftHasSmallerPublicKey);
    currentAggregator.ResetGeneratePrivateKey(true, false);
    currentAggregator.allPublicKeys.clear();
    for (unsigned i = 0; i < currentSigners.size(); i ++) {
        currentAggregator.allPublicKeys.push_back(currentSigners[i].myPublicKey);
    }
    if (!currentAggregator.InitializePublicKeys(currentAggregator.allPublicKeys, &errorStream)) {
        errorStream << "Failed to initialize aggregator's public keys. ";
        result.pushKV("error", errorStream.str());
        return result;
    }
    for (unsigned i = 0; i < currentSigners.size(); i ++) {
        if (!currentSigners[i].InitializePublicKeys(currentAggregator.allPublicKeys, &errorStream)) {
            errorStream << "Failed to initialize signer inded " << i << ".";
            result.pushKV("error", errorStream.str());
            return result;
        }
    }
    appendSignerStateToUniValue(result);
    result.pushKV("aggregator", currentAggregator.toUniValueTransitionState__SENSITIVE());
    return result;
}

UniValue testaggregatesignatureinitialize(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "testaggregatesignatureinitialize (desiredPrivateKeysBase64)\n"
            "\nTests schnorr aggregate signature initialization."
            "\nTo be documented further.\n"
        );
    std::vector<PrivateKeyKanban> desiredPublicKeys;
    std::stringstream errorStream;
    UniValue result;
    result.setObject();
    result.pushKV("input", request.params[0]);
    //std::cout << "DEBUG: GOT to here. First parameter: " << request.params[0].write() << std::endl;
    if (!getVectorOfSecp256k1Scalars(request.params[0], desiredPublicKeys, &errorStream)) {
        errorStream << "Failed to read the desired private keys from: " << request.params[0].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    return testaggregatesignaturegeneratepublic(result, desiredPublicKeys);
}

UniValue testaggregatesignaturegenerateprivatekeys(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "testaggregatesignaturegenerateprivatekeys (numberOfSigners)\n"
            "\nTests schnorr aggregate signature initialization."
            "\nTo be documented further.\n"
        );
    }
    UniValue result;
    result.setObject();
    result.pushKV("input", request.params);
    int numPrivateKeys = - 1;
    std::stringstream errorStream;
    if (request.params[0].isNum()) {
        numPrivateKeys = request.params[0].get_int();
    } else if (request.params[0].isStr()) {
        std::stringstream converter(request.params[0].get_str());
        converter >> numPrivateKeys;
    } else {
        errorStream << "Failed to extract number of public keys from your input. ";
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (numPrivateKeys < 1 || numPrivateKeys > 256) {
        errorStream
        << "The number of private keys is expected to be a number between 1 and 256. "
        << "Your input was: " << numPrivateKeys << ".";
        result.pushKV("error", errorStream.str());
        return result;
    }
    std::vector<PrivateKeyKanban> desiredPrivateKeys;
    desiredPrivateKeys.resize(numPrivateKeys);
    for (unsigned i = 0; i < desiredPrivateKeys.size(); i ++) {
        if (!desiredPrivateKeys[i].GenerateRandomSecurely()) {
            errorStream
            << "Failed to generate public key index " << i << " (" << i + 1 << " out of " << numPrivateKeys << "). "
            << "This means the random generator is not working: perhaps something is wrong with the crypto library? ";
            result.pushKV("error", errorStream.str());
            return result;
        }
    }
    if (numPrivateKeys < 15) {
        std::sort(
            desiredPrivateKeys.begin(),
            desiredPrivateKeys.end(),
            PrivateKeyKanban::leftSmallerThanRightByPublicKeyCompressed
        );
    }
    UniValue privateKeyArray;
    privateKeyArray.setArray();
    for (unsigned i = 0; i < desiredPrivateKeys.size(); i ++) {
        privateKeyArray.push_back(desiredPrivateKeys[i].ToBase58NonConstFABMainnetPrefix());
    }
    result.pushKV("privateKeys", privateKeyArray);
    return result;
}

UniValue testaggregateverificationcomplete(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            "testaggregateverificationcomplete(...)\n"
            "\nTests schnorr aggregate signature aggregation. Available in -testkanban mode only."
            "\nTo be documented further.\n"
        );
    UniValue result;
    result.setObject();
    result.pushKV("input", request.params);
    std::stringstream errorStream;
    SignatureAggregate theVerifier;
    theVerifier.currentState = theVerifier.stateVerifyingAggregateSignatures;
    if (!request.params[0].isStr()) {
        errorStream << "The first argument (signatureComplete), is not a string, as expected. Instead, it is: " << request.params[0].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (!request.params[1].isStr()) {
        errorStream << "The second argument (messageHex), is not a string, as expected. Instead, it is: " << request.params[1].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    std::vector<unsigned char> decodedMessageVector;
    if (!Encodings::fromHex(request.params[1].get_str(), decodedMessageVector, &errorStream)) {
        errorStream << "Failed to hex-decode your input: " << request.params[1].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    std::string decodedMessage((const char*)decodedMessageVector.data(), decodedMessageVector.size());
    std::vector<unsigned char> decodedCompleteSignature;
    if (!Encodings::fromHex(request.params[0].get_str(), decodedCompleteSignature, &errorStream)) {
        errorStream << "Failed to hex-decode your input: " << request.params[0].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    bool resultBool = theVerifier.VerifyFromSignatureComplete(decodedCompleteSignature, decodedMessage, &errorStream);
    result.pushKV("result", resultBool);
    if (resultBool) {
        result.pushKV("resultHTML", "<b style='color:green'>Verified</b>");
    } else {
        result.pushKV("resultHTML", "<b style='color:red'>Failed</b>");
    }
    if (!resultBool) {
        result.pushKV("verifier", theVerifier.toUniValueTransitionState__SENSITIVE());
        result.pushKV("reason", errorStream.str());
    }
    return result;
}

UniValue testaggregatesignatureverification(const JSONRPCRequest& request) {
    if (request.fHelp || request.params.size() != 3) {
        throw std::runtime_error(
            "testaggregatesignatureaggregation ( ...)\n"
            "\nTests schnorr aggregate signature aggregation. Available in -testkanban mode only."
            "\nTo be documented further.\n"
        );
    }
    std::lock_guard<std::mutex> guard1(aggregateSignatureLock);
    UniValue result;
    result.setObject();
    result.pushKV("input", request.params);
    std::stringstream errorStream;
    if (!request.params[0].isStr()) {
        result.pushKV("error", "The first parameter expected to be a string (hex-encoded message). ");
        return result;
    }
    if (!request.params[1].isStr()) {
        result.pushKV("error", "The second parameter expected to be a string (hex-encoded signature bytes).");
        return result;
    }
    if (!request.params[2].isStr()) {
        result.pushKV("error", "The fourth parameter expected to be a string (hex-encoded then comma-separated public keys).");
        return result;
    }
    std::string messageHex = request.params[0].get_str();
    std::vector<unsigned char> messageBytes;
    std::string signatureHex = request.params[1].get_str();

    std::vector<unsigned char> signatureBytes;
    std::vector<std::vector<unsigned char> > publicKeyBytes;
    if (!Encodings::fromHex(messageHex, messageBytes, &errorStream)) {
        errorStream << "Failed to hex-decode your message: " << messageHex;
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (!Encodings::fromHex(signatureHex, signatureBytes, &errorStream)) {
        errorStream << "Failed to hex-decode your signature bytes: " << signatureHex;
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (!getVectorsUnsigned(request.params[2], publicKeyBytes, &errorStream) ) {
        errorStream << "Failed to read signature serialization. ";
        result.pushKV("error", errorStream.str());
        return result;
    }
    SignatureAggregate theVerifier;
    theVerifier.flagIsAggregator = true;
    theVerifier.currentState = theVerifier.stateVerifyingAggregateSignatures;
    bool resultBool = theVerifier.VerifyMessageSignatureUncompressedPublicKeysDeserialized(
        messageBytes, signatureBytes, publicKeyBytes, &errorStream, true
    );
    result.pushKV("result", resultBool);
    if (resultBool) {
        result.pushKV("resultHTML", "<b style='color:green'>Verified</b>");
    } else {
        result.pushKV("resultHTML", "<b style='color:red'>Failed</b>");
    }
    if (!resultBool) {
        result.pushKV("verifier", theVerifier.toUniValueTransitionState__SENSITIVE());
        result.pushKV("aggregator", currentAggregator.toUniValueTransitionState__SENSITIVE());
        result.pushKV("reason", errorStream.str());
    }
    return result;
}

UniValue testaggregatesignatureaggregation(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "testaggregatesignatureaggregation ( solutionsBase58CheckThenCommaSeparatedThenBase64Encoded)\n"
            "\nTests schnorr aggregate signature aggregation. Available in -testkanban mode only."
            "\nTo be documented further.\n"
        );
    std::lock_guard<std::mutex> guard1(aggregateSignatureLock);
    UniValue result;
    result.setObject();
    result.pushKV("input", request.params);
    std::stringstream errorStream;
    std::vector<PrivateKeyKanban> inputSolutions;
    if (!getVectorOfSecp256k1Scalars(request.params[0], inputSolutions, & errorStream)) {
        errorStream << "Failed to read solutions. ";
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (!currentAggregator.TransitionSignerState4or5ToState6(inputSolutions, &errorStream)) {
        errorStream << "Failed to aggregate the solutions. ";
        result.pushKV("error", errorStream.str());
        return result;
    }
    currentAggregator.ComputeCompleteSignature();
    result.pushKV("aggregator", currentAggregator.toUniValueTransitionState__SENSITIVE());
    return result;
}

UniValue testaggregatesignaturesolutions(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 4)
        throw std::runtime_error(
            "testaggregatesignaturesolutions ( committedSignersBitmap, challengeBase58CheckEncoded, aggregateCommitmentHex, aggregatePublicKeyHex)\n"
            "\nTests schnorr aggregate signature solutions. Available in -testkanban mode only."
            "\nTo be documented further.\n"
        );
    std::lock_guard<std::mutex> guard1(aggregateSignatureLock);
    UniValue result;
    result.setObject();
    result.pushKV("input", request.params);
    PrivateKeyKanban inputChallenge;
    PublicKeyKanban inputAggregateKey;
    PublicKeyKanban inputAggregateCommitment;
    std::stringstream errorStream;
    std::vector<bool> inputBitmap;
    if (!getBitmap(request.params[0], inputBitmap, &errorStream)) {
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (!inputChallenge.MakeFromUniValueRecognizeFormat(request.params[1], &errorStream)) {
        errorStream << "Failed to read input challenge";
        result.pushKV("error", errorStream.str());
        return result;
    }
    //std::cout << "DEBUG: Read challenge: " << inputChallenge.ToBase58Check() << " from input: " << request.params[1].write() << std::endl;
    if (!inputAggregateCommitment.MakeFromUniValueRecognizeFormat(request.params[2], &errorStream)) {
        errorStream << "Failed to read input aggregate commitment";
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (!inputAggregateKey.MakeFromUniValueRecognizeFormat(request.params[3], &errorStream)) {
        errorStream << "Failed to read input aggregate public key";
        result.pushKV("error", errorStream.str());
        return result;
    }
    //std::cout << "DEBUG: got to before transitioning signers" << std::endl;
    for (unsigned i = 0; i < currentSigners.size(); i ++) {
        if (!currentSigners[i].TransitionSignerState3or4ToState5
             (inputBitmap, inputChallenge, inputAggregateCommitment, inputAggregateKey, &errorStream)
        ) {
            errorStream
            << "Failed to transition the state of signer " << i
            << " (" << i + 1 << " out of " << currentSigners.size() << "). ";
            result.pushKV("error", errorStream.str());
            return result;
        }
    }
    //std::cout << "DEBUG: got to after transitioning signers" << std::endl;
    appendSignerStateToUniValue(result);
    return result;
}

UniValue testaggregatesignaturechallenge(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            "testaggregatesignaturechallenge (committedBitmap, commitmentsCommaSeparatedThenBase64Encoded)\n"
            "\nTests schnorr aggregate signature challenge. Available in -testkanban mode only."
            "\nTo be documented further.\n"
        );
    std::lock_guard<std::mutex> guard1(aggregateSignatureLock);
    UniValue result;
    result.setObject();
    result.pushKV("input", request.params);
    std::vector<PublicKeyKanban> theCommitments;
    std::stringstream errorStream, comments;
    //std::cout << "DEBUG: got to before elliptic curve elts. " << std::endl;
    if (!getVectorOfEllipticCurveElements(request.params[1], theCommitments, &errorStream))
    {
        errorStream << "Failed to extract commitments from your input: " << request.params[1].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (currentSigners.size() < theCommitments.size()) {
        errorStream << "Currently, I have " << currentSigners.size() << " signers but I got more commitments: " << theCommitments.size();
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (theCommitments.size() == 0) {
        errorStream << "0 commitments not allowed";
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (currentSigners.size() > theCommitments.size()) {
        comments << "Got fewer commitments: " << theCommitments.size() << " than signers: " << currentSigners.size()
        << ". I am assuming the commitments belong to the first " << theCommitments.size() << " signers. ";
    }
    std::vector<bool> committedSigners;
    //std::cout << "DEBUG: got to before getbitmap. " << std::endl;
    if (!getBitmap(request.params[0], committedSigners, &errorStream)) {
        errorStream << "Failed to extract bitmap from the first argument: " << request.params[0].write() << ". ";
        result.pushKV("error", errorStream.str());
        return result;
    }
    //std::cout << "DEBUG: got to after getbitmap. " << std::endl;
    theCommitments.resize(currentSigners.size());
    if (!currentAggregator.TransitionSignerState2or3ToState4(theCommitments, committedSigners, nullptr, &errorStream)) {
        errorStream << "Aggregation of commitments failed. ";
        result.pushKV("error", errorStream.str());
        if (comments.str() != "") {
            result.pushKV("comments", comments.str());
        }
        return result;
    }
    if (comments.str() != "") {
        result.pushKV("comments", comments.str());
    }
    result.pushKV("aggregator", currentAggregator.toUniValueTransitionState__SENSITIVE());
    //std::cout << "DEBUG: got to before retun result. " << std::endl;
    return result;
}

UniValue testaggregatesignaturecommit(const JSONRPCRequest& request) {
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2) {
        throw std::runtime_error(
            "testaggregatesignaturecommit(messageHex, desiredNoncesCommaSeparatedBase64)\n"
            "\nTests schnorr aggregate signature commitment. Available in -testkanban mode only."
            "\nTo be documented further.\n"
        );
    }
    std::lock_guard<std::mutex> guard1(aggregateSignatureLock);
    UniValue result;
    result.setObject();
    result.pushKV("input", request.params);
    if (!request.params[0].isStr()) {
        result.pushKV("error", "Parameter 0 expected to be a hex-encoded string.");
        return result;
    }
    std::string messageHex = request.params[0].get_str();
    std::string theMessage;
    std::stringstream errorStream;
    if (!Encodings::fromHex(messageHex, theMessage, &errorStream)) {
        errorStream << "Failed to hex-decode your message: " << messageHex << ". ";
        result.pushKV("error", errorStream.str());
        return result;
    }
    std::vector<std::string> inputNonces;
    if (request.params.size() == 2) {
        if (!getVectorOfStrings(request.params[1], inputNonces, &errorStream)) {
            errorStream << "Failed to extract vector of string from: " << request.params[1].write();
            result.pushKV("error", errorStream.str());
            return result;
        }
    }
    UniValue signersReport;
    signersReport.setArray();
    //compute commitments:
    for (unsigned i = 0; i < currentSigners.size(); i ++) {
        PrivateKeyKanban nonceParser;
        PrivateKeyKanban* desiredNonce = 0;
        if (i < inputNonces.size()) {
            if (!nonceParser.MakeFromBase58DetectCheck(inputNonces[i], &errorStream)) {
                errorStream << "Failed to read nonce index: " << i;
                result.pushKV("error", errorStream.str());
                return result;
            }
            desiredNonce = &nonceParser;
        }
        if (!currentSigners[i].TransitionSignerState1or2ToState3(theMessage, &errorStream, desiredNonce)) {
            errorStream << "Signer " << i << " failed to transition from state 1 to state 3. ";
            result.pushKV("error", errorStream.str());
            return result;
        }
        signersReport.push_back(currentSigners[i].toUniValueTransitionState__SENSITIVE());
    }
    result.pushKV("signers", signersReport);
    if (!currentAggregator.TransitionAggregatorState1ToState2(theMessage, &errorStream)) {
        errorStream << "Failed to transition the aggregator from state 1 to state 2.";
        result.pushKV("error", errorStream.str());
        return result;
    }
    result.pushKV("aggregator", currentAggregator.toUniValueTransitionState__SENSITIVE());
    return result;
}

enum signatureType {
    signatureEcdsaDoubleSha256,
    signatureSchnorrSha3,
};

UniValue testsignature(const JSONRPCRequest& request, signatureType theSignatureType) {

    UniValue result(UniValue::VOBJ);
    result.pushKV("input", request.params);
    std::stringstream errorStream;
    if (request.params.size() != 2 && request.params.size() != 3) {
        errorStream << "Function expects 2 or 3 arguments, got " << request.params.size() << " instead. ";
        result.pushKV("error", errorStream.str());
        return result;
    }
    PrivateKeyKanban thePrivateKey;
    if (!thePrivateKey.MakeFromUniValueRecognizeFormat(request.params[0], &errorStream)) {
        errorStream << "Failed to convert input to secret (private key)";
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (!request.params[1].isStr()) {
        errorStream << "Expected second argument to be a string, instead got: " << request.params[1].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    std::string message;
    if (!Encodings::fromHex(request.params[1].get_str(), message, &errorStream)) {
        errorStream << "Failed to hex-decode the message: " << request.params[2].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    PrivateKeyKanban nonce;
    bool hasNonce = (request.params.size() == 3);
    if (hasNonce)
        hasNonce = request.params[2].isStr();
    if (hasNonce)
        hasNonce = (request.params[2].get_str() != "");
    if (hasNonce) {
        if (!nonce.MakeFromUniValueRecognizeFormat(request.params[2], &errorStream)) {
            result.pushKV("error", errorStream.str());
            return result;
        }
    }
    if (theSignatureType == signatureSchnorrSha3) {
        result.pushKV("signatureType", "SchnorrSha3");
        SignatureSchnorr output;
        if (hasNonce) {
            output.Sign(thePrivateKey, message, &nonce, &result);
        } else {
            output.Sign(thePrivateKey, message, nullptr, &result);
        }
        result.pushKV("challengeHex", output.challenge.ToHexCompressed());
        result.pushKV("solutionBase58Check", output.solution.ToBase58CheckNonConst());
        result.pushKV("signatureBase58", output.ToBase58());
        result.pushKV("signatureBase58Check", output.ToBase58Check());
    }
    if (theSignatureType == signatureEcdsaDoubleSha256) {
        result.pushKV("signatureType", "ECDSADoubleSha256");
        SignatureECDSA output;
        output.messageImplied = std::vector<unsigned char> (message.begin(), message.end());
        std::stringstream commentsSensitive;
        output.SignSha256Squared(thePrivateKey, &commentsSensitive);

        result.pushKV("challengeHex", output.challengeR.ToHexNonConst());
        result.pushKV("solutionBase58Check", output.solutionsS.ToBase58CheckNonConst());
        result.pushKV("signature", HexStr(output.signatureBytes));
    }

    result.pushKV("privateKeyBase58Check", thePrivateKey.ToBase58CheckNonConst());
    return result;
}

UniValue testschnorrverification(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 3)
        throw std::runtime_error(
            "testschnorrverification ( message )\n"
            "\nTests schnorr signature verification. Available in -testkanban mode only."
            "\nTo be documented further.\n"
        );
    UniValue result;
    result.setObject();
    result.pushKV("input", request.params);
    SignatureSchnorr theSignature;
    std::stringstream errorStream;
    if (!theSignature.MakeFromUniValueRecognizeFormat(request.params[0], &errorStream)) {
        errorStream << "Failed to read signature from: " << request.params[0].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (!theSignature.publicKeyImplied.MakeFromUniValueRecognizeFormat(request.params[1], &errorStream)) {
        errorStream << "Failed to read public key from: " << request.params[1].write() << ". ";
        result.pushKV("error", errorStream.str());
        return result;

    }
    if (!request.params[2].isStr()) {
        errorStream << "Expected string for the third argument (the message), instead I got: " << request.params[2].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (!Encodings::fromHex(request.params[2].get_str(), theSignature.messageImplied, &errorStream)) {
        errorStream << "Failed to hex-decode the message: " << request.params[2].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    result.pushKV("challengeHex", theSignature.challenge.ToHexCompressed());
    result.pushKV("solutionBase58Check", theSignature.solution.ToBase58CheckNonConst());
    result.pushKV("publicKeyHex", theSignature.publicKeyImplied.ToHexCompressed());
    bool verificationResult = theSignature.Verify(&errorStream);
    result.pushKV("result", verificationResult);
    if (verificationResult) {
        result.pushKV("resultHTML", "<b style = 'color:green'>Verified</b>");
    } else  {
        result.pushKV("resultHTML", "<b style = 'color:red'>Failed</b>");
    }
    return result;
}

UniValue testecdsaverification(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 3)
        throw std::runtime_error(
            "testecdsaverification ( message )\n"
            "\nTests ecdsa signature verification. Available in -testkanban mode only."
            "\nTo be documented further.\n"
        );
    UniValue result;
    result.setObject();
    result.pushKV("input", request.params);
    SignatureECDSA theSignature;
    std::stringstream errorStream;
    if (!request.params[0].isStr()) {
        errorStream << "First argument is expected to be a string, instead it is: " << request.params[0].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    std::string signatureBytes = request.params[0].get_str();
    if (!theSignature.MakeFromBytes(std::vector<unsigned char>(signatureBytes.begin(), signatureBytes.end()) , &errorStream)) {
        errorStream << "Failed to read signature from: " << request.params[0].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (!theSignature.publicKeyImplied.MakeFromUniValueRecognizeFormat(request.params[1], &errorStream)) {
        errorStream << "Failed to read public key from: " << request.params[1].write() << ". ";
        result.pushKV("error", errorStream.str());
        return result;

    }
    if (!request.params[2].isStr()) {
        errorStream << "Expected string for the third argument (the message), instead I got: " << request.params[2].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    if (!Encodings::fromHex(request.params[2].get_str(), theSignature.messageImplied, &errorStream)) {
        errorStream << "Failed to hex-decode the message: " << request.params[2].write();
        result.pushKV("error", errorStream.str());
        return result;
    }
    result.pushKV("challengeHex", theSignature.challengeR.ToHexNonConst());
    result.pushKV("solutionBase58Check", theSignature.solutionsS.ToBase58CheckNonConst());
    result.pushKV("publicKeyHex", theSignature.publicKeyImplied.ToHexCompressed());
    bool verificationResult = theSignature.Verify(&errorStream);
    result.pushKV("result", verificationResult);
    if (verificationResult) {
        result.pushKV("resultHTML", "<b style = 'color:green'>Verified</b>");
    } else  {
        result.pushKV("resultHTML", "<b style = 'color:red'>Failed</b>");
    }
    return result;
}

UniValue testschnorrsignature(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "testschnorrsignature ( message )\n"
            "\nTests schnorr signature. Available in -testkanban mode only."
            "\nTo be documented further.\n"
        );
    return testsignature(request, signatureSchnorrSha3);
}

UniValue testecdsasignature(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            "testschnorrsignature ( message )\n"
            "\nTests schnorr signature. Available in -testkanban mode only."
            "\nTo be documented further.\n"
        );
    return testsignature(request, signatureEcdsaDoubleSha256);
}

UniValue testshathree(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "testshathree ( message )\n"
            "\nTests sha3 hash function. Available in -testkanban mode only."
            "\nTo be documented further.\n"
        );


    std::string message = DecodeBase64(request.params[0].get_str() );
    Sha3 theSha3;
    UniValue result(UniValue::VOBJ);
    result.pushKV("sha3_256", Encodings::toHexString(theSha3.computeSha3_256(message)));
    result.pushKV("keccak_256", Encodings::toHexString(theSha3.computeKeccak3_256(message)));
    return result;
}

UniValue testprivatekeygeneration(const JSONRPCRequest& request) {
    if (request.fHelp)
        throw std::runtime_error(
            "testprivatekeygeneration ( privatekey )\n"
            "\nTests private key generation. Available in -regtest only."
            "\nTo be documented further.\n"
        );
    PrivateKeyKanban thePrivateKey;
    thePrivateKey.GenerateRandomSecurely();
    UniValue result(UniValue::VOBJ);
    result.pushKV("privateKeyHex", thePrivateKey.ToHexNonConst());
    result.pushKV("privateKeyBase58WithoutCheck", thePrivateKey.ToBase58NonConst());
    result.pushKV("privateKeyBase58Check", thePrivateKey.ToBase58CheckNonConst());
    return result;
}

UniValue testpublickeyfromprivate(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "testpublickeyfromprivate ( privatekey )\n"
            "\nTests public key generation. Available in -testkanban mode only."
            "\nTo be documented further.\n"
        );


    UniValue result(UniValue::VOBJ);
    PrivateKeyKanban thePrivateKey;
    std::stringstream errorStream;
    if (!thePrivateKey.MakeFromUniValueRecognizeFormat(request.params[0], &errorStream)) {
        result.pushKV("error", errorStream.str());
        return result;
    }
    PublicKeyKanban thePublicKey;

    if (!thePrivateKey.ComputePublicKey(thePublicKey, &errorStream)) {
        result.pushKV("error", errorStream.str());
        return result;
    }
    //std::cout << "DEBUG: Public key computed:  " << thePublicKey.ToHexCompressedWithLength() << std::endl;
    result.pushKV("secretHex",  thePrivateKey.ToHexNonConst());
    result.pushKV("privateKeyBase58Check", thePrivateKey.ToBase58CheckNonConst());
    result.pushKV("publicKeyHexCompressed",  thePublicKey.ToHexCompressed());

    result.pushKV("input", request.params);
    return result;
}

UniValue insertaggregatesignature(const JSONRPCRequest& request)
{
    std::stringstream errorStream;
    if (request.fHelp || request.params.size() != 2) {
        errorStream << "insertaggregatesignature not fully documented yet. \n"
                    << "Inputs: rawtransaction (hexstring), rawAggregateSignatureHex (hex string). \n";
        throw std::runtime_error(errorStream.str());
    }
    if (!request.params[0].isStr()) {
        errorStream << "First pararameter is expected to be a hex string, got: " << request.params[0].write() << " instead. ";
        throw JSONRPCError(RPC_INVALID_PARAMETER, errorStream.str());
    }
    if (!request.params[1].isStr()) {
        errorStream << "Third pararameter is expected to be a hex string, got: " << request.params[2].write() << " instead. ";
        throw JSONRPCError(RPC_INVALID_PARAMETER, errorStream.str());
    }
    std::string inputRawTransaction = request.params[0].get_str();
    std::string aggregateSignatureHex = request.params[1].get_str();
    UniValue result;
    result.setObject();
    CMutableTransaction transactionWithoutSignatures;

    if (!DecodeHexTx(transactionWithoutSignatures, inputRawTransaction, true)) {
        errorStream << "Failed to decode your input to raw transaction. Your input: " << inputRawTransaction << ". ";
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, errorStream.str());
    }
    std::vector<unsigned char> aggregateSignatureBytes;
    if (!Encodings::fromHex(aggregateSignatureHex, aggregateSignatureBytes, &errorStream)) {
        errorStream << "Failed to decode your raw aggregate signature. Your input: " << aggregateSignatureHex << ". ";
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, errorStream.str());
    }
    if (!DecodeHexTx(transactionWithoutSignatures, inputRawTransaction, true)) {
        errorStream << "Failed to decode your input to raw transaction. Your input: " << request.params[0].write() << ". ";
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, errorStream.str());
    }
    result.pushKV("inputRawTransaction", inputRawTransaction);
    result.pushKV("inputTransactionDecodedAndRecoded", EncodeHexTx(transactionWithoutSignatures));
    PrecomputedTransactionDatA dataBeforeSignature(transactionWithoutSignatures);


    std::stringstream commentsStream;
    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : transactionWithoutSignatures.vin) {
            view.AccessCoin(txin.prevout); // Load entries from viewChain into view; can fail.
        }
        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }
    for (unsigned int i = 0; i < transactionWithoutSignatures.vin.size(); i++) {
        CTxIn& currentInput = transactionWithoutSignatures.vin[i];
        const Coin& coin = view.AccessCoin(currentInput.prevout);
        if (coin.IsSpent()) {
            errorStream << "Input " << currentInput.ToString() << " not found or already spent. <br>";
            result.pushKV("comments", commentsStream.str());
            result.pushKV("error", errorStream.str());
            return result;
        }
        commentsStream << "Input coin: " << currentInput.ToString() << " is unspent as expected. <br>";
        //const CScript& prevPubKey = coin.out.scriptPubKey;
        //const CAmount& amount = coin.out.nValue;
        if (!coin.out.scriptPubKey.IsPayToAggregateSignature()) {
            commentsStream << "Found unspent UTXO but it is not an aggregate one: " << currentInput.ToString();
            continue;
        }
        commentsStream << "Found unspent aggregate UTXO: " << currentInput.ToString();
        currentInput.scriptSig << aggregateSignatureBytes;
    }
    PrecomputedTransactionDatA dataAfterSignature(transactionWithoutSignatures);

    result.pushKV("precomputedTransactionDataBeforeSignature", dataBeforeSignature.ToString());
    result.pushKV("precomputedTransactionDataAfterSignature", dataAfterSignature.ToString());
    result.pushKV("comments", commentsStream.str());
    result.pushKV("hex", EncodeHexTx(transactionWithoutSignatures));
    return result;
}

static const CRPCCommand testCommands[] =
{ //  category name                                          actor (function)                            okSafe   argNames
  //  -------- -------------------------------------------- ------------------------------------------- -------- ---------------------
  { "test",     "testprivatekeygeneration",                   &testprivatekeygeneration,                  true,    {} },
  { "test",     "testpublickeyfromprivate",                   &testpublickeyfromprivate,                  true,    {"privatekey"} },
  { "test",     "testshathree",                               &testshathree,                              true,    {"message"} },
  { "test",     "testschnorrsignature",                       &testschnorrsignature,                      true,    {"secret", "message", "nonce"} },
  { "test",     "testschnorrverification",                    &testschnorrverification,                   true,    {"publickey", "message", "signature"} },
  { "test",     "testecdsasignature",                         &testecdsasignature,                        true,    {"secret", "message", "nonce"} },
  { "test",     "testecdsaverification",                      &testecdsaverification,                     true,    {"publickey", "message", "signature"} },
  { "test",     "testaggregatesignaturegenerateprivatekeys",  &testaggregatesignaturegenerateprivatekeys, true,    {"numberOfSigners"} },
  { "test",     "testaggregatesignatureinitialize",           &testaggregatesignatureinitialize,          true,    {"publicKeys"} },
  { "test",     "testaggregatesignaturecommit",               &testaggregatesignaturecommit,              true,    {"message", "desiredNoncesCommaSeparatedBase64"} },
  { "test",     "testaggregatesignaturechallenge",            &testaggregatesignaturechallenge,           true,    {"committedSignersBitmap", "commitments"} },
  { "test",     "testaggregatesignaturesolutions",            &testaggregatesignaturesolutions,           true,    {"committedSignersBitmap", "challenge", "aggregateCommitment", "aggregatePublicKey"} },
  { "test",     "testaggregatesignatureaggregation",          &testaggregatesignatureaggregation,         true,    {"solutions"} },
  { "test",     "testaggregatesignatureverification",         &testaggregatesignatureverification,        true,    {"signature", "committedSignersBitmap", "publicKeys", "message"} },
  { "test",     "testaggregateverificationcomplete",          &testaggregateverificationcomplete,         true,    {"signatureComplete", "messageBase64"} },
  { "test",     "insertaggregatesignature",                   &insertaggregatesignature,                  true,    {"hexstring", "number", "hexstring"} },
  { "test",     "getlogfile",                                 &getlogfile,                                true,    {"string"} },
};

void RegisterTestCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(testCommands); vcidx++)
        t.appendCommand(testCommands[vcidx].name, &testCommands[vcidx]);
}
