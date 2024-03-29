// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <chain.h>
#include <coins.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <init.h>
#include <keystore.h>
#include <validation.h>
#include <merkleblock.h>
#include <net.h>
#include <policy/policy.h>
#include <policy/rbf.h>
#include <primitives/transaction.h>
#include <rpc/server.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/standard.h>
#include <txmempool.h>
#include <uint256.h>
#include <utilstrencodings.h>
#include <utilmoneystr.h>
#include "aggregate_schnorr_signature.h"
#include <encodings_crypto.h>
#ifdef ENABLE_WALLET
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#endif

#include <stdint.h>

#include <univalue.h>

void avoidCompilerWarningsDefinedButNotUsedRawTransaction() {
    (void) FetchSCARShardPublicKeysInternalPointer;
}

void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry)
{
    // Call into TxToUniv() in fabcoin-common to decode the transaction hex.
    //
    // Blockchain contextual information (confirmations and blocktime) is not
    // available to code in fabcoin-common, so we query them here and push the
    // data into the returned UniValue.
    TxToUniv(tx, uint256(), entry, true, RPCSerializationFlags());

    if (!hashBlock.IsNull()) {
        entry.push_back(Pair("blockhash", hashBlock.GetHex()));
        BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end() && (*mi).second) {
            CBlockIndex* pindex = (*mi).second;
            if (chainActive.Contains(pindex)) {
                entry.push_back(Pair("confirmations", 1 + chainActive.Height() - pindex->nHeight));
                entry.push_back(Pair("time", pindex->GetBlockTime()));
                entry.push_back(Pair("blocktime", pindex->GetBlockTime()));
            }
            else
                entry.push_back(Pair("confirmations", 0));
        }
    }
}

UniValue gethexaddress(const JSONRPCRequest& request) {
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 1)
        throw std::runtime_error(
                "gethexaddress \"address\"\n"

                        "\nConverts a base58 pubkeyhash address to a hex address for use in smart contracts.\n"

                        "\nArguments:\n"
                        "1. \"address\"      (string, required) The base58 address\n"

                        "\nResult:\n"
                        "\"hexaddress\"      (string) The raw hex pubkeyhash address for use in smart contracts\n"

                        "\nExamples:\n"
                + HelpExampleCli("gethexaddress", "\"address\"")
                + HelpExampleRpc("gethexaddress", "\"address\"")
        );

    CFabcoinAddress address(request.params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Fabcoin address");

    if(!address.IsPubKeyHash())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Only pubkeyhash addresses are supported");

    return boost::get<CKeyID>(address.Get()).GetReverseHex();
}

UniValue fromhexaddress(const JSONRPCRequest& request) {
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 1)
        throw std::runtime_error(
                "fromhexaddress \"hexaddress\"\n"

                        "\nConverts a raw hex address to a base58 pubkeyhash address\n"

                        "\nArguments:\n"
                        "1. \"hexaddress\"      (string, required) The raw hex address\n"

                        "\nResult:\n"
                        "\"address\"      (string) The base58 pubkeyhash address\n"

                        "\nExamples:\n"
                + HelpExampleCli("fromhexaddress", "\"hexaddress\"")
                + HelpExampleRpc("fromhexaddress", "\"hexaddress\"")
        );
    if (request.params[0].get_str().size() != 40)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid pubkeyhash hex size (should be 40 hex characters)");
    CKeyID raw;
    raw.SetReverseHex(request.params[0].get_str());
    CFabcoinAddress address(raw);

    return address.ToString();
}

UniValue getrawtransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "getrawtransaction \"txid\" ( verbose )\n"

            "\nNOTE: By default this function only works for mempool transactions. If the -txindex option is\n"
            "enabled, it also works for blockchain transactions.\n"
            "DEPRECATED: for now, it also works for transactions with unspent outputs.\n"

            "\nReturn the raw transaction data.\n"
            "\nIf verbose is 'true', returns an Object with information about 'txid'.\n"
            "If verbose is 'false' or omitted, returns a string that is serialized, hex-encoded data for 'txid'.\n"

            "\nArguments:\n"
            "1. \"txid\"      (string, required) The transaction id\n"
            "2. verbose       (bool, optional, default=false) If false, return a string, otherwise return a json object\n"

            "\nResult (if verbose is not set or set to false):\n"
            "\"data\"      (string) The serialized, hex-encoded data for 'txid'\n"

            "\nResult (if verbose is set to true):\n"
            "{\n"
            "  \"hex\" : \"data\",       (string) The serialized, hex-encoded data for 'txid'\n"
            "  \"txid\" : \"id\",        (string) The transaction id (same as provided)\n"
            "  \"hash\" : \"id\",        (string) The transaction hash (differs from txid for witness transactions)\n"
            "  \"size\" : n,             (numeric) The serialized transaction size\n"
            "  \"vsize\" : n,            (numeric) The virtual transaction size (differs from size for witness transactions)\n"
            "  \"version\" : n,          (numeric) The version\n"
            "  \"locktime\" : ttt,       (numeric) The lock time\n"
            "  \"vin\" : [               (array of json objects)\n"
            "     {\n"
            "       \"txid\": \"id\",    (string) The transaction id\n"
            "       \"vout\": n,         (numeric) \n"
            "       \"scriptSig\": {     (json object) The script\n"
            "         \"asm\": \"asm\",  (string) asm\n"
            "         \"hex\": \"hex\"   (string) hex\n"
            "       },\n"
            "       \"sequence\": n      (numeric) The script sequence number\n"
            "       \"txinwitness\": [\"hex\", ...] (array of string) hex-encoded witness data (if any)\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "  \"vout\" : [              (array of json objects)\n"
            "     {\n"
            "       \"value\" : x.xxx,            (numeric) The value in " + CURRENCY_UNIT + "\n"
            "       \"n\" : n,                    (numeric) index\n"
            "       \"scriptPubKey\" : {          (json object)\n"
            "         \"asm\" : \"asm\",          (string) the asm\n"
            "         \"hex\" : \"hex\",          (string) the hex\n"
            "         \"reqSigs\" : n,            (numeric) The required sigs\n"
            "         \"type\" : \"pubkeyhash\",  (string) The type, eg 'pubkeyhash'\n"
            "         \"addresses\" : [           (json array of string)\n"
            "           \"address\"        (string) fabcoin address\n"
            "           ,...\n"
            "         ]\n"
            "       }\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "  \"blockhash\" : \"hash\",   (string) the block hash\n"
            "  \"confirmations\" : n,      (numeric) The confirmations\n"
            "  \"time\" : ttt,             (numeric) The transaction time in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"blocktime\" : ttt         (numeric) The block time in seconds since epoch (Jan 1 1970 GMT)\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("getrawtransaction", "\"mytxid\"")
            + HelpExampleCli("getrawtransaction", "\"mytxid\" true")
            + HelpExampleRpc("getrawtransaction", "\"mytxid\", true")
        );

    LOCK(cs_main);

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    // Accept either a bool (true) or a num (>=1) to indicate verbose output.
    bool fVerbose = false;
    if (!request.params[1].isNull()) {
        if (request.params[1].isNum()) {
            if (request.params[1].get_int() != 0) {
                fVerbose = true;
            }
        }
        else if(request.params[1].isBool()) {
            if(request.params[1].isTrue()) {
                fVerbose = true;
            }
        }
        else {
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid type provided. Verbose parameter must be a boolean.");
        }
    }

    CTransactionRef tx;
    uint256 hashBlock;
    if (!GetTransaction(hash, tx, Params().GetConsensus(), hashBlock, true))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string(fTxIndex ? "No such mempool or blockchain transaction"
            : "No such mempool transaction. Use -txindex to enable blockchain transaction queries") +
            ". Use gettransaction for wallet transactions.");

    if (!fVerbose)
        return EncodeHexTx(*tx, RPCSerializationFlags());

    UniValue result(UniValue::VOBJ);
    TxToJSON(*tx, hashBlock, result);
    return result;
}

