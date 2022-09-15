#include "kaitaistream.h"

#define KS_STR_ENCODING_NONE

// macOS
#if defined(__APPLE__)
#include <machine/endian.h>
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)
#define __BYTE_ORDER    BYTE_ORDER
#define __BIG_ENDIAN    BIG_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
// Windows with MS compiler
#elif defined(_MSC_VER)
#include <stdlib.h>
#define __LITTLE_ENDIAN     1234
#define __BIG_ENDIAN        4321
#define __BYTE_ORDER        __LITTLE_ENDIAN
#define bswap_16(x) _byteswap_ushort(x)
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)
// BSD
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#include <sys/endian.h>
#define bswap_16(x) bswap16(x)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
// Everything else
#else
#include <endian.h>
#include <byteswap.h>
#endif

#include <iostream>
#include <vector>
#include <stdexcept>

kaitai::kstream::kstream(std::istream *io) {
    m_io = io;
    init();
}

kaitai::kstream::kstream(const std::string &data) : m_io_str(data) {
    m_io = &m_io_str;
    init();
}

void kaitai::kstream::init() {
    exceptions_enable();
    align_to_byte();
}

void kaitai::kstream::close() {
    //  m_io->close();
}

void kaitai::kstream::exceptions_enable() const {
    m_io->exceptions(
        std::istream::eofbit |
        std::istream::failbit |
        std::istream::badbit
    );
}

// ========================================================================
// Stream positioning
// ========================================================================

bool kaitai::kstream::is_eof() const {
    if (m_bits_left > 0) {
        return false;
    }
    char t;
    m_io->exceptions(std::istream::badbit);
    m_io->get(t);
    if (m_io->eof()) {
        m_io->clear();
        exceptions_enable();
        return true;
    } else {
        m_io->unget();
        exceptions_enable();
        return false;
    }
}

void kaitai::kstream::seek(uint64_t pos) {
    m_io->seekg(pos);
}

uint64_t kaitai::kstream::pos() {
    return m_io->tellg();
}

uint64_t kaitai::kstream::size() {
    std::iostream::pos_type cur_pos = m_io->tellg();
    m_io->seekg(0, std::ios::end);
    std::iostream::pos_type len = m_io->tellg();
    m_io->seekg(cur_pos);
    return len;
}

// ========================================================================
// Integer numbers
// ========================================================================

// ------------------------------------------------------------------------
// Signed
// ------------------------------------------------------------------------

int8_t kaitai::kstream::read_s1() {
    char t;
    m_io->get(t);
    return t;
}

// ........................................................................
// Big-endian
// ........................................................................

int16_t kaitai::kstream::read_s2be() {
    int16_t t;
    m_io->read(reinterpret_cast<char *>(&t), 2);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    t = bswap_16(t);
#endif
    return t;
}

int32_t kaitai::kstream::read_s4be() {
    int32_t t;
    m_io->read(reinterpret_cast<char *>(&t), 4);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    t = bswap_32(t);
#endif
    return t;
}

int64_t kaitai::kstream::read_s8be() {
    int64_t t;
    m_io->read(reinterpret_cast<char *>(&t), 8);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    t = bswap_64(t);
#endif
    return t;
}

// ........................................................................
// Little-endian
// ........................................................................

int16_t kaitai::kstream::read_s2le() {
    int16_t t;
    m_io->read(reinterpret_cast<char *>(&t), 2);
#if __BYTE_ORDER == __BIG_ENDIAN
    t = bswap_16(t);
#endif
    return t;
}

int32_t kaitai::kstream::read_s4le() {
    int32_t t;
    m_io->read(reinterpret_cast<char *>(&t), 4);
#if __BYTE_ORDER == __BIG_ENDIAN
    t = bswap_32(t);
#endif
    return t;
}

