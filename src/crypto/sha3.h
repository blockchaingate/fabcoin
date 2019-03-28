#ifndef SHA3_H
#define SHA3_H
#include <vector>

class Sha3
{
public:
  // 'Words' here refers to uint64_t
  static const int numberOfSpongeWords = (((1600) / 8) / sizeof(uint64_t));
  static const int numberOfKeccakRounds = 24;
  // The portion of the input message that we didn't consume yet.
  bool flagUseKeccak;
  uint64_t saved;
  union
  { // Keccak's state
    uint64_t s[Sha3::numberOfSpongeWords];
    uint8_t sb[Sha3::numberOfSpongeWords * 8];
  };
  // 0..7--the next byte after the set one (starts from 0; 0--none are buffered)
  unsigned byteIndex;

  // 0..24--the next word to integrate input (starts from 0)
  unsigned wordIndex;
  // The double size of the hash output in words (e.g. 16 for Keccak 512)
  unsigned capacityWords;
  std::string computeSha3_256(const std::string& input);
  std::string computeKeccak3_256(const std::string& input);
  std::string computeKeccakOrSha3(const std::string& input);
  static void computeSha3_256_static(const std::string& input, std::vector<unsigned char>& output) {
      Sha3 theHasher;
      return theHasher.computeSha3_256(input, output);
  }
  static void computeSha3_256_static(const std::vector<unsigned char>& input, std::vector<unsigned char>& output){
      Sha3 theHasher;
      return theHasher.computeSha3_256(input, output);
  }
  void computeSha3_256(const std::string& input, std::vector<unsigned char>& output);
  void computeSha3_256(const std::vector<unsigned char>& input, std::vector<unsigned char>& output);
  void computeKeccak3_256(const std::string& input, std::vector<unsigned char>& output);

  void getResultVector(std::vector<unsigned char>& output);
  std::string getResultString();
  void init();
  void update(std::vector<unsigned char>& input);
  void update(const std::string& input);
  void update(void const* inputBuffer, size_t length);
  void finalize();
  void const *sha3_Finalize(void *priv);
  Sha3();
  static void sha3_Init256(void* priv);
  static void sha3_Init384(void* priv);
  static void sha3_Init512(void* priv);
  static void sha3_Update(void *priv, void const *bufIn, size_t len);
  static inline uint64_t rotl64(uint64_t x, unsigned y) {
      return ((x << y) | (x >> ((sizeof(uint64_t) * 8) - y)));
  }
};

#endif
