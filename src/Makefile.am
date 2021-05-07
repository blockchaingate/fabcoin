# Copyright (c) 2013-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS)
AM_CXXFLAGS = $(HARDENED_CXXFLAGS) $(ERROR_CXXFLAGS) -DETH_FATDB=1
AM_CPPFLAGS = $(HARDENED_CPPFLAGS) -DFASC_BUILD
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

FABCOIN_CONFIG_INCLUDES=-I$(builddir)/config
FABCOIN_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS) -I$(srcdir)/cpp-ethereum-fab/utils

FABCOIN_INCLUDES += -I$(srcdir)/secp256k1/include
FABCOIN_INCLUDES += $(UNIVALUE_CFLAGS)

FABCOIN_INCLUDES += -I$(srcdir)/cpp-ethereum-fab
FABCOIN_INCLUDES += -I$(srcdir)/cpp-ethereum-fab/evmc/include
FABCOIN_INCLUDES += -I$(srcdir)/cpp-ethereum-fab/ethash/include
LIBFABCOIN_SERVER=libfabcoin_server.a
LIBFABCOIN_COMMON=libfabcoin_common.a
LIBFABCOIN_CONSENSUS=libfabcoin_consensus.a
LIBFABCOIN_CLI=libfabcoin_cli.a
LIBFABCOIN_UTIL=libfabcoin_util.a
LIBFABCOIN_CRYPTO=crypto/libfabcoin_crypto.a
LIBFABCOINQT=qt/libfabcoinqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la
LIBCRYPTOPP=cryptopp/libcryptopp.a

if ENABLE_ZMQ
LIBFABCOIN_ZMQ=libfabcoin_zmq.a
endif
if BUILD_FABCOIN_LIBS
LIBFABCOINCONSENSUS=libfabcoinconsensus.la
endif
if ENABLE_WALLET
LIBFABCOIN_WALLET=libfabcoin_wallet.a
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBFABCOIN_CRYPTO) \
  $(LIBFABCOIN_UTIL) \
  $(LIBFABCOIN_COMMON) \
  $(LIBFABCOIN_CONSENSUS) \
  $(LIBFABCOIN_SERVER) \
  $(LIBFABCOIN_CLI) \
  $(LIBFABCOIN_WALLET) \
  $(LIBFABCOIN_ZMQ)

lib_LTLIBRARIES = $(LIBFABCOINCONSENSUS)

bin_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_FABCOIND
  bin_PROGRAMS += fabcoind
endif

if BUILD_FABCOIN_UTILS
  bin_PROGRAMS += fabcoin-cli fabcoin-tx
endif

.PHONY: FORCE check-symbols check-security
# fabcoin core #
FABCOIN_CORE_H = \
  addrdb.h \
  addrman.h \
  base58.h \
  bloom.h \
  blockencodings.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coins.h \
  compat.h \
  compat/byteswap.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  consensus/tx_verify.h \
  core_io.h \
  core_memusage.h \
  cuckoocache.h \
  fs.h \
  httprpc.h \
  httpserver.h \
  indirectmap.h \
  init.h \
  key.h \
  keystore.h \
  dbwrapper.h \
  limitedmap.h \
  memusage.h \
  merkleblock.h \
  miner.h \
  net.h \
  net_processing.h \
  netaddress.h \
  netbase.h \
  netmessagemaker.h \
  noui.h \
  policy/feerate.h \
  policy/fees.h \
  policy/policy.h \
  policy/rbf.h \
  pow.h \
  protocol.h \
  random.h \
  reverse_iterator.h \
  reverselock.h \
  rpc/blockchain.h \
  rpc/client.h \
  rpc/mining.h \
  rpc/protocol.h \
  rpc/server.h \
  rpc/register.h \
  rpc/test.h \
  encodings_crypto.h \
  aggregate_schnorr_signature.h \
  log_session.h \
  crypto/secp256k1_all_in_one.h \
  scheduler.h \
  script/sigcache.h \
  script/sign.h \
  script/standard.h \
  script/ismine.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  threadsafety.h \
  threadinterrupt.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  undo.h \
  util.h \
  log_session.h \
  utilmoneystr.h \
  utiltime.h \
  validation.h \
  validationinterface.h \
  versionbits.h \
  wallet/coincontrol.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/feebumper.h \
  wallet/rpcwallet.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  warnings.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqconfig.h\
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  fasc/fascstate.h \
  fasc/fasctransaction.h \
  fasc/fascDGP.h \
  fasc/storageresults.h


