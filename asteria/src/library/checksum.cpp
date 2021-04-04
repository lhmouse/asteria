// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "checksum.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../utils.hpp"
#include <sys/stat.h>

namespace asteria {
namespace {

::std::reference_wrapper<V_opaque>
do_open_private(Reference&& self, const phsh_string& name)
  {
    self.push_modifier_object_key(name);
    auto& value = self.dereference_mutable();
    return value.open_opaque();
  }

void
do_set_private(V_object& result, const phsh_string& name, V_opaque&& h)
  {
    result.insert_or_assign(name, ::std::move(h));
  }

template<uint32_t valT, uint32_t divT, int N>
struct CRC32_Generator
  : CRC32_Generator<(valT >> 1) ^ (-(valT & 1) & divT), divT, N + 1>
  { };

template<uint32_t valT, uint32_t divT>
struct CRC32_Generator<valT, divT, 8>
  : ::std::integral_constant<uint32_t, valT>
  { };

template<uint32_t divT, size_t... S>
constexpr
array<uint32_t, 256>
do_CRC32_table_impl(const index_sequence<S...>&)
  noexcept
  { return { CRC32_Generator<uint8_t(S), divT, 0>::value... };  }

template<uint32_t divT>
constexpr
array<uint32_t, 256>
do_CRC32_table()
  noexcept
  { return do_CRC32_table_impl<divT>(::std::make_index_sequence<256>());  }

constexpr auto s_iso3309_CRC32_table = do_CRC32_table<0xEDB88320>();

class CRC32_Hasher
  final
  : public Abstract_Opaque
  {
  private:
    uint32_t m_reg = UINT32_MAX;

  public:
    tinyfmt&
    describe(tinyfmt& fmt)
      const override
      { return fmt << "instance of `std.checksum.CRC32` at `" << this << "`";  }

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
      const override
      { return callback;  }

    CRC32_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& output)
      const override
      { return noadl::clone_opaque(output, *this);  }

    void
    update(const void* data, size_t size)
      noexcept
      {
        auto bp = static_cast<const uint8_t*>(data);
        auto ep = bp + size;

        // Hash bytes one by one.
        uint32_t r = this->m_reg;
        while(bp != ep)
          r = s_iso3309_CRC32_table[((r ^ *(bp++)) & 0xFF)] ^ (r >> 8);
        this->m_reg = r;
      }

    V_integer
    finish()
      noexcept
      {
        // Get the checksum.
        uint32_t ck = ~(this->m_reg);

        // Reset internal states.
        this->m_reg = UINT32_MAX;
        return ck;
      }
  };

void
do_construct_CRC32(V_object& result)
  {
    static constexpr auto uuid = sref("#{2C78B9D8-A8F4-4CE9-36E7-12B9EE14AD3D}");
    do_set_private(result, uuid, std_checksum_CRC32_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING_BEGIN("std.checksum.CRC32::update", self, global, reader) {
        const auto href = do_open_private(::std::move(self), uuid);
        V_string data;

        reader.start_overload();
        reader.required(data);    // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_CRC32_update, href, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING_BEGIN("std.checksum.CRC32::finish", self, global, reader) {
        const auto href = do_open_private(::std::move(self), uuid);

        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_CRC32_finish, href);
      }
      ASTERIA_BINDING_END);
  }

class FNV1a32_Hasher
  final
  : public Abstract_Opaque
  {
  public:
    enum : uint32_t
      {
        prime   = 16777619,
        offset  = 2166136261,
      };

  private:
    uint32_t m_reg = offset;

  public:
    tinyfmt&
    describe(tinyfmt& fmt)
      const override
      { return fmt << "instance of `std.checksum.FNV1a32` at `" << this << "`";  }

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
      const override
      { return callback;  }

    FNV1a32_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& output)
      const override
      { return noadl::clone_opaque(output, *this);  }

    void
    update(const void* data, size_t size)
      noexcept
      {
        auto bp = static_cast<const uint8_t*>(data);
        auto ep = bp + size;

        // Hash bytes one by one.
        uint32_t r = this->m_reg;
        while(bp != ep)
          r = (r ^ *(bp++)) * prime;
        this->m_reg = r;
      }

    V_integer
    finish()
      noexcept
      {
        // Get the checksum.
        uint32_t ck = this->m_reg;

        // Reset internal states.
        this->m_reg = offset;
        return ck;
      }
  };

void
do_construct_FNV1a32(V_object& result)
  {
    static constexpr auto uuid = sref("#{2C79571C-5D7B-4674-056A-6C0D075A82FC}");
    do_set_private(result, uuid, std_checksum_FNV1a32_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING_BEGIN("std.checksum.FNV1a32::update", self, global, reader) {
        const auto href = do_open_private(::std::move(self), uuid);
        V_string data;

        reader.start_overload();
        reader.required(data);    // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_FNV1a32_update, href, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING_BEGIN("std.checksum.FNV1a32::finish", self, global, reader) {
        const auto href = do_open_private(::std::move(self), uuid);

        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_FNV1a32_finish, href);
      }
      ASTERIA_BINDING_END);
  }

template<size_t N>
cow_string
do_print_words_be(const array<uint32_t, N>& words)
  {
    cow_string str;
    str.reserve(N * 2);

    uint32_t ch;
    for(uint32_t word : words) {
      for(uint32_t k = 0;  k != 8;  ++k) {
        ch = word >> 28;
        str += static_cast<char>('0' + ch + ((9 - ch) >> 29));
        word <<= 4;
      }
    }
    return str;
  }

