#ifndef FASCDGP_H
#define FASCDGP_H

#include <fasc/fascstate.h>
#include <primitives/block.h>
#include <validation.h>
#include <utilstrencodings.h>

static const dev::Address GasScheduleDGP = dev::Address("0000000000000000000000000000000000000080");
static const dev::Address BlockSizeDGP = dev::Address("0000000000000000000000000000000000000081");
static const dev::Address GasPriceDGP = dev::Address("0000000000000000000000000000000000000082");
static const dev::Address DGPCONTRACT4 = dev::Address("0000000000000000000000000000000000000083");
static const dev::Address BlockGasLimitDGP = dev::Address("0000000000000000000000000000000000000084");

static const uint32_t MIN_BLOCK_SIZE_DGP = 500000;
static const uint32_t MAX_BLOCK_SIZE_DGP = 32000000;
static const uint32_t DEFAULT_BLOCK_SIZE_DGP = 2000000;

static const uint64_t MIN_MIN_GAS_PRICE_DGP = 1;
static const uint64_t MAX_MIN_GAS_PRICE_DGP = 10000;
static const uint64_t DEFAULT_MIN_GAS_PRICE_DGP = 40;

static const uint64_t MIN_BLOCK_GAS_LIMIT_DGP_v1 = 10000000; // 10 million
static const uint64_t MAX_BLOCK_GAS_LIMIT_DGP_v1 = 4000000000; // 4 billion
static const uint64_t DEFAULT_BLOCK_GAS_LIMIT_DGP_v1 = 400000000; // 400 million

class FascDGP {

public:

    FascDGP(FascState* _state, bool _dgpevm = true) : dgpevm(_dgpevm), state(_state) {
        initDataSchedule();
    }

    dev::eth::EVMSchedule getGasSchedule(unsigned int blockHeight);

    uint32_t getBlockSize(unsigned int blockHeight);

    uint64_t getMinGasPrice(unsigned int blockHeight);

    uint64_t getBlockGasLimit(unsigned int blockHeight);

private:

    bool initStorages(const dev::Address& addr, unsigned int blockHeight, std::vector<unsigned char> data = std::vector<unsigned char>());

    void initStorageDGP(const dev::Address& addr);

    void initStorageTemplate(const dev::Address& addr);

    void initDataTemplate(const dev::Address& addr, std::vector<unsigned char>& data);

    void initDataSchedule();

    bool checkLimitSchedule(const std::vector<uint32_t>& defaultData, const std::vector<uint32_t>& checkData, int blockHeight);

    void createParamsInstance();

    dev::Address getAddressForBlock(unsigned int blockHeight);

    uint64_t getUint64FromDGP(unsigned int blockHeight, const dev::Address& contract, std::vector<unsigned char> data);

    void parseStorageScheduleContract(std::vector<uint32_t>& uint32Values);

    void parseDataScheduleContract(std::vector<uint32_t>& uint32Values);

    void parseStorageOneUint64(uint64_t& blockSize);

    void parseDataOneUint64(uint64_t& value);

    dev::eth::EVMSchedule createEVMSchedule(const dev::eth::EVMSchedule &_schedule, int blockHeight);

    void clear();



    bool dgpevm;

    const FascState* state;

    dev::Address templateContract;

    std::map<dev::h256, std::pair<dev::u256, dev::u256> > storageDGP;

    std::map<dev::h256, std::pair<dev::u256, dev::u256> > storageTemplate;

    std::vector<unsigned char> dataTemplate;

    std::vector<std::pair<unsigned int, dev::Address> > paramsInstance;

    std::vector<uint32_t> dataSchedule;

};
#endif
