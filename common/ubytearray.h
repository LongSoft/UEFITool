/* ubytearray.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef UBYTEARRAY_H
#define UBYTEARRAY_H

#if defined(QT_CORE_LIB)
// Use Qt class, if Qt is available
#include <QByteArray>
#define UByteArray QByteArray
#else
// Use own implementation
#include <stdint.h>
#include <string>
#include <vector>
#include <algorithm>

class UByteArray
{
public:
    UByteArray() : d() {}
    UByteArray(const UByteArray & ba) : d(ba.d) {}
    UByteArray(const std::basic_string<char> & bs) : d(bs) {}
    UByteArray(const std::vector<char> & bc) : d(bc.data(), bc.size()) {}
    UByteArray(const char* bytes, int32_t size) : d(bytes, size) {}
    UByteArray(const size_t n, char c) : d(n, c) {}
    ~UByteArray() {}

    bool isEmpty() const { return d.length() == 0; }
       
    char* data() { return d.length() == 0 ? NULL : &(d.at(0)); /* Feels dirty, but works for all basic_string implementations I know, is fully OK in C++11 and later*/ }
    const char* data() const { return d.c_str(); }
    const char* constData() const { return d.c_str(); }
    void clear() { d.clear(); }

    UByteArray toUpper() { std::basic_string<char> s = d; std::transform(s.begin(), s.end(), s.begin(), ::toupper); return UByteArray(s); }
    uint32_t toUInt(bool* ok = NULL, const uint8_t base = 10) { return (uint32_t)strtoul(d.c_str(), NULL, base); }

    int32_t size() const { return d.size();  }
    int32_t count(char ch) const { return std::count(d.begin(), d.end(), ch); }
    char at(uint32_t i) const { return d.at(i); }
    char operator[](uint32_t i) const { return d[i]; }
    char& operator[](uint32_t i) { return d[i]; }

    bool startsWith(const UByteArray & ba) const { return 0 == d.find(ba.d, 0); }
    int indexOf(const UByteArray & ba, int from = 0) const { return d.find(ba.d, from); }
    int lastIndexOf(const UByteArray & ba, int from = 0) const {
        size_t old_index = d.npos;
        size_t index = d.find(ba.d, from);
        while (index != d.npos) {
            old_index = index;
            index = d.find(ba.d, index + 1);
        }
        return old_index;
    }

    UByteArray left(int32_t len) const { return d.substr(0, len); }
    UByteArray right(int32_t len) const { return d.substr(d.size() - 1 - len, len); }
    UByteArray mid(int32_t pos, int32_t len = -1) const { return d.substr(pos, len); }

    UByteArray & operator=(const UByteArray & ba) { d = ba.d; return *this; }
    UByteArray & operator+=(const UByteArray & ba) { d += ba.d; return *this; }
    bool operator== (const UByteArray & ba) const { return d == ba.d; }
    bool operator!= (const UByteArray & ba) const { return d != ba.d; }
    inline void swap(UByteArray &other) { std::swap(d, other.d); }
    UByteArray toHex() {
        std::basic_string<char> hex(size() * 2, '\x00');
        for (int32_t i = 0; i < size(); i++) {
            uint8_t low  = d[i] & 0x0F;
            uint8_t high = (d[i] & 0xF0) >> 4;
            low += (low < 10 ? '0' : 'a');
            high += (high < 10 ? '0' : 'a');
            hex[2*i] = low;
            hex[2*i + 1] = high;
        }
        std::reverse(hex.begin(), hex.end());
        return UByteArray(hex);
    }

    std::basic_string<char>::iterator begin() {return d.begin();}
    std::basic_string<char>::iterator end() {return d.end();}
    std::basic_string<char>::const_iterator begin() const {return d.begin();}
    std::basic_string<char>::const_iterator end() const {return d.end();}
    
private:
    std::basic_string<char> d;
};

inline const UByteArray operator+(const UByteArray &a1, const UByteArray &a2)
{
    return UByteArray(a1) += a2;
}

#endif // QT_CORE_LIB
#endif // UBYTEARRAY_H

