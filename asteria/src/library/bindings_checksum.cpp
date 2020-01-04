// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_checksum.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/collector.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

template<uint32_t valueT, uint32_t divisorT, int roundT>
    struct CRC32_Generator : CRC32_Generator<(valueT >> 1) ^ (-(valueT & 1) & divisorT), divisorT, roundT + 1>
  {
  };
template<uint32_t valueT, uint32_t divisorT>
    struct CRC32_Generator<valueT, divisorT, 8> : ::std::integral_constant<uint32_t, valueT>
  {
  };
template<uint32_t divisorT, size_t... indicesT>
    constexpr array<uint32_t, 256> do_CRC32_table_impl(const index_sequence<indicesT...>&) noexcept
  {
    return { CRC32_Generator<uint8_t(indicesT), divisorT, 0>::value... };
  }
template<uint32_t divisorT>
    constexpr array<uint32_t, 256> do_CRC32_table() noexcept
  {
    return do_CRC32_table_impl<divisorT>(::std::make_index_sequence<256>());
  }
constexpr auto s_iso3309_CRC32_table = do_CRC32_table<0xEDB88320>();

class CRC32_Hasher final : public Abstract_Opaque
  {
  private:
    uint32_t m_reg = UINT32_MAX;

  public:
    CRC32_Hasher() noexcept
      {
      }

  public:
    tinyfmt& describe(tinyfmt& fmt) const override
      {
        return fmt << "CRC-32 hasher";
      }
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
      {
        return callback;
      }

    void write(const Sval& data) noexcept
      {
        const auto p = reinterpret_cast<const uint8_t*>(data.data());
        const auto n = data.size();
        uint32_t r = this->m_reg;
        // Hash bytes one by one.
        for(size_t i = 0; i != n; ++i) {
          r = s_iso3309_CRC32_table[((r ^ p[i]) & 0xFF)] ^ (r >> 8);
        }
        this->m_reg = r;
      }

    Ival finish() noexcept
      {
        // Get the checksum.
        uint32_t ck = ~(this->m_reg);
        // Reset internal states.
        this->m_reg = UINT32_MAX;
        return ck;
      }
  };

class FNV1a32_Hasher final : public Abstract_Opaque
  {
  private:
    static constexpr uint32_t prime = 16777619;
    static constexpr uint32_t offset = 2166136261;

    uint32_t m_reg = offset;

  public:
    FNV1a32_Hasher() noexcept
      {
      }

  public:
    tinyfmt& describe(tinyfmt& fmt) const override
      {
        return fmt << "FNV-1a hasher (32-bit)";
      }
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
      {
        return callback;
      }

    void write(const Sval& data) noexcept
      {
        const auto p = reinterpret_cast<const uint8_t*>(data.data());
        const auto n = data.size();
        uint32_t r = this->m_reg;
        // Hash bytes one by one.
        for(size_t i = 0; i != n; ++i) {
          r = (r ^ p[i]) * prime;
        }
        this->m_reg = r;
      }

    Ival finish() noexcept
      {
        // Get the checksum.
        uint32_t ck = this->m_reg;
        // Reset internal states.
        this->m_reg = offset;
        return ck;
      }
  };

template<uint32_t valueT>
    struct Hexdigit : ::std::integral_constant<char, char('0' + valueT + ((9 - valueT) >> 29))>
  {
  };
template<uint32_t valueT>
    constexpr array<char, 2> do_generate_hex_digits_for_byte() noexcept
  {
    return { Hexdigit<valueT/16>::value, Hexdigit<valueT%16>::value };
  };
template<size_t... indicesT>
    constexpr array<char, 256, 2> do_generate_hexdigits_impl(const index_sequence<indicesT...>&) noexcept
  {
    return { do_generate_hex_digits_for_byte<uint8_t(indicesT)>()... };
  }
constexpr auto s_hexdigits = do_generate_hexdigits_impl(::std::make_index_sequence<256>());

template<bool bigendT, typename WordT> Sval& do_pdigits_impl(Sval& str, const WordT& ref)
  {
    static_assert(::std::is_unsigned<WordT>::value, "");
    array<uint8_t, sizeof(WordT)> stor_le;
    uint64_t word = static_cast<uint64_t>(ref);
    // Write the word in little-endian order.
    for(size_t i = 0; i != stor_le.size(); ++i) {
      stor_le[i] = word & 0xFF;
      word >>= 8;
    }
    // Append hexadecimal digits.
    if(bigendT)
      ::std::for_each(stor_le.rbegin(), stor_le.rend(), [&](uint8_t b) { str.append(s_hexdigits[b].data(), 2);  });
    else
      ::std::for_each(stor_le.begin(), stor_le.end(), [&](uint8_t b) { str.append(s_hexdigits[b].data(), 2);  });
    return str;
  }

template<typename WordT> Sval& do_pdigits_be(Sval& str, const WordT& ref)
  {
    return do_pdigits_impl<1, WordT>(str, ref);
  }

template<typename WordT> Sval& do_pdigits_le(Sval& str, const WordT& ref)
  {
    return do_pdigits_impl<0, WordT>(str, ref);
  }

template<bool bigendT, typename WordT> WordT& do_load_impl(WordT& ref, const uint8_t* ptr)
  {
    static_assert(::std::is_unsigned<WordT>::value, "");
    array<uint8_t, sizeof(WordT)> stor_be;
    uint64_t word = 0;
    // Re-arrange bytes.
    if(bigendT)
      ::std::copy_n(ptr, stor_be.size(), stor_be.mut_begin());
    else
      ::std::copy_n(ptr, stor_be.size(), stor_be.mut_rbegin());
    // Assemble the word.
    for(uint8_t byte : stor_be) {
      word <<= 8;
      word |= byte;
    }
    return ref = static_cast<WordT>(word);
  }

template<typename WordT> WordT& do_load_be(WordT& ref, const uint8_t* ptr)
  {
    return do_load_impl<1, WordT>(ref, ptr);
  }

template<typename WordT> WordT& do_load_le(WordT& ref, const uint8_t* ptr)
  {
    return do_load_impl<0, WordT>(ref, ptr);
  }

template<typename WordT> constexpr WordT do_rotl(const WordT& ref, size_t bits)
  {
    constexpr auto width = sizeof(WordT) * 8;
    auto sum = (ref << (+bits) % width) | (ref >> (-bits) % width);
    return static_cast<WordT>(sum);
  }

template<typename WordT, size_t sizeT>
    inline void do_padd(array<WordT, sizeT>& lhs, const array<WordT, sizeT>& rhs)
  {
    ::rocket::ranged_for(size_t(0), sizeT, [&](size_t i) { lhs[i] += rhs[i];  });
  }