if ENABLE_GPU
LIBGPUSOLVER_H = \
  libgpusolver/libgpusolver.h \
  libgpusolver/gpuconfig.h \
  libgpusolver/kernels/silentarmy.h \
  libgpusolver/libclwrapper.h \
  libgpusolver/cl.hpp \
  libgpusolver/blake.h \
  libgpusolver/param.h
endif
  
obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libfabcoin_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between fabcoind and fabcoin-qt
libfabcoin_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(FABCOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS) @CUDA_INCLUDES@
libfabcoin_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfabcoin_server_a_SOURCES = \
  addrdb.cpp \
  addrman.cpp \
  bloom.cpp \
  blockencodings.cpp \
  chain.cpp \
  checkpoints.cpp \
  consensus/tx_verify.cpp \
  httprpc.cpp \
  httpserver.cpp \
  init.cpp \
  dbwrapper.cpp \
  merkleblock.cpp \
  miner.cpp \
  net.cpp \
  net_processing.cpp \
  noui.cpp \
  policy/fees.cpp \
  policy/policy.cpp \
  policy/rbf.cpp \
  pow.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  rpc/aggregate_signature_test.cpp \
  rpc/log_files.cpp \
  script/sigcache.cpp \
  script/ismine.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  ui_interface.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  fasc/fascstate.cpp \
  fasc/fasctransaction.cpp \
  fasc/fascDGP.cpp \
  consensus/consensus.cpp \
  fasc/storageresults.cpp \
  $(FABCOIN_CORE_H) 

if ENABLE_GPU
libgpusolver_SOURCES = libgpusolver/libgpusolver.cpp libgpusolver/libclwrapper.cpp libgpusolver/blake.cpp $(LIBGPUSOLVER_H) 
libfabcoin_server_a_SOURCES += $(libgpusolver_SOURCES)
endif

if USE_CUDA
libcuda_SOURCES = cuda/cuda_equi.cu cuda/blake2b.cu cuda/blake2/blake2bx.cpp cuda/equi_miner.cu
libfabcoin_server_a_SOURCES += $(libcuda_SOURCES)
endif

if ENABLE_ZMQ
libfabcoin_zmq_a_CPPFLAGS = $(FABCOIN_INCLUDES) $(ZMQ_CFLAGS)
libfabcoin_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfabcoin_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp
endif


# wallet: shared between fabcoind and fabcoin-qt, but only linked
# when wallet enabled
libfabcoin_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(FABCOIN_INCLUDES)
libfabcoin_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfabcoin_wallet_a_SOURCES = \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/feebumper.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  policy/rbf.cpp \
  $(FABCOIN_CORE_H)

# crypto primitives library
crypto_libfabcoin_crypto_a_CPPFLAGS = $(AM_CPPFLAGS) $(FABCOIN_CONFIG_INCLUDES)
crypto_libfabcoin_crypto_a_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
crypto_libfabcoin_crypto_a_CFLAGS = $(AM_CXXFLAGS) -fPIC
crypto_libfabcoin_crypto_a_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/common.h \
  crypto/equihash.cpp \
  crypto/equihash.h \
  crypto/equihash.tcc \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/ripemd160.cpp \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha512.cpp \
  crypto/sha512.h \
  crypto/sha3.cpp \
  crypto/sha3.h  

if USE_ASM
crypto_libfabcoin_crypto_a_SOURCES += crypto/sha256_sse4.cpp
endif