template<size_t N>
cow_string
do_print_words_le(const array<uint32_t, N>& words)
  {
    cow_string str;
    str.reserve(N * 2);

    uint32_t ch;
    for(uint32_t word : words) {
      for(uint32_t k = 0;  k != 4;  ++k) {
        ch = (word >> 4) & 0x0F;
        str += static_cast<char>('0' + ch + ((9 - ch) >> 29));
        ch = word & 0x0F;
        str += static_cast<char>('0' + ch + ((9 - ch) >> 29));
        word >>= 8;
      }
    }
    return str;
  }

template<size_t N>
void
do_accumulate_words(array<uint32_t, N>& lhs, const array<uint32_t, N>& rhs)
  noexcept
  {
    for(size_t k = 0;  k != N;  ++k)
      lhs[k] += rhs[k];
  }

uint32_t
do_load_be(const uint8_t* ptr)
  noexcept
  {
    uint32_t word;
    ::std::memcpy(&word, ptr, 4);
    return be32toh(word);
  }

uint32_t
do_load_le(const uint8_t* ptr)
  noexcept
  {
    uint32_t word;
    ::std::memcpy(&word, ptr, 4);
    return le32toh(word);
  }

constexpr
uint32_t
do_rotl(uint32_t value, size_t n)
  noexcept
  {
    return value << n % 32 | value >> (32 - n) % 32;
  }