class MD5_Hasher final : public Abstract_Opaque
  {
  private:
    static constexpr array<uint32_t, 4> init() noexcept
      {
        return { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476 };
      }

    array<uint32_t, 4> m_regs = init();
    uint64_t m_size = 0;
    array<uint8_t, 64> m_chunk;

  public:
    MD5_Hasher() noexcept
      {
      }

  private:
    void do_consume_chunk(const uint8_t* p) noexcept
      {
        uint32_t w;
        uint32_t f, g;
        // https://en.wikipedia.org/wiki/MD5
        auto update = [&](uint32_t i, auto&& specx, uint32_t& a, uint32_t& b, uint32_t& c,
                          uint32_t& d, uint32_t k, uint8_t r)
          {
            specx(i, b, c, d);
            do_load_le(w, p + g * 4);
            w = a + f + k + w;
            a = b + do_rotl(w, r);
          };
        auto spec0 = [&](uint32_t i, uint32_t& b, uint32_t& c, uint32_t& d)
          {
            f = d ^ (b & (c ^ d));
            g = i;
          };
        auto spec1 = [&](uint32_t i, uint32_t& b, uint32_t& c, uint32_t& d)
          {
            f = c ^ (d & (b ^ c));
            g = (5 * i + 1) % 16;
          };
        auto spec2 = [&](uint32_t i, uint32_t& b, uint32_t& c, uint32_t& d)
          {
            f = b ^ c ^ d;
            g = (3 * i + 5) % 16;
          };
        auto spec3 = [&](uint32_t i, uint32_t& b, uint32_t& c, uint32_t& d)
          {
            f = c ^ (b | ~d);
            g = (7 * i) % 16;
          };
        // Unroll loops by hand.
        auto r = this->m_regs;
        // 0 * 16
        update( 0, spec0, r[0], r[1], r[2], r[3], 0xD76AA478,  7);
        update( 1, spec0, r[3], r[0], r[1], r[2], 0xE8C7B756, 12);
        update( 2, spec0, r[2], r[3], r[0], r[1], 0x242070DB, 17);
        update( 3, spec0, r[1], r[2], r[3], r[0], 0xC1BDCEEE, 22);
        update( 4, spec0, r[0], r[1], r[2], r[3], 0xF57C0FAF,  7);
        update( 5, spec0, r[3], r[0], r[1], r[2], 0x4787C62A, 12);
        update( 6, spec0, r[2], r[3], r[0], r[1], 0xA8304613, 17);
        update( 7, spec0, r[1], r[2], r[3], r[0], 0xFD469501, 22);
        update( 8, spec0, r[0], r[1], r[2], r[3], 0x698098D8,  7);
        update( 9, spec0, r[3], r[0], r[1], r[2], 0x8B44F7AF, 12);
        update(10, spec0, r[2], r[3], r[0], r[1], 0xFFFF5BB1, 17);
        update(11, spec0, r[1], r[2], r[3], r[0], 0x895CD7BE, 22);
        update(12, spec0, r[0], r[1], r[2], r[3], 0x6B901122,  7);
        update(13, spec0, r[3], r[0], r[1], r[2], 0xFD987193, 12);
        update(14, spec0, r[2], r[3], r[0], r[1], 0xA679438E, 17);
        update(15, spec0, r[1], r[2], r[3], r[0], 0x49B40821, 22);
        // 1 * 16
        update(16, spec1, r[0], r[1], r[2], r[3], 0xF61E2562,  5);
        update(17, spec1, r[3], r[0], r[1], r[2], 0xC040B340,  9);
        update(18, spec1, r[2], r[3], r[0], r[1], 0x265E5A51, 14);
        update(19, spec1, r[1], r[2], r[3], r[0], 0xE9B6C7AA, 20);
        update(20, spec1, r[0], r[1], r[2], r[3], 0xD62F105D,  5);
        update(21, spec1, r[3], r[0], r[1], r[2], 0x02441453,  9);
        update(22, spec1, r[2], r[3], r[0], r[1], 0xD8A1E681, 14);
        update(23, spec1, r[1], r[2], r[3], r[0], 0xE7D3FBC8, 20);
        update(24, spec1, r[0], r[1], r[2], r[3], 0x21E1CDE6,  5);
        update(25, spec1, r[3], r[0], r[1], r[2], 0xC33707D6,  9);
        update(26, spec1, r[2], r[3], r[0], r[1], 0xF4D50D87, 14);
        update(27, spec1, r[1], r[2], r[3], r[0], 0x455A14ED, 20);
        update(28, spec1, r[0], r[1], r[2], r[3], 0xA9E3E905,  5);
        update(29, spec1, r[3], r[0], r[1], r[2], 0xFCEFA3F8,  9);
        update(30, spec1, r[2], r[3], r[0], r[1], 0x676F02D9, 14);
        update(31, spec1, r[1], r[2], r[3], r[0], 0x8D2A4C8A, 20);
        // 2 * 16
        update(32, spec2, r[0], r[1], r[2], r[3], 0xFFFA3942,  4);
        update(33, spec2, r[3], r[0], r[1], r[2], 0x8771F681, 11);
        update(34, spec2, r[2], r[3], r[0], r[1], 0x6D9D6122, 16);
        update(35, spec2, r[1], r[2], r[3], r[0], 0xFDE5380C, 23);
        update(36, spec2, r[0], r[1], r[2], r[3], 0xA4BEEA44,  4);
        update(37, spec2, r[3], r[0], r[1], r[2], 0x4BDECFA9, 11);
        update(38, spec2, r[2], r[3], r[0], r[1], 0xF6BB4B60, 16);
        update(39, spec2, r[1], r[2], r[3], r[0], 0xBEBFBC70, 23);
        update(40, spec2, r[0], r[1], r[2], r[3], 0x289B7EC6,  4);
        update(41, spec2, r[3], r[0], r[1], r[2], 0xEAA127FA, 11);
        update(42, spec2, r[2], r[3], r[0], r[1], 0xD4EF3085, 16);
        update(43, spec2, r[1], r[2], r[3], r[0], 0x04881D05, 23);
        update(44, spec2, r[0], r[1], r[2], r[3], 0xD9D4D039,  4);
        update(45, spec2, r[3], r[0], r[1], r[2], 0xE6DB99E5, 11);
        update(46, spec2, r[2], r[3], r[0], r[1], 0x1FA27CF8, 16);
        update(47, spec2, r[1], r[2], r[3], r[0], 0xC4AC5665, 23);
        // 3 * 16
        update(48, spec3, r[0], r[1], r[2], r[3], 0xF4292244,  6);
        update(49, spec3, r[3], r[0], r[1], r[2], 0x432AFF97, 10);
        update(50, spec3, r[2], r[3], r[0], r[1], 0xAB9423A7, 15);
        update(51, spec3, r[1], r[2], r[3], r[0], 0xFC93A039, 21);
        update(52, spec3, r[0], r[1], r[2], r[3], 0x655B59C3,  6);
        update(53, spec3, r[3], r[0], r[1], r[2], 0x8F0CCC92, 10);
        update(54, spec3, r[2], r[3], r[0], r[1], 0xFFEFF47D, 15);
        update(55, spec3, r[1], r[2], r[3], r[0], 0x85845DD1, 21);
        update(56, spec3, r[0], r[1], r[2], r[3], 0x6FA87E4F,  6);
        update(57, spec3, r[3], r[0], r[1], r[2], 0xFE2CE6E0, 10);
        update(58, spec3, r[2], r[3], r[0], r[1], 0xA3014314, 15);
        update(59, spec3, r[1], r[2], r[3], r[0], 0x4E0811A1, 21);
        update(60, spec3, r[0], r[1], r[2], r[3], 0xF7537E82,  6);
        update(61, spec3, r[3], r[0], r[1], r[2], 0xBD3AF235, 10);
        update(62, spec3, r[2], r[3], r[0], r[1], 0x2AD7D2BB, 15);
        update(63, spec3, r[1], r[2], r[3], r[0], 0xEB86D391, 21);
        // Accumulate the result.
        do_padd(this->m_regs, r);
      }

  public:
    tinyfmt& describe(tinyfmt& fmt) const override
      {
        return fmt << "MD5 hasher";
      }
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
      {
        return callback;
      }

    void write(const Sval& data) noexcept
      {
        auto bp = reinterpret_cast<const uint8_t*>(data.data());
        auto ep = bp + data.size();
        auto bc = this->m_chunk.mut_begin() + this->m_size % 64;
        auto ec = this->m_chunk.mut_end();
        ptrdiff_t n;
        // If the last chunk was not empty, ...
        if(bc != this->m_chunk.begin()) {
          // ... append data to the last chunk, ...
          n = ::rocket::min(ep - bp, ec - bc);
          ::std::copy_n(bp, n, bc);
          this->m_size += static_cast<uint64_t>(n);
          bp += n;
          bc += n;
          // ... and if is still not full, there aren't going to be any more data.
          if(bc != ec) {
            ROCKET_ASSERT(bp == ep);
            return;
          }
          // Consume the last chunk.
          ROCKET_ASSERT(this->m_size % 64 == 0);
          this->do_consume_chunk(this->m_chunk.data());
          bc = this->m_chunk.mut_begin();
        }
        // Consume as many chunks as possible; don't bother copying them.
        while(ep - bp >= 64) {
          this->do_consume_chunk(bp);
          bp += 64;
          this->m_size += 64;
        }
        // Append any bytes remaining to the last chunk.
        n = ep - bp;
        if(n != 0) {
          ::std::copy_n(bp, n, bc);
          this->m_size += static_cast<uint64_t>(n);
          bp += n;
          bc += n;
        }
        ROCKET_ASSERT(bp == ep);
      }

    Sval finish() noexcept
      {
        // Finalize the hasher.
        auto bc = this->m_chunk.mut_begin() + this->m_size % 64;
        auto ec = this->m_chunk.mut_end();
        ptrdiff_t n;
        // Append a `0x80` byte followed by zeroes.
        *(bc++) = 0x80;
        n = ec - bc;
        if(n < 8) {
          // Wrap.
          ::std::fill_n(bc, n, 0);
          this->do_consume_chunk(this->m_chunk.data());
          bc = this->m_chunk.mut_begin();
        }
        n = ec - bc - 8;
        if(n > 0) {
          // Fill zeroes.
          ::std::fill_n(bc, n, 0);
          bc += n;
        }
        ROCKET_ASSERT(ec - bc == 8);
        // Write the number of bits in little-endian order.
        auto bits = this->m_size * 8;
        for(ptrdiff_t i = 0; i != 8; ++i) {
          bc[i] = bits & 0xFF;
          bits >>= 8;
        }
        this->do_consume_chunk(this->m_chunk.data());
        // Get the checksum.
        Sval ck;
        ck.reserve(this->m_regs.size() * 8);
        ::rocket::for_each(this->m_regs, [&](uint32_t w) { do_pdigits_le(ck, w);  });
        // Reset internal states.
        this->m_regs = init();
        this->m_size = 0;
        return ck;
      }
  };

