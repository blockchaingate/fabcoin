#include <sstream>
#include <validation.h>
#include <chainparams.h>
#include <fasc/fascstate.h>
#include "log_session.h"
#include <libethereum/ExtVM.h>

void avoidCompilerWarningsDefinedButNotUsedFascState() {
    (void) FetchSCARShardPublicKeysInternalPointer;
}

using namespace std;
using namespace dev;
using namespace dev::eth;

FascState::FascState(u256 const& _accountStartNonce, OverlayDB const& _db, const string& _path, BaseState _bs) :
    State(_accountStartNonce, _db, _bs) {
    dbUTXO = FascState::openDB(_path + "/fascDB", sha3(rlp("")), WithExisting::Trust);
    stateUTXO = SecureTrieDB<Address, OverlayDB>(&dbUTXO);
}

FascState::FascState() : dev::eth::State(dev::Invalid256, dev::OverlayDB(), dev::eth::BaseState::PreExisting) {
    dbUTXO = OverlayDB();
    stateUTXO = SecureTrieDB<Address, OverlayDB>(&dbUTXO);
}

const std::string feeAcceptance = "__________________Fees accepted.";

u256 FascState::GetFeesPromisedByLogs(const std::vector<dev::eth::LogEntry>& logs)
{
    for (unsigned i = 0; i < logs.size(); i ++) {
        const dev::eth::LogEntry& currentLog = logs[i];
        if (currentLog.data.size() != feeAcceptance.size()) {
            continue;
        }
        bool foundFeeCoverage = true;
        for (unsigned j = 0; j < currentLog.data.size(); j ++) {
            if (currentLog.data[j] != feeAcceptance[j]) {
                foundFeeCoverage = false;
                break;
            }
        }
        if (!foundFeeCoverage) {
            continue;
        }
        if (i + 1 >= logs.size()) {
            return 0;
        }
        u256 result = 0;
        const std::vector<unsigned char>& feesPromised = logs[i + 1].data;
        for (unsigned j = 0; j < feesPromised.size(); j ++) {
            result *= 256;
            result += feesPromised[j];
        }
        return result;
    }
    return 0;
}

bool FascState::accountFeesCoveredByContract(
    const Address& contractAddress,
    const u256& feesCovered,
    std::stringstream* commentsOnFailure
) {
    if (feesCovered == 0) {
        return true;
    }
    dev::u256 ballanceOnHand = this->balance(contractAddress);
    if (feesCovered > ballanceOnHand) {
        if (commentsOnFailure != nullptr) {
            *commentsOnFailure << "Insufficient funds: contract " << contractAddress
                               << " requires at least "
                               << feesCovered << " to cover fees but it has only "
                               << ballanceOnHand << " available.\n";
        }
        return false;
    }
    this->subBalance(contractAddress, feesCovered);
    return true;
}