class MD5_Hasher
  final
  : public Abstract_Opaque
  {
  private:
    static constexpr
    array<uint32_t, 4>
    init()
      noexcept
      { return { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476 };  }

  private:
    array<uint32_t, 4> m_regs = init();
    uint64_t m_size = 0;
    array<uint8_t, 64> m_chunk;

  private:
    void
    do_consume_chunk(const uint8_t* p)
      noexcept
      {
        // https://en.wikipedia.org/wiki/MD5
        auto r = this->m_regs;
        uint32_t w, f, g;

#define XHASH(i, sp, a, b, c, d, k, rb)  \
        sp(i, b, c, d);  \
        w = do_load_le(p + g * 4);  \
        w += a + f + k;  \
        a = b + do_rotl(w, rb);

#define SP_0(i, b, c, d)  \
        f = d ^ (b & (c ^ d));  \
        g = i;

#define SP_1(i, b, c, d)  \
        f = c ^ (d & (b ^ c));  \
        g = (5 * i + 1) % 16;

#define SP_2(i, b, c, d)  \
        f = b ^ c ^ d;  \
        g = (3 * i + 5) % 16;

#define SP_3(i, b, c, d)  \
        f = c ^ (b | ~d);  \
        g = (7 * i) % 16;

        XHASH( 0, SP_0, r[0], r[1], r[2], r[3], 0xD76AA478,  7);
        XHASH( 1, SP_0, r[3], r[0], r[1], r[2], 0xE8C7B756, 12);
        XHASH( 2, SP_0, r[2], r[3], r[0], r[1], 0x242070DB, 17);
        XHASH( 3, SP_0, r[1], r[2], r[3], r[0], 0xC1BDCEEE, 22);
        XHASH( 4, SP_0, r[0], r[1], r[2], r[3], 0xF57C0FAF,  7);
        XHASH( 5, SP_0, r[3], r[0], r[1], r[2], 0x4787C62A, 12);
        XHASH( 6, SP_0, r[2], r[3], r[0], r[1], 0xA8304613, 17);
        XHASH( 7, SP_0, r[1], r[2], r[3], r[0], 0xFD469501, 22);
        XHASH( 8, SP_0, r[0], r[1], r[2], r[3], 0x698098D8,  7);
        XHASH( 9, SP_0, r[3], r[0], r[1], r[2], 0x8B44F7AF, 12);
        XHASH(10, SP_0, r[2], r[3], r[0], r[1], 0xFFFF5BB1, 17);
        XHASH(11, SP_0, r[1], r[2], r[3], r[0], 0x895CD7BE, 22);
        XHASH(12, SP_0, r[0], r[1], r[2], r[3], 0x6B901122,  7);
        XHASH(13, SP_0, r[3], r[0], r[1], r[2], 0xFD987193, 12);
        XHASH(14, SP_0, r[2], r[3], r[0], r[1], 0xA679438E, 17);
        XHASH(15, SP_0, r[1], r[2], r[3], r[0], 0x49B40821, 22);

        XHASH(16, SP_1, r[0], r[1], r[2], r[3], 0xF61E2562,  5);
        XHASH(17, SP_1, r[3], r[0], r[1], r[2], 0xC040B340,  9);
        XHASH(18, SP_1, r[2], r[3], r[0], r[1], 0x265E5A51, 14);
        XHASH(19, SP_1, r[1], r[2], r[3], r[0], 0xE9B6C7AA, 20);
        XHASH(20, SP_1, r[0], r[1], r[2], r[3], 0xD62F105D,  5);
        XHASH(21, SP_1, r[3], r[0], r[1], r[2], 0x02441453,  9);
        XHASH(22, SP_1, r[2], r[3], r[0], r[1], 0xD8A1E681, 14);
        XHASH(23, SP_1, r[1], r[2], r[3], r[0], 0xE7D3FBC8, 20);
        XHASH(24, SP_1, r[0], r[1], r[2], r[3], 0x21E1CDE6,  5);
        XHASH(25, SP_1, r[3], r[0], r[1], r[2], 0xC33707D6,  9);
        XHASH(26, SP_1, r[2], r[3], r[0], r[1], 0xF4D50D87, 14);
        XHASH(27, SP_1, r[1], r[2], r[3], r[0], 0x455A14ED, 20);
        XHASH(28, SP_1, r[0], r[1], r[2], r[3], 0xA9E3E905,  5);
        XHASH(29, SP_1, r[3], r[0], r[1], r[2], 0xFCEFA3F8,  9);
        XHASH(30, SP_1, r[2], r[3], r[0], r[1], 0x676F02D9, 14);
        XHASH(31, SP_1, r[1], r[2], r[3], r[0], 0x8D2A4C8A, 20);

        XHASH(32, SP_2, r[0], r[1], r[2], r[3], 0xFFFA3942,  4);
        XHASH(33, SP_2, r[3], r[0], r[1], r[2], 0x8771F681, 11);
        XHASH(34, SP_2, r[2], r[3], r[0], r[1], 0x6D9D6122, 16);
        XHASH(35, SP_2, r[1], r[2], r[3], r[0], 0xFDE5380C, 23);
        XHASH(36, SP_2, r[0], r[1], r[2], r[3], 0xA4BEEA44,  4);
        XHASH(37, SP_2, r[3], r[0], r[1], r[2], 0x4BDECFA9, 11);
        XHASH(38, SP_2, r[2], r[3], r[0], r[1], 0xF6BB4B60, 16);
        XHASH(39, SP_2, r[1], r[2], r[3], r[0], 0xBEBFBC70, 23);
        XHASH(40, SP_2, r[0], r[1], r[2], r[3], 0x289B7EC6,  4);
        XHASH(41, SP_2, r[3], r[0], r[1], r[2], 0xEAA127FA, 11);
        XHASH(42, SP_2, r[2], r[3], r[0], r[1], 0xD4EF3085, 16);
        XHASH(43, SP_2, r[1], r[2], r[3], r[0], 0x04881D05, 23);
        XHASH(44, SP_2, r[0], r[1], r[2], r[3], 0xD9D4D039,  4);
        XHASH(45, SP_2, r[3], r[0], r[1], r[2], 0xE6DB99E5, 11);
        XHASH(46, SP_2, r[2], r[3], r[0], r[1], 0x1FA27CF8, 16);
        XHASH(47, SP_2, r[1], r[2], r[3], r[0], 0xC4AC5665, 23);

        XHASH(48, SP_3, r[0], r[1], r[2], r[3], 0xF4292244,  6);
        XHASH(49, SP_3, r[3], r[0], r[1], r[2], 0x432AFF97, 10);
        XHASH(50, SP_3, r[2], r[3], r[0], r[1], 0xAB9423A7, 15);
        XHASH(51, SP_3, r[1], r[2], r[3], r[0], 0xFC93A039, 21);
        XHASH(52, SP_3, r[0], r[1], r[2], r[3], 0x655B59C3,  6);
        XHASH(53, SP_3, r[3], r[0], r[1], r[2], 0x8F0CCC92, 10);
        XHASH(54, SP_3, r[2], r[3], r[0], r[1], 0xFFEFF47D, 15);
        XHASH(55, SP_3, r[1], r[2], r[3], r[0], 0x85845DD1, 21);
        XHASH(56, SP_3, r[0], r[1], r[2], r[3], 0x6FA87E4F,  6);
        XHASH(57, SP_3, r[3], r[0], r[1], r[2], 0xFE2CE6E0, 10);
        XHASH(58, SP_3, r[2], r[3], r[0], r[1], 0xA3014314, 15);
        XHASH(59, SP_3, r[1], r[2], r[3], r[0], 0x4E0811A1, 21);
        XHASH(60, SP_3, r[0], r[1], r[2], r[3], 0xF7537E82,  6);
        XHASH(61, SP_3, r[3], r[0], r[1], r[2], 0xBD3AF235, 10);
        XHASH(62, SP_3, r[2], r[3], r[0], r[1], 0x2AD7D2BB, 15);
        XHASH(63, SP_3, r[1], r[2], r[3], r[0], 0xEB86D391, 21);

#undef XHASH
#undef SP_0
#undef SP_1
#undef SP_2
#undef SP_3

        do_accumulate_words(this->m_regs, r);
      }

  public:
    tinyfmt&
    describe(tinyfmt& fmt)
      const override
      { return fmt << "instance of `std.checksum.MD5` at `" << this << "`";  }

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
      const override
      { return callback;  }

    MD5_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& output)
      const override
      { return noadl::clone_opaque(output, *this);  }

    void
    update(const void* data, size_t size)
      noexcept
      {
        auto bp = static_cast<const uint8_t*>(data);
        auto ep = bp + size;
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

    V_string
    finish()
      noexcept
      {
        auto bc = this->m_chunk.mut_begin() + this->m_size % 64;
        auto ec = this->m_chunk.mut_end();
        ptrdiff_t n;

        // Append a `0x80` byte followed by zeroes.
        *(bc++) = 0x80;
        n = ec - bc;
        if(n < 8) {
          ::std::fill_n(bc, n, 0);
          this->do_consume_chunk(this->m_chunk.data());
          bc = this->m_chunk.mut_begin();
        }
        n = ec - bc - 8;
        if(n > 0) {
          ::std::fill_n(bc, n, 0);
          bc += n;
        }
        ROCKET_ASSERT(ec - bc == 8);

        // Write the number of bits in little-endian order.
        uint64_t bits = htole64(this->m_size * 8);
        ::std::memcpy(bc, &bits, 8);
        this->do_consume_chunk(this->m_chunk.data());

        // Get the checksum.
        cow_string ck = do_print_words_le(this->m_regs);

        // Reset internal states.
        this->m_regs = init();
        this->m_size = 0;
        return ck;
      }
  };

