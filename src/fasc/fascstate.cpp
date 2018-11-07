#include <sstream>
#include <util.h>
#include <validation.h>
#include <chainparams.h>
#include <fasc/fascstate.h>

using namespace std;
using namespace dev;
using namespace dev::eth;

extern std::fstream& myDebugLogFile();

FascState::FascState(u256 const& _accountStartNonce, OverlayDB const& _db, const string& _path, BaseState _bs) :
    State(_accountStartNonce, _db, _bs) {
    dbUTXO = FascState::openDB(_path + "/fascDB", sha3(rlp("")), WithExisting::Trust);
    stateUTXO = SecureTrieDB<Address, OverlayDB>(&dbUTXO);
}

FascState::FascState() : dev::eth::State(dev::Invalid256, dev::OverlayDB(), dev::eth::BaseState::PreExisting) {
    dbUTXO = OverlayDB();
    stateUTXO = SecureTrieDB<Address, OverlayDB>(&dbUTXO);
}

ResultExecute FascState::execute(EnvInfo const& _envInfo, SealEngineFace const& _sealEngine, FascTransaction const& _t, Permanence _p, OnOpFunc const& _onOp) {

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
        if (!e.execute()) {
            e.go(onOp);
        } else {

            e.revert();
            throw Exception();
        }
        e.finalize();
        //using bytes = std::vector<byte>;
        //using bytesRef = vector_ref<byte>;
        //using bytesConstRef = vector_ref<byte const>;
        //

        bool flagAggregationFound = false;
        std::vector<std::vector<uint8_t> > aggregationData;
        std::string kanbanShardCreationString = "KanbanAggregateSignatureUnlock";
        dev::h160 contractAddress;
        //std::stringstream debugOut2;
        //debugOut2 << "DEBUG: author address: ";
        //debugOut2 << std::hex << _envInfo.author().asBytes() << "\n";
        //LogPrintStr(debugOut2.str());
        std::vector<uint8_t> kanbanShardCreationToken;
        for (int unsigned i = 0; i < kanbanShardCreationString.size(); i ++) {
            kanbanShardCreationToken.push_back(kanbanShardCreationString[i]);
        }
        const std::vector<dev::eth::LogEntry>& theLogEntries = e.logs();
        myDebugLogFile() << "about to process smart contract logs" << theLogEntries.size() << "\n";
        for (unsigned i = 0; i < theLogEntries.size(); i ++) {
            //std::stringstream debugOut;
            const dev::eth::LogEntry& current = theLogEntries[i];
            const std::vector<uint8_t>& data = current.data;
            if (flagAggregationFound) {
                aggregationData.push_back(data);
            }
            if (data == kanbanShardCreationToken) {
                //LogPrintStr("DEBUG: Found kanban shard creation token!!!!\n");
                flagAggregationFound = true;
                contractAddress = current.address;
            }
            std::stringstream out;
            out << std::hex << data << "\n";
            myDebugLogFile() << out.str();
            //debugOut << "DEBUG: log entry: " << std::hex << data << "\n";
            //LogPrintStr(debugOut.str());
        }

        //std::stringstream debugOut;
        //debugOut << "DEBUG: number of transfers this time around: " << this->transfers.size() << "\n";
        //LogPrintStr(debugOut.str());
        //for (const TransferInfo& currentTransfer : this->transfers) {
        //    std::stringstream out;
        //    out << "DEBUG: current transfer: from: " << currentTransfer.from << ", to: "
        //        << currentTransfer.to << ", value: " << currentTransfer.value << "\n";
        //    LogPrintStr(out.str());
        //}


        if (_p == Permanence::Reverted) {
            m_cache.clear();
            cacheUTXO.clear();
        } else {
            deleteAccounts(_sealEngine.deleteAddresses);
            if(res.excepted == TransactionException::None) {
                CondensingTX ctx(this, transfers, _t, _sealEngine.deleteAddresses);

                tx = MakeTransactionRef(ctx.createCondensingTX(aggregationData, contractAddress));
                if(ctx.reachedVoutLimit()) {
                    voutLimit = true;
                    e.revert();
                    throw Exception();
                }
                std::unordered_map<dev::Address, Vin> vins = ctx.createVin(*tx);
                updateUTXO(vins);
            } else {
                printfErrorLog(res.excepted);
            }

            fasc::commit(cacheUTXO, stateUTXO, m_cache);
            cacheUTXO.clear();
            bool removeEmptyAccounts = _envInfo.number() >= _sealEngine.chainParams().u256Param("EIP158ForkBlock");
            commit(removeEmptyAccounts ? State::CommitBehaviour::RemoveEmptyAccounts : State::CommitBehaviour::KeepEmptyAccounts);
        }
    }
    catch(Exception const& _e) {

        printfErrorLog(dev::eth::toTransactionException(_e));
        res.excepted = dev::eth::toTransactionException(_e);
        res.gasUsed = _t.gas();
            const Consensus::Params& consensusParams = Params().GetConsensus();
            if(chainActive.Height() < consensusParams.nFixUTXOCacheHFHeight  && _p != Permanence::Reverted){
                deleteAccounts(_sealEngine.deleteAddresses);
                commit(CommitBehaviour::RemoveEmptyAccounts);
            } else {
                m_cache.clear();
                cacheUTXO.clear();
            }
    }

    if(!_t.isCreation())
        res.newAddress = _t.receiveAddress();
    newAddress = dev::Address();
    transfers.clear();
    if(voutLimit) {
        //use old and empty states to create virtual Out Of Gas exception
        LogEntries logs;
        u256 gas = _t.gas();
        ExecutionResult ex;
        ex.gasRefunded=0;
        ex.gasUsed=gas;
        ex.excepted=TransactionException();
        //create a refund tx to send back any coins that were suppose to be sent to the contract
        CMutableTransaction refund;
        if(_t.value() > 0) {
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

// void FascState::commit(CommitBehaviour _commitBehaviour)
// {
//     if (_commitBehaviour == CommitBehaviour::RemoveEmptyAccounts)
//         removeEmptyAccounts();

//     fasc::commit(cacheUTXO, stateUTXO, m_cache);
//     cacheUTXO.clear();

//     m_touched += dev::eth::commit(m_cache, m_state);
//     m_changeLog.clear();
//     m_cache.clear();
//     m_unchangedCacheEntries.clear();
// }

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
    }
    else
    {
        if(!addressInUse(newAddress) && newAddress != dev::Address()) {
            const_cast<dev::Address&>(_id) = newAddress;
            newAddress = dev::Address();
        }
        createAccount(_id, {requireAccountStartNonce(), _amount});
    }

    if (_amount)
        m_changeLog.emplace_back(dev::eth::detail::Change::Balance, _id, _amount);
}