UniValue gettxoutproof(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 1 && request.params.size() != 2))
        throw std::runtime_error(
            "gettxoutproof [\"txid\",...] ( blockhash )\n"
            "\nReturns a hex-encoded proof that \"txid\" was included in a block.\n"
            "\nNOTE: By default this function only works sometimes. This is when there is an\n"
            "unspent output in the utxo for this transaction. To make it always work,\n"
            "you need to maintain a transaction index, using the -txindex command line option or\n"
            "specify the block in which the transaction is included manually (by blockhash).\n"
            "\nArguments:\n"
            "1. \"txids\"       (string) A json array of txids to filter\n"
            "    [\n"
            "      \"txid\"     (string) A transaction hash\n"
            "      ,...\n"
            "    ]\n"
            "2. \"blockhash\"   (string, optional) If specified, looks for txid in the block with this hash\n"
            "\nResult:\n"
            "\"data\"           (string) A string that is a serialized, hex-encoded data for the proof.\n"
        );

    std::set<uint256> setTxids;
    uint256 oneTxid;
    UniValue txids = request.params[0].get_array();
    for (unsigned int idx = 0; idx < txids.size(); idx++) {
        const UniValue& txid = txids[idx];
        if (txid.get_str().length() != 64 || !IsHex(txid.get_str()))
            throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid txid ")+txid.get_str());
        uint256 hash(uint256S(txid.get_str()));
        if (setTxids.count(hash))
            throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated txid: ")+txid.get_str());
       setTxids.insert(hash);
       oneTxid = hash;
    }

    LOCK(cs_main);

    CBlockIndex* pblockindex = nullptr;

    uint256 hashBlock;
    if (!request.params[1].isNull())
    {
        hashBlock = uint256S(request.params[1].get_str());
        if (!mapBlockIndex.count(hashBlock))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        pblockindex = mapBlockIndex[hashBlock];
    } else {
        // Loop through txids and try to find which block they're in. Exit loop once a block is found.
        for (const auto& tx : setTxids) {
            const Coin& coin = AccessByTxid(*pcoinsTip, tx);
            if (!coin.IsSpent()) {
                pblockindex = chainActive[coin.nHeight];
                break;
            }
        }
    }

    if (pblockindex == nullptr)
    {
        CTransactionRef tx;
        if (!GetTransaction(oneTxid, tx, Params().GetConsensus(), hashBlock, false) || hashBlock.IsNull())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not yet in block");
        if (!mapBlockIndex.count(hashBlock))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Transaction index corrupt");
        pblockindex = mapBlockIndex[hashBlock];
    }

    CBlock block;
    if(!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus()))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");

    unsigned int ntxFound = 0;
    for (const auto& tx : block.vtx)
        if (setTxids.count(tx->GetHash()))
            ntxFound++;
    if (ntxFound != setTxids.size())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Not all transactions found in specified or retrieved block");

    CDataStream ssMB(SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS);
    CMerkleBlock mb(block, setTxids);
    ssMB << mb;
    std::string strHex = HexStr(ssMB.begin(), ssMB.end());
    return strHex;
}

UniValue verifytxoutproof(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "verifytxoutproof \"proof\"\n"
            "\nVerifies that a proof points to a transaction in a block, returning the transaction it commits to\n"
            "and throwing an RPC error if the block is not in our best chain\n"
            "\nArguments:\n"
            "1. \"proof\"    (string, required) The hex-encoded proof generated by gettxoutproof\n"
            "\nResult:\n"
            "[\"txid\"]      (array, strings) The txid(s) which the proof commits to, or empty array if the proof is invalid\n"
        );

    CDataStream ssMB(ParseHexV(request.params[0], "proof"), SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS);
    CMerkleBlock merkleBlock;
    ssMB >> merkleBlock;

    UniValue res(UniValue::VARR);

    std::vector<uint256> vMatch;
    std::vector<unsigned int> vIndex;
    if (merkleBlock.txn.ExtractMatches(vMatch, vIndex) != merkleBlock.header.hashMerkleRoot)
        return res;

    LOCK(cs_main);

    if (!mapBlockIndex.count(merkleBlock.header.GetHash()) || !chainActive.Contains(mapBlockIndex[merkleBlock.header.GetHash()]))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found in chain");

    for (const uint256& hash : vMatch)
        res.push_back(hash.GetHex());
    return res;
}

bool GetPublicKeysHexFromArray(const UniValue& input, std::vector<std::string>& outputAppend, std::stringstream* reasonForFailure)
{
    unsigned maxNumberOfPublicKeys = 2048;
    if (!input.isArray()) {
        if (reasonForFailure != 0) {
            *reasonForFailure << "The public keys entry: " << input.write() << " does not appear to be an array. ";
        }
        return false;
    }
    if (input.size() > maxNumberOfPublicKeys) {
        if (reasonForFailure != 0) {
            *reasonForFailure << "Too many (" << input.size() << ") public keys. ";
        }
        return false;
    }
    for (unsigned i = 0; i < input.size(); i ++) {
        if (!input[i].isStr()) {
            if (reasonForFailure != 0) {
                *reasonForFailure << "Failed to extract public key string at index: "
                                  << i << "(" << i + 1 << " out of " << input.size() << "). "
                                  << "The input was: " << input[i].write();
            }
            return false;
        }
        outputAppend.push_back(input[i].get_str());
    }
    return true;
}