class SHA1_Hasher final : public Abstract_Opaque
  {
  private:
    static constexpr array<uint32_t, 5> init() noexcept
      {
        return { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
      }

    array<uint32_t, 5> m_regs = init();
    uint64_t m_size = 0;
    array<uint8_t, 64> m_chunk;

  public:
    SHA1_Hasher() noexcept
      {
      }

  private:
    void do_consume_chunk(const uint8_t* p) noexcept
      {
        array<uint32_t, 80> w;
        uint32_t f, k;
        // https://en.wikipedia.org/wiki/SHA-1
        for(size_t i =  0; i < 16; ++i) {
          do_load_be(w[i], p + i * 4);
        }
        for(size_t i = 16; i < 32; ++i) {
          w[i] = do_rotl(w[i-3] ^ w[i- 8] ^ w[i-14] ^ w[i-16], 1);
        }
        for(size_t i = 32; i < 80; ++i) {
          w[i] = do_rotl(w[i-6] ^ w[i-16] ^ w[i-28] ^ w[i-32], 2);
        }
        auto update = [&](uint32_t i, auto&& specx, uint32_t& a, uint32_t& b, uint32_t& c,
                          uint32_t& d, uint32_t& e)
          {
            specx(b, c, d);
            e += do_rotl(a, 5) + f + k + w[i];
            b = do_rotl(b, 30);
          };
        auto spec0 = [&](uint32_t& b, uint32_t& c, uint32_t& d)
          {
            f = d ^ (b & (c ^ d));
            k = 0x5A827999;
          };
        auto spec1 = [&](uint32_t& b, uint32_t& c, uint32_t& d)
          {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
          };
        auto spec2 = [&](uint32_t& b, uint32_t& c, uint32_t& d)
          {
            f = (b & (c | d)) | (c & d);
            k = 0x8F1BBCDC;
          };
        auto spec3 = [&](uint32_t& b, uint32_t& c, uint32_t& d)
          {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
          };
        // Unroll loops by hand.
        auto r = this->m_regs;
        // 0 * 20
        update( 0, spec0, r[0], r[1], r[2], r[3], r[4]);
        update( 1, spec0, r[4], r[0], r[1], r[2], r[3]);
        update( 2, spec0, r[3], r[4], r[0], r[1], r[2]);
        update( 3, spec0, r[2], r[3], r[4], r[0], r[1]);
        update( 4, spec0, r[1], r[2], r[3], r[4], r[0]);
        update( 5, spec0, r[0], r[1], r[2], r[3], r[4]);
        update( 6, spec0, r[4], r[0], r[1], r[2], r[3]);
        update( 7, spec0, r[3], r[4], r[0], r[1], r[2]);
        update( 8, spec0, r[2], r[3], r[4], r[0], r[1]);
        update( 9, spec0, r[1], r[2], r[3], r[4], r[0]);
        update(10, spec0, r[0], r[1], r[2], r[3], r[4]);
        update(11, spec0, r[4], r[0], r[1], r[2], r[3]);
        update(12, spec0, r[3], r[4], r[0], r[1], r[2]);
        update(13, spec0, r[2], r[3], r[4], r[0], r[1]);
        update(14, spec0, r[1], r[2], r[3], r[4], r[0]);
        update(15, spec0, r[0], r[1], r[2], r[3], r[4]);
        update(16, spec0, r[4], r[0], r[1], r[2], r[3]);
        update(17, spec0, r[3], r[4], r[0], r[1], r[2]);
        update(18, spec0, r[2], r[3], r[4], r[0], r[1]);
        update(19, spec0, r[1], r[2], r[3], r[4], r[0]);
        // 1
        update(20, spec1, r[0], r[1], r[2], r[3], r[4]);
        update(21, spec1, r[4], r[0], r[1], r[2], r[3]);
        update(22, spec1, r[3], r[4], r[0], r[1], r[2]);
        update(23, spec1, r[2], r[3], r[4], r[0], r[1]);
        update(24, spec1, r[1], r[2], r[3], r[4], r[0]);
        update(25, spec1, r[0], r[1], r[2], r[3], r[4]);
        update(26, spec1, r[4], r[0], r[1], r[2], r[3]);
        update(27, spec1, r[3], r[4], r[0], r[1], r[2]);
        update(28, spec1, r[2], r[3], r[4], r[0], r[1]);
        update(29, spec1, r[1], r[2], r[3], r[4], r[0]);
        update(30, spec1, r[0], r[1], r[2], r[3], r[4]);
        update(31, spec1, r[4], r[0], r[1], r[2], r[3]);
        update(32, spec1, r[3], r[4], r[0], r[1], r[2]);
        update(33, spec1, r[2], r[3], r[4], r[0], r[1]);
        update(34, spec1, r[1], r[2], r[3], r[4], r[0]);
        update(35, spec1, r[0], r[1], r[2], r[3], r[4]);
        update(36, spec1, r[4], r[0], r[1], r[2], r[3]);
        update(37, spec1, r[3], r[4], r[0], r[1], r[2]);
        update(38, spec1, r[2], r[3], r[4], r[0], r[1]);
        update(39, spec1, r[1], r[2], r[3], r[4], r[0]);
        // 2
        update(40, spec2, r[0], r[1], r[2], r[3], r[4]);
        update(41, spec2, r[4], r[0], r[1], r[2], r[3]);
        update(42, spec2, r[3], r[4], r[0], r[1], r[2]);
        update(43, spec2, r[2], r[3], r[4], r[0], r[1]);
        update(44, spec2, r[1], r[2], r[3], r[4], r[0]);
        update(45, spec2, r[0], r[1], r[2], r[3], r[4]);
        update(46, spec2, r[4], r[0], r[1], r[2], r[3]);
        update(47, spec2, r[3], r[4], r[0], r[1], r[2]);
        update(48, spec2, r[2], r[3], r[4], r[0], r[1]);
        update(49, spec2, r[1], r[2], r[3], r[4], r[0]);
        update(50, spec2, r[0], r[1], r[2], r[3], r[4]);
        update(51, spec2, r[4], r[0], r[1], r[2], r[3]);
        update(52, spec2, r[3], r[4], r[0], r[1], r[2]);
        update(53, spec2, r[2], r[3], r[4], r[0], r[1]);
        update(54, spec2, r[1], r[2], r[3], r[4], r[0]);
        update(55, spec2, r[0], r[1], r[2], r[3], r[4]);
        update(56, spec2, r[4], r[0], r[1], r[2], r[3]);
        update(57, spec2, r[3], r[4], r[0], r[1], r[2]);
        update(58, spec2, r[2], r[3], r[4], r[0], r[1]);
        update(59, spec2, r[1], r[2], r[3], r[4], r[0]);
        // 3
        update(60, spec3, r[0], r[1], r[2], r[3], r[4]);
        update(61, spec3, r[4], r[0], r[1], r[2], r[3]);
        update(62, spec3, r[3], r[4], r[0], r[1], r[2]);
        update(63, spec3, r[2], r[3], r[4], r[0], r[1]);
        update(64, spec3, r[1], r[2], r[3], r[4], r[0]);
        update(65, spec3, r[0], r[1], r[2], r[3], r[4]);
        update(66, spec3, r[4], r[0], r[1], r[2], r[3]);
        update(67, spec3, r[3], r[4], r[0], r[1], r[2]);
        update(68, spec3, r[2], r[3], r[4], r[0], r[1]);
        update(69, spec3, r[1], r[2], r[3], r[4], r[0]);
        update(70, spec3, r[0], r[1], r[2], r[3], r[4]);
        update(71, spec3, r[4], r[0], r[1], r[2], r[3]);
        update(72, spec3, r[3], r[4], r[0], r[1], r[2]);
        update(73, spec3, r[2], r[3], r[4], r[0], r[1]);
        update(74, spec3, r[1], r[2], r[3], r[4], r[0]);
        update(75, spec3, r[0], r[1], r[2], r[3], r[4]);
        update(76, spec3, r[4], r[0], r[1], r[2], r[3]);
        update(77, spec3, r[3], r[4], r[0], r[1], r[2]);
        update(78, spec3, r[2], r[3], r[4], r[0], r[1]);
        update(79, spec3, r[1], r[2], r[3], r[4], r[0]);
        // Accumulate the result.
        do_padd(this->m_regs, r);
      }

  public:
    tinyfmt& describe(tinyfmt& fmt) const override
      {
        return fmt << "SHA-1 hasher";
      }
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
      {
        return callback;
      }

    void write(const Sval& data) noexcept
      {
        auto bp = reinterpret_cast<const uint8_t*>(data.data());
        auto ep = bp + data.size();
        auto bc = this->m_chunk.mut_begin() + this->m_size % 64;
        auto ec = this->m_chunk.mut_end();
        ptrdiff_t n;
        // If the last chunk was not empty, ...
        if(bc != this->m_chunk.begin()) {
          // ... append data to the last chunk, ...
          n = ::rocket::min(ep - bp, ec - bc);
          ::std::copy_n(bp, n, bc);
          this->m_size += static_cast<uint64_t>(n);
          bp += n;
          bc += n;
          // ... and if is still not full, there aren't going to be any more data.
          if(bc != ec) {
            ROCKET_ASSERT(bp == ep);
            return;
          }
          // Consume the last chunk.
          ROCKET_ASSERT(this->m_size % 64 == 0);
          this->do_consume_chunk(this->m_chunk.data());
          bc = this->m_chunk.mut_begin();
        }
        // Consume as many chunks as possible; don't bother copying them.
        while(ep - bp >= 64) {
          this->do_consume_chunk(bp);
          bp += 64;
          this->m_size += 64;
        }
        // Append any bytes remaining to the last chunk.
        n = ep - bp;
        if(n != 0) {
          ::std::copy_n(bp, n, bc);
          this->m_size += static_cast<uint64_t>(n);
          bp += n;
          bc += n;
        }
        ROCKET_ASSERT(bp == ep);
      }

    Sval finish() noexcept
      {
        // Finalize the hasher.
        auto bc = this->m_chunk.mut_begin() + this->m_size % 64;
        auto ec = this->m_chunk.mut_end();
        ptrdiff_t n;
        // Append a `0x80` byte followed by zeroes.
        *(bc++) = 0x80;
        n = ec - bc;
        if(n < 8) {
          // Wrap.
          ::std::fill_n(bc, n, 0);
          this->do_consume_chunk(this->m_chunk.data());
          bc = this->m_chunk.mut_begin();
        }
        n = ec - bc - 8;
        if(n > 0) {
          // Fill zeroes.
          ::std::fill_n(bc, n, 0);
          bc += n;
        }
        ROCKET_ASSERT(ec - bc == 8);
        // Write the number of bits in big-endian order.
        auto bits = this->m_size * 8;
        for(ptrdiff_t i = 7; i != -1; --i) {
          bc[i] = bits & 0xFF;
          bits >>= 8;
        }
        this->do_consume_chunk(this->m_chunk.data());
        // Get the checksum.
        Sval ck;
        ck.reserve(this->m_regs.size() * 8);
        ::rocket::for_each(this->m_regs, [&](uint32_t w) { do_pdigits_be(ck, w);  });
        // Reset internal states.
        this->m_regs = init();
        this->m_size = 0;
        return ck;
      }
  };

class SHA256_Hasher final : public Abstract_Opaque
  {
  private:
    static constexpr array<uint32_t, 8> init() noexcept
      {
        return { 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C,
                 0x1F83D9AB, 0x5BE0CD19 };
      }

    array<uint32_t, 8> m_regs = init();
    uint64_t m_size = 0;
    array<uint8_t, 64> m_chunk;

  public:
    SHA256_Hasher() noexcept
      {
      }

  private:
    void do_consume_chunk(const uint8_t* p) noexcept
      {
        array<uint32_t, 64> w;
        uint32_t s0, maj, t2, s1, ch, t1;
        // https://en.wikipedia.org/wiki/SHA-2
        for(size_t i =  0; i < 16; ++i) {
          do_load_be(w[i], p + i * 4);
        }
        for(size_t i = 16; i < 64; ++i) {
          t1 = w[i-15];
          s0 = do_rotl(t1, 14) ^ do_rotl(t1, 25) ^ (t1 >>  3);
          t2 = w[i- 2];
          s1 = do_rotl(t2, 13) ^ do_rotl(t2, 15) ^ (t2 >> 10);
          w[i] = w[i-16] + w[i-7] + s0 + s1;
        }
        auto update = [&](uint32_t i, uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d,
                          uint32_t& e, uint32_t& f, uint32_t& g, uint32_t& h, uint32_t k)
          {
            s0 = do_rotl(a, 10) ^ do_rotl(a, 19) ^ do_rotl(a, 30);
            maj = (a & b) | (c & (a ^ b));
            t2 = s0 + maj;
            s1 = do_rotl(e,  7) ^ do_rotl(e, 21) ^ do_rotl(e, 26);
            ch = g ^ (e & (f ^ g));
            t1 = h + s1 + ch + k + w[i];
            d += t1;
            h = t1 + t2;
          };
        // Unroll loops by hand.
        auto r = this->m_regs;
        // 0 * 16
        update( 0, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0x428A2F98);
        update( 1, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0x71374491);
        update( 2, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0xB5C0FBCF);
        update( 3, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0xE9B5DBA5);
        update( 4, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0x3956C25B);
        update( 5, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0x59F111F1);
        update( 6, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0x923F82A4);
        update( 7, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0xAB1C5ED5);
        update( 8, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0xD807AA98);
        update( 9, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0x12835B01);
        update(10, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0x243185BE);
        update(11, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0x550C7DC3);
        update(12, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0x72BE5D74);
        update(13, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0x80DEB1FE);
        update(14, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0x9BDC06A7);
        update(15, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0xC19BF174);
        // 1 * 16
        update(16, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0xE49B69C1);
        update(17, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0xEFBE4786);
        update(18, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0x0FC19DC6);
        update(19, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0x240CA1CC);
        update(20, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0x2DE92C6F);
        update(21, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0x4A7484AA);
        update(22, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0x5CB0A9DC);
        update(23, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0x76F988DA);
        update(24, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0x983E5152);
        update(25, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0xA831C66D);
        update(26, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0xB00327C8);
        update(27, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0xBF597FC7);
        update(28, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0xC6E00BF3);
        update(29, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0xD5A79147);
        update(30, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0x06CA6351);
        update(31, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0x14292967);
        // 2 * 16
        update(32, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0x27B70A85);
        update(33, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0x2E1B2138);
        update(34, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0x4D2C6DFC);
        update(35, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0x53380D13);
        update(36, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0x650A7354);
        update(37, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0x766A0ABB);
        update(38, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0x81C2C92E);
        update(39, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0x92722C85);
        update(40, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0xA2BFE8A1);
        update(41, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0xA81A664B);
        update(42, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0xC24B8B70);
        update(43, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0xC76C51A3);
        update(44, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0xD192E819);
        update(45, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0xD6990624);
        update(46, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0xF40E3585);
        update(47, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0x106AA070);
        // 3 * 16
        update(48, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0x19A4C116);
        update(49, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0x1E376C08);
        update(50, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0x2748774C);
        update(51, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0x34B0BCB5);
        update(52, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0x391C0CB3);
        update(53, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0x4ED8AA4A);
        update(54, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0x5B9CCA4F);
        update(55, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0x682E6FF3);
        update(56, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0x748F82EE);
        update(57, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0x78A5636F);
        update(58, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0x84C87814);
        update(59, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0x8CC70208);
        update(60, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0x90BEFFFA);
        update(61, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0xA4506CEB);
        update(62, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0xBEF9A3F7);
        update(63, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0xC67178F2);
        // Accumulate the result.
        do_padd(this->m_regs, r);
      }

  public:
    tinyfmt& describe(tinyfmt& fmt) const override
      {
        return fmt << "SHA-256 hasher";
      }
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
      {
        return callback;
      }

    void write(const Sval& data) noexcept
      {
        auto bp = reinterpret_cast<const uint8_t*>(data.data());
        auto ep = bp + data.size();
        auto bc = this->m_chunk.mut_begin() + this->m_size % 64;
        auto ec = this->m_chunk.mut_end();
        ptrdiff_t n;
        // If the last chunk was not empty, ...
        if(bc != this->m_chunk.begin()) {
          // ... append data to the last chunk, ...
          n = ::rocket::min(ep - bp, ec - bc);
          ::std::copy_n(bp, n, bc);
          this->m_size += static_cast<uint64_t>(n);
          bp += n;
          bc += n;
          // ... and if is still not full, there aren't going to be any more data.
          if(bc != ec) {
            ROCKET_ASSERT(bp == ep);
            return;
          }
          // Consume the last chunk.
          ROCKET_ASSERT(this->m_size % 64 == 0);
          this->do_consume_chunk(this->m_chunk.data());
          bc = this->m_chunk.mut_begin();
        }
        // Consume as many chunks as possible; don't bother copying them.
        while(ep - bp >= 64) {
          this->do_consume_chunk(bp);
          bp += 64;
          this->m_size += 64;
        }
        // Append any bytes remaining to the last chunk.
        n = ep - bp;
        if(n != 0) {
          ::std::copy_n(bp, n, bc);
          this->m_size += static_cast<uint64_t>(n);
          bp += n;
          bc += n;
        }
        ROCKET_ASSERT(bp == ep);
      }

    Sval finish() noexcept
      {
        // Finalize the hasher.
        auto bc = this->m_chunk.mut_begin() + this->m_size % 64;
        auto ec = this->m_chunk.mut_end();
        ptrdiff_t n;
        // Append a `0x80` byte followed by zeroes.
        *(bc++) = 0x80;
        n = ec - bc;
        if(n < 8) {
          // Wrap.
          ::std::fill_n(bc, n, 0);
          this->do_consume_chunk(this->m_chunk.data());
          bc = this->m_chunk.mut_begin();
        }
        n = ec - bc - 8;
        if(n > 0) {
          // Fill zeroes.
          ::std::fill_n(bc, n, 0);
          bc += n;
        }
        ROCKET_ASSERT(ec - bc == 8);
        // Write the number of bits in big-endian order.
        auto bits = this->m_size * 8;
        for(ptrdiff_t i = 7; i != -1; --i) {
          bc[i] = bits & 0xFF;
          bits >>= 8;
        }
        this->do_consume_chunk(this->m_chunk.data());
        // Get the checksum.
        Sval ck;
        ck.reserve(this->m_regs.size() * 8);
        ::rocket::for_each(this->m_regs, [&](uint32_t w) { do_pdigits_be(ck, w);  });
        // Reset internal states.
        this->m_regs = init();
        this->m_size = 0;
        return ck;
      }
  };

}  // namespace

