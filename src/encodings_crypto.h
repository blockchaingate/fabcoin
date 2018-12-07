#ifndef ENCODINGS_CRYPTO_H
#define ENCODINGS_CRYPTO_H
#include <iostream>
#include <vector>
#include <iomanip>
#include <iostream>
#include <sstream>

class Encodings
{
public:
    static bool Base58ToBytes(const std::string& inputBigEndian, std::vector<unsigned char>& outputBigEndian, std::stringstream *commentsOnFailure);
    template <typename anyType>
    static anyType toLowerLatinAlphabet(const anyType& input)
    {
        anyType result;
        result.resize(input.size());
        for (unsigned i = 0; i < input.size(); i ++) {
            if ( 'A' <= input[i] && input[i] <= 'Z') {
                result[i] = input[i] - ('A' - 'a');
            }
        }
        return result;
    }

    template<typename otherType>
    static std::string toHexString(const otherType& other) {
      std::stringstream out;
      for (unsigned i = 0; i < other.size(); i ++)
        out << std::hex << std::setfill('0') << std::setw(2) << ((int) ((unsigned char) other[i]));
      return out.str();
    }

    template<typename otherType>
    static std::string toHexStringOffset(const otherType& other, int offset) {
      std::stringstream out;
      for (unsigned i = offset; i < other.size(); i ++)
        out << std::hex << std::setfill('0') << std::setw(2) << ((int) ((unsigned char) other[i]));
      return out.str();
    }

    static bool char2int(char input, int& output, std::stringstream* commentsOnFailure);
    template<typename inputType, typename outputType>
    static bool fromHex(const inputType& input, outputType& result, std::stringstream* commentsOnFailure)
    {
        result.clear();
        int currentHigh, currentLow;
        for (unsigned i = 0; i + 1 < input.size(); i += 2) {
            if (!Encodings::char2int(input[i], currentHigh, commentsOnFailure))
                return false;
            if (!Encodings::char2int(input[i + 1], currentLow, commentsOnFailure))
                return false;
            unsigned char current = ((unsigned char) currentHigh) * 16 + ((unsigned char) currentLow);
            result.push_back(current);
        }
        return true;
    }
};
#endif