void
do_construct_MD5(V_object& result)
  {
    static constexpr auto uuid = sref("#{2C795808-7290-4675-056A-D3825905F8E1}");
    do_set_private(result, uuid, std_checksum_MD5_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING_BEGIN("std.checksum.MD5::update", self, global, reader) {
        const auto href = do_open_private(::std::move(self), uuid);
        V_string data;

        reader.start_overload();
        reader.required(data);    // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_MD5_update, href, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING_BEGIN("std.checksum.MD5::finish", self, global, reader) {
        const auto href = do_open_private(::std::move(self), uuid);

        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_MD5_finish, href);
      }
      ASTERIA_BINDING_END);
  }

class SHA1_Hasher
  final
  : public Abstract_Opaque
  {
  private:
    static constexpr
    array<uint32_t, 5>
    init()
      noexcept
      { return { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };  }

  private:
    array<uint32_t, 5> m_regs = init();
    uint64_t m_size = 0;
    array<uint8_t, 64> m_chunk;

  private:
    void
    do_consume_chunk(const uint8_t* p)
      noexcept
      {
        // https://en.wikipedia.org/wiki/SHA-1
        auto r = this->m_regs;
        uint32_t f, k;
        array<uint32_t, 80> w;

#define XHASH(i, sp, a, b, c, d, e)  \
        sp(b, c, d);  \
        e += do_rotl(a, 5) + f + k + w[i];  \
        b = do_rotl(b, 30);

#define SP_0(b, c, d)  \
        f = d ^ (b & (c ^ d));  \
        k = 0x5A827999;

#define SP_1(b, c, d)  \
        f = b ^ c ^ d;  \
        k = 0x6ED9EBA1;

#define SP_2(b, c, d)  \
        f = (b & (c | d)) | (c & d);  \
        k = 0x8F1BBCDC;

#define SP_3(b, c, d)  \
        f = b ^ c ^ d;  \
        k = 0xCA62C1D6;

        // Fill `w`.
        for(k = 0;  k < 16;  ++k)
          w[k] = do_load_be(p + k * 4);

        for(k = 16;  k < 32;  ++k)
          w[k] = do_rotl(w[k - 3] ^ w[k - 8] ^ w[k - 14] ^ w[k - 16], 1);

        for(k = 32;  k < 80;  ++k)
          w[k] = do_rotl(w[k - 6] ^ w[k - 16] ^ w[k - 28] ^ w[k - 32], 2);

        // 0 * 20
        XHASH( 0, SP_0, r[0], r[1], r[2], r[3], r[4]);
        XHASH( 1, SP_0, r[4], r[0], r[1], r[2], r[3]);
        XHASH( 2, SP_0, r[3], r[4], r[0], r[1], r[2]);
        XHASH( 3, SP_0, r[2], r[3], r[4], r[0], r[1]);
        XHASH( 4, SP_0, r[1], r[2], r[3], r[4], r[0]);
        XHASH( 5, SP_0, r[0], r[1], r[2], r[3], r[4]);
        XHASH( 6, SP_0, r[4], r[0], r[1], r[2], r[3]);
        XHASH( 7, SP_0, r[3], r[4], r[0], r[1], r[2]);
        XHASH( 8, SP_0, r[2], r[3], r[4], r[0], r[1]);
        XHASH( 9, SP_0, r[1], r[2], r[3], r[4], r[0]);
        XHASH(10, SP_0, r[0], r[1], r[2], r[3], r[4]);
        XHASH(11, SP_0, r[4], r[0], r[1], r[2], r[3]);
        XHASH(12, SP_0, r[3], r[4], r[0], r[1], r[2]);
        XHASH(13, SP_0, r[2], r[3], r[4], r[0], r[1]);
        XHASH(14, SP_0, r[1], r[2], r[3], r[4], r[0]);
        XHASH(15, SP_0, r[0], r[1], r[2], r[3], r[4]);
        XHASH(16, SP_0, r[4], r[0], r[1], r[2], r[3]);
        XHASH(17, SP_0, r[3], r[4], r[0], r[1], r[2]);
        XHASH(18, SP_0, r[2], r[3], r[4], r[0], r[1]);
        XHASH(19, SP_0, r[1], r[2], r[3], r[4], r[0]);

        // 1 * 20
        XHASH(20, SP_1, r[0], r[1], r[2], r[3], r[4]);
        XHASH(21, SP_1, r[4], r[0], r[1], r[2], r[3]);
        XHASH(22, SP_1, r[3], r[4], r[0], r[1], r[2]);
        XHASH(23, SP_1, r[2], r[3], r[4], r[0], r[1]);
        XHASH(24, SP_1, r[1], r[2], r[3], r[4], r[0]);
        XHASH(25, SP_1, r[0], r[1], r[2], r[3], r[4]);
        XHASH(26, SP_1, r[4], r[0], r[1], r[2], r[3]);
        XHASH(27, SP_1, r[3], r[4], r[0], r[1], r[2]);
        XHASH(28, SP_1, r[2], r[3], r[4], r[0], r[1]);
        XHASH(29, SP_1, r[1], r[2], r[3], r[4], r[0]);
        XHASH(30, SP_1, r[0], r[1], r[2], r[3], r[4]);
        XHASH(31, SP_1, r[4], r[0], r[1], r[2], r[3]);
        XHASH(32, SP_1, r[3], r[4], r[0], r[1], r[2]);
        XHASH(33, SP_1, r[2], r[3], r[4], r[0], r[1]);
        XHASH(34, SP_1, r[1], r[2], r[3], r[4], r[0]);
        XHASH(35, SP_1, r[0], r[1], r[2], r[3], r[4]);
        XHASH(36, SP_1, r[4], r[0], r[1], r[2], r[3]);
        XHASH(37, SP_1, r[3], r[4], r[0], r[1], r[2]);
        XHASH(38, SP_1, r[2], r[3], r[4], r[0], r[1]);
        XHASH(39, SP_1, r[1], r[2], r[3], r[4], r[0]);

        // 2 * 20
        XHASH(40, SP_2, r[0], r[1], r[2], r[3], r[4]);
        XHASH(41, SP_2, r[4], r[0], r[1], r[2], r[3]);
        XHASH(42, SP_2, r[3], r[4], r[0], r[1], r[2]);
        XHASH(43, SP_2, r[2], r[3], r[4], r[0], r[1]);
        XHASH(44, SP_2, r[1], r[2], r[3], r[4], r[0]);
        XHASH(45, SP_2, r[0], r[1], r[2], r[3], r[4]);
        XHASH(46, SP_2, r[4], r[0], r[1], r[2], r[3]);
        XHASH(47, SP_2, r[3], r[4], r[0], r[1], r[2]);
        XHASH(48, SP_2, r[2], r[3], r[4], r[0], r[1]);
        XHASH(49, SP_2, r[1], r[2], r[3], r[4], r[0]);
        XHASH(50, SP_2, r[0], r[1], r[2], r[3], r[4]);
        XHASH(51, SP_2, r[4], r[0], r[1], r[2], r[3]);
        XHASH(52, SP_2, r[3], r[4], r[0], r[1], r[2]);
        XHASH(53, SP_2, r[2], r[3], r[4], r[0], r[1]);
        XHASH(54, SP_2, r[1], r[2], r[3], r[4], r[0]);
        XHASH(55, SP_2, r[0], r[1], r[2], r[3], r[4]);
        XHASH(56, SP_2, r[4], r[0], r[1], r[2], r[3]);
        XHASH(57, SP_2, r[3], r[4], r[0], r[1], r[2]);
        XHASH(58, SP_2, r[2], r[3], r[4], r[0], r[1]);
        XHASH(59, SP_2, r[1], r[2], r[3], r[4], r[0]);

        // 3 * 20
        XHASH(60, SP_3, r[0], r[1], r[2], r[3], r[4]);
        XHASH(61, SP_3, r[4], r[0], r[1], r[2], r[3]);
        XHASH(62, SP_3, r[3], r[4], r[0], r[1], r[2]);
        XHASH(63, SP_3, r[2], r[3], r[4], r[0], r[1]);
        XHASH(64, SP_3, r[1], r[2], r[3], r[4], r[0]);
        XHASH(65, SP_3, r[0], r[1], r[2], r[3], r[4]);
        XHASH(66, SP_3, r[4], r[0], r[1], r[2], r[3]);
        XHASH(67, SP_3, r[3], r[4], r[0], r[1], r[2]);
        XHASH(68, SP_3, r[2], r[3], r[4], r[0], r[1]);
        XHASH(69, SP_3, r[1], r[2], r[3], r[4], r[0]);
        XHASH(70, SP_3, r[0], r[1], r[2], r[3], r[4]);
        XHASH(71, SP_3, r[4], r[0], r[1], r[2], r[3]);
        XHASH(72, SP_3, r[3], r[4], r[0], r[1], r[2]);
        XHASH(73, SP_3, r[2], r[3], r[4], r[0], r[1]);
        XHASH(74, SP_3, r[1], r[2], r[3], r[4], r[0]);
        XHASH(75, SP_3, r[0], r[1], r[2], r[3], r[4]);
        XHASH(76, SP_3, r[4], r[0], r[1], r[2], r[3]);
        XHASH(77, SP_3, r[3], r[4], r[0], r[1], r[2]);
        XHASH(78, SP_3, r[2], r[3], r[4], r[0], r[1]);
        XHASH(79, SP_3, r[1], r[2], r[3], r[4], r[0]);

#undef XHASH
#undef SP_0
#undef SP_1
#undef SP_2
#undef SP_3

        do_accumulate_words(this->m_regs, r);
      }

  public:
    tinyfmt&
    describe(tinyfmt& fmt)
      const override
      { return fmt << "instance of `std.checksum.SHA1` at `" << this << "`";  }

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
      const override
      { return callback;  }

    SHA1_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& output)
      const override
      { return noadl::clone_opaque(output, *this);  }

    void
    update(const void* data, size_t size)
      noexcept
      {
        auto bp = static_cast<const uint8_t*>(data);
        auto ep = bp + size;
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

    V_string
    finish()
      noexcept
      {
        auto bc = this->m_chunk.mut_begin() + this->m_size % 64;
        auto ec = this->m_chunk.mut_end();
        ptrdiff_t n;

        // Append a `0x80` byte followed by zeroes.
        *(bc++) = 0x80;
        n = ec - bc;
        if(n < 8) {
          ::std::fill_n(bc, n, 0);
          this->do_consume_chunk(this->m_chunk.data());
          bc = this->m_chunk.mut_begin();
        }
        n = ec - bc - 8;
        if(n > 0) {
          ::std::fill_n(bc, n, 0);
          bc += n;
        }
        ROCKET_ASSERT(ec - bc == 8);

        // Write the number of bits in big-endian order.
        uint64_t bits = htobe64(this->m_size * 8);
        ::std::memcpy(bc, &bits, 8);
        this->do_consume_chunk(this->m_chunk.data());

        // Get the checksum.
        cow_string ck = do_print_words_be(this->m_regs);

        // Reset internal states.
        this->m_regs = init();
        this->m_size = 0;
        return ck;
      }
  };