int64_t kaitai::kstream::read_s8le() {
    int64_t t;
    m_io->read(reinterpret_cast<char *>(&t), 8);
#if __BYTE_ORDER == __BIG_ENDIAN
    t = bswap_64(t);
#endif
    return t;
}

// ------------------------------------------------------------------------
// Unsigned
// ------------------------------------------------------------------------

uint8_t kaitai::kstream::read_u1() {
    char t;
    m_io->get(t);
    return t;
}

// ........................................................................
// Big-endian
// ........................................................................

uint16_t kaitai::kstream::read_u2be() {
    uint16_t t;
    m_io->read(reinterpret_cast<char *>(&t), 2);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    t = bswap_16(t);
#endif
    return t;
}

uint32_t kaitai::kstream::read_u4be() {
    uint32_t t;
    m_io->read(reinterpret_cast<char *>(&t), 4);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    t = bswap_32(t);
#endif
    return t;
}

uint64_t kaitai::kstream::read_u8be() {
    uint64_t t;
    m_io->read(reinterpret_cast<char *>(&t), 8);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    t = bswap_64(t);
#endif
    return t;
}

// ........................................................................
// Little-endian
// ........................................................................

uint16_t kaitai::kstream::read_u2le() {
    uint16_t t;
    m_io->read(reinterpret_cast<char *>(&t), 2);
#if __BYTE_ORDER == __BIG_ENDIAN
    t = bswap_16(t);
#endif
    return t;
}

uint32_t kaitai::kstream::read_u4le() {
    uint32_t t;
    m_io->read(reinterpret_cast<char *>(&t), 4);
#if __BYTE_ORDER == __BIG_ENDIAN
    t = bswap_32(t);
#endif
    return t;
}

uint64_t kaitai::kstream::read_u8le() {
    uint64_t t;
    m_io->read(reinterpret_cast<char *>(&t), 8);
#if __BYTE_ORDER == __BIG_ENDIAN
    t = bswap_64(t);
#endif
    return t;
}

// ========================================================================
// Floating point numbers
// ========================================================================

// ........................................................................
// Big-endian
// ........................................................................

float kaitai::kstream::read_f4be() {
    uint32_t t;
    m_io->read(reinterpret_cast<char *>(&t), 4);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    t = bswap_32(t);
#endif
    return reinterpret_cast<float &>(t);
}

double kaitai::kstream::read_f8be() {
    uint64_t t;
    m_io->read(reinterpret_cast<char *>(&t), 8);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    t = bswap_64(t);
#endif
    return reinterpret_cast<double &>(t);
}

// ........................................................................
// Little-endian
// ........................................................................

float kaitai::kstream::read_f4le() {
    uint32_t t;
    m_io->read(reinterpret_cast<char *>(&t), 4);
#if __BYTE_ORDER == __BIG_ENDIAN
    t = bswap_32(t);
#endif
    return reinterpret_cast<float &>(t);
}

double kaitai::kstream::read_f8le() {
    uint64_t t;
    m_io->read(reinterpret_cast<char *>(&t), 8);
#if __BYTE_ORDER == __BIG_ENDIAN
    t = bswap_64(t);
#endif
    return reinterpret_cast<double &>(t);
}

// ========================================================================
// Unaligned bit values
// ========================================================================

void kaitai::kstream::align_to_byte() {
    m_bits_left = 0;
    m_bits = 0;
}