bool GetPublicKeysHexFromString(const UniValue& input, std::vector<std::string>& outputAppend, std::stringstream* reasonForFailure)
{
    unsigned maxNumberOfPublicKeys = 2048;
    unsigned lengthPublicKeyCompressed = PublicKeyKanban::lengthPublicKeySerializationCompressed; // = 33
    if (!input.isStr()) {
        if (reasonForFailure != 0) {
            *reasonForFailure << "The public keys entry: " << input.write() << " is not a hex string, as expected. ";
        }
        return false;
    }
    std::string inputString = input.get_str();
    if (inputString.size() > maxNumberOfPublicKeys * lengthPublicKeyCompressed * 2 + 4) {
        if (reasonForFailure != 0) {
            *reasonForFailure << "Input too large ( " << inputString.size() << " bytes). ";
        }
        return false;
    }

    if (inputString.size() < 4) {
        if (reasonForFailure != 0) {
            *reasonForFailure << "Too few hex characters in public keys hex entry: "
                              << inputString << ". At least 2 bytes = 4 hex characters expected. ";
        }
        return false;
    }
    std::string firstFour = inputString.substr(0, 4);
    std::vector<unsigned char> firstTwoBytes;
    if (!Encodings::fromHex(firstFour, firstTwoBytes, reasonForFailure)) {
        if (reasonForFailure != 0) {
            *reasonForFailure << "Failed to extract 2 bytes from the first 4 hex characters of your input: "
            << firstFour;
        }
        return false;
    }
    if (firstTwoBytes.size() != 2) {
        if (reasonForFailure != 0) {
            *reasonForFailure << "Failed to extract 2 bytes from the first 4 characters of your input: " << firstFour;
        }
        return false;
    }
    unsigned expectedNumberOfPublicKeys = ((unsigned int)(firstTwoBytes[0])) * 256 + (unsigned int)(firstTwoBytes[1]);
    unsigned expectedSize = 4 + lengthPublicKeyCompressed * 2 * expectedNumberOfPublicKeys;
    if (inputString.size() != expectedSize) {
        if (reasonForFailure != 0) {
            *reasonForFailure << "Expected 4 + 33 * 2 * " << expectedNumberOfPublicKeys
                              << " = " << expectedSize  << " hex characters, got " << inputString.size()
                              << " instead. ";
        }
        return false;
    }
    unsigned offset = 4;
    for (unsigned i = 0; i < expectedNumberOfPublicKeys; i ++) {
        std::string currentSlice = inputString.substr(offset, lengthPublicKeyCompressed * 2);
        outputAppend.push_back(currentSlice);
        offset += 2 * lengthPublicKeyCompressed;
    }
    return true;
}

bool GetPublicKeysFromAggregateSignature(
    const UniValue& input, std::vector<unsigned char>& outputPublicKeysSerialized, std::stringstream* reasonForFailure
) {
    unsigned maxNumberOfPublicKeys = 2048;
    unsigned lengthPublicKeyCompressed = PublicKeyKanban::lengthPublicKeySerializationCompressed; //=33
    std::vector<std::string> publicKeysHex;
    if (input.exists("publicKeys")) {
        UniValue publicKeysUniValue = input["publicKeys"];
        if (!GetPublicKeysHexFromArray(publicKeysUniValue, publicKeysHex, reasonForFailure)) {
            return false;
        }
    }
    if (input.exists("publicKeysHex")) {
        UniValue publicKeysUniValue = input["publicKeysHex"];
        if (!GetPublicKeysHexFromString(publicKeysUniValue, publicKeysHex, reasonForFailure)) {
            return false;
        }
    }
    if (publicKeysHex.size() > maxNumberOfPublicKeys) {
        if (reasonForFailure != 0) {
            *reasonForFailure << "Too many public keys: " << publicKeysHex.size() << ". Maximum allowed: " << maxNumberOfPublicKeys << ". ";
        }
        return false;
    }
    unsigned numKeys = publicKeysHex.size();
    std::vector<PublicKeyKanban> thePublicKeys;
    thePublicKeys.resize(numKeys);
    for (unsigned i = 0; i < numKeys; i ++) {
        if (!thePublicKeys[i].MakeFromStringRecognizeFormat(publicKeysHex[i], reasonForFailure)) {
            if (reasonForFailure != 0) {
                *reasonForFailure
                << "Failed to extract public key index "
                << i << " (" << i + 1 << " out of " << numKeys << "). ";
            }
            return false;
        }
    }

    outputPublicKeysSerialized.clear();
    outputPublicKeysSerialized.reserve( 2 + numKeys * lengthPublicKeyCompressed);
    unsigned char topByte = (unsigned char) (numKeys / 256);
    unsigned char bottomByte = (unsigned char) (numKeys % 256);
    outputPublicKeysSerialized.push_back(topByte);
    outputPublicKeysSerialized.push_back(bottomByte);
    std::vector<unsigned char> currentBytes;
    for (unsigned i = 0; i < numKeys; i ++) {
        thePublicKeys[i].SerializeCompressed(currentBytes);
        for (unsigned j = 0; j < currentBytes.size(); j ++) {
            outputPublicKeysSerialized.push_back(currentBytes[j]);
        }
    }
    return true;
}