void
do_construct_SHA1(V_object& result)
  {
    static constexpr auto uuid = sref("#{2C795747-7003-4674-056A-91122125D892}");
    do_set_private(result, uuid, std_checksum_SHA1_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING_BEGIN("std.checksum.SHA1::update", self, global, reader) {
        const auto href = do_open_private(::std::move(self), uuid);
        V_string data;

        reader.start_overload();
        reader.required(data);    // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_SHA1_update, href, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING_BEGIN("std.checksum.SHA1::finish", self, global, reader) {
        const auto href = do_open_private(::std::move(self), uuid);

        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_SHA1_finish, href);
      }
      ASTERIA_BINDING_END);
  }

class SHA256_Hasher
  final
  : public Abstract_Opaque
  {
  private:
    static constexpr
    array<uint32_t, 8>
    init()
      noexcept
      { return { 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
                 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19 };  }

  private:
    array<uint32_t, 8> m_regs = init();
    uint64_t m_size = 0;
    array<uint8_t, 64> m_chunk;

  private:
    void
    do_consume_chunk(const uint8_t* p)
      noexcept
      {
        // https://en.wikipedia.org/wiki/SHA-2
        auto r = this->m_regs;
        uint32_t s0, maj, t2, s1, ch, t1;
        array<uint32_t, 64> w;

#define XHASH(i, a, b, c, d, e, f, g, h, k)  \
        s0 = do_rotl(a, 10) ^ do_rotl(a, 19) ^ do_rotl(a, 30);  \
        maj = (a & b) | (c & (a ^ b));  \
        t2 = s0 + maj;  \
        s1 = do_rotl(e,  7) ^ do_rotl(e, 21) ^ do_rotl(e, 26);  \
        ch = g ^ (e & (f ^ g));  \
        t1 = h + s1 + ch + k + w[i];  \
        d += t1;  \
        h = t1 + t2;

        // Fill `w`.
        for(ch =  0;  ch < 16;  ++ch)
          w[ch] = do_load_be(p + ch * 4);

        for(ch = 16;  ch < 64;  ++ch) {
          t1 = w[ch - 15];
          s0 = do_rotl(t1, 14) ^ do_rotl(t1, 25) ^ (t1 >> 3);
          t2 = w[ch - 2];
          s1 = do_rotl(t2, 13) ^ do_rotl(t2, 15) ^ (t2 >> 10);
          w[ch] = w[ch - 16] + w[ch - 7] + s0 + s1;
        }

        // 0 * 16
        XHASH( 0, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0x428A2F98);
        XHASH( 1, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0x71374491);
        XHASH( 2, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0xB5C0FBCF);
        XHASH( 3, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0xE9B5DBA5);
        XHASH( 4, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0x3956C25B);
        XHASH( 5, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0x59F111F1);
        XHASH( 6, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0x923F82A4);
        XHASH( 7, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0xAB1C5ED5);
        XHASH( 8, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0xD807AA98);
        XHASH( 9, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0x12835B01);
        XHASH(10, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0x243185BE);
        XHASH(11, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0x550C7DC3);
        XHASH(12, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0x72BE5D74);
        XHASH(13, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0x80DEB1FE);
        XHASH(14, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0x9BDC06A7);
        XHASH(15, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0xC19BF174);

        // 1 * 16
        XHASH(16, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0xE49B69C1);
        XHASH(17, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0xEFBE4786);
        XHASH(18, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0x0FC19DC6);
        XHASH(19, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0x240CA1CC);
        XHASH(20, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0x2DE92C6F);
        XHASH(21, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0x4A7484AA);
        XHASH(22, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0x5CB0A9DC);
        XHASH(23, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0x76F988DA);
        XHASH(24, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0x983E5152);
        XHASH(25, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0xA831C66D);
        XHASH(26, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0xB00327C8);
        XHASH(27, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0xBF597FC7);
        XHASH(28, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0xC6E00BF3);
        XHASH(29, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0xD5A79147);
        XHASH(30, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0x06CA6351);
        XHASH(31, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0x14292967);

        // 2 * 16
        XHASH(32, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0x27B70A85);
        XHASH(33, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0x2E1B2138);
        XHASH(34, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0x4D2C6DFC);
        XHASH(35, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0x53380D13);
        XHASH(36, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0x650A7354);
        XHASH(37, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0x766A0ABB);
        XHASH(38, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0x81C2C92E);
        XHASH(39, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0x92722C85);
        XHASH(40, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0xA2BFE8A1);
        XHASH(41, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0xA81A664B);
        XHASH(42, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0xC24B8B70);
        XHASH(43, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0xC76C51A3);
        XHASH(44, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0xD192E819);
        XHASH(45, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0xD6990624);
        XHASH(46, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0xF40E3585);
        XHASH(47, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0x106AA070);

        // 3 * 16
        XHASH(48, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0x19A4C116);
        XHASH(49, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0x1E376C08);
        XHASH(50, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0x2748774C);
        XHASH(51, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0x34B0BCB5);
        XHASH(52, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0x391C0CB3);
        XHASH(53, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0x4ED8AA4A);
        XHASH(54, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0x5B9CCA4F);
        XHASH(55, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0x682E6FF3);
        XHASH(56, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], 0x748F82EE);
        XHASH(57, r[7], r[0], r[1], r[2], r[3], r[4], r[5], r[6], 0x78A5636F);
        XHASH(58, r[6], r[7], r[0], r[1], r[2], r[3], r[4], r[5], 0x84C87814);
        XHASH(59, r[5], r[6], r[7], r[0], r[1], r[2], r[3], r[4], 0x8CC70208);
        XHASH(60, r[4], r[5], r[6], r[7], r[0], r[1], r[2], r[3], 0x90BEFFFA);
        XHASH(61, r[3], r[4], r[5], r[6], r[7], r[0], r[1], r[2], 0xA4506CEB);
        XHASH(62, r[2], r[3], r[4], r[5], r[6], r[7], r[0], r[1], 0xBEF9A3F7);
        XHASH(63, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[0], 0xC67178F2);

#undef XHASH

        do_accumulate_words(this->m_regs, r);
      }

  public:
    tinyfmt&
    describe(tinyfmt& fmt)
      const override
      { return fmt << "instance of `std.checksum.SHA256` at `" << this << "`";  }

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
      const override
      { return callback;  }

    SHA256_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& output)
      const override
      { return noadl::clone_opaque(output, *this);  }

    void
    update(const void* data, size_t size)
      noexcept
      {
        auto bp = static_cast<const uint8_t*>(data);
        auto ep = bp + size;
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

    V_string
    finish()
      noexcept
      {
        auto bc = this->m_chunk.mut_begin() + this->m_size % 64;
        auto ec = this->m_chunk.mut_end();
        ptrdiff_t n;

        // Append a `0x80` byte followed by zeroes.
        *(bc++) = 0x80;
        n = ec - bc;
        if(n < 8) {
          ::std::fill_n(bc, n, 0);
          this->do_consume_chunk(this->m_chunk.data());
          bc = this->m_chunk.mut_begin();
        }
        n = ec - bc - 8;
        if(n > 0) {
          ::std::fill_n(bc, n, 0);
          bc += n;
        }
        ROCKET_ASSERT(ec - bc == 8);

        // Write the number of bits in big-endian order.
        uint64_t bits = htobe64(this->m_size * 8);
        ::std::memcpy(bc, &bits, 8);
        this->do_consume_chunk(this->m_chunk.data());

        // Get the checksum.
        cow_string ck = do_print_words_be(this->m_regs);

        // Reset internal states.
        this->m_regs = init();
        this->m_size = 0;
        return ck;
      }
  };