uint64_t kaitai::kstream::read_bits_int_be(int n) {
    uint64_t res = 0;

    int bits_needed = n - m_bits_left;
    m_bits_left = -bits_needed & 7; // `-bits_needed mod 8`

    if (bits_needed > 0) {
        // 1 bit  => 1 byte
        // 8 bits => 1 byte
        // 9 bits => 2 bytes
        int bytes_needed = ((bits_needed - 1) / 8) + 1; // `ceil(bits_needed / 8)`
        if (bytes_needed > 8)
            throw std::runtime_error("read_bits_int_be: more than 8 bytes requested");
        uint8_t buf[8];
        m_io->read(reinterpret_cast<char *>(buf), bytes_needed);
        for (int i = 0; i < bytes_needed; i++) {
            res = res << 8 | buf[i];
        }

        uint64_t new_bits = res;
        res = res >> m_bits_left | (bits_needed < 64 ? m_bits << bits_needed : 0); // avoid undefined behavior of `x << 64`
        m_bits = new_bits; // will be masked at the end of the function
    } else {
        res = m_bits >> -bits_needed; // shift unneeded bits out
    }

    uint64_t mask = (UINT64_C(1) << m_bits_left) - 1; // `m_bits_left` is in range 0..7, so `(1 << 64)` does not have to be considered
    m_bits &= mask;

    return res;
}

// Deprecated, use read_bits_int_be() instead.
uint64_t kaitai::kstream::read_bits_int(int n) {
    return read_bits_int_be(n);
}

uint64_t kaitai::kstream::read_bits_int_le(int n) {
    uint64_t res = 0;
    int bits_needed = n - m_bits_left;

    if (bits_needed > 0) {
        // 1 bit  => 1 byte
        // 8 bits => 1 byte
        // 9 bits => 2 bytes
        int bytes_needed = ((bits_needed - 1) / 8) + 1; // `ceil(bits_needed / 8)`
        if (bytes_needed > 8)
            throw std::runtime_error("read_bits_int_le: more than 8 bytes requested");
        uint8_t buf[8];
        m_io->read(reinterpret_cast<char *>(buf), bytes_needed);
        for (int i = 0; i < bytes_needed; i++) {
            res |= static_cast<uint64_t>(buf[i]) << (i * 8);
        }

        // NB: for bit shift operators in C++, "if the value of the right operand is
        // negative or is greater or equal to the number of bits in the promoted left
        // operand, the behavior is undefined." (see
        // https://en.cppreference.com/w/cpp/language/operator_arithmetic#Bitwise_shift_operators)
        // So we define our desired behavior here.
        uint64_t new_bits = bits_needed < 64 ? res >> bits_needed : 0;
        res = res << m_bits_left | m_bits;
        m_bits = new_bits;
    } else {
        res = m_bits;
        m_bits >>= n;
    }

    m_bits_left = -bits_needed & 7; // `-bits_needed mod 8`

    if (n < 64) {
        uint64_t mask = (UINT64_C(1) << n) - 1;
        res &= mask;
    }
    // if `n == 64`, do nothing
    return res;
}

// ========================================================================
// Byte arrays
// ========================================================================

std::string kaitai::kstream::read_bytes(std::streamsize len) {
    std::vector<char> result(len);

    // NOTE: streamsize type is signed, negative values are only *supposed* to not be used.
    // http://en.cppreference.com/w/cpp/io/streamsize
    if (len < 0) {
        throw std::runtime_error("read_bytes: requested a negative amount");
    }

    if (len > 0) {
        m_io->read(&result[0], len);
    }

    return std::string(result.begin(), result.end());
}

std::string kaitai::kstream::read_bytes_full() {
    std::iostream::pos_type p1 = m_io->tellg();
    m_io->seekg(0, std::ios::end);
    std::iostream::pos_type p2 = m_io->tellg();
    size_t len = p2 - p1;

    // Note: this requires a std::string to be backed with a
    // contiguous buffer. Officially, it's a only requirement since
    // C++11 (C++98 and C++03 didn't have this requirement), but all
    // major implementations had contiguous buffers anyway.
    std::string result(len, ' ');
    m_io->seekg(p1);
    m_io->read(&result[0], len);

    return result;
}

std::string kaitai::kstream::read_bytes_term(char term, bool include, bool consume, bool eos_error) {
    std::string result;
    std::getline(*m_io, result, term);
    if (m_io->eof()) {
        // encountered EOF
        if (eos_error) {
            throw std::runtime_error("read_bytes_term: encountered EOF");
        }
    } else {
        // encountered terminator
        if (include)
            result.push_back(term);
        if (!consume)
            m_io->unget();
    }
    return result;
}

