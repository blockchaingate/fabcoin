#ifndef FASCTRANSACTION_H
#define FASCTRANSACTION_H

#include <libethcore/TransactionBase.h>

struct VersionVM {
    //this should be portable, see https://stackoverflow.com/questions/31726191/is-there-a-portable-alternative-to-c-bitfields
# if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t format : 2;
    uint8_t rootVM : 6;
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint8_t rootVM : 6;
    uint8_t format : 2;
#endif
    uint8_t vmVersion;
    uint16_t flagOptions;
    // CONSENSUS CRITICAL!
    // Do not add any other fields to this struct

    uint32_t toRaw() const {
        return *(uint32_t*)this;
    }
    static VersionVM fromRaw(uint32_t val) {
        //Please note: shorter cast sequences may raise compiler warnings
        //in gcc. Please avoid such warnings.
        uint32_t* valPointer = &val;
        void* valVoid = (void*) valPointer;
        VersionVM* versionPointer = (VersionVM*) valVoid;
        VersionVM x = *versionPointer;
        return x;
    }
    static VersionVM GetNoExec() {
        VersionVM x;
        x.flagOptions=0;
        x.rootVM=0;
        x.format=0;
        x.vmVersion=0;
        return x;
    }
    static VersionVM GetEVMDefault() {
        VersionVM x;
        x.flagOptions=0;
        x.rootVM=1;
        x.format=0;
        x.vmVersion=0;
        return x;
    }
    static VersionVM fromVector(const std::vector<unsigned char>& input) {
        unsigned numEntries = input.size();
        uint32_t versionUint32 = 0;
        if (numEntries > 4) {
            numEntries = 4;
        }
        for (unsigned i = 0; i < numEntries; i ++) {
            versionUint32 *= 256;
            versionUint32 += (unsigned) input[i];
        }
        return fromRaw(versionUint32);
    }
} __attribute__((__packed__));

class FascTransaction : public dev::eth::Transaction {

public:

    FascTransaction() : nVout(0) {}

    FascTransaction(dev::u256 const& _value, dev::u256 const& _gasPrice, dev::u256 const& _gas, dev::bytes const& _data, dev::u256 const& _nonce = dev::Invalid256):
        dev::eth::Transaction(_value, _gasPrice, _gas, _data, _nonce) {}

    FascTransaction(dev::u256 const& _value,
                    dev::u256 const& _gasPrice,
                    dev::u256 const& _gas,
                    dev::Address const& _dest, //<- Also called: contract address, to address.
                    dev::bytes const& _data,
                    dev::u256 const& _nonce = dev::Invalid256):
        dev::eth::Transaction(_value, _gasPrice, _gas, _dest, _data, _nonce) {
    }

    void setHashWith(const dev::h256 hash) {
        m_hashWith = hash;
    }

    dev::h256 getHashWith() const {
        return m_hashWith;
    }

    void setNVout(uint32_t vout) {
        nVout = vout;
    }

    uint32_t getNVout() const {
        return nVout;
    }

    void setVersion(VersionVM v) {
        version=v;
    }
    VersionVM getVersion() const {
        return version;
    }
private:

    uint32_t nVout;
    VersionVM version;

};
#endif