# consensus: shared between all executables that validate any consensus rules.
libfabcoin_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(FABCOIN_INCLUDES)
libfabcoin_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -fPIC
libfabcoin_consensus_a_SOURCES = \
  amount.h \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/validation.h \
  hash.cpp \
  hash.h \
  prevector.h \
  primitives/block.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/script.cpp \
  script/script.h \
  script/script_error.cpp \
  script/script_error.h \
  serialize.h \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  utilstrencodings.cpp \
  utilstrencodings.h \
  version.h

# common: shared between fabcoind, and fabcoin-qt and non-server tools
libfabcoin_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(FABCOIN_INCLUDES) -DFASC_BUILD
libfabcoin_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfabcoin_common_a_CFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS) -std=c99
libfabcoin_common_a_SOURCES = \
  base58.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  key.cpp \
  keystore.cpp \
  netaddress.cpp \
  netbase.cpp \
  policy/feerate.cpp \
  primitives/block.cpp \
  protocol.cpp \
  scheduler.cpp \
  script/sign.cpp \
  script/standard.cpp \
  warnings.cpp \
  crypto/sha3.cpp \
  crypto/secp256k1_all_in_one.cpp \
  encodings_crypto.cpp \
  aggregate_schnorr_signature.cpp \
  aggregate_schnorr_signature.h \
  script/fabcoinconsensus.cpp \
  script/interpreter.cpp \
  script/interpreter.h \
  cpp-ethereum-fab/utils/json_spirit/JsonSpiritHeaders.h \
  cpp-ethereum-fab/utils/json_spirit/json_spirit.h \
  cpp-ethereum-fab/utils/json_spirit/json_spirit_error_position.h \
  cpp-ethereum-fab/utils/json_spirit/json_spirit_reader.h \
  cpp-ethereum-fab/utils/json_spirit/json_spirit_reader_template.h \
  cpp-ethereum-fab/utils/json_spirit/json_spirit_stream_reader.h \
  cpp-ethereum-fab/utils/json_spirit/json_spirit_utils.h \
  cpp-ethereum-fab/utils/json_spirit/json_spirit_value.h \
  cpp-ethereum-fab/utils/json_spirit/json_spirit_writer.h \
  cpp-ethereum-fab/utils/json_spirit/json_spirit_writer_template.h \
  cpp-ethereum-fab/libaleth-interpreter/VM.cpp \
  cpp-ethereum-fab/libaleth-interpreter/VM.h \
  cpp-ethereum-fab/libaleth-interpreter/VMOpt.cpp \
  cpp-ethereum-fab/libaleth-interpreter/VMCalls.cpp \
  cpp-ethereum-fab/evmc/lib/instructions/instruction_metrics.c \
  cpp-ethereum-fab/evmc/lib/instructions/instruction_names.c \
  cpp-ethereum-fab/evmc/include/evmc/evmc.h \
  cpp-ethereum-fab/evmc/include/evmc/evmc.hpp \
  cpp-ethereum-fab/evmc/include/evmc/helpers.h \
  cpp-ethereum-fab/evmc/include/evmc/instructions.h \
  cpp-ethereum-fab/evmc/include/evmc/loader.h \
  cpp-ethereum-fab/evmc/include/evmc/utils.h \
  cpp-ethereum-fab/ethash/include/ethash/hash_types.h \
  cpp-ethereum-fab/ethash/include/ethash/hash_types.hpp \
  cpp-ethereum-fab/ethash/include/ethash/keccak.h \
  cpp-ethereum-fab/ethash/include/ethash/keccak.hpp \
  cpp-ethereum-fab/ethash/lib/ethash/bit_manipulation.h \
  cpp-ethereum-fab/ethash/lib/ethash/builtins.h \
  cpp-ethereum-fab/ethash/lib/ethash/endianness.hpp \
  cpp-ethereum-fab/ethash/lib/ethash/ethash-internal.hpp \
  cpp-ethereum-fab/ethash/lib/ethash/kiss99.hpp \
  cpp-ethereum-fab/ethash/lib/ethash/managed.cpp \
  cpp-ethereum-fab/ethash/lib/ethash/primes.c \
  cpp-ethereum-fab/ethash/lib/ethash/primes.h \
  cpp-ethereum-fab/ethash/lib/keccak/keccak.c \
  cpp-ethereum-fab/libaleth-interpreter/interpreter.h \
  cpp-ethereum-fab/libaleth-interpreter/VMConfig.h \
  cpp-ethereum-fab/libdevcore/SHA3.cpp \
  cpp-ethereum-fab/libdevcore/SHA3.h \
  cpp-ethereum-fab/libdevcore/Address.cpp \
  cpp-ethereum-fab/libdevcore/Address.h \
  cpp-ethereum-fab/libdevcore/Base64.cpp \
  cpp-ethereum-fab/libdevcore/Base64.h \
  cpp-ethereum-fab/libdevcore/Common.cpp \
  cpp-ethereum-fab/libdevcore/Common.h \
  cpp-ethereum-fab/libdevcore/CommonData.cpp \
  cpp-ethereum-fab/libdevcore/CommonData.h \
  cpp-ethereum-fab/libdevcore/CommonIO.cpp \
  cpp-ethereum-fab/libdevcore/CommonIO.h \
  cpp-ethereum-fab/libdevcore/CommonJS.cpp \
  cpp-ethereum-fab/libdevcore/CommonJS.h \
  cpp-ethereum-fab/libdevcore/FileSystem.cpp \
  cpp-ethereum-fab/libdevcore/FileSystem.h \
  cpp-ethereum-fab/libdevcore/FixedHash.cpp \
  cpp-ethereum-fab/libdevcore/FixedHash.h \
  cpp-ethereum-fab/libdevcore/Guards.cpp \
  cpp-ethereum-fab/libdevcore/Guards.h \
  cpp-ethereum-fab/libdevcore/Log.cpp \
  cpp-ethereum-fab/libdevcore/Log.h \
  cpp-ethereum-fab/libdevcore/LevelDB.cpp \
  cpp-ethereum-fab/libdevcore/LevelDB.h \
  cpp-ethereum-fab/libdevcore/MemoryDB.cpp \
  cpp-ethereum-fab/libdevcore/MemoryDB.h \
  cpp-ethereum-fab/libdevcore/OverlayDB.cpp \
  cpp-ethereum-fab/libdevcore/OverlayDB.h \
  cpp-ethereum-fab/libdevcore/StateCacheDB.cpp \
  cpp-ethereum-fab/libdevcore/StateCacheDB.h \
  cpp-ethereum-fab/libdevcore/RLP.cpp \
  cpp-ethereum-fab/libdevcore/RLP.h \
  cpp-ethereum-fab/libdevcore/TransientDirectory.cpp \
  cpp-ethereum-fab/libdevcore/TransientDirectory.h \
  cpp-ethereum-fab/libdevcore/TrieCommon.cpp \
  cpp-ethereum-fab/libdevcore/TrieCommon.h \
  cpp-ethereum-fab/libdevcore/Worker.cpp \
  cpp-ethereum-fab/libdevcore/Worker.h \
  cpp-ethereum-fab/libdevcore/DBFactory.h \
  cpp-ethereum-fab/libdevcore/DBFactory.cpp \
  cpp-ethereum-fab/libdevcore/JsonUtils.h \
  cpp-ethereum-fab/libdevcore/JsonUtils.cpp \
  cpp-ethereum-fab/libdevcore/dbfwd.h \
  cpp-ethereum-fab/libdevcore/RLP.cpp \
  cpp-ethereum-fab/libdevcore/TrieHash.cpp \
  cpp-ethereum-fab/libdevcore/TrieDB.h \
  cpp-ethereum-fab/libdevcore/RLP.h \
  cpp-ethereum-fab/libdevcore/TrieHash.h \
  cpp-ethereum-fab/libdevcore/vector_ref.h \
  cpp-ethereum-fab/libdevcore/Exceptions.h \
  cpp-ethereum-fab/libdevcore/db.h \
  cpp-ethereum-fab/libdevcore/concurrent_queue.h \
  cpp-ethereum-fab/libdevcore/Terminal.h \
  cpp-ethereum-fab/libdevcore/Assertions.h \
  cpp-ethereum-fab/libdevcore/LruCache.h \
  cpp-ethereum-fab/libdevcore/UndefMacros.h \
  cpp-ethereum-fab/libdevcrypto/Hash.cpp \
  cpp-ethereum-fab/libdevcrypto/Hash.h \
  cpp-ethereum-fab/libdevcrypto/Common.cpp \
  cpp-ethereum-fab/libdevcrypto/CryptoPP.cpp \
  cpp-ethereum-fab/libdevcrypto/AES.cpp \
  cpp-ethereum-fab/libdevcrypto/Common.h \
  cpp-ethereum-fab/libdevcrypto/CryptoPP.h \
  cpp-ethereum-fab/libdevcrypto/AES.h \
  cpp-ethereum-fab/libdevcrypto/Exceptions.h \
  cpp-ethereum-fab/libdevcrypto/Blake2.cpp \
  cpp-ethereum-fab/libdevcrypto/Blake2.h \
  cpp-ethereum-fab/libp2p/All.h \
  cpp-ethereum-fab/libp2p/Capability.h \
  cpp-ethereum-fab/libp2p/CapabilityHost.h \
  cpp-ethereum-fab/libp2p/Common.h \
  cpp-ethereum-fab/libp2p/Host.h \
  cpp-ethereum-fab/libp2p/Network.h \
  cpp-ethereum-fab/libp2p/NodeTable.h \
  cpp-ethereum-fab/libp2p/Peer.h \
  cpp-ethereum-fab/libp2p/RLPXFrameCoder.h \
  cpp-ethereum-fab/libp2p/RLPXPacket.h \
  cpp-ethereum-fab/libp2p/RLPXSocket.h \
  cpp-ethereum-fab/libp2p/RLPxHandshake.h \
  cpp-ethereum-fab/libp2p/Session.h \
  cpp-ethereum-fab/libp2p/UDP.h \
  cpp-ethereum-fab/libp2p/UPnP.h \
  cpp-ethereum-fab/libethcore/EVMSchedule.h \
  cpp-ethereum-fab/libethcore/EVMSchedule.cpp \
  cpp-ethereum-fab/libethcore/ABI.cpp \
  cpp-ethereum-fab/libethcore/ChainOperationParams.cpp \
  cpp-ethereum-fab/libethcore/Common.cpp \
  cpp-ethereum-fab/libethcore/Precompiled.cpp \
  cpp-ethereum-fab/libethcore/TransactionBase.cpp \
  cpp-ethereum-fab/libethcore/BlockHeader.cpp \
  cpp-ethereum-fab/libethcore/SealEngine.cpp \
  cpp-ethereum-fab/libethcore/ABI.h \
  cpp-ethereum-fab/libethcore/ChainOperationParams.h \
  cpp-ethereum-fab/libethcore/Common.h \
  cpp-ethereum-fab/libethcore/Precompiled.h \
  cpp-ethereum-fab/libethcore/LogEntry.h \
  cpp-ethereum-fab/libethcore/LogEntry.cpp \
  cpp-ethereum-fab/libethcore/Exceptions.h \
  cpp-ethereum-fab/libethcore/TransactionBase.h \
  cpp-ethereum-fab/libethcore/BlockHeader.h \
  cpp-ethereum-fab/libethcore/SealEngine.h \
  cpp-ethereum-fab/libethcore/CommonJS.h \
  cpp-ethereum-fab/libevm/EVMC.cpp \
  cpp-ethereum-fab/libevm/EVMC.h \
  cpp-ethereum-fab/libevm/ExtVMFace.cpp \
  cpp-ethereum-fab/libevm/ExtVMFace.h \
  cpp-ethereum-fab/libevm/VMFactory.cpp \
  cpp-ethereum-fab/libevm/VMFactory.h \
  cpp-ethereum-fab/libevm/Instruction.cpp \
  cpp-ethereum-fab/libevm/Instruction.h \
  cpp-ethereum-fab/libevm/LegacyVM.h \
  cpp-ethereum-fab/libevm/LegacyVM.cpp \
  cpp-ethereum-fab/libevm/LegacyVMCalls.cpp \
  cpp-ethereum-fab/libevm/LegacyVMConfig.h \
  cpp-ethereum-fab/libevm/LegacyVMOpt.cpp \
  cpp-ethereum-fab/libevm/VMFace.h \
  cpp-ethereum-fab/libethereum/ImportPerformanceLogger.cpp \
  cpp-ethereum-fab/libethereum/ImportPerformanceLogger.h \
  cpp-ethereum-fab/libethereum/Account.cpp \
  cpp-ethereum-fab/libethereum/GasPricer.cpp \
  cpp-ethereum-fab/libethereum/ChainParams.cpp \
  cpp-ethereum-fab/libethereum/Transaction.cpp \
  cpp-ethereum-fab/libethereum/ExtVM.cpp \
  cpp-ethereum-fab/libethereum/BlockDetails.cpp \
  cpp-ethereum-fab/libethereum/TransactionQueue.cpp \
  cpp-ethereum-fab/libethereum/TransactionReceipt.cpp \
  cpp-ethereum-fab/libethereum/GenesisInfo.cpp \
  cpp-ethereum-fab/libethereum/ValidationSchemes.h \
  cpp-ethereum-fab/libethereum/ValidationSchemes.cpp \
  cpp-ethereum-fab/libethereum/DatabasePaths.h \
  cpp-ethereum-fab/libethereum/DatabasePaths.cpp \
  cpp-ethereum-fab/libethereum/State.cpp \
  cpp-ethereum-fab/libethereum/Executive.cpp \
  cpp-ethereum-fab/libethereum/Account.h \
  cpp-ethereum-fab/libethereum/GasPricer.h \
  cpp-ethereum-fab/libethereum/SecureTrieDB.h \
  cpp-ethereum-fab/libethereum/State.h \
  cpp-ethereum-fab/libethereum/ChainParams.h \
  cpp-ethereum-fab/libethereum/Transaction.h \
  cpp-ethereum-fab/libethereum/Executive.h \
  cpp-ethereum-fab/libethereum/ExtVM.h \
  cpp-ethereum-fab/libethereum/Block.h \
  cpp-ethereum-fab/libethereum/BlockChain.h \
  cpp-ethereum-fab/libethereum/BlockDetails.h \
  cpp-ethereum-fab/libethereum/TransactionQueue.h \
  cpp-ethereum-fab/libethereum/BlockQueue.h \
  cpp-ethereum-fab/libethereum/TransactionReceipt.h \
  cpp-ethereum-fab/libethereum/GenesisInfo.h \
  cpp-ethereum-fab/libethereum/CodeSizeCache.h \
  cpp-ethereum-fab/libethereum/VerifiedBlock.h \
  cpp-ethereum-fab/libethereum/GenericMiner.h \
  cpp-ethereum-fab/libethereum/GenericFarm.h \
  cpp-ethereum-fab/libethereum/LastBlockHashesFace.h \
  cpp-ethereum-fab/libethereum/Interface.h \
  cpp-ethereum-fab/libethereum/Client.h \
  cpp-ethereum-fab/libethereum/BlockChainImporter.h \
  cpp-ethereum-fab/libethereum/EthereumCapability.h \
  cpp-ethereum-fab/libethereum/ImportPerformanceLogger.h \
  cpp-ethereum-fab/libethereum/SnapshotImporter.h \
  cpp-ethereum-fab/libethereum/SnapshotStorage.h \
  cpp-ethereum-fab/libethereum/StateImporter.h \
  cpp-ethereum-fab/libethereum/ValidationSchemes.h \
  cpp-ethereum-fab/libethereum/WarpCapability.h \
  cpp-ethereum-fab/libethereum/ClientBase.h \
  cpp-ethereum-fab/libethereum/ClientTest.h \
  cpp-ethereum-fab/libethereum/LogFilter.h \
  cpp-ethereum-fab/libethereum/CommonNet.h \
  cpp-ethereum-fab/libethereum/StandardTrace.h \
  cpp-ethereum-fab/libethashseal/GenesisInfo.cpp \
  cpp-ethereum-fab/libethashseal/GenesisInfo.h \
  cpp-ethereum-fab/libethashseal/Ethash.h \
  cpp-ethereum-fab/libethashseal/EthashCPUMiner.h \
  cpp-ethereum-fab/libethashseal/EthashProofOfWork.h \
  cpp-ethereum-fab/libethashseal/genesis/mainNetwork.cpp \
  cpp-ethereum-fab/libethashseal/genesis/fascMainNetwork.cpp \
  cpp-ethereum-fab/libethashseal/genesis/ropsten.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/berlinTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/byzantiumNoProofTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/byzantiumTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/byzantiumTransitionTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/constantinopleFixTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/constantinopleNoProofTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/constantinopleTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/eip150Test.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/eip158Test.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/experimentalTransitionTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/frontierNoProofTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/frontierTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/homesteadTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/istanbulTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/istanbulTransitionTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/mainNetworkNoProofTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/mainNetworkTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/muirGlacierTest.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/ByzantiumToConstantinopleFixAt5Test.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/EIP158ToByzantiumAt5Test.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/frontierToHomesteadAt5Test.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/homesteadToDaoAt5Test.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/homesteadToEIP150At5Test.cpp \
  cpp-ethereum-fab/libethashseal/genesis/test/transitionnetTest.cpp \
  $(FABCOIN_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libfabcoin_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(FABCOIN_INCLUDES)
libfabcoin_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfabcoin_util_a_SOURCES = \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  fs.cpp \
  random.cpp \
  rpc/protocol.cpp \
  support/cleanse.cpp \
  sync.cpp \
  threadinterrupt.cpp \
  util.cpp \
  log_session.cpp \
  utilmoneystr.cpp \
  utilstrencodings.cpp \
  utiltime.cpp \
  $(FABCOIN_CORE_H)

if GLIBC_BACK_COMPAT
libfabcoin_util_a_SOURCES += compat/glibc_compat.cpp
endif

# cli: shared between fabcoin-cli and fabcoin-qt
libfabcoin_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(FABCOIN_INCLUDES)
libfabcoin_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libfabcoin_cli_a_SOURCES = \
  rpc/client.cpp \
  $(FABCOIN_CORE_H)

nodist_libfabcoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# fabcoind binary #
fabcoind_SOURCES = fabcoind.cpp
fabcoind_CPPFLAGS = $(AM_CPPFLAGS) $(FABCOIN_INCLUDES)
fabcoind_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
fabcoind_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
fabcoind_SOURCES += fabcoind-res.rc
endif

fabcoind_LDADD = \
  $(LIBFABCOIN_SERVER) \
  $(LIBFABCOIN_CONSENSUS) \
  $(LIBFABCOIN_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBFABCOIN_UTIL) \
  $(LIBFABCOIN_WALLET) \
  $(LIBFABCOIN_ZMQ) \
  $(LIBFABCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBCRYPTOPP) \
  $(LIBSECP256K1)

fabcoind_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS) $(LIBEQUIHASH_LIBS)