Pval std_checksum_crc32_new_private()
  {
    return Pval(::rocket::make_refcnt<CRC32_Hasher>());
  }

void std_checksum_crc32_new_write(Pval& h, Sval data)
  {
    return h->share_this<CRC32_Hasher>()->write(data);
  }

Ival std_checksum_crc32_new_finish(Pval& h)
  {
    return h->share_this<CRC32_Hasher>()->finish();
  }

Oval std_checksum_crc32_new()
  {
    Oval result;
    //===================================================================
    // * private data
    //===================================================================
    result.insert_or_assign(::rocket::sref("$h"),
      std_checksum_crc32_new_private());
    //===================================================================
    // `.write(data)`
    //===================================================================
    result.insert_or_assign(::rocket::sref("write"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        ::rocket::sref("std.checksum.crc32_new().write"),
        [](cow_vector<Reference>&& args, Reference&& self) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.crc32_new().write"), ::rocket::ref(args));
          // Get the hasher.
          Reference_Modifier::S_object_key xmod = { ::rocket::sref("$h") };
          self.zoom_in(::rocket::move(xmod));
          // Parse arguments.
          Sval data;
          if(!reader.I().g(data).F()) {
            reader.throw_no_matching_function_call();
          }
          // Call the binding function.
          std_checksum_crc32_new_write(self.open().open_opaque(), ::rocket::move(data));
          return Reference_Root::S_null();
        })
      ));
    //===================================================================
    // `.finish()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("finish"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        ::rocket::sref("std.checksum.crc32_new().finish"),
        [](cow_vector<Reference>&& args, Reference&& self) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.crc32_new().finish"), ::rocket::ref(args));
          // Get the hasher.
          Reference_Modifier::S_object_key xmod = { ::rocket::sref("$h") };
          self.zoom_in(::rocket::move(xmod));
          // Parse arguments.
          if(!reader.I().F()) {
            reader.throw_no_matching_function_call();
          }
          // Call the binding function.
          Reference_Root::S_temporary xref = { std_checksum_crc32_new_finish(self.open().open_opaque()) };
          return ::rocket::move(xref);
        })
      ));
    //===================================================================
    // End of `std.checksum.crc32_new()`
    //===================================================================
    return result;
  }