UniValue createrawtransaction(const JSONRPCRequest& request) {
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 4)
        throw std::runtime_error(
            "createrawtransaction [{\"txid\":\"id\",\"vout\":n},...] {\"aggregation\": {...(see below)}, \"address\":amount,\"data\":\"hex\",...} ( locktime ) ( replaceable )\n"
            "\nCreate a transaction spending the given inputs and creating new outputs.\n"
            "Outputs can be addresses or data.\n"
            "Returns hex-encoded raw transaction.\n"
            "Note that the transaction's inputs are not signed, and\n"
            "it is not stored in the wallet or transmitted to the network.\n"

            "\nArguments:\n"
            "1. \"inputs\"                (array, required) A json array of json objects\n"
            "     [\n"
            "       {\n"
            "         \"txid\":\"id\",    (string, required) The transaction id\n"
            "         \"vout\":n,         (numeric, required) The output number\n"
            "         \"sequence\":n      (numeric, optional) The sequence number\n"
            "       } \n"
            "       ,...\n"
            "     ]\n"
            "2. \"outputs\"               (object, required) a json object with outputs\n"
            "    {\n"
            "      \"aggregateSignature\":{\n       (required when address not present)\n"
            "           \"publicKeys\": [\"pubKeyCompressedHex1\", \"pubKeyCompressedHex2\", ...],      (array, required when pubKeysHex not present) \n"
            "           \"publicKeysHex\": \"serializedPubKeysHex\",                                    (string, required when pubKeys not present) First two bytes: number of pubkeys, big-endian. Next 33*numPubKeys: the public keys compressed. All data hex-encoded.\n"
            "           \"amount\": x.xxx (numeric or string) Amount to be transferred."
            "      },\n"
            "      \"address\": x.xxx,    (numeric or string, required when aggregation not present) The key is the fabcoin address, the numeric value (can be string) is the " + CURRENCY_UNIT + " amount\n"
            "      \"data\": \"hex\"      (string, required) The key is \"data\", the value is hex encoded data\n"
            "      \"contract\":{\n"
            "         \"contractAddress\":\"address\", (string, required) Valid contract address (valid hash160 hex data)\n"
            "         \"data\":\"hex\",                (string, required) Hex data to add in the call output\n"
            "         \"amount\":x.xxx,                (numeric, optional) Value in fasc to send with the call, should be a valid amount, default 0\n"
            "         \"gasLimit\":x,                  (numeric, optional) The gas limit for the transaction\n"
            "         \"gasPrice\":x.xxxxxxxx          (numeric, optional) The gas price for the transaction\n"
            "       } \n"
            "      ,...\n"
            "    }\n"
            "3. locktime                  (numeric, optional, default=0) Raw locktime. Non-0 value also locktime-activates inputs\n"
            "4. replaceable               (boolean, optional, default=false) Marks this transaction as BIP125 replaceable.\n"
            "                             Allows this transaction to be replaced by a transaction with higher fees. If provided, it is an error if explicit sequence numbers are incompatible.\n"
            "\nResult:\n"
            "\"transaction\"              (string) hex string of the transaction\n"

            "\nExamples:\n"
            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"address\\\":0.01}\"")
            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"data\\\":\\\"00010203\\\"}\"")
            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"contract\\\":{\\\"contractAddress\\\":\\\"mycontract\\\","
                                                     "\\\"data\\\":\\\"00\\\", \\\"gasLimit\\\":250000, \\\"gasPrice\\\":0.00000040, \\\"amount\\\":0}}\"")
            + HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"address\\\":0.01}\"")
            + HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"data\\\":\\\"00010203\\\"}\"")
            + HelpExampleRpc("createrawtransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"contract\\\":{\\\"contractAddress\\\":\\\"mycontract\\\","
                                                     "\\\"data\\\":\\\"00\\\", \\\"gasLimit\\\":250000, \\\"gasPrice\\\":0.00000040, \\\"amount\\\":0}}\"")

        );

    RPCTypeCheck(request.params, {UniValue::VARR, UniValue::VOBJ, UniValue::VNUM}, true);
    if (request.params[0].isNull() || request.params[1].isNull())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, arguments 1 and 2 must be non-null");

    UniValue inputs = request.params[0].get_array();
    UniValue sendTo = request.params[1].get_obj();

    CMutableTransaction rawTx;

    if (request.params.size() > 2 && !request.params[2].isNull()) {
        int64_t nLockTime = request.params[2].get_int64();
        if (nLockTime < 0 || nLockTime > std::numeric_limits<uint32_t>::max())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, locktime out of range");
        rawTx.nLockTime = nLockTime;
    }

    bool rbfOptIn = request.params.size() > 3 ? request.params[3].isTrue() : false;

    for (unsigned int idx = 0; idx < inputs.size(); idx++) {
        const UniValue& input = inputs[idx];
        const UniValue& o = input.get_obj();

        uint256 txid = ParseHashO(o, "txid");

        const UniValue& vout_v = find_value(o, "vout");
        if (!vout_v.isNum())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing vout key");
        int nOutput = vout_v.get_int();
        if (nOutput < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");

        uint32_t nSequence;
        if (rbfOptIn) {
            nSequence = MAX_BIP125_RBF_SEQUENCE;
        } else if (rawTx.nLockTime) {
            nSequence = std::numeric_limits<uint32_t>::max() - 1;
        } else {
            nSequence = std::numeric_limits<uint32_t>::max();
        }

        // set the sequence number if passed in the parameters object
        const UniValue& sequenceObj = find_value(o, "sequence");
        if (sequenceObj.isNum()) {
            int64_t seqNr64 = sequenceObj.get_int64();
            if (seqNr64 < 0 || seqNr64 > std::numeric_limits<uint32_t>::max()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, sequence number is out of range");
            } else {
                nSequence = (uint32_t)seqNr64;
            }
        }

        CTxIn in(COutPoint(txid, nOutput), CScript(), nSequence);

        rawTx.vin.push_back(in);
    }

    std::set<CFabcoinAddress> setAddress;
    std::vector<std::string> addrList = sendTo.getKeys();
    for (const std::string& name_ : addrList) {

        if (name_ == "data") {
            std::vector<unsigned char> data = ParseHexV(sendTo[name_].getValStr(),"Data");

            CTxOut out(0, CScript() << OP_RETURN << data);
            rawTx.vout.push_back(out);
        } else if (name_ == "contract") {
            if ( chainActive.Height() <  (int)Params().GetConsensus().ContractHeight  )
               throw JSONRPCError(RPC_METHOD_NOT_FOUND, std::string ("This method can only be used after fasc fork, after block ") + std::to_string(Params().GetConsensus().ContractHeight ));

            // Get the call object
            UniValue Contract = sendTo[name_];
            if(!Contract.isObject())
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, need to be object: ")+name_);

            // Get dgp gas limit and gas price
            LOCK(cs_main);
            FascDGP fascDGP(globalState.get(), fGettingValuesDGP);
            uint64_t blockGasLimit = fascDGP.getBlockGasLimit(chainActive.Height());
            uint64_t minGasPrice = CAmount(fascDGP.getMinGasPrice(chainActive.Height()));
            CAmount nGasPrice = (minGasPrice>DEFAULT_GAS_PRICE)?minGasPrice:DEFAULT_GAS_PRICE;

            // Get the contract address
            if(!Contract.exists("contractAddress") || !Contract["contractAddress"].isStr())
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, contract address is mandatory."));

            std::string contractaddress = Contract["contractAddress"].get_str();
            if(contractaddress.size() != 40 || !CheckHex(contractaddress))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Incorrect contract address");

            dev::Address addrAccount(contractaddress);
            if(!globalState->addressInUse(addrAccount))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "contract address does not exist");

            // Get the contract data
            if(!Contract.exists("data") || !Contract["data"].isStr())
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, contract data is mandatory."));

            std::string datahex = Contract["data"].get_str();
            if(datahex.size() % 2 != 0 || !CheckHex(datahex))
                throw JSONRPCError(RPC_TYPE_ERROR, "Invalid data (data not hex)");

            // Get amount
            CAmount nAmount = 0;
            if (Contract.exists("amount")){
                nAmount = AmountFromValue(Contract["amount"]);
                if (nAmount < 0)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for call contract");
            }

            // Get gas limit
            uint64_t nGasLimit = DEFAULT_GAS_LIMIT_OP_SEND;
            if (Contract.exists("gasLimit")){
                nGasLimit = Contract["gasLimit"].get_int64();
                if (nGasLimit > blockGasLimit)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Maximum is: "+i64tostr(blockGasLimit)+")");
                if (nGasLimit < MINIMUM_GAS_LIMIT)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Minimum is: "+i64tostr(MINIMUM_GAS_LIMIT)+")");
                if (nGasLimit <= 0)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit");
            }

            // Get gas price
            if (Contract.exists("gasPrice")){
                UniValue uGasPrice = Contract["gasPrice"];
                if(!ParseMoney(uGasPrice.getValStr(), nGasPrice))
                {
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice");
                }
                CAmount maxRpcGasPrice = gArgs.GetArg("-rpcmaxgasprice", MAX_RPC_GAS_PRICE);
                if (nGasPrice > (int64_t)maxRpcGasPrice)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice, Maximum allowed in RPC calls is: "+FormatMoney(maxRpcGasPrice)+" (use -rpcmaxgasprice to change it)");
                if (nGasPrice < (int64_t)minGasPrice)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice (Minimum is: "+FormatMoney(minGasPrice)+")");
                if (nGasPrice <= 0)
                    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice");
            }

            // Add call contract output
            CScript scriptPubKey = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(nGasLimit) << CScriptNum(nGasPrice) << ParseHex(datahex) << ParseHex(contractaddress) << OP_CALL;
            CTxOut out(nAmount, scriptPubKey);
            rawTx.vout.push_back(out);
        } else if (name_ == "aggregateSignature" ) {
            std::stringstream errorStream;
            if ( chainActive.Height() <  (int)Params().GetConsensus().AggregateSignatureHeight) {
                errorStream << "This method can only be used after aggregate signature fork, block "
                            << Params().GetConsensus().AggregateSignatureHeight << ". ";
                throw JSONRPCError(RPC_METHOD_NOT_FOUND, errorStream.str());
            }
            UniValue aggregation = sendTo[name_];
            std::vector<unsigned char> publicKeysSerialized;
            if (!GetPublicKeysFromAggregateSignature(aggregation, publicKeysSerialized, &errorStream)) {
                errorStream << "Error processing aggregation input: " << aggregation.write();
                throw JSONRPCError(RPC_INVALID_PARAMETER, errorStream.str());
            }
            // Get amount
            CAmount nAmount = 0;
            if (aggregation.exists("amount")){
                nAmount = AmountFromValue(aggregation["amount"]);
                errorStream << "Error: the amount I got: " << nAmount << " appears to be negative. ";
                if (nAmount < 0)
                    throw JSONRPCError(RPC_TYPE_ERROR, errorStream.str());
            }
            CScript scriptPubKey = CScript() << publicKeysSerialized << OP_AGGREGATEVERIFY;
            CTxOut out(nAmount, scriptPubKey);
            rawTx.vout.push_back(out);
        } else {
            CFabcoinAddress address(name_);
            if (!address.IsValid())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Fabcoin address: ")+name_);

            if (setAddress.count(address))
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated address: ")+name_);
            setAddress.insert(address);

            CScript scriptPubKey = GetScriptForDestination(address.Get());
            CAmount nAmount = AmountFromValue(sendTo[name_]);

            CTxOut out(nAmount, scriptPubKey);
            rawTx.vout.push_back(out);
        }
    }

    if (!request.params[3].isNull() && rbfOptIn != SignalsOptInRBF(rawTx)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter combination: Sequence number(s) contradict replaceable option");
    }

    return EncodeHexTx(rawTx);
}

