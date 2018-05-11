#include <stdlib.h>

#include <boost/algorithm/string.hpp>

#include "S7nodavePlcAddress.h"

using boost::optional;
using std::string;

s7nodavePlcArea S7nodavePlcAddress::getArea() const
{
    return this->area;
}

int S7nodavePlcAddress::getAreaNumber() const
{
    return this->areaNumber;
}

s7nodavePlcDataSize S7nodavePlcAddress::getDataSize() const
{
    return this->dataSize;
}

int S7nodavePlcAddress::getStartByte() const
{
    return this->startByte;
}

char S7nodavePlcAddress::getStartBit() const
{
    return this->startBit;
}

string S7nodavePlcAddress::toString() const
{
    // Buffer for holding temporary string generated by snprintf.
    // The 32 characters are sufficient for all numbers valid in PLC addresses.
    char buf[32];
    string output;
    output.append(areaToString(this->area));
    if (this->area == plcAreaDb) {
        snprintf(buf, 32, "%d", this->areaNumber);
        output.append(buf);
        output.append(".");
        output.append(areaToString(plcAreaDb));
    }
    if (this->area != plcAreaTimer && this->area != plcAreaCounter) {
        output.append(dataSizeToString(this->dataSize, this->area == plcAreaDb));
    }
    snprintf(buf, 32, "%d", this->startByte);
    output.append(buf);
    if (this->dataSize == plcDataSizeBit) {
        output.append(".");
        snprintf(buf, 32, "%d", this->startBit);
        output.append(buf);
    }
    return output;
}

string S7nodavePlcAddress::areaToString(s7nodavePlcArea area)
{
    switch (area) {
    case plcAreaDb:
        return "DB";
    case plcAreaFlags:
        return "F";
    case plcAreaInputs:
        return "I";
    case plcAreaOutputs:
        return "Q";
    case plcAreaTimer:
        return "T";
    case plcAreaCounter:
        return "C";
    default:
        return "";
    }
}

optional<s7nodavePlcArea> S7nodavePlcAddress::stringToArea(string areaString)
{
    boost::to_upper(areaString);
    if (areaString.size() == 1) {
        char c = areaString[0];
        switch (c) {
        case 'F':
        case 'M':
            return plcAreaFlags;
        case 'I':
        case 'E':
            return plcAreaInputs;
        case 'Q':
        case 'A':
            return plcAreaOutputs;
        case 'T':
            return plcAreaTimer;
        case 'C':
        case 'Z':
            return plcAreaCounter;
        default:
            return optional<s7nodavePlcArea>();
        }
    } else if (areaString.size() == 2) {
        char c1 = areaString[0];
        char c2 = areaString[1];
        if (c1 == 'D' && c2 == 'B') {
            return plcAreaDb;
        } else {
            return optional<s7nodavePlcArea>();
        }
    } else {
        return optional<s7nodavePlcArea>();
    }
}

std::string S7nodavePlcAddress::dataSizeToString(s7nodavePlcDataSize dataSize, bool isDB)
{
    switch (dataSize) {
    case plcDataSizeBit:
        if (isDB) {
            return "X";
        } else {
            // Outside of DB blocks, bit addresses do not have a prefix.
            return "";
        }
    case plcDataSizeByte:
        return "B";
    case plcDataSizeWord:
        return "W";
    case plcDataSizeDword:
        return "D";
    default:
        // We return the empty string both for bit and unknown data size.
        return "";
    }
}

boost::optional<s7nodavePlcDataSize> S7nodavePlcAddress::stringToDataSize(std::string dataSizeString, bool isDB)
{
    boost::to_upper(dataSizeString);
    if (dataSizeString.size() == 0 && !isDB) {
        return plcDataSizeBit;
    } else if (dataSizeString.size() == 1) {
        char c = dataSizeString[0];
        switch (c) {
        case 'B':
            return plcDataSizeByte;
        case 'W':
            return plcDataSizeWord;
        case 'D':
            return plcDataSizeDword;
        case 'X':
            if (isDB) {
                return plcDataSizeBit;
            } else {
                return optional<s7nodavePlcDataSize>();
            }
        default:
            return optional<s7nodavePlcDataSize>();
        }
    } else {
        return optional<s7nodavePlcDataSize>();
    }
}

int S7nodavePlcAddress::dataSizeInBits(s7nodavePlcDataSize dataSize)
{
    switch (dataSize) {
    case plcDataSizeBit:
        return 1;
    case plcDataSizeByte:
        return 8;
    case plcDataSizeWord:
        return 16;
    case plcDataSizeDword:
        return 32;
    default:
        return -1;
    }
}