Ival std_checksum_crc32(Sval data)
  {
    CRC32_Hasher h;
    h.write(data);
    return h.finish();
  }

Pval std_checksum_fnv1a32_new_private()
  {
    return Pval(::rocket::make_refcnt<FNV1a32_Hasher>());
  }

void std_checksum_fnv1a32_new_write(Pval& h, Sval data)
  {
    return h->share_this<FNV1a32_Hasher>()->write(data);
  }

Ival std_checksum_fnv1a32_new_finish(Pval& h)
  {
    return h->share_this<FNV1a32_Hasher>()->finish();
  }

Oval std_checksum_fnv1a32_new()
  {
    Oval result;
    //===================================================================
    // * private data
    //===================================================================
    result.insert_or_assign(::rocket::sref("$h"),
      std_checksum_fnv1a32_new_private());
    //===================================================================
    // `.write(data)`
    //===================================================================
    result.insert_or_assign(::rocket::sref("write"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        ::rocket::sref("std.checksum.fnv1a32_new().write"),
        [](cow_vector<Reference>&& args, Reference&& self) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.fnv1a32_new().write"), ::rocket::ref(args));
          // Get the hasher.
          Reference_Modifier::S_object_key xmod = { ::rocket::sref("$h") };
          self.zoom_in(::rocket::move(xmod));
          // Parse arguments.
          Sval data;
          if(!reader.I().g(data).F()) {
            reader.throw_no_matching_function_call();
          }
          // Call the binding function.
          std_checksum_fnv1a32_new_write(self.open().open_opaque(), ::rocket::move(data));
          return Reference_Root::S_null();
        })
      ));
    //===================================================================
    // `.finish()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("finish"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        ::rocket::sref("std.checksum.fnv1a32_new().finish"),
        [](cow_vector<Reference>&& args, Reference&& self) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.fnv1a32_new().finish"), ::rocket::ref(args));
          // Get the hasher.
          Reference_Modifier::S_object_key xmod = { ::rocket::sref("$h") };
          self.zoom_in(::rocket::move(xmod));
          // Parse arguments.
          if(!reader.I().F()) {
            reader.throw_no_matching_function_call();
          }
          // Call the binding function.
          Reference_Root::S_temporary xref = { std_checksum_fnv1a32_new_finish(self.open().open_opaque()) };
          return ::rocket::move(xref);
        })
      ));
    //===================================================================
    // End of `std.checksum.fnv1a32_new()`
    //===================================================================
    return result;
  }

