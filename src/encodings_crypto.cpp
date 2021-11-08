// Copyright (c) 2018 FA Enterprise System
// Copyright (c) 2014-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "encodings_crypto.h"
#include <string.h>
#include <assert.h>


/** All alphanumeric characters except for "0", "I", "O", and "l" */
static const char* pszBase58new = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

bool Encodings::Base58ToBytes(const std::string& inputBigEndian, std::vector<unsigned char>& outputBigEndian, std::stringstream* commentsOnFailure)
{
    // Skip leading spaces.
    unsigned currentIndex = 0;
    while (currentIndex < inputBigEndian.size() && isspace(inputBigEndian[currentIndex]))
        currentIndex ++;
    //std::cout << "DEBUG: converting from base58: " << inputBigEndian << std::endl;
    //Count the leading '1's.
    //The output will be prefixed with numberOfLeadingZeroes zero bytes.
    int numberOfLeadingZeroes = 0;
    while (inputBigEndian[currentIndex] == '1') {
        numberOfLeadingZeroes ++;
        currentIndex ++;
    }
    //std::cout << "DEBUG: number of leading 1's: " << numberOfLeadingZeroes << "; currentIndex: "
    //          << currentIndex << std::endl;
    // Allocate enough space in big-endian base256 representation.
    int maxSize = inputBigEndian.size() * 733 / 1000 + 1; // log(58) / log(256), rounded up.
    std::vector<unsigned char> outputLittleEndianUnpadded; //<- less significant digits have smaller indices.
    outputLittleEndianUnpadded.reserve(maxSize);
    // Process the characters.
    for (; currentIndex < inputBigEndian.size(); currentIndex ++) {
        if (isspace(inputBigEndian[currentIndex]))
            break;
        // Decode base58 character
        const char* ch = strchr(pszBase58new, inputBigEndian[currentIndex]);
        if (ch == nullptr)
            return false;
        // Apply "outputLittleEndianUnpadded = outputLittleEndianUnpadded * 58 + ch".
        int carry = ch - pszBase58new;
        for (unsigned i = 0; i < outputLittleEndianUnpadded.size(); i ++) {
            carry += 58 * ((int) outputLittleEndianUnpadded[i]);
            outputLittleEndianUnpadded[i] = carry % 256;
            carry /= 256;
        }
        if (carry >= 256) {
            std::cout << "Fatal error: bad carryover. " << std::endl;
            assert(false);
        }
        if (carry > 0) {
            outputLittleEndianUnpadded.push_back((unsigned char) carry);
        }
    }
    outputBigEndian.reserve(numberOfLeadingZeroes + outputLittleEndianUnpadded.size());
    outputBigEndian.assign(numberOfLeadingZeroes, 0x00);
    for (auto theIterator = outputLittleEndianUnpadded.rbegin(); theIterator != outputLittleEndianUnpadded.rend(); theIterator ++) {
        outputBigEndian.push_back(*theIterator);
    }
    return true;
}


bool Encodings::char2int(char input, int& output, std::stringstream* commentsOnFailure)
{
    if (input >= '0' && input <= '9') {
        output = input - '0';
        return true;
    }
    if (input >= 'A' && input <= 'F') {
        output = input - 'A' + 10;
        return true;
    }
    if (input >= 'a' && input <= 'f') {
        output = input - 'a' + 10;
        return true;
    }
    if (commentsOnFailure != 0)
        *commentsOnFailure << "Failed to interpret character: " << input << " as a hex digit. ";
    return false;
}


/*
std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend)
{
    // Skip & count leading zeroes.
    int zeroes = 0;
    int length = 0;
    while (pbegin != pend && *pbegin == 0) {
        pbegin ++;
        zeroes ++;
    }
    // Allocate enough space in big-endian base58 representation.
    int size = (pend - pbegin) * 138 / 100 + 1; // log(256) / log(58), rounded up.
    std::vector<unsigned char> b58(size);
    // Process the bytes.
    while (pbegin != pend) {
        int carry = *pbegin;
        int i = 0;
        // Apply "b58 = b58 * 256 + ch".
        for (std::vector<unsigned char>::reverse_iterator it = b58.rbegin(); (carry != 0 || i < length) && (it != b58.rend()); it++, i++) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }

        assert(carry == 0);
        length = i;
        pbegin++;
    }
    // Skip leading zeroes in base58 result.
    std::vector<unsigned char>::iterator it = b58.begin() + (size - length);
    while (it != b58.end() && *it == 0)
        it++;
    // Translate the result into a string.
    std::string str;
    str.reserve(zeroes + (b58.end() - it));
    str.assign(zeroes, '1');
    while (it != b58.end())
        str += pszBase58new[*(it++)];
    return str;
}
*/