UniValue decoderawtransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "decoderawtransaction \"hexstring\"\n"
            "\nReturn a JSON object representing the serialized, hex-encoded transaction.\n"

            "\nArguments:\n"
            "1. \"hexstring\"      (string, required) The transaction hex string\n"

            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"id\",        (string) The transaction id\n"
            "  \"hash\" : \"id\",        (string) The transaction hash (differs from txid for witness transactions)\n"
            "  \"size\" : n,             (numeric) The transaction size\n"
            "  \"vsize\" : n,            (numeric) The virtual transaction size (differs from size for witness transactions)\n"
            "  \"version\" : n,          (numeric) The version\n"
            "  \"locktime\" : ttt,       (numeric) The lock time\n"
            "  \"vin\" : [               (array of json objects)\n"
            "     {\n"
            "       \"txid\": \"id\",    (string) The transaction id\n"
            "       \"vout\": n,         (numeric) The output number\n"
            "       \"scriptSig\": {     (json object) The script\n"
            "         \"asm\": \"asm\",  (string) asm\n"
            "         \"hex\": \"hex\"   (string) hex\n"
            "       },\n"
            "       \"txinwitness\": [\"hex\", ...] (array of string) hex-encoded witness data (if any)\n"
            "       \"sequence\": n     (numeric) The script sequence number\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "  \"vout\" : [             (array of json objects)\n"
            "     {\n"
            "       \"value\" : x.xxx,            (numeric) The value in " + CURRENCY_UNIT + "\n"
            "       \"n\" : n,                    (numeric) index\n"
            "       \"scriptPubKey\" : {          (json object)\n"
            "         \"asm\" : \"asm\",          (string) the asm\n"
            "         \"hex\" : \"hex\",          (string) the hex\n"
            "         \"reqSigs\" : n,            (numeric) The required sigs\n"
            "         \"type\" : \"pubkeyhash\",  (string) The type, eg 'pubkeyhash'\n"
            "         \"addresses\" : [           (json array of string)\n"
            "           \"12tvKAXCxZjSmdNbao16dKXC8tRWfcF5oc\"   (string) fabcoin address\n"
            "           ,...\n"
            "         ]\n"
            "       }\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("decoderawtransaction", "\"hexstring\"")
            + HelpExampleRpc("decoderawtransaction", "\"hexstring\"")
        );

    LOCK(cs_main);
    RPCTypeCheck(request.params, {UniValue::VSTR});

    CMutableTransaction mtx;

    if (!DecodeHexTx(mtx, request.params[0].get_str(), true))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

    UniValue result(UniValue::VOBJ);
    TxToUniv(CTransaction(std::move(mtx)), uint256(), result, false);

    return result;
}

UniValue getcontractaddress(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getcontractaddress \"hexstring\"\n"
            "\nReturn the expected contract address of a transaction.\n"

            "\nArguments:\n"
            "1. \"hexstring\"      (string, required) The transaction hex string\n"

            "\nResult:\n"
            "\"address\"      (string) The fabcoin address of the contract.\n"

            "\nExamples:\n"
            + HelpExampleCli("getcontractaddress", "\"hexstring\"")
            + HelpExampleRpc("getcontractaddress", "\"hexstring\"")
        );

    LOCK(cs_main);
    RPCTypeCheck(request.params, {UniValue::VSTR});

    CMutableTransaction mtx;

    if (!DecodeHexTx(mtx, request.params[0].get_str(), true))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

    std::vector<unsigned char> SHA256TxVout(32);
    std::vector<unsigned char> contractAddress(20);
    std::vector<unsigned char> txIdAndVout(mtx.GetHash().begin(), mtx.GetHash().end());

    /* uint256 GetHash is meant to provide a length 32 array, but reading from GDB
     * the size and contents of GetHash is nondeterministic; I'm pretty sure it's
     * picking up garbage from somewhere and throwing it in with the correct
     * initial 32 chars.
     * We resize the array after calling constructor to ensure we're getting a hash
     * of the correct length.
     */
    txIdAndVout.resize(32);
    uint32_t voutNumber = 0;
    bool txHasOpCreate = false;
    for (auto txout : mtx.vout) {
        if (txout.scriptPubKey.HasOpCreate()) {
            std::vector<unsigned char> voutNumberChrs;
            if (voutNumberChrs.size() < sizeof(voutNumber))
                voutNumberChrs.resize(sizeof(voutNumber));
            std::memcpy(voutNumberChrs.data(), &voutNumber, sizeof(voutNumber));
            txIdAndVout.insert(txIdAndVout.end(), voutNumberChrs.begin(), voutNumberChrs.end());
            txHasOpCreate = true;
            break;
        }
        voutNumber++;
    }

    if (!txHasOpCreate)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Given transaction does not create a contract");

    CSHA256().Write(txIdAndVout.data(), txIdAndVout.size()).Finalize(SHA256TxVout.data());
    CRIPEMD160().Write(SHA256TxVout.data(), SHA256TxVout.size()).Finalize(contractAddress.data());

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("address", HexStr(contractAddress)));

    return result;
}