Ival std_checksum_fnv1a32(Sval data)
  {
    FNV1a32_Hasher h;
    h.write(data);
    return h.finish();
  }

Pval std_checksum_md5_new_private()
  {
    return Pval(::rocket::make_refcnt<MD5_Hasher>());
  }

void std_checksum_md5_new_write(Pval& h, Sval data)
  {
    return h->share_this<MD5_Hasher>()->write(data);
  }

Sval std_checksum_md5_new_finish(Pval& h)
  {
    return h->share_this<MD5_Hasher>()->finish();
  }

Oval std_checksum_md5_new()
  {
    Oval result;
    //===================================================================
    // * private data
    //===================================================================
    result.insert_or_assign(::rocket::sref("$h"),
      std_checksum_md5_new_private());
    //===================================================================
    // `.write(data)`
    //===================================================================
    result.insert_or_assign(::rocket::sref("write"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        ::rocket::sref("std.checksum.md5_new().write"),
        [](cow_vector<Reference>&& args, Reference&& self) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.md5_new().write"), ::rocket::ref(args));
          // Get the hasher.
          Reference_Modifier::S_object_key xmod = { ::rocket::sref("$h") };
          self.zoom_in(::rocket::move(xmod));
          // Parse arguments.
          Sval data;
          if(!reader.I().g(data).F()) {
            reader.throw_no_matching_function_call();
          }
          // Call the binding function.
          std_checksum_md5_new_write(self.open().open_opaque(), ::rocket::move(data));
          return Reference_Root::S_null();
        })
      ));
    //===================================================================
    // `.finish()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("finish"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        ::rocket::sref("std.checksum.md5_new().finish"),
        [](cow_vector<Reference>&& args, Reference&& self) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.md5_new().finish"), ::rocket::ref(args));
          // Get the hasher.
          Reference_Modifier::S_object_key xmod = { ::rocket::sref("$h") };
          self.zoom_in(::rocket::move(xmod));
          // Parse arguments.
          if(!reader.I().F()) {
            reader.throw_no_matching_function_call();
          }
          // Call the binding function.
          Reference_Root::S_temporary xref = { std_checksum_md5_new_finish(self.open().open_opaque()) };
          return ::rocket::move(xref);
        })
      ));
    //===================================================================
    // End of `std.checksum.md5_new()`
    //===================================================================
    return result;
  }