# fabcoin-cli binary #
fabcoin_cli_SOURCES = fabcoin-cli.cpp
fabcoin_cli_CPPFLAGS = $(AM_CPPFLAGS) $(FABCOIN_INCLUDES) $(EVENT_CFLAGS)
fabcoin_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
fabcoin_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
fabcoin_cli_SOURCES += fabcoin-cli-res.rc
endif

fabcoin_cli_LDADD = \
  $(LIBFABCOIN_CLI) \
  $(LIBFABCOIN_COMMON) \
  $(LIBFABCOIN_CONSENSUS) \
  $(LIBUNIVALUE) \
  $(LIBFABCOIN_UTIL) \
  $(LIBFABCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBCRYPTOPP) \
  $(LIBSECP256K1)

fabcoin_cli_LDADD += $(BOOST_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(EVENT_LIBS) $(LIBEQUIHASH_LIBS)
#

# fabcoin-tx binary #
fabcoin_tx_SOURCES = fabcoin-tx.cpp consensus/consensus.cpp
fabcoin_tx_CPPFLAGS = $(AM_CPPFLAGS) $(FABCOIN_INCLUDES)
fabcoin_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
fabcoin_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
fabcoin_tx_SOURCES += fabcoin-tx-res.rc
endif

# WARNING: the build order of files matters.
# Shuffling files in the list below may break the build (or fix it).
# You've been warned.
fabcoin_tx_LDADD = \
  $(LIBFABCOIN_COMMON) \
  $(LIBFABCOIN_UTIL) \
  $(LIBFABCOIN_CONSENSUS) \
  $(LIBFABCOIN_CRYPTO) \
  $(LIBUNIVALUE) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBCRYPTOPP) \
  $(LIBSECP256K1) \
  $(LIBEQUIHASH_LIBS)