UniValue decodescript(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "decodescript \"hexstring\"\n"
            "\nDecode a hex-encoded script.\n"
            "\nArguments:\n"
            "1. \"hexstring\"     (string) the hex encoded script\n"
            "\nResult:\n"
            "{\n"
            "  \"asm\":\"asm\",   (string) Script public key\n"
            "  \"hex\":\"hex\",   (string) hex encoded public key\n"
            "  \"type\":\"type\", (string) The output type\n"
            "  \"reqSigs\": n,    (numeric) The required signatures\n"
            "  \"addresses\": [   (json array of string)\n"
            "     \"address\"     (string) fabcoin address\n"
            "     ,...\n"
            "  ],\n"
            "  \"p2sh\",\"address\" (string) address of P2SH script wrapping this redeem script (not returned if the script is already a P2SH).\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("decodescript", "\"hexstring\"")
            + HelpExampleRpc("decodescript", "\"hexstring\"")
        );

    RPCTypeCheck(request.params, {UniValue::VSTR});

    UniValue r(UniValue::VOBJ);
    CScript script;
    if (request.params[0].get_str().size() > 0){
        std::vector<unsigned char> scriptData(ParseHexV(request.params[0], "argument"));
        script = CScript(scriptData.begin(), scriptData.end());
    } else {
        // Empty scripts are valid
    }
    ScriptPubKeyToUniv(script, r, false);

    UniValue type;
    type = find_value(r, "type");

    if (type.isStr() && type.get_str() != "scripthash") {
        // P2SH cannot be wrapped in a P2SH. If this script is already a P2SH,
        // don't return the address for a P2SH of the P2SH.
        r.push_back(Pair("p2sh", CFabcoinAddress(CScriptID(script)).ToString()));
    }

    return r;
}

/** Pushes a JSON object for script verification or signing errors to vErrorsRet. */
static void TxInErrorToJSON(const CTxIn& txin, UniValue& vErrorsRet, const std::string& strMessage)
{
    UniValue entry(UniValue::VOBJ);
    entry.push_back(Pair("txid", txin.prevout.hash.ToString()));
    entry.push_back(Pair("vout", (uint64_t)txin.prevout.n));
    UniValue witness(UniValue::VARR);
    for (unsigned int i = 0; i < txin.scriptWitness.stack.size(); i++) {
        witness.push_back(HexStr(txin.scriptWitness.stack[i].begin(), txin.scriptWitness.stack[i].end()));
    }
    entry.push_back(Pair("witness", witness));
    entry.push_back(Pair("scriptSig", HexStr(txin.scriptSig.begin(), txin.scriptSig.end())));
    entry.push_back(Pair("sequence", (uint64_t)txin.nSequence));
    entry.push_back(Pair("error", strMessage));
    vErrorsRet.push_back(entry);
}

UniValue combinerawtransaction(const JSONRPCRequest& request)
{

    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "combinerawtransaction [\"hexstring\",...]\n"
            "\nCombine multiple partially signed transactions into one transaction.\n"
            "The combined transaction may be another partially signed transaction or a \n"
            "fully signed transaction."

            "\nArguments:\n"
            "1. \"txs\"         (string) A json array of hex strings of partially signed transactions\n"
            "    [\n"
            "      \"hexstring\"     (string) A transaction hash\n"
            "      ,...\n"
            "    ]\n"

            "\nResult:\n"
            "\"hex\"            (string) The hex-encoded raw transaction with signature(s)\n"

            "\nExamples:\n"
            + HelpExampleCli("combinerawtransaction", "[\"myhex1\", \"myhex2\", \"myhex3\"]")
        );


    UniValue txs = request.params[0].get_array();
    std::vector<CMutableTransaction> txVariants(txs.size());

    for (unsigned int idx = 0; idx < txs.size(); idx++) {
        if (!DecodeHexTx(txVariants[idx], txs[idx].get_str(), true)) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("TX decode failed for tx %d", idx));
        }
    }

    if (txVariants.empty()) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Missing transactions");
    }

    // mergedTx will end up with all the signatures; it
    // starts as a clone of the rawtx:
    CMutableTransaction mergedTx(txVariants[0]);

    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(cs_main);
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : mergedTx.vin) {
            view.AccessCoin(txin.prevout); // Load entries from viewChain into view; can fail.
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(mergedTx);
    // Sign what we can:
    for (unsigned int i = 0; i < mergedTx.vin.size(); i++) {
        CTxIn& txin = mergedTx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            throw JSONRPCError(RPC_VERIFY_ERROR, "Input not found or already spent");
        }
        const CScript& prevPubKey = coin.out.scriptPubKey;
        const CAmount& amount = coin.out.nValue;

        SignatureData sigdata;

        // ... and merge in other signatures:
        for (const CMutableTransaction& txv : txVariants) {
            if (txv.vin.size() > i) {
                sigdata = CombineSignatures(prevPubKey, TransactionSignatureChecker(&txConst, i, amount), sigdata, DataFromTransaction(txv, i));
            }
        }

        UpdateTransaction(mergedTx, i, sigdata);
    }

    return EncodeHexTx(mergedTx);
}