ResultExecute FascState::execute(
        EnvInfo const& _envInfo,
        SealEngineFace const& _sealEngine,
        FascTransaction const& _t,
        Permanence _p,
        OnOpFunc const& _onOp,
        std::stringstream* commentsNullForNone
) {

    assert(_t.getVersion().toRaw() == VersionVM::GetEVMDefault().toRaw());

    addBalance(_t.sender(), _t.value() + (_t.gas() * _t.gasPrice()));
    newAddress = _t.isCreation() ? createFascAddress(_t.getHashWith(), _t.getNVout()) : dev::Address();

    _sealEngine.deleteAddresses.insert({_t.sender(), _envInfo.author()});

    h256 oldStateRoot = rootHash();
    bool voutLimit = false;

    auto onOp = _onOp;
#if ETH_VMTRACE
    if (isChannelVisible<VMTraceChannel>())
        onOp = Executive::simpleTrace(); // override tracer
#endif
    // Create and initialize the executive. This will throw fairly cheaply and quickly if the
    // transaction is bad in any way.
    Executive e(*this, _envInfo, _sealEngine);
    ExecutionResult res;
    e.setResultRecipient(res);
    CTransactionRef tx;
    u256 startGasUsed;
    try {
        if (_t.isCreation() && _t.value())
            BOOST_THROW_EXCEPTION(CreateWithValue());

        e.initialize(_t);
        // OK - transaction looks valid - execute.
        startGasUsed = _envInfo.gasUsed();
        //Please note: e.execute returns 0 if execution went normally.
        if (!e.execute(commentsNullForNone)) {
            //normal path assumed by a well-executing contract.
            e.go(onOp, commentsNullForNone);
        } else {
            //something went wrong with execution
            e.revert();
            throw Exception();
        }
        e.finalize();
        dev::h160 contractAddress = _t.to();
        //std::vector<std::vector<uint8_t> > aggregationData;
        //e.GetAggregationData(aggregationData, contractAddress, commentsNullForNone);
        dev::u256 feesCoveredByLogs = this->GetFeesPromisedByLogs(e.logs());
        if (!this->accountFeesCoveredByContract(contractAddress, feesCoveredByLogs, commentsNullForNone)) {
            if (commentsNullForNone != nullptr) {
                *commentsNullForNone << "Failed to account fees covered by contract.\n";
            }
            throw Exception();
        }
        if (res.excepted != TransactionException::None && commentsNullForNone != nullptr) {
            if (commentsNullForNone != nullptr) {
                *commentsNullForNone << "Error during EVM execution: " << res.excepted << ".\n";
            }
            ExtVM* theExternalVMFace = e.getExternalVMFace().get();
            LogEntries& theLogs = theExternalVMFace->logsGenerated;
            if (commentsNullForNone != nullptr) {
                *commentsNullForNone << e.ToStringLogs(theLogs);
            }
        }
        if (_p == Permanence::Reverted) {
            this->m_cache.clear();
            this->cacheUTXO.clear();
        } else {
            deleteAccounts(_sealEngine.deleteAddresses);
            if (res.excepted == TransactionException::None) {
                CondensingTX ctx(this, transfers, _t, _sealEngine.deleteAddresses);
                tx = MakeTransactionRef(ctx.createCondensingTX(feesCoveredByLogs,
                                                               //aggregationData,
                                                               contractAddress,
                                                               commentsNullForNone));
                if (ctx.reachedVoutLimit()) {
                    voutLimit = true;
                    e.revert();
                    throw Exception();
                }
                std::unordered_map<dev::Address, Vin> vins = ctx.createVin(*tx);
                updateUTXO(vins);
            } else {
                if (commentsNullForNone != nullptr) {
                    *commentsNullForNone << "Error during EVM execution: " << res.excepted << ".\n";
                }
                printfErrorLog(res.excepted);
            }
            fasc::commit(cacheUTXO, stateUTXO, m_cache);
            cacheUTXO.clear();
            bool removeEmptyAccounts = _envInfo.number() >= _sealEngine.chainParams().u256Param("EIP158ForkBlock");
            commit(removeEmptyAccounts ? State::CommitBehaviour::RemoveEmptyAccounts : State::CommitBehaviour::KeepEmptyAccounts);
        }
    }
    catch (Exception const& _e) {
        if (commentsNullForNone != nullptr) {
            *commentsNullForNone << "EVM exception. " << e.ToStringLogs(e.logs()) << "\n";
        }
        printfErrorLog(dev::eth::toTransactionException(_e), _e.what());
        res.excepted = dev::eth::toTransactionException(_e);
        res.gasUsed = _t.gas();
        const Consensus::Params& consensusParams = Params().GetConsensus();
        if(chainActive.Height() < consensusParams.nFixUTXOCacheHFHeight && _p != Permanence::Reverted) {
            deleteAccounts(_sealEngine.deleteAddresses);
            commit(CommitBehaviour::RemoveEmptyAccounts);
        } else {
            m_cache.clear();
            cacheUTXO.clear();
        }
    }
    if (!_t.isCreation())
        res.newAddress = _t.receiveAddress();
    newAddress = dev::Address();
    transfers.clear();
    if (voutLimit) {
        //use old and empty states to create virtual Out Of Gas exception
        LogEntries logs;
        u256 gas = _t.gas();
        ExecutionResult ex;
        ex.gasRefunded = 0;
        ex.gasUsed = gas;
        ex.excepted = TransactionException();
        //create a refund tx to send back any coins that were suppose to be sent to the contract
        CMutableTransaction refund;
        if (_t.value() > 0) {
            refund.vin.push_back(CTxIn(h256Touint(_t.getHashWith()), _t.getNVout(), CScript() << OP_SPEND));
            //note, if sender was a non-standard tx, this will send the coins to pubkeyhash 0x00, effectively destroying the coins
            CScript script(CScript() << OP_DUP << OP_HASH160 << _t.sender().asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
            refund.vout.push_back(CTxOut(CAmount(_t.value().convert_to<uint64_t>()), script));
        }
        //make sure to use empty transaction if no vouts made
        return ResultExecute{ex, dev::eth::TransactionReceipt(oldStateRoot, gas, e.logs()), refund.vout.empty() ? CTransaction() : CTransaction(refund)};
    } else {
        return ResultExecute{res, dev::eth::TransactionReceipt(rootHash(), startGasUsed + e.gasUsed(), e.logs()), tx ? *tx : CTransaction()};
    }
}

std::unordered_map<dev::Address, Vin> FascState::vins() const // temp
{
    std::unordered_map<dev::Address, Vin> ret;
    for (auto& i: cacheUTXO)
        if (i.second.alive)
            ret[i.first] = i.second;
    auto addrs = addresses();
    for (auto& i : addrs) {
        if (cacheUTXO.find(i.first) == cacheUTXO.end() && vin(i.first))
            ret[i.first] = *vin(i.first);
    }
    return ret;
}

void FascState::transferBalance(dev::Address const& _from, dev::Address const& _to, dev::u256 const& _value) {
    subBalance(_from, _value);
    addBalance(_to, _value);
    if (_value > 0)
        transfers.push_back({_from, _to, _value});
}

Vin const* FascState::vin(dev::Address const& _a) const
{
    return const_cast<FascState*>(this)->vin(_a);
}

Vin* FascState::vin(dev::Address const& _addr)
{
    auto it = cacheUTXO.find(_addr);
    if (it == cacheUTXO.end()) {
        std::string stateBack = stateUTXO.at(_addr);
        if (stateBack.empty())
            return nullptr;

        dev::RLP state(stateBack);
        auto i = cacheUTXO.emplace(
                     std::piecewise_construct,
                     std::forward_as_tuple(_addr),
                     std::forward_as_tuple(Vin{state[0].toHash<dev::h256>(), state[1].toInt<uint32_t>(), state[2].toInt<dev::u256>(), state[3].toInt<uint8_t>()})
                 );
        return &i.first->second;
    }
    return &it->second;
}

void FascState::kill(dev::Address _addr)
{
    // If the account is not in the db, nothing to kill.
    if (auto a = account(_addr))
        a->kill();
    if (auto v = vin(_addr))
        v->alive = 0;
}

void FascState::addBalance(dev::Address const& _id, dev::u256 const& _amount)
{
    if (dev::eth::Account* a = account(_id))
    {
        // Log empty account being touched. Empty touched accounts are cleared
        // after the transaction, so this event must be also reverted.
        // We only log the first touch (not dirty yet), and only for empty
        // accounts, as other accounts does not matter.
        // TODO: to save space we can combine this event with Balance by having
        //       Balance and Balance+Touch events.
        if (!a->isDirty() && a->isEmpty())
            m_changeLog.emplace_back(dev::eth::detail::Change::Touch, _id);

        // Increase the account balance. This also is done for value 0 to mark
        // the account as dirty. Dirty account are not removed from the cache
        // and are cleared if empty at the end of the transaction.
        a->addBalance(_amount);
    } else {
        if (!addressInUse(newAddress) && newAddress != dev::Address()) {
            const_cast<dev::Address&>(_id) = newAddress;
            newAddress = dev::Address();
        }
        createAccount(_id, {requireAccountStartNonce(), _amount});
    }

    if (_amount)
        m_changeLog.emplace_back(dev::eth::detail::Change::Balance, _id, _amount);
}

dev::Address FascState::createFascAddress(dev::h256 hashTx, uint32_t voutNumber)
{
    uint256 hashTXid(h256Touint(hashTx));
    std::vector<unsigned char> txIdAndVout(hashTXid.begin(), hashTXid.end());
    std::vector<unsigned char> voutNumberChrs;
    if (voutNumberChrs.size() < sizeof(voutNumber))voutNumberChrs.resize(sizeof(voutNumber));
    std::memcpy(voutNumberChrs.data(), &voutNumber, sizeof(voutNumber));
    txIdAndVout.insert(txIdAndVout.end(),voutNumberChrs.begin(),voutNumberChrs.end());

    std::vector<unsigned char> SHA256TxVout(32);
    CSHA256().Write(txIdAndVout.data(), txIdAndVout.size()).Finalize(SHA256TxVout.data());

    std::vector<unsigned char> hashTxIdAndVout(20);
    CRIPEMD160().Write(SHA256TxVout.data(), SHA256TxVout.size()).Finalize(hashTxIdAndVout.data());

    return dev::Address(hashTxIdAndVout);
}

void FascState::deleteAccounts(std::set<dev::Address>& addrs) {
    for(dev::Address addr : addrs) {
        dev::eth::Account* acc = const_cast<dev::eth::Account*>(account(addr));
        if(acc)
            acc->kill();
        Vin* in = const_cast<Vin*>(vin(addr));
        if(in)
            in->alive = 0;
    }
}

void FascState::updateUTXO(const std::unordered_map<dev::Address, Vin>& vins) {
    for(auto& v : vins) {
        Vin* vi = const_cast<Vin*>(vin(v.first));

        if(vi) {
            vi->hash = v.second.hash;
            vi->nVout = v.second.nVout;
            vi->value = v.second.value;
            vi->alive = v.second.alive;
        } else if(v.second.alive > 0) {
            cacheUTXO[v.first] = v.second;
        }
    }
}

void FascState::printfErrorLog(const dev::eth::TransactionException er, const std::string& errorMessage) {
    std::stringstream ss;
    ss << "VM exception: " << er << ". " << errorMessage;
    clog(ExecutiveWarnChannel) << ss.str();
}

///////////////////////////////////////////////////////////////////////////////////////////
void CondensingTX::createAggregationVouts(const std::vector<std::vector<uint8_t> >& aggregationData,
                                          CMutableTransaction& outputTransaction,
                                          stringstream *commentsNullForNone)
{
    (void) commentsNullForNone;
    if (aggregationData.size() == 0) {
        return;
    }
    CScript extraScript = CScript();
    for (unsigned counterData = 0; counterData < aggregationData.size(); counterData ++) {
        extraScript << aggregationData[counterData];
    }
    extraScript << OP_AGGREGATEVERIFY;
    outputTransaction.vout.push_back(CTxOut(0, extraScript));
    if (outputTransaction.vin.size() == 0) {

        CTxIn theTX(h256Touint(this->transaction.getHashWith()), this->transaction.getNVout(), CScript() << OP_SPEND);
        outputTransaction.vin.push_back(theTX);
    }
}

CTransaction CondensingTX::createCondensingTX(
        const dev::u256& feesPromisedByContract,
        //const std::vector<std::vector<uint8_t> >* aggregationData,
        const dev::h160& contractAddress,
        std::stringstream* commentsNullForNone
) {
    this->selectionVin(feesPromisedByContract, contractAddress, commentsNullForNone);
    bool shouldAddExtraVin = false;
    if (this->vins.size() == 0) {
        if (feesPromisedByContract > 0)
            shouldAddExtraVin = true;
    }
    if (shouldAddExtraVin) {
        this->vins[contractAddress] = Vin{transaction.getHashWith(), transaction.getNVout(), 0, 1};
    }

    this->calculatePlusAndMinus(feesPromisedByContract, contractAddress);
    if (!this->createNewBalances(commentsNullForNone)) {
        if (commentsNullForNone != nullptr) {
            *commentsNullForNone << "Failed to create new balances.\n";
        }
        return CTransaction();
    }
    CMutableTransaction result;
    result.vin = createVins(commentsNullForNone);
    result.vout = this->createVout(commentsNullForNone);
    //this->createAggregationVouts(*aggregationData, commentsNullForNone);
    return !result.vin.size() || !result.vout.size() ? CTransaction() : CTransaction(result);
}

std::unordered_map<dev::Address, Vin> CondensingTX::createVin(const CTransaction& tx) {
    std::unordered_map<dev::Address, Vin> vins;
    for (auto& b : balances) {
        if(b.first == transaction.sender())
            continue;

        if(b.second > 0) {
            vins[b.first] = Vin{uintToh256(tx.GetHash()), nVouts[b.first], b.second, 1};
        } else {
            vins[b.first] = Vin{uintToh256(tx.GetHash()), 0, 0, 0};
        }
    }
    return vins;
}

void CondensingTX::selectionVin(const dev::u256& feesPromisedByContract, const dev::h160& contractAddress, std::stringstream* comments)
{
    (void) comments; //<- avoid unused variable compiler warning.
    int i  = 0;
    for (const TransferInfo& theTransferInfo : this->transfers) {
        i ++;
        bool senderHasInput = this->vins.count(theTransferInfo.from);
        if (!senderHasInput) {
            Vin* senderInput = this->state->vin(theTransferInfo.from);
            if (senderInput != nullptr) {
                this->vins[theTransferInfo.from] = *senderInput;
            }
            if (theTransferInfo.from == this->transaction.sender() && this->transaction.value() > 0) {
                this->vins[theTransferInfo.from] = Vin{
                        this->transaction.getHashWith(),
                        this->transaction.getNVout(),
                        this->transaction.value(),
                        1 //<- alive
                };
            }
        }
        if (!this->vins.count(theTransferInfo.to)) {
            Vin* a = this->state->vin(theTransferInfo.to);
            if (a != nullptr) {
                this->vins[theTransferInfo.to] = *a;
            }
        }
    }
    if (feesPromisedByContract > 0) {
        //If fees are covered by the contract call,
        //a coin must be selected for the contract to pay.
        Vin* contractInput = this->state->vin(contractAddress);
        if (contractInput != nullptr) {
            this->vins[contractAddress] = *contractInput;
        }
    }
}

void CondensingTX::calculatePlusAndMinusOneTransfer(const TransferInfo& ti)
{
    if (!plusMinusInfo.count(ti.from)) {
        plusMinusInfo[ti.from] = std::make_pair(0, ti.value);
    } else {
        plusMinusInfo[ti.from] = std::make_pair(plusMinusInfo[ti.from].first, plusMinusInfo[ti.from].second + ti.value);
    }
    if (!plusMinusInfo.count(ti.to)) {
        plusMinusInfo[ti.to] = std::make_pair(ti.value, 0);
    } else {
        plusMinusInfo[ti.to] = std::make_pair(plusMinusInfo[ti.to].first + ti.value, plusMinusInfo[ti.to].second);
    }
}

void CondensingTX::calculatePlusAndMinus(const dev::u256& feesPromisedByContract, const Address &contractAddress)
{
    for (const TransferInfo& ti : transfers) {
        this->calculatePlusAndMinusOneTransfer(ti);
    }
    if (feesPromisedByContract == 0) {
        return;
    }
    if (!this->plusMinusInfo.count(contractAddress)) {
        plusMinusInfo[contractAddress] = std::make_pair(0, 0);
    }
    const auto& contractChange = this->plusMinusInfo[contractAddress];
    this->plusMinusInfo[contractAddress] = std::make_pair(contractChange.first, contractChange.second + feesPromisedByContract);
}

bool CondensingTX::createNewBalances(std::stringstream* commentsOnFailure) {
    for (auto& p : plusMinusInfo) {
        dev::u256 balance = 0;
        dev::u256 startingValue = 0;
        if ((vins.count(p.first) && vins[p.first].alive) || (!vins[p.first].alive && !checkDeleteAddress(p.first))) {
            startingValue = vins[p.first].value;
            balance = startingValue;
        }
        balance += p.second.first;
        if (balance < p.second.second) {
            if (commentsOnFailure != nullptr) {
                *commentsOnFailure << "Failed to create balance. Starting value: " << startingValue
                                   << ", credits: " << p.second.first << ", debits: " << p.second.second << ".\n";
            }
            return false;
        }
        balance -= p.second.second;
        balances[p.first] = balance;
    }
    return true;
}

std::vector<CTxIn> CondensingTX::createVins(stringstream* comments)
{
    (void) comments;
    std::vector<CTxIn> ins;
    for(auto& v : vins) {
        if ((v.second.value > 0 && v.second.alive) || (v.second.value > 0 && !vins[v.first].alive && !checkDeleteAddress(v.first))) {
            CTxIn theTX(h256Touint(v.second.hash), v.second.nVout, CScript() << OP_SPEND);
            ins.push_back(theTX);
        }
    }
    return ins;
}

std::vector<CTxOut> CondensingTX::createVout(stringstream* comments)
{
    size_t count = 0;
    std::vector<CTxOut> outs;
    for (auto& b : balances) {
        if (b.second > 0) {
            CScript script;
            auto* a = state->account(b.first);
            if(a && a->isAlive()) {
                //create a no-exec contract output
                script = CScript() << valtype{0} << valtype{0} << valtype{0} << valtype{0} << b.first.asBytes() << OP_CALL;
            } else {
                script = CScript() << OP_DUP << OP_HASH160 << b.first.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG;
            }
            outs.push_back(CTxOut(CAmount(b.second), script));
            nVouts[b.first] = count;
            count ++;
        }
        if (count > MAX_CONTRACT_VOUTS) {
            this->voutOverflow = true;
            return outs;
        }
    }
    return outs;
}

bool CondensingTX::checkDeleteAddress(dev::Address addr) {
    return deleteAddresses.count(addr) != 0;
}
///////////////////////////////////////////////////////////////////////////////////////////