Sval std_checksum_md5(Sval data)
  {
    MD5_Hasher h;
    h.write(data);
    return h.finish();
  }

Pval std_checksum_sha1_new_private()
  {
    return Pval(::rocket::make_refcnt<SHA1_Hasher>());
  }

void std_checksum_sha1_new_write(Pval& h, Sval data)
  {
    return h->share_this<SHA1_Hasher>()->write(data);
  }

Sval std_checksum_sha1_new_finish(Pval& h)
  {
    return h->share_this<SHA1_Hasher>()->finish();
  }

Oval std_checksum_sha1_new()
  {
    Oval result;
    //===================================================================
    // * private data
    //===================================================================
    result.insert_or_assign(::rocket::sref("$h"),
      std_checksum_sha1_new_private());
    //===================================================================
    // `.write(data)`
    //===================================================================
    result.insert_or_assign(::rocket::sref("write"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        ::rocket::sref("std.checksum.sha1_new().write"),
        [](cow_vector<Reference>&& args, Reference&& self) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.sha1_new().write"), ::rocket::ref(args));
          // Get the hasher.
          Reference_Modifier::S_object_key xmod = { ::rocket::sref("$h") };
          self.zoom_in(::rocket::move(xmod));
          // Parse arguments.
          Sval data;
          if(!reader.I().g(data).F()) {
            reader.throw_no_matching_function_call();
          }
          // Call the binding function.
          std_checksum_sha1_new_write(self.open().open_opaque(), ::rocket::move(data));
          return Reference_Root::S_null();
        })
      ));
    //===================================================================
    // `.finish()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("finish"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        ::rocket::sref("std.checksum.sha1_new().finish"),
        [](cow_vector<Reference>&& args, Reference&& self) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.sha1_new().finish"), ::rocket::ref(args));
          // Get the hasher.
          Reference_Modifier::S_object_key xmod = { ::rocket::sref("$h") };
          self.zoom_in(::rocket::move(xmod));
          // Parse arguments.
          if(!reader.I().F()) {
            reader.throw_no_matching_function_call();
          }
          // Call the binding function.
          Reference_Root::S_temporary xref = { std_checksum_sha1_new_finish(self.open().open_opaque()) };
          return ::rocket::move(xref);
        })
      ));
    //===================================================================
    // End of `std.checksum.sha1_new()`
    //===================================================================
    return result;
  }

Sval std_checksum_sha1(Sval data)
  {
    SHA1_Hasher h;
    h.write(data);
    return h.finish();
  }

Pval std_checksum_sha256_new_private()
  {
    return Pval(::rocket::make_refcnt<SHA256_Hasher>());
  }

void std_checksum_sha256_new_write(Pval& h, Sval data)
  {
    return h->share_this<SHA256_Hasher>()->write(data);
  }

Sval std_checksum_sha256_new_finish(Pval& h)
  {
    return h->share_this<SHA256_Hasher>()->finish();
  }

Oval std_checksum_sha256_new()
  {
    Oval result;
    //===================================================================
    // * private data
    //===================================================================
    result.insert_or_assign(::rocket::sref("$h"),
      std_checksum_sha256_new_private());
    //===================================================================
    // `.write(data)`
    //===================================================================
    result.insert_or_assign(::rocket::sref("write"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        ::rocket::sref("std.checksum.sha256_new().write"),
        [](cow_vector<Reference>&& args, Reference&& self) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.sha256_new().write"), ::rocket::ref(args));
          // Get the hasher.
          Reference_Modifier::S_object_key xmod = { ::rocket::sref("$h") };
          self.zoom_in(::rocket::move(xmod));
          // Parse arguments.
          Sval data;
          if(!reader.I().g(data).F()) {
            reader.throw_no_matching_function_call();
          }
          // Call the binding function.
          std_checksum_sha256_new_write(self.open().open_opaque(), ::rocket::move(data));
          return Reference_Root::S_null();
        })
      ));
    //===================================================================
    // `.finish()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("finish"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        ::rocket::sref("std.checksum.sha256_new().finish"),
        [](cow_vector<Reference>&& args, Reference&& self) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.sha256_new().finish"), ::rocket::ref(args));
          // Get the hasher.
          Reference_Modifier::S_object_key xmod = { ::rocket::sref("$h") };
          self.zoom_in(::rocket::move(xmod));
          // Parse arguments.
          if(!reader.I().F()) {
            reader.throw_no_matching_function_call();
          }
          // Call the binding function.
          Reference_Root::S_temporary xref = { std_checksum_sha256_new_finish(self.open().open_opaque()) };
          return ::rocket::move(xref);
        })
      ));
    //===================================================================
    // End of `std.checksum.sha256_new()`
    //===================================================================
    return result;
  }

Sval std_checksum_sha256(Sval data)
  {
    SHA256_Hasher h;
    h.write(data);
    return h.finish();
  }