fabcoin_tx_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS)
#

# fabcoinconsensus library #
if BUILD_FABCOIN_LIBS
include_HEADERS = script/fabcoinconsensus.h
libfabcoinconsensus_la_SOURCES = $(crypto_libfabcoin_crypto_a_SOURCES) $(libfabcoin_consensus_a_SOURCES) cpp-ethereum-fab/libdevcore/CommonData.cpp

if GLIBC_BACK_COMPAT
  libfabcoinconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libfabcoinconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libfabcoinconsensus_la_LIBADD = $(LIBSECP256K1) $(BOOST_LIBS) $(LIBEQUIHASH_LIBS)
libfabcoinconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include $(FABCOIN_INCLUDES) -DBUILD_FABCOIN_INTERNAL -DNO_UTIL_LOG
libfabcoinconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

endif
#

CTAES_DIST =  crypto/ctaes/bench.c
CTAES_DIST += crypto/ctaes/ctaes.c
CTAES_DIST += crypto/ctaes/ctaes.h
CTAES_DIST += crypto/ctaes/README.md
CTAES_DIST += crypto/ctaes/test.c

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = $(CTAES_DIST)

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f a/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno 
	-rm -f cryptopp/*.o
	-rm -f config.h
	-rm -rf test/__pycache__

if USE_CUDA
nvcc_ARCH :=
nvcc_ARCH += -gencode=arch=compute_52,code=\"sm_52,compute_52\"
nvcc_ARCH += -gencode=arch=compute_50,code=\"sm_50,compute_50\"

nvcc_FLAGS = $(nvcc_ARCH) @CUDA_INCLUDES@ -I. @CUDA_CFLAGS@
nvcc_FLAGS += $(JANSSON_INCLUDES) --ptxas-options="-v"
endif

if USE_CUDA
# we're now targeting all major compute architectures within one binary.
.cu.o:
	$(NVCC) $(nvcc_FLAGS) --maxrregcount=128 -o $@ -c $<

cuda/cuda_equi.o: cuda/cuda_equi.cu
	$(NVCC) $(nvcc_FLAGS) --maxrregcount=64 -o $@ -c $<
cuda/equi_miner.o: cuda/equi_miner.cu
	$(NVCC) $(nvcc_FLAGS) --maxrregcount=64 -o $@ -c $<
cuda/blake2b.o: cuda/blake2b.cu
	$(NVCC) $(nvcc_FLAGS) --maxrregcount=64 -o $@ -c $<
endif

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

.mm.o:
	$(AM_V_CXX) $(OBJCXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	  $(CPPFLAGS) $(AM_CXXFLAGS) $(QT_INCLUDES) $(AM_CXXFLAGS) $(PIE_FLAGS) $(CXXFLAGS) -c -o $@ $<

check-symbols: $(bin_PROGRAMS)
if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(top_srcdir)/contrib/devtools/symbol-check.py < $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) $(top_srcdir)/contrib/devtools/security-check.py < $(bin_PROGRAMS)
endif

%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(<D) $<

if EMBEDDED_LEVELDB
include Makefile.leveldb.include
endif

include Makefile.cryptopp.include
if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