void
do_construct_SHA256(V_object& result)
  {
    static constexpr auto uuid = sref("#{2C795749-6F4F-4674-056A-B0E8CF4BBD7D}");
    do_set_private(result, uuid, std_checksum_SHA256_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING_BEGIN("std.checksum.SHA256::update", self, global, reader) {
        const auto href = do_open_private(::std::move(self), uuid);
        V_string data;

        reader.start_overload();
        reader.required(data);    // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_SHA256_update, href, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING_BEGIN("std.checksum.SHA256::finish", self, global, reader) {
        const auto href = do_open_private(::std::move(self), uuid);

        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_SHA256_finish, href);
      }
      ASTERIA_BINDING_END);
  }

template<typename HasherT>
inline
rcptr<HasherT>
do_cast_hasher(V_opaque& h)
  {
    auto hptr = h.open_opt<HasherT>();
    if(!hptr)
      ASTERIA_THROW("Invalid hasher type (invalid dynamic_cast to `$1` from `$2`)",
                    typeid(HasherT).name(), h.type().name());
    return hptr;
  }

template<typename HasherT>
decltype(auto)
do_hash_bytes(const V_string& data)
  {
    HasherT h;
    h.update(data.data(), data.size());
    return h.finish();
  }

template<typename HasherT>
decltype(auto)
do_hash_file(const V_string& path)
  {
    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), O_RDONLY), ::close);
    if(!fd)
      ASTERIA_THROW("Could not open file '$2'\n"
                    "[`open()` failed: $1]",
                    format_errno(errno), path);

    // Get the file mode and preferred I/O block size.
    struct ::stat stb;
    if(::fstat(fd, &stb) != 0)
      ASTERIA_THROW("Could not get information about source file '$2'\n"
                    "[`fstat()` failed: $1]",
                    format_errno(errno), path);

    // Allocate the I/O buffer.
    size_t nbuf = static_cast<size_t>(stb.st_blksize | 0x1000);
    auto pbuf = ::rocket::make_unique_handle(new char[nbuf], [](char* p) { delete[] p;  });
    HasherT h;

    // Read bytes from the file and hash them.
    ::ssize_t nread;
    while((nread = ::read(fd, pbuf, nbuf)) > 0)
      h.update(pbuf, static_cast<size_t>(nread));

    if(nread < 0)
      ASTERIA_THROW("Error reading file '$2'\n"
                    "[`read()` failed: $1]",
                    format_errno(errno), path);

    // Finalize the hasher.
    return h.finish();
  }

}  // namespace