void create_bindings_checksum(Oval& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.checksum.crc32_new()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("crc32_new"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.checksum.crc32_new()`\n"
          "\n"
          "  * Creates a CRC-32 hasher according to ISO/IEC 3309. The divisor\n"
          "    is `0x04C11DB7` (or `0xEDB88320` in reverse form).\n"
          "\n"
          "  * Returns the hasher as an `object` consisting of the following\n"
          "    members:\n"
          "\n"
          "    * `write(data)`\n"
          "    * `finish()`\n"
          "\n"
          "    The function `write()` is used to put data into the hasher,\n"
          "    which shall be of type `string`. After all data have been put,\n"
          "    the function `finish()` extracts the checksum as an `integer`\n"
          "    (whose high-order 32 bits are always zeroes), then resets the\n"
          "    hasher, making it suitable for further data as if it had just\n"
          "    been created.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.crc32_new"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.I().F()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_checksum_crc32_new() };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.checksum.crc32()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("crc32"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.checksum.crc32(data)`\n"
          "\n"
          "  * Calculates the CRC-32 checksum of `data` which must be of type\n"
          "    `string`, as if this function was defined as\n"
          "\n"
          "    ```\n"
          "      std.checksum.crc32 = func(data) {\n"
          "        var h = this.crc32_new();\n"
          "        h.write(data);\n"
          "        return h.F();\n"
          "      };\n"
          "    ```\n"
          "\n"
          "    This function is expected to be both more efficient and easier\n"
          "    to use.\n"
          "\n"
          "  * Returns the CRC-32 checksum as an `integer`. The high-order 32\n"
          "    bits are always zeroes.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.crc32"), ::rocket::ref(args));
          // Parse arguments.
          Sval data;
          if(reader.I().g(data).F()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_checksum_crc32(::rocket::move(data)) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.checksum.fnv1a32_new()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("fnv1a32_new"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.checksum.fnv1a32_new()`\n"
          "\n"
          "  * Creates a 32-bit Fowler-Noll-Vo (a.k.a. FNV) hasher of the\n"
          "    32-bit FNV-1a variant. The FNV prime is `16777619` and the FNV\n"
          "    offset basis is `2166136261`.\n"
          "\n"
          "  * Returns the hasher as an `object` consisting of the following\n"
          "    members:\n"
          "\n"
          "    * `write(data)`\n"
          "    * `finish()`\n"
          "\n"
          "    The function `write()` is used to put data into the hasher,\n"
          "    which shall be of type `string`. After all data have been put,\n"
          "    the function `finish()` extracts the checksum as an `integer`\n"
          "    (whose high-order 32 bits are always zeroes), then resets the\n"
          "    hasher, making it suitable for further data as if it had just\n"
          "    been created.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.fnv1a32_new"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.I().F()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_checksum_fnv1a32_new() };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.checksum.fnv1a32()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("fnv1a32"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.checksum.fnv1a32(data)`\n"
          "\n"
          "  * Calculates the 32-bit FNV-1a checksum of `data` which must be\n"
          "    of type `string`, as if this function was defined as\n"
          "\n"
          "    ```\n"
          "      std.checksum.fnv1a32 = func(data) {\n"
          "        var h = this.fnv1a32_new();\n"
          "        h.write(data);\n"
          "        return h.F();\n"
          "      };\n"
          "    ```\n"
          "\n"
          "    This function is expected to be both more efficient and easier\n"
          "    to use.\n"
          "\n"
          "  * Returns the 32-bit FNV-1a checksum as an `integer`. The\n"
          "    high-order 32 bits are always zeroes.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.fnv1a32"), ::rocket::ref(args));
          // Parse arguments.
          Sval data;
          if(reader.I().g(data).F()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_checksum_fnv1a32(::rocket::move(data)) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.checksum.md5_new()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("md5_new"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.checksum.md5_new()`\n"
          "\n"
          "  * Creates an MD5 hasher.\n"
          "\n"
          "  * Returns the hasher as an `object` consisting of the following\n"
          "    members:\n"
          "\n"
          "    * `write(data)`\n"
          "    * `finish()`\n"
          "\n"
          "    The function `write()` is used to put data into the hasher,\n"
          "    which shall be of type `string`. After all data have been put,\n"
          "    the function `finish()` extracts the checksum as a `string` of\n"
          "    32 hexadecimal digits in uppercase, then resets the hasher,\n"
          "    making it suitable for further data as if it had just been\n"
          "    created.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.md5_new"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.I().F()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_checksum_md5_new() };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.checksum.md5()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("md5"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.checksum.md5(data)`\n"
          "\n"
          "  * Calculates the MD5 checksum of `data` which must be of type\n"
          "    `string`, as if this function was defined as\n"
          "\n"
          "    ```\n"
          "      std.checksum.md5 = func(data) {\n"
          "        var h = this.md5_new();\n"
          "        h.write(data);\n"
          "        return h.F();\n"
          "      };\n"
          "    ```\n"
          "\n"
          "    This function is expected to be both more efficient and easier\n"
          "    to use.\n"
          "\n"
          "  * Returns the 2 checksum as a `string` of 32 hexadecimal digits\n"
          "    in uppercase.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.md5"), ::rocket::ref(args));
          // Parse arguments.
          Sval data;
          if(reader.I().g(data).F()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_checksum_md5(::rocket::move(data)) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.checksum.sha1_new()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sha1_new"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.checksum.sha1_new()`\n"
          "\n"
          "  * Creates an SHA-1 hasher.\n"
          "\n"
          "  * Returns the hasher as an `object` consisting of the following\n"
          "    members:\n"
          "\n"
          "    * `write(data)`\n"
          "    * `finish()`\n"
          "\n"
          "    The function `write()` is used to put data into the hasher,\n"
          "    which shall be of type `string`. After all data have been put,\n"
          "    the function `finish()` extracts the checksum as a `string` of\n"
          "    40 hexadecimal digits in uppercase, then resets the hasher,\n"
          "    making it suitable for further data as if it had just been\n"
          "    created.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.sha1_new"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.I().F()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_checksum_sha1_new() };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.checksum.sha1()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sha1"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.checksum.sha1(data)`\n"
          "\n"
          "  * Calculates the SHA-1 checksum of `data` which must be of type\n"
          "    `string`, as if this function was defined as\n"
          "\n"
          "    ```\n"
          "      std.checksum.sha1 = func(data) {\n"
          "        var h = this.sha1_new();\n"
          "        h.write(data);\n"
          "        return h.F();\n"
          "      };\n"
          "    ```\n"
          "\n"
          "    This function is expected to be both more efficient and easier\n"
          "    to use.\n"
          "\n"
          "  * Returns the SHA-1 checksum as a `string` of 40 hexadecimal\n"
          "    digits in uppercase.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.sha1"), ::rocket::ref(args));
          // Parse arguments.
          Sval data;
          if(reader.I().g(data).F()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_checksum_sha1(::rocket::move(data)) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.checksum.sha256_new()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sha256_new"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.checksum.sha256_new()`\n"
          "\n"
          "  * Creates an SHA-256 hasher.\n"
          "\n"
          "  * Returns the hasher as an `object` consisting of the following\n"
          "    members:\n"
          "\n"
          "    * `write(data)`\n"
          "    * `finish()`\n"
          "\n"
          "    The function `write()` is used to put data into the hasher,\n"
          "    which shall be of type `string`. After all data have been put,\n"
          "    the function `finish()` extracts the checksum as a `string` of\n"
          "    64 hexadecimal digits in uppercase, then resets the hasher,\n"
          "    making it suitable for further data as if it had just been\n"
          "    created.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.sha256_new"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.I().F()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_checksum_sha256_new() };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.checksum.sha256()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sha256"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.checksum.sha256(data)`\n"
          "\n"
          "  * Calculates the SHA-256 checksum of `data` which must be of type\n"
          "    `string`, as if this function was defined as\n"
          "\n"
          "    ```\n"
          "      std.checksum.sha256 = func(data) {\n"
          "        var h = this.sha256_new();\n"
          "        h.write(data);\n"
          "        return h.F();\n"
          "      };\n"
          "    ```\n"
          "\n"
          "    This function is expected to be both more efficient and easier\n"
          "    to use.\n"
          "\n"
          "  * Returns the SHA-256 checksum as a `string` of 64 hexadecimal\n"
          "    digits in uppercase.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.checksum.sha256"), ::rocket::ref(args));
          // Parse arguments.
          Sval data;
          if(reader.I().g(data).F()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_checksum_sha256(::rocket::move(data)) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // End of `std.checksum`
    //===================================================================
  }

}  // namespace Asteria
