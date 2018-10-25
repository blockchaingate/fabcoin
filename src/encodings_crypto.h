#ifndef ENCODINGS_CRYPTO_H
#define ENCODINGS_CRYPTO_H
#include <iostream>
#include <vector>

class Encodings
{
public:
    static bool Base58ToBytes(const std::string& inputBigEndian, std::vector<unsigned char>& outputBigEndian, std::stringstream *commentsOnFailure);

};
#endif