V_opaque
std_checksum_CRC32_private()
  {
    return ::rocket::make_refcnt<CRC32_Hasher>();
  }

void
std_checksum_CRC32_update(V_opaque& h, V_string data)
  {
    return do_cast_hasher<CRC32_Hasher>(h)->update(data.data(), data.size());
  }

V_integer
std_checksum_CRC32_finish(V_opaque& h)
  {
    return do_cast_hasher<CRC32_Hasher>(h)->finish();
  }

V_object
std_checksum_CRC32()
  {
    V_object result;
    do_construct_CRC32(result);
    return result;
  }

V_integer
std_checksum_crc32(V_string data)
  {
    return do_hash_bytes<CRC32_Hasher>(data);
  }

V_integer
std_checksum_crc32_file(V_string path)
  {
    return do_hash_file<CRC32_Hasher>(path);
  }

V_opaque
std_checksum_FNV1a32_private()
  {
    return ::rocket::make_refcnt<FNV1a32_Hasher>();
  }

void
std_checksum_FNV1a32_update(V_opaque& h, V_string data)
  {
    return do_cast_hasher<FNV1a32_Hasher>(h)->update(data.data(), data.size());
  }

V_integer
std_checksum_FNV1a32_finish(V_opaque& h)
  {
    return do_cast_hasher<FNV1a32_Hasher>(h)->finish();
  }

