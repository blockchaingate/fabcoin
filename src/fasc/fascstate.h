#pragma once

#include <libethereum/State.h>
#include <libevm/ExtVMFace.h>
#include <crypto/sha256.h>
#include <crypto/ripemd160.h>
#include <uint256.h>
#include <primitives/transaction.h>
#include <fasc/fasctransaction.h>

#include <libethereum/Executive.h>
#include <libethcore/SealEngine.h>

using OnOpFunc = std::function<void(uint64_t, uint64_t, dev::eth::Instruction, dev::bigint, dev::bigint,
                                    dev::bigint, dev::eth::VMFace const*, dev::eth::ExtVMFace const*)>;
using plusAndMinus = std::pair<dev::u256, dev::u256>;
using valtype = std::vector<unsigned char>;

struct TransferInfo {
    dev::Address from;
    dev::Address to;
    dev::u256 value;
};

struct Vin {
    dev::h256 hash;
    uint32_t nVout;
    dev::u256 value;
    uint8_t alive;
};

class FascTransactionReceipt: public dev::eth::TransactionReceipt {
public:
    FascTransactionReceipt(dev::h256 const& state_root, dev::h256 const& utxo_root, dev::u256 const& gas_used, dev::eth::LogEntries const& log) : dev::eth::TransactionReceipt(state_root, gas_used, log), m_utxoRoot(utxo_root) {}

    dev::h256 const& utxoRoot() const {
        return m_utxoRoot;
    }
private:
    dev::h256 m_utxoRoot;
};

struct ResultExecute{
    dev::eth::ExecutionResult execRes;
    FascTransactionReceipt txRec;
    CTransaction tx;
};

namespace fasc {
template <class DB>
dev::AddressHash commit(std::unordered_map<dev::Address, Vin> const& _cache, dev::eth::SecureTrieDB<dev::Address, DB>& _state, std::unordered_map<dev::Address, dev::eth::Account> const& _cacheAcc)
{
    dev::AddressHash ret;
    for (auto const& i: _cache) {
        if(i.second.alive == 0) {
            _state.remove(i.first);
        } else {
            dev::RLPStream s(4);
            s << i.second.hash << i.second.nVout << i.second.value << i.second.alive;
            _state.insert(i.first, &s.out());
        }
        ret.insert(i.first);
    }
    return ret;
}
}

class CondensingTX;

class FascState : public dev::eth::State {

public:

    FascState();

    FascState(dev::u256 const& _accountStartNonce, dev::OverlayDB const& _db, const std::string& _path, dev::eth::BaseState _bs = dev::eth::BaseState::PreExisting);

    ResultExecute execute(dev::eth::EnvInfo const& _envInfo, dev::eth::SealEngineFace const& _sealEngine,
                          FascTransaction const& _t, dev::eth::Permanence _p = dev::eth::Permanence::Committed,
                          dev::eth::OnOpFunc const& _onOp = OnOpFunc(), std::stringstream *commentsNullForNone = nullptr);

    static dev::u256 GetFeesPromisedByLogs(const std::vector<dev::eth::LogEntry>& logs);
    void setRootUTXO(dev::h256 const& _r) {
        cacheUTXO.clear();
        stateUTXO.setRoot(_r);
    }

    void setCacheUTXO(dev::Address const& address, Vin const& vin) {
        cacheUTXO.insert(std::make_pair(address, vin));
    }

    dev::h256 rootHashUTXO() const {
        return stateUTXO.root();
    }
    bool accountFeesCoveredByContract(
            const dev::Address& contractAddress,
            const dev::u256& feesCovered,
            std::stringstream* commentsOnFailure
    );

    std::unordered_map<dev::Address, Vin> vins() const; // temp

    dev::OverlayDB const& dbUtxo() const {
        return dbUTXO;
    }

    dev::OverlayDB& dbUtxo() {
        return dbUTXO;
    }

    virtual ~FascState() {}

    friend CondensingTX;

private:

    void transferBalance(dev::Address const& _from, dev::Address const& _to, dev::u256 const& _value);

    Vin const* vin(dev::Address const& _a) const;

    Vin* vin(dev::Address const& _addr);

