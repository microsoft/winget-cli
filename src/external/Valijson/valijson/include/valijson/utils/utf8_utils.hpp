#pragma once

#include <assert.h>
#include <stdexcept>
#include <string>

#include <valijson/exceptions.hpp>

/*
  Basic UTF-8 manipulation routines, adapted from code that was released into
  the public domain by Jeff Bezanson.
*/

namespace valijson {
namespace utils {

static const uint32_t offsetsFromUTF8[6] = {
    0x00000000UL, 0x00003080UL, 0x000E2080UL,
    0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

/* is c the start of a utf8 sequence? */
inline bool isutf(char c) {
    return ((c & 0xC0) != 0x80);
}

/* reads the next utf-8 sequence out of a string, updating an index */
inline uint64_t u8_nextchar(const char *s, uint64_t *i)
{
    uint64_t ch = 0;
    int sz = 0;

    do {
        ch <<= 6;
        ch += static_cast<unsigned char>(s[(*i)++]);
        sz++;
    } while (s[*i] && !isutf(s[*i]));
    ch -= offsetsFromUTF8[sz-1];

    return ch;
}

/* number of characters */
inline uint64_t u8_strlen(const char *s)
{
    constexpr auto maxLength = std::numeric_limits<uint64_t>::max();
    uint64_t count = 0;
    uint64_t i = 0;

    while (s[i] != 0 && u8_nextchar(s, &i) != 0) {
        if (i == maxLength) {
            throwRuntimeError(
                    "String exceeded maximum size of " +
                    std::to_string(maxLength) + " bytes.");
        }
        count++;
    }

    return count;
}

}  // namespace utils
}  // namespace valijson