V_object
std_checksum_FNV1a32()
  {
    V_object result;
    do_construct_FNV1a32(result);
    return result;
  }

V_integer
std_checksum_fnv1a32(V_string data)
  {
    return do_hash_bytes<FNV1a32_Hasher>(data);
  }

V_integer
std_checksum_fnv1a32_file(V_string path)
  {
    return do_hash_file<FNV1a32_Hasher>(path);
  }

V_opaque
std_checksum_MD5_private()
  {
    return ::rocket::make_refcnt<MD5_Hasher>();
  }

void
std_checksum_MD5_update(V_opaque& h, V_string data)
  {
    return do_cast_hasher<MD5_Hasher>(h)->update(data.data(), data.size());
  }

V_string
std_checksum_MD5_finish(V_opaque& h)
  {
    return do_cast_hasher<MD5_Hasher>(h)->finish();
  }

V_object
std_checksum_MD5()
  {
    V_object result;
    do_construct_MD5(result);
    return result;
  }

V_string
std_checksum_md5(V_string data)
  {
    return do_hash_bytes<MD5_Hasher>(data);
  }

V_string
std_checksum_md5_file(V_string path)
  {
    return do_hash_file<MD5_Hasher>(path);
  }

V_opaque
std_checksum_SHA1_private()
  {
    return ::rocket::make_refcnt<SHA1_Hasher>();
  }

void
std_checksum_SHA1_update(V_opaque& h, V_string data)
  {
    return do_cast_hasher<SHA1_Hasher>(h)->update(data.data(), data.size());
  }

V_string
std_checksum_SHA1_finish(V_opaque& h)
  {
    return do_cast_hasher<SHA1_Hasher>(h)->finish();
  }

V_object
std_checksum_SHA1()
  {
    V_object result;
    do_construct_SHA1(result);
    return result;
  }

V_string
std_checksum_sha1(V_string data)
  {
    return do_hash_bytes<SHA1_Hasher>(data);
  }

V_string
std_checksum_sha1_file(V_string path)
  {
    return do_hash_file<SHA1_Hasher>(path);
  }

V_opaque
std_checksum_SHA256_private()
  {
    return ::rocket::make_refcnt<SHA256_Hasher>();
  }

void
std_checksum_SHA256_update(V_opaque& h, V_string data)
  {
    return do_cast_hasher<SHA256_Hasher>(h)->update(data.data(), data.size());
  }

V_string
std_checksum_SHA256_finish(V_opaque& h)
  {
    return do_cast_hasher<SHA256_Hasher>(h)->finish();
  }

V_object
std_checksum_SHA256()
  {
    V_object result;
    do_construct_SHA256(result);
    return result;
  }

V_string
std_checksum_sha256(V_string data)
  {
    return do_hash_bytes<SHA256_Hasher>(data);
  }

V_string
std_checksum_sha256_file(V_string path)
  {
    return do_hash_file<SHA256_Hasher>(path);
  }

void
create_bindings_checksum(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("CRC32"),
      ASTERIA_BINDING_BEGIN("std.checksum.CRC32", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_CRC32);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("crc32"),
      ASTERIA_BINDING_BEGIN("std.checksum.crc32", self, global, reader) {
        V_string data;

        reader.start_overload();
        reader.required(data);    // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_crc32, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("crc32_file"),
      ASTERIA_BINDING_BEGIN("std.checksum.crc32_file", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);    // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_crc32_file, path);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("FNV1a32"),
      ASTERIA_BINDING_BEGIN("std.checksum.FNV1a32", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_FNV1a32);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("fnv1a32"),
      ASTERIA_BINDING_BEGIN("std.checksum.fnv1a32", self, global, reader) {
        V_string data;

        reader.start_overload();
        reader.required(data);    // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_fnv1a32, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("fnv1a32_file"),
      ASTERIA_BINDING_BEGIN("std.checksum.fnv1a32_file", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);    // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_fnv1a32_file, path);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("MD5"),
      ASTERIA_BINDING_BEGIN("std.checksum.MD5", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_MD5);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("md5"),
      ASTERIA_BINDING_BEGIN("std.checksum.md5", self, global, reader) {
        V_string data;

        reader.start_overload();
        reader.required(data);    // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_md5, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("md5_file"),
      ASTERIA_BINDING_BEGIN("std.checksum.md5_file", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);    // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_md5_file, path);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("SHA1"),
      ASTERIA_BINDING_BEGIN("std.checksum.SHA1", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_SHA1);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("sha1"),
      ASTERIA_BINDING_BEGIN("std.checksum.sha1", self, global, reader) {
        V_string data;

        reader.start_overload();
        reader.required(data);    // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_sha1, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("sha1_file"),
      ASTERIA_BINDING_BEGIN("std.checksum.sha1_file", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);    // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_sha1_file, path);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("SHA256"),
      ASTERIA_BINDING_BEGIN("std.checksum.SHA256", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_SHA256);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("sha256"),
      ASTERIA_BINDING_BEGIN("std.checksum.sha256", self, global, reader) {
        V_string data;

        reader.start_overload();
        reader.required(data);    // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_sha256, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("sha256_file"),
      ASTERIA_BINDING_BEGIN("std.checksum.sha256_file", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);    // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_checksum_sha256_file, path);
      }
      ASTERIA_BINDING_END);
  }

}  // namespace asteria