UniValue signrawtransaction(const JSONRPCRequest& request)
{
#ifdef ENABLE_WALLET
    CWallet * const pwallet = GetWalletForJSONRPCRequest(request);
#endif

    if (request.fHelp || request.params.size() < 1 || request.params.size() > 4)
        throw std::runtime_error(
            "signrawtransaction \"hexstring\" ( [{\"txid\":\"id\",\"vout\":n,\"scriptPubKey\":\"hex\",\"redeemScript\":\"hex\"},...] [\"privatekey1\",...] sighashtype )\n"
            "\nSign inputs for raw transaction (serialized, hex-encoded).\n"
            "The second optional argument (may be null) is an array of previous transaction outputs that\n"
            "this transaction depends on but may not yet be in the block chain.\n"
            "The third optional argument (may be null) is an array of base58-encoded private\n"
            "keys that, if given, will be the only keys used to sign the transaction.\n"
#ifdef ENABLE_WALLET
            + HelpRequiringPassphrase(pwallet) + "\n"
#endif

            "\nArguments:\n"
            "1. \"hexstring\"     (string, required) The transaction hex string\n"
            "2. \"prevtxs\"       (string, optional) An json array of previous dependent transaction outputs\n"
            "     [               (json array of json objects, or 'null' if none provided)\n"
            "       {\n"
            "         \"txid\":\"id\",             (string, required) The transaction id\n"
            "         \"vout\":n,                  (numeric, required) The output number\n"
            "         \"scriptPubKey\": \"hex\",   (string, required) script key\n"
            "         \"redeemScript\": \"hex\",   (string, required for P2SH or P2WSH) redeem script\n"
            "         \"amount\": value            (numeric, required) The amount spent\n"
            "       }\n"
            "       ,...\n"
            "    ]\n"
            "3. \"privkeys\"     (string, optional) A json array of base58-encoded private keys for signing\n"
            "    [                  (json array of strings, or 'null' if none provided)\n"
            "      \"privatekey\"   (string) private key in base58-encoding\n"
            "      ,...\n"
            "    ]\n"
            "4. \"sighashtype\"     (string, optional, default=ALL) The signature hash type. Must be one of\n"
            "       \"ALL\"\n"
            "       \"NONE\"\n"
            "       \"SINGLE\"\n"
            "       \"ALL|ANYONECANPAY\"\n"
            "       \"NONE|ANYONECANPAY\"\n"
            "       \"SINGLE|ANYONECANPAY\"\n"

            "\nResult:\n"
            "{\n"
            "  \"hex\" : \"value\",           (string) The hex-encoded raw transaction with signature(s)\n"
            "  \"complete\" : true|false,   (boolean) If the transaction has a complete set of signatures\n"
            "  \"errors\" : [                 (json array of objects) Script verification errors (if there are any)\n"
            "    {\n"
            "      \"txid\" : \"hash\",           (string) The hash of the referenced, previous transaction\n"
            "      \"vout\" : n,                (numeric) The index of the output to spent and used as input\n"
            "      \"scriptSig\" : \"hex\",       (string) The hex-encoded signature script\n"
            "      \"sequence\" : n,            (numeric) Script sequence number\n"
            "      \"error\" : \"text\"           (string) Verification or signing error related to the input\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("signrawtransaction", "\"myhex\"")
            + HelpExampleRpc("signrawtransaction", "\"myhex\"")
        );

#ifdef ENABLE_WALLET
    LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
#else
    LOCK(cs_main);
#endif
    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR, UniValue::VARR, UniValue::VSTR}, true);

    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str(), true))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : mtx.vin) {
            view.AccessCoin(txin.prevout); // Load entries from viewChain into view; can fail.
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    bool fGivenKeys = false;
    CBasicKeyStore tempKeystore;
    if (request.params.size() > 2 && !request.params[2].isNull()) {
        fGivenKeys = true;
        UniValue keys = request.params[2].get_array();
        for (unsigned int idx = 0; idx < keys.size(); idx++) {
            UniValue k = keys[idx];
            CFabcoinSecret vchSecret;
            bool fGood = vchSecret.SetString(k.get_str());
            if (!fGood)
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
            CKey key = vchSecret.GetKey();
            if (!key.IsValid())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key outside allowed range");
            tempKeystore.AddKey(key);
        }
    }
#ifdef ENABLE_WALLET
    else if (pwallet) {
        EnsureWalletIsUnlocked(pwallet);
    }
#endif

    // Add previous txouts given in the RPC call:
    if (request.params.size() > 1 && !request.params[1].isNull()) {
        UniValue prevTxs = request.params[1].get_array();
        for (unsigned int idx = 0; idx < prevTxs.size(); idx++) {
            const UniValue& p = prevTxs[idx];
            if (!p.isObject())
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"txid'\",\"vout\",\"scriptPubKey\"}");

            UniValue prevOut = p.get_obj();

            RPCTypeCheckObj(prevOut,
                {
                    {"txid", UniValueType(UniValue::VSTR)},
                    {"vout", UniValueType(UniValue::VNUM)},
                    {"scriptPubKey", UniValueType(UniValue::VSTR)},
                });

            uint256 txid = ParseHashO(prevOut, "txid");

            int nOut = find_value(prevOut, "vout").get_int();
            if (nOut < 0)
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "vout must be positive");

            COutPoint out(txid, nOut);
            std::vector<unsigned char> pkData(ParseHexO(prevOut, "scriptPubKey"));
            CScript scriptPubKey(pkData.begin(), pkData.end());

            {
                const Coin& coin = view.AccessCoin(out);
                if (!coin.IsSpent() && coin.out.scriptPubKey != scriptPubKey) {
                    std::string err("Previous output scriptPubKey mismatch:\n");
                    err = err + ScriptToAsmStr(coin.out.scriptPubKey) + "\nvs:\n"+
                        ScriptToAsmStr(scriptPubKey);
                    throw JSONRPCError(RPC_DESERIALIZATION_ERROR, err);
                }
                Coin newcoin;
                newcoin.out.scriptPubKey = scriptPubKey;
                newcoin.out.nValue = 0;
                if (prevOut.exists("amount")) {
                    newcoin.out.nValue = AmountFromValue(find_value(prevOut, "amount"));
                }
                newcoin.nHeight = 1;
                view.AddCoin(out, std::move(newcoin), true);
            }

            // if redeemScript given and not using the local wallet (private keys
            // given), add redeemScript to the tempKeystore so it can be signed:
            if (fGivenKeys && (scriptPubKey.IsPayToScriptHash() || scriptPubKey.IsPayToWitnessScriptHash())) {
                RPCTypeCheckObj(prevOut,
                    {
                        {"txid", UniValueType(UniValue::VSTR)},
                        {"vout", UniValueType(UniValue::VNUM)},
                        {"scriptPubKey", UniValueType(UniValue::VSTR)},
                        {"redeemScript", UniValueType(UniValue::VSTR)},
                    });
                UniValue v = find_value(prevOut, "redeemScript");
                if (!v.isNull()) {
                    std::vector<unsigned char> rsData(ParseHexV(v, "redeemScript"));
                    CScript redeemScript(rsData.begin(), rsData.end());
                    tempKeystore.AddCScript(redeemScript);
                }
            }
        }
    }

#ifdef ENABLE_WALLET
    const CKeyStore& keystore = ((fGivenKeys || !pwallet) ? tempKeystore : *pwallet);
#else
    const CKeyStore& keystore = tempKeystore;