dev::Address FascState::createFascAddress(dev::h256 hashTx, uint32_t voutNumber) {
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

void FascState::printfErrorLog(const dev::eth::TransactionException er) {
    std::stringstream ss;
    ss << er;
    clog(ExecutiveWarnChannel) << "VM exception:" << ss.str();
}

///////////////////////////////////////////////////////////////////////////////////////////
CTransaction CondensingTX::createCondensingTX(const std::vector<std::vector<uint8_t> >& aggregationData, const dev::h160& contractAddress) {
    for (const TransferInfo& ti : transfers) {
        std::stringstream debugOut;
        debugOut << "DEBUG: transfer from: " << ti.from << ", to: " << ti.to << ", value: " << ti.value << "\n";
        LogPrintStr(debugOut.str());
    }
    if (aggregationData.size() > 0) {
        std::stringstream debugOut;
        debugOut << "DEBUG: got non-trivial aggregation data! \n";
        LogPrintStr(debugOut.str());
    }
    selectionVin();
    if (this->vins.size() == 0 && aggregationData.size() > 0) {
        std::stringstream debugOut;
        debugOut << "DEBUG: I'm in the vin case. \n";
        LogPrintStr(debugOut.str());
        this->vins[contractAddress] = Vin{transaction.getHashWith(), transaction.getNVout(), 0, 1};
    }

    calculatePlusAndMinus();
    if(!createNewBalances())
        return CTransaction();
    CMutableTransaction tx;
    LogPrintStr("DEBUG: about to create vins.\n");
    for (auto& theVIN : this->vins) {
        std::stringstream debugOut;
        debugOut << "vin_" << theVIN.first << ": hash: " << theVIN.second.hash.asBytesReversed() << ", value: " << theVIN.second.value << "\n";
        LogPrintStr(debugOut.str());
    }
    tx.vin = createVins();
    Vin* proposedTX = this->state->vin(contractAddress);
    std::stringstream debugOut1;
    debugOut1 << "DEBUG: state vin:  this->state->vin(ti.from) ptr: " << proposedTX;
    if (proposedTX != 0){
         debugOut1 << ", hash: " << proposedTX->hash;
    }
    debugOut1 << "\n";
    LogPrintStr(debugOut1.str());

    std::stringstream debugOut2;
    debugOut2 << "DEBUG: transaction gethashwith: " << this->transaction.getHashWith() << "\n ";
    debugOut2 << "DEBUG: transaction nvout: " << this->transaction.getNVout() << "\n ";
    debugOut2 << "DEBUG: about to create vouts.\n";
    LogPrintStr(debugOut2.str());
    tx.vout = createVout();

    if (aggregationData.size() > 0) {
        CScript extraScript = CScript();
        for (unsigned counterData = 0; counterData < aggregationData.size(); counterData ++) {
            extraScript << aggregationData[counterData];
        }
        extraScript << OP_AGGREGATEVERIFY;
        tx.vout.push_back(CTxOut(0, extraScript));
        if (tx.vin.size() == 0) {

            CTxIn theTX(h256Touint(this->transaction.getHashWith()), this->transaction.getNVout(), CScript() << OP_SPEND);
            std::stringstream debugOut;
            debugOut << "I'm adding the transaction: " << theTX.ToString() << "\n";
            tx.vin.push_back(theTX);
            debugOut << "DEBUG: problem: tx.vin.size() is 0. vins size: " << this->vins.size() << "\n";
            LogPrintStr(debugOut.str());
        }
    }
    std::stringstream debugOut3;
    debugOut3 << "DEBUG: result has: " << tx.vout.size() << " vouts and " << tx.vin.size() << " vins. \n**********\n*********\n";
    for (unsigned i = 0; i < tx.vout.size(); i ++) {
        debugOut3 << "DEBUG: tx.vout[" << i << "]: " << tx.vout[i].ToString() << "\n";
    }
    for (unsigned i = 0; i < tx.vin.size(); i ++) {
        debugOut3 << "DEBUG: tx.vin[" << i << "]: " << tx.vin[i].ToString() << "\n";
    }
    LogPrintStr(debugOut3.str());
    return !tx.vin.size() || !tx.vout.size() ? CTransaction() : CTransaction(tx);
}

std::unordered_map<dev::Address, Vin> CondensingTX::createVin(const CTransaction& tx) {
    std::unordered_map<dev::Address, Vin> vins;
    for(auto& b : balances) {
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

void CondensingTX::selectionVin() {
    int i  = 0;
    for(const TransferInfo& ti : transfers) {
        i++;
        if(!vins.count(ti.from)) {
            if(auto a = state->vin(ti.from)) {
                this->vins[ti.from] = *a;
                std::stringstream debugStream;
                debugStream << "DEBUG: auto a case: vin address: " << ti.from << ", vin hash: " << this->vins[ti.from].hash.asBytesReversed() << ", vinvout: "
                            << this->vins[ti.from].nVout << "\n";
                LogPrintStr(debugStream.str());
            }
            if(ti.from == transaction.sender() && transaction.value() > 0) {
                vins[ti.from] = Vin{transaction.getHashWith(), transaction.getNVout(), transaction.value(), 1};
                std::stringstream debugStream;
                debugStream << "DEBUG: positive tr. value: vin address: " << ti.from << ", vin hash: " << vins[ti.from].hash.asBytesReversed() << ", vinvout: "
                            << this->vins[ti.from].nVout << "\n";
                LogPrintStr(debugStream.str());
            }
        }

        if(!vins.count(ti.to)) {
            if(auto a = state->vin(ti.to)) {
                vins[ti.to] = *a;
                std::stringstream debugStream;
                debugStream << "DEBUG: case no tito: vin address: " << ti.to << ", vin hash: " << this->vins[ti.to].hash.asBytesReversed() << ", vinvout: "
                            << this->vins[ti.to].nVout << "\n";
                LogPrintStr(debugStream.str());
            }
        }
    }
    std::stringstream debugStream2;
    debugStream2 << "DEBUG: Selection vin ran " << i  << " times. \n";
    LogPrintStr(debugStream2.str());
}

void CondensingTX::calculatePlusAndMinus() {
    for(const TransferInfo& ti : transfers) {
        if(!plusMinusInfo.count(ti.from)) {
            plusMinusInfo[ti.from] = std::make_pair(0, ti.value);
        } else {
            plusMinusInfo[ti.from] = std::make_pair(plusMinusInfo[ti.from].first, plusMinusInfo[ti.from].second + ti.value);
        }

        if(!plusMinusInfo.count(ti.to)) {
            plusMinusInfo[ti.to] = std::make_pair(ti.value, 0);
        } else {
            plusMinusInfo[ti.to] = std::make_pair(plusMinusInfo[ti.to].first + ti.value, plusMinusInfo[ti.to].second);
        }
    }
}

bool CondensingTX::createNewBalances() {
    for(auto& p : plusMinusInfo) {
        dev::u256 balance = 0;
        if((vins.count(p.first) && vins[p.first].alive) || (!vins[p.first].alive && !checkDeleteAddress(p.first))) {
            balance = vins[p.first].value;
        }
        balance += p.second.first;
        if(balance < p.second.second)
            return false;
        balance -= p.second.second;
        balances[p.first] = balance;
    }
    return true;
}

std::vector<CTxIn> CondensingTX::createVins() {
    std::vector<CTxIn> ins;
    for(auto& v : vins) {
        std::stringstream debugOut;
        Vin& vinRefDebug = v.second;
        const dev::Address& theAddress = v.first;
        debugOut << "DEBUG: considering: vin address: " << theAddress.asBytes() << " with vin hash: " << vinRefDebug.hash.asBytesReversed()
                 << ", vin nvout: " << vinRefDebug.nVout << "\n";
        if((v.second.value > 0 && v.second.alive) || (v.second.value > 0 && !vins[v.first].alive && !checkDeleteAddress(v.first))) {
            CTxIn theTX(h256Touint(v.second.hash), v.second.nVout, CScript() << OP_SPEND);
            debugOut << "DEBUG: the considered vin results in the transaction: " << theTX.ToString() << "\n";
            ins.push_back(theTX);
        }
        LogPrintStr(debugOut.str());
    }
    return ins;
}

std::vector<CTxOut> CondensingTX::createVout() {
    size_t count = 0;
    std::vector<CTxOut> outs;
    for(auto& b : balances) {
        std::stringstream debugStream;
        debugStream << "DEBUG: creavevout: address: " << b.first << ", value: " << b.second << "\n";
        LogPrintStr(debugStream.str());
        if(b.second > 0) {
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
            count++;
        }
        if(count > MAX_CONTRACT_VOUTS) {
            voutOverflow=true;
            return outs;
        }
    }
    return outs;
}

bool CondensingTX::checkDeleteAddress(dev::Address addr) {
    return deleteAddresses.count(addr) != 0;
}
///////////////////////////////////////////////////////////////////////////////////////////