optional<S7nodavePlcAddress> S7nodavePlcAddress::create(s7nodavePlcArea area, int areaNumber, s7nodavePlcDataSize dataSize, int startByte, int startBit)
{
    optional<S7nodavePlcAddress> emptyResult;
    if (area != plcAreaDb && areaNumber != 0) {
        // area number must be null for all areas except DB
        return emptyResult;
    }
    if (dataSize != plcDataSizeBit && startBit != 0) {
        // start bit is only allowed for bits
        return emptyResult;
    }
    if (startBit < 0 || startBit > 7) {
        // start bit must be a number between 0 and 7
        return emptyResult;
    }
    if (startByte < 0) {
        // startByte must be positive
        return emptyResult;
    }
    return S7nodavePlcAddress(area, areaNumber, dataSize, startByte, startBit);
}

static const string digits = "0123456789";

optional<S7nodavePlcAddress> S7nodavePlcAddress::create(string addressString)
{
    optional<S7nodavePlcAddress> emptyResult;
    if (addressString.size() < 2) {
        // The shortest valid address has two characters
        return emptyResult;
    }
    optional<s7nodavePlcArea> area = stringToArea(addressString.substr(0, 2));
    if (area) {
        addressString = addressString.substr(2);
    } else {
        area = stringToArea(addressString.substr(0, 1));
        if (area) {
            addressString = addressString.substr(1);
        } else {
            return emptyResult;
        }
    }
    if (addressString.size() < 1) {
        // An address which only specifies an area, is not valid.
        return emptyResult;
    }

    // We need some special handling for DB-, counter- and timer-type addresses
    int areaNumber = 0;
    if (*area == plcAreaDb) {
        // The next characters are the area number, until we see a dot
        size_t pos = 0;
        pos = addressString.find_first_not_of(digits);
        if (pos == string::npos) {
            // We reached the end of the string without finding a dot.
            return emptyResult;
        }
        string areaNumberString = addressString.substr(0, pos);
        if (areaNumberString.size() == 0) {
            // DB number is missing
            return emptyResult;
        }
        areaNumber = atoi(areaNumberString.c_str());
        // Cut number and dot
        addressString = addressString.substr(pos + 1);
        if (addressString.size() < 2) {
            // We can expect at least two characters for the following "DB"
            return emptyResult;
        }
        // We can misuse stringToArea to look for the "DB" string
        optional<s7nodavePlcArea> tempArea = stringToArea(addressString.substr(0, 2));
        if (tempArea && *tempArea == plcAreaDb) {
            // Cut "DB"
            addressString = addressString.substr(2);
        } else {
            // Not a valid address
            return emptyResult;
        }
    } else if (*area == plcAreaTimer || *area == plcAreaCounter) {
        // Counters and timer do not specify a data size, they are always
        // words.
        // The remaining characters should be digits only.
        if (addressString.find_first_not_of(digits) != string::npos) {
            return emptyResult;
        }
        int number = atoi(addressString.c_str());
        return S7nodavePlcAddress(*area, 0, plcDataSizeWord, number, 0);
    }

    // Now we have to determine the data size.
    if (addressString.length() < 2) {
        // We can expect at least two characters
        return emptyResult;
    }
    optional<s7nodavePlcDataSize> dataSize = stringToDataSize(addressString.substr(0, 1), *area == plcAreaDb);
    if (!dataSize) {
        if (*area != plcAreaDb && addressString.find_first_of(digits) == 0) {
            // The next character is a digit, so might be a bit
            dataSize = plcDataSizeBit;
        } else {
            // Invalid address
            return emptyResult;
        }
    } else {
        addressString = addressString.substr(1);
    }
    // If we expect a bit address, we need a more complex parsing,
    // otherwise all remaining characters should be digits.
    int startByte = 0;
    char startBit = 0;
    if (*dataSize == plcDataSizeBit) {
        // Search for the dot
        size_t pos = addressString.find_first_not_of(digits);
        if (pos == string::npos || addressString[pos] != '.') {
            // We reached the end of the string without finding a dot.
            return emptyResult;
        }
        startByte = atoi(addressString.substr(0, pos).c_str());
        addressString = addressString.substr(pos + 1);
        if (addressString.size() != 1 || addressString.find_first_not_of(digits) != string::npos) {
            // There must be exactly one digit after the dot
            return emptyResult;
        }
        startBit = atoi(addressString.c_str());
        if (startBit < 0 || startBit > 7) {
            // Invalid start bit
            return emptyResult;
        }
    } else {
        if (addressString.find_first_not_of(digits) != string::npos) {
            // Illegal character
            return emptyResult;
        }
        startByte = atoi(addressString.c_str());
    }
    return S7nodavePlcAddress(*area, areaNumber, *dataSize, startByte, startBit);
}

S7nodavePlcAddress::S7nodavePlcAddress(s7nodavePlcArea area, int areaNumber, s7nodavePlcDataSize dataSize, int startByte, char startBit)
{
    this->area = area;
    this->areaNumber = areaNumber;
    this->dataSize = dataSize;
    this->startByte = startByte;
    this->startBit = startBit;
}