std::string kaitai::kstream::ensure_fixed_contents(std::string expected) {
    std::string actual = read_bytes(expected.length());

    if (actual != expected) {
        // NOTE: I think printing it outright is not best idea, it could contain non-ASCII characters
        // like backspace and beeps and whatnot. It would be better to print hexlified version, and
        // also to redirect it to stderr.
        throw std::runtime_error("ensure_fixed_contents: actual data does not match expected data");
    }

    return actual;
}

std::string kaitai::kstream::bytes_strip_right(std::string src, char pad_byte) {
    std::size_t new_len = src.length();

    while (new_len > 0 && src[new_len - 1] == pad_byte)
        new_len--;

    return src.substr(0, new_len);
}

std::string kaitai::kstream::bytes_terminate(std::string src, char term, bool include) {
    std::size_t new_len = 0;
    std::size_t max_len = src.length();

    while (new_len < max_len && src[new_len] != term)
        new_len++;

    if (include && new_len < max_len)
        new_len++;

    return src.substr(0, new_len);
}

// ========================================================================
// Byte array processing
// ========================================================================

std::string kaitai::kstream::process_xor_one(std::string data, uint8_t key) {
    size_t len = data.length();
    std::string result(len, ' ');

    for (size_t i = 0; i < len; i++)
        result[i] = data[i] ^ key;

    return result;
}

std::string kaitai::kstream::process_xor_many(std::string data, std::string key) {
    size_t len = data.length();
    size_t kl = key.length();
    std::string result(len, ' ');

    size_t ki = 0;
    for (size_t i = 0; i < len; i++) {
        result[i] = data[i] ^ key[ki];
        ki++;
        if (ki >= kl)
            ki = 0;
    }

    return result;
}

std::string kaitai::kstream::process_rotate_left(std::string data, int amount) {
    size_t len = data.length();
    std::string result(len, ' ');

    for (size_t i = 0; i < len; i++) {
        uint8_t bits = data[i];
        result[i] = (bits << amount) | (bits >> (8 - amount));
    }

    return result;
}

#ifdef KS_ZLIB
#include <zlib.h>

std::string kaitai::kstream::process_zlib(std::string data) {
    int ret;

    unsigned char *src_ptr = reinterpret_cast<unsigned char *>(&data[0]);
    std::stringstream dst_strm;

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    ret = inflateInit(&strm);
    if (ret != Z_OK)
        throw std::runtime_error("process_zlib: inflateInit error");

    strm.next_in = src_ptr;
    strm.avail_in = data.length();

    unsigned char outbuffer[ZLIB_BUF_SIZE];
    std::string outstring;

    // get the decompressed bytes blockwise using repeated calls to inflate
    do {
        strm.next_out = reinterpret_cast<Bytef *>(outbuffer);
        strm.avail_out = sizeof(outbuffer);

        ret = inflate(&strm, 0);

        if (outstring.size() < strm.total_out)
            outstring.append(reinterpret_cast<char *>(outbuffer), strm.total_out - outstring.size());
    } while (ret == Z_OK);

    if (ret != Z_STREAM_END) { // an error occurred that was not EOF
        std::ostringstream exc_msg;
        exc_msg << "process_zlib: error #" << ret << "): " << strm.msg;
        throw std::runtime_error(exc_msg.str());
    }

    if (inflateEnd(&strm) != Z_OK)
        throw std::runtime_error("process_zlib: inflateEnd error");

    return outstring;
}
#endif

// ========================================================================
// Misc utility methods
// ========================================================================

int kaitai::kstream::mod(int a, int b) {
    if (b <= 0)
        throw std::invalid_argument("mod: divisor b <= 0");
    int r = a % b;
    if (r < 0)
        r += b;
    return r;
}