#endif

    int nHashType = SIGHASH_ALL;
    if (request.params.size() > 3 && !request.params[3].isNull()) {
        static std::map<std::string, int> mapSigHashValues = {
            {std::string("ALL"), int(SIGHASH_ALL)},
            {std::string("ALL|ANYONECANPAY"), int(SIGHASH_ALL|SIGHASH_ANYONECANPAY)},
            {std::string("NONE"), int(SIGHASH_NONE)},
            {std::string("NONE|ANYONECANPAY"), int(SIGHASH_NONE|SIGHASH_ANYONECANPAY)},
            {std::string("SINGLE"), int(SIGHASH_SINGLE)},
            {std::string("SINGLE|ANYONECANPAY"), int(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY)},
        };
        std::string strHashType = request.params[3].get_str();
        if (mapSigHashValues.count(strHashType))
            nHashType = mapSigHashValues[strHashType];
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid sighash param");
    }

    bool fHashSingle = ((nHashType & ~SIGHASH_ANYONECANPAY) == SIGHASH_SINGLE);

    // Script verification errors
    UniValue vErrors(UniValue::VARR);

    //std::stringstream transactionConstructionProgress;
    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(mtx);
    //Comments is a pointer to a stream that will contain technical but non-sensitive details on the signature,
    //including the actual bytes being signed by the ECDSA algorithm.
    //Once again, no sensitive information is written in the comments.
    //Without having timed it, I expect that the comments have a neglible effect on penalty,
    //so passing on the comments stream on mainnet should pose no risks.
    //The comments are turned on only when the fIncludeTestCommands flag is set, i.e., on regtest and
    //regtestwithnet.
    std::stringstream bufferStream;
    std::stringstream* commentsNonSensitive = nullptr;
    if (Params().IncludeTestCommands()) {
        commentsNonSensitive = &bufferStream;
    }
    // Sign what we can:
    for (unsigned int i = 0; i < mtx.vin.size(); i++) {
        CTxIn& txin = mtx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            TxInErrorToJSON(txin, vErrors, "Input not found or already spent");
            continue;
        }
        const CScript& prevPubKey = coin.out.scriptPubKey;
        const CAmount& amount = coin.out.nValue;

        SignatureData sigdata;
        // Only sign SIGHASH_SINGLE if there's a corresponding output:
        if (!fHashSingle || (i < mtx.vout.size()))
            ProduceSignature(MutableTransactionSignatureCreator(&keystore, &mtx, i, amount, nHashType), prevPubKey, sigdata, commentsNonSensitive);
        sigdata = CombineSignatures(prevPubKey, TransactionSignatureChecker(&txConst, i, amount), sigdata, DataFromTransaction(mtx, i));

        UpdateTransaction(mtx, i, sigdata);

        ScriptError serror = SCRIPT_ERR_OK;
        if (!VerifyScript(txin.scriptSig, prevPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txConst, i, amount), &serror)) {
            TxInErrorToJSON(txin, vErrors, ScriptErrorString(serror));
        }
    }
    bool fComplete = vErrors.empty();

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("hex", EncodeHexTx(mtx)));
    result.push_back(Pair("complete", fComplete));
    if (!vErrors.empty()) {
        result.push_back(Pair("errors", vErrors));
    }
    if (commentsNonSensitive != nullptr) {
        result.pushKV("comments", commentsNonSensitive->str());
    }
    return result;
}

UniValue sendrawtransaction(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "sendrawtransaction \"hexstring\" ( allowhighfees )\n"
            "\nSubmits raw transaction (serialized, hex-encoded) to local node and network.\n"
            "\nAlso see createrawtransaction and signrawtransaction calls.\n"
            "\nArguments:\n"
            "1. \"hexstring\"    (string, required) The hex string of the raw transaction)\n"
            "2. allowhighfees    (boolean, optional, default=false) Allow high fees\n"
            "\nResult:\n"
            "\"hex\"             (string) The transaction hash in hex\n"
            "\nExamples:\n"
            "\nCreate a transaction\n"
            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\" : \\\"mytxid\\\",\\\"vout\\\":0}]\" \"{\\\"myaddress\\\":0.01}\"") +
            "Sign the transaction, and get back the hex\n"
            + HelpExampleCli("signrawtransaction", "\"myhex\"") +
            "\nSend the transaction (signed hex)\n"
            + HelpExampleCli("sendrawtransaction", "\"signedhex\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("sendrawtransaction", "\"signedhex\"")
        );

    LOCK(cs_main);
    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VBOOL});

    // parse hex string from parameter
    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    CTransactionRef tx(MakeTransactionRef(std::move(mtx)));
    const uint256& hashTx = tx->GetHash();

    CAmount nMaxRawTxFee = maxTxFee;
    if (request.params.size() > 1 && request.params[1].get_bool())
        nMaxRawTxFee = 0;

    CCoinsViewCache &view = *pcoinsTip;
    bool fHaveChain = false;
    for (size_t o = 0; !fHaveChain && o < tx->vout.size(); o++) {
        const Coin& existingCoin = view.AccessCoin(COutPoint(hashTx, o));
        fHaveChain = !existingCoin.IsSpent();
    }
    bool fHaveMempool = mempool.exists(hashTx);
    std::stringstream comments;
    if (!fHaveMempool && !fHaveChain) {
        // push to local node and sync with wallets
        CValidationState state;
        bool fMissingInputs;
        bool fLimitFree = true;
        if (!AcceptToMemoryPool(mempool, state, std::move(tx), fLimitFree, &fMissingInputs, nullptr, false, nMaxRawTxFee, true, &comments)) {
            comments << state.GetRejectCode() << ": " << state.GetRejectReason();
            if (state.IsInvalid()) {
                throw JSONRPCError(RPC_TRANSACTION_REJECTED, comments.str());
            } else {
                if (fMissingInputs) {
                    comments << "Missing inputs.\n";
                    throw JSONRPCError(RPC_TRANSACTION_ERROR, comments.str());
                }
                throw JSONRPCError(RPC_TRANSACTION_ERROR, comments.str());
            }
        }
    } else if (fHaveChain) {
        throw JSONRPCError(RPC_TRANSACTION_ALREADY_IN_CHAIN, "transaction already in block chain");
    }
    if(!g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    CInv inv(MSG_TX, hashTx);
    g_connman->ForEachNode([&inv](CNode* pnode)
    {
        pnode->PushInventory(inv);
    });
    UniValue result;
    result.setObject();
    result.pushKV("txid", hashTx.GetHex());
    result.pushKV("comments", comments.str());
    return result;
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "rawtransactions",    "getrawtransaction",      &getrawtransaction,      true,  {"txid","verbose"} },
    { "rawtransactions",    "createrawtransaction",   &createrawtransaction,   true,  {"inputs","outputs","locktime","replaceable"} },
    { "rawtransactions",    "decoderawtransaction",   &decoderawtransaction,   true,  {"hexstring"} },
    { "rawtransactions",    "getcontractaddress",     &getcontractaddress,     true,  {"hexstring"} },
    { "rawtransactions",    "decodescript",           &decodescript,           true,  {"hexstring"} },
    { "rawtransactions",    "sendrawtransaction",     &sendrawtransaction,     false, {"hexstring","allowhighfees"} },
    { "rawtransactions",    "combinerawtransaction",  &combinerawtransaction,  true,  {"txs"} },
    { "rawtransactions",    "signrawtransaction",     &signrawtransaction,     false, {"hexstring","prevtxs","privkeys","sighashtype"} }, /* uses wallet if enabled */
    { "rawtransactions",    "gethexaddress",          &gethexaddress,          true,  {"address",} },
    { "rawtransactions",    "fromhexaddress",         &fromhexaddress,         true,  {"hexaddress",} },

    { "blockchain",         "gettxoutproof",          &gettxoutproof,          true,  {"txids", "blockhash"} },
    { "blockchain",         "verifytxoutproof",       &verifytxoutproof,       true,  {"proof"} },
};

void RegisterRawTransactionRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
