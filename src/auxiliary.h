#pragma once

#ifndef AUXILIARY_H
#define AUXILIARY_H

#include <algorithm>
#include <pugixml.hpp>

#if defined(__cpp_lib_endian) && __cpp_lib_endian >= 201907L
constexpr bool g_target_lendian = std::endian::native == std::endian::little;

#else
#   warning "std::endian not supported; target machine *must* be little-endian"

constexpr bool g_target_lendian = true;

#endif


namespace tomo {


template <typename CharT>
static bool charcoll(CharT c1, CharT c2)
{
    return std::tolower(c1) == std::tolower(c2);
}


template <typename CharT>
static bool strequal(const std::basic_string<CharT> &s1,
                     const std::basic_string<CharT> &s2)
{
    return std::equal(s1.begin(), s1.end(), s2.begin(), s2.end(), charcoll<CharT>);
}


/** Case-insensitive suffix matching */
template <typename CharT>
static bool has_suffix(const std::basic_string<CharT> &str,
                       const std::basic_string<CharT> &suff)
{
    long n = str.length() - suff.length();
    bool res;

    if (n < 0) {
        return false;
    }
    res = std::equal(str.begin() + n, str.end(), suff.begin(), charcoll<CharT>);
    return res;
}


/** Case-insensitive substring matching. Uses linear memory */
template <typename CharT>
static bool has_substr(const std::basic_string<CharT> &str,
                       const std::basic_string<CharT> &sub)
{
    std::basic_string<CharT> str_cpy, sub_cpy;

    str_cpy.resize(str.size());
    sub_cpy.resize(sub.size());
    std::transform(str.begin(), str.end(), str_cpy.begin(), static_cast<int (*)(int)>(tolower));
    std::transform(sub.begin(), sub.end(), sub_cpy.begin(), static_cast<int (*)(int)>(tolower));
    return str_cpy.find(sub_cpy) != str_cpy.npos;
}


/** Case-SENSITIVE prefix matching
 *  I should just let myself use C++20
 */
template <typename CharT>
static bool prefixed(const std::basic_string<CharT> &str,
                     const std::basic_string<CharT> &pfix)
{
    if (str.size() < pfix.size()) {
        return false;
    } else {
        return std::equal(pfix.begin(), pfix.end(), str.begin());
    }
}


/** oh my god
 * 
 *  java
 * 
 *  why are you like this
 * 
 *  i never realized this was needed because i stopped using java 14 years ago
 */
template <class PrimT>
static void endianswap(PrimT &prim)
{
#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L
    prim = std::byteswap(prim);

#else
    union {
        PrimT val;
        std::array<char, sizeof (PrimT)> buf;
    } u = { prim };

    std::reverse(u.buf.begin(), u.buf.end());
    prim = u.val;

#endif
}


/** DCMTK implements this functionality as well... not gonna include it though */
template <class InputIt>
static void endianswap(InputIt begin, InputIt end)
{
    while (begin != end) {
        endianswap(*begin);
        ++begin;
    }
}


};


#endif /* AUXILIARY_H */