#include <algorithm>
void kaitai::kstream::unsigned_to_decimal(uint64_t number, char *buffer) {
    // Implementation from https://ideone.com/nrQfA8 by Alf P. Steinbach
    // (see https://www.zverovich.net/2013/09/07/integer-to-string-conversion-in-cplusplus.html#comment-1033931478)
    if (number == 0) {
        *buffer++ = '0';
    } else {
        char *p_first = buffer;
        while (number != 0) {
            *buffer++ = static_cast<char>('0' + number % 10);
            number /= 10;
        }
        std::reverse(p_first, buffer);
    }
    *buffer = '\0';
}

std::string kaitai::kstream::reverse(std::string val) {
    std::reverse(val.begin(), val.end());

    return val;
}

uint8_t kaitai::kstream::byte_array_min(const std::string val) {
    uint8_t min = 0xff; // UINT8_MAX
    std::string::const_iterator end = val.end();
    for (std::string::const_iterator it = val.begin(); it != end; ++it) {
        uint8_t cur = static_cast<uint8_t>(*it);
        if (cur < min) {
            min = cur;
        }
    }
    return min;
}

uint8_t kaitai::kstream::byte_array_max(const std::string val) {
    uint8_t max = 0; // UINT8_MIN
    std::string::const_iterator end = val.end();
    for (std::string::const_iterator it = val.begin(); it != end; ++it) {
        uint8_t cur = static_cast<uint8_t>(*it);
        if (cur > max) {
            max = cur;
        }
    }
    return max;
}

// ========================================================================
// Other internal methods
// ========================================================================

#ifndef KS_STR_DEFAULT_ENCODING
#define KS_STR_DEFAULT_ENCODING "UTF-8"
#endif

#ifdef KS_STR_ENCODING_ICONV

#include <iconv.h>
#include <cerrno>
#include <stdexcept>

std::string kaitai::kstream::bytes_to_str(std::string src, std::string src_enc) {
    iconv_t cd = iconv_open(KS_STR_DEFAULT_ENCODING, src_enc.c_str());

    if (cd == (iconv_t)-1) {
        if (errno == EINVAL) {
            throw std::runtime_error("bytes_to_str: invalid encoding pair conversion requested");
        } else {
            throw std::runtime_error("bytes_to_str: error opening iconv");
        }
    }

    size_t src_len = src.length();
    size_t src_left = src_len;

    // Start with a buffer length of double the source length.
    size_t dst_len = src_len * 2;
    std::string dst(dst_len, ' ');
    size_t dst_left = dst_len;

    char *src_ptr = &src[0];
    char *dst_ptr = &dst[0];

    while (true) {
        size_t res = iconv(cd, &src_ptr, &src_left, &dst_ptr, &dst_left);

        if (res == (size_t)-1) {
            if (errno == E2BIG) {
                // dst buffer is not enough to accomodate whole string
                // enlarge the buffer and try again
                size_t dst_used = dst_len - dst_left;
                dst_left += dst_len;
                dst_len += dst_len;
                dst.resize(dst_len);

                // dst.resize might have allocated destination buffer in another area
                // of memory, thus our previous pointer "dst" will be invalid; re-point
                // it using "dst_used".
                dst_ptr = &dst[dst_used];
            } else {
                throw std::runtime_error("bytes_to_str: iconv error");
            }
        } else {
            // conversion successful
            dst.resize(dst_len - dst_left);
            break;
        }
    }

    if (iconv_close(cd) != 0) {
        throw std::runtime_error("bytes_to_str: iconv close error");
    }

    return dst;
}
#elif defined(KS_STR_ENCODING_NONE)
std::string kaitai::kstream::bytes_to_str(std::string src, std::string src_enc) {
    (void)src_enc;
    return src;
}
#else
#error Need to decide how to handle strings: please define one of: KS_STR_ENCODING_ICONV, KS_STR_ENCODING_NONE
#endif