    // void commit(CommitBehaviour _commitBehaviour);

    void kill(dev::Address _addr);

    void addBalance(dev::Address const& _id, dev::u256 const& _amount);

    dev::Address createFascAddress(dev::h256 hashTx, uint32_t voutNumber);

    void deleteAccounts(std::set<dev::Address>& addrs);

    void updateUTXO(const std::unordered_map<dev::Address, Vin>& vins);

    void printfErrorLog(const dev::eth::TransactionException er, const std::string &errorMessage = "");

    dev::Address newAddress;

    std::vector<TransferInfo> transfers;

    dev::OverlayDB dbUTXO;

    dev::eth::SecureTrieDB<dev::Address, dev::OverlayDB> stateUTXO;

    std::unordered_map<dev::Address, Vin> cacheUTXO;
};


struct TemporaryState {
    std::unique_ptr<FascState>& globalStateRef;
    dev::h256 oldHashStateRoot;
    dev::h256 oldHashUTXORoot;

    TemporaryState(std::unique_ptr<FascState>& _globalStateRef) :
        globalStateRef(_globalStateRef),
        oldHashStateRoot(globalStateRef->rootHash()),
        oldHashUTXORoot(globalStateRef->rootHashUTXO()) {}

    void SetRoot(dev::h256 newHashStateRoot, dev::h256 newHashUTXORoot)
    {
        globalStateRef->setRoot(newHashStateRoot);
        globalStateRef->setRootUTXO(newHashUTXORoot);
    }

    ~TemporaryState() {
        globalStateRef->setRoot(oldHashStateRoot);
        globalStateRef->setRootUTXO(oldHashUTXORoot);
    }
    TemporaryState() = delete;
    TemporaryState(const TemporaryState&) = delete;
    TemporaryState& operator=(const TemporaryState&) = delete;
    TemporaryState(TemporaryState&&) = delete;
    TemporaryState& operator=(TemporaryState&&) = delete;
};


///////////////////////////////////////////////////////////////////////////////////////////
class CondensingTX {

public:

    CondensingTX(FascState* _state, const std::vector<TransferInfo>& _transfers, const FascTransaction& _transaction, std::set<dev::Address> _deleteAddresses = std::set<dev::Address>()) : transfers(_transfers), deleteAddresses(_deleteAddresses), transaction(_transaction), state(_state) {}

    CTransaction createCondensingTX(const dev::u256& feesPromisedByContract,
        //const std::vector<std::vector<uint8_t> >* aggregationData,
        const dev::h160& contractAddress,
        std::stringstream* commentsNullForNone);

    std::unordered_map<dev::Address, Vin> createVin(const CTransaction& tx);

    bool reachedVoutLimit() {
        return voutOverflow;
    }

private:

    void selectionVin(const dev::u256 &feesPromisedByContract, const dev::h160 &contractAddress, std::stringstream* comments);

    void calculatePlusAndMinus(const dev::u256 &feesPromisedByContract, const dev::Address& contractAddress);
    void calculatePlusAndMinusOneTransfer(const TransferInfo& ti);

    bool createNewBalances(std::stringstream *commentsOnFailure);

    std::vector<CTxIn> createVins(std::stringstream* comments);

    std::vector<CTxOut> createVout(std::stringstream* comments);
    void createAggregationVouts(const std::vector<std::vector<uint8_t> >& aggregationData,
                                CMutableTransaction& outputTransaction,
                                std::stringstream* comments);

    bool checkDeleteAddress(dev::Address addr);

    std::map<dev::Address, plusAndMinus> plusMinusInfo;

    std::map<dev::Address, dev::u256> balances;

    std::map<dev::Address, uint32_t> nVouts;

    std::map<dev::Address, Vin> vins;

    const std::vector<TransferInfo>& transfers;

    //We don't need the ordered nature of "set" here, but unordered_set's theoretical worst complexity is O(n), whereas set is O(log n)
    //So, making this unordered_set could be an attack vector
    const std::set<dev::Address> deleteAddresses;

    const FascTransaction& transaction;

    FascState* state;

    bool voutOverflow = false;

};
///////////////////////////////////////////////////////////////////////////////////////////
