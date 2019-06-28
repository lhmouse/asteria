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
    namespace CRC32 {

    template<std::uint32_t valueT, std::uint32_t divisorT, int roundT> struct Generator : Generator<(valueT >> 1) ^ (-(valueT & 1) & divisorT), divisorT, roundT + 1>
      {
      };
    template<std::uint32_t valueT, std::uint32_t divisorT> struct Generator<valueT, divisorT, 8> : std::integral_constant<std::uint32_t, valueT>
      {
      };
    template<std::uint32_t divisorT, std::size_t... indicesT> constexpr std::array<std::uint32_t, sizeof...(indicesT)> do_generate_table_impl(const std::index_sequence<indicesT...>&) noexcept
      {
        return {{ Generator<std::uint8_t(indicesT), divisorT, 0>::value... }};
      }
    template<std::uint32_t divisorT> constexpr std::array<std::uint32_t, 256> do_generate_table() noexcept
      {
        return do_generate_table_impl<divisorT>(std::make_index_sequence<256>());
      }
    constexpr auto s_iso3309_table = do_generate_table<0xEDB88320>();

    constexpr std::uint32_t s_init = UINT32_MAX;

    class Hasher : public Abstract_Opaque
      {
      private:
        std::uint32_t m_reg;

      public:
        Hasher() noexcept
          : m_reg(s_init)
          {
          }

      public:
        void describe(std::ostream& os) const override
          {
            os << "CRC-32 hasher";
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
            // There is nothing to do.
          }

        void write(const G_string& data) noexcept
          {
            const auto p = reinterpret_cast<const std::uint8_t*>(data.data());
            const auto n = data.size();
            auto r = this->m_reg;
            // Hash bytes one by one.
            for(std::size_t i = 0; i != n; ++i) {
              std::uint8_t b = p[i];
              b ^= static_cast<std::uint8_t>(r);
              r >>= 8;
              r ^= s_iso3309_table[b];
            }
            this->m_reg = r;
          }
        G_integer finish() noexcept
          {
            // Get the checksum.
            auto ck = ~this->m_reg;
            // Reset internal states.
            this->m_reg = s_init;
            return ck;
          }
      };

    }
    }

G_object std_checksum_crc32_new()
  {
    G_object r;
    r.insert_or_assign(rocket::sref("!h"),  // details
      G_opaque(
        rocket::make_refcnt<CRC32::Hasher>()
      ));
    r.insert_or_assign(rocket::sref("write"),
      G_function(
        make_simple_binding(
          // Description
          rocket::sref("<std.checksum.crc32_new()>.write"),
          // Opaque parameter
          G_null(),
          // Definition
          [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& self, Cow_Vector<Reference>&& args) -> Reference
            {
              Argument_Reader reader(rocket::sref("<std.checksum.crc32_new()>.write"), args);
              // Get the hasher.
              Reference_Modifier::S_object_key xmod = { rocket::sref("!h") };
              self.zoom_in(rocket::move(xmod));
              auto& h = dynamic_cast<CRC32::Hasher&>(self.open().mut_opaque().mut());
              // Parse arguments.
              G_string data;
              if(reader.start().g(data).finish()) {
                h.write(data);
                return Reference_Root::S_null();
              }
              reader.throw_no_matching_function_call();
            }
        )
      ));
    r.insert_or_assign(rocket::sref("finish"),
      G_function(
        make_simple_binding(
          // Description
          rocket::sref("<std.checksum.crc32_new()>.finish"),
          // Opaque parameter
          G_null(),
          // Definition
          [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& self, Cow_Vector<Reference>&& args) -> Reference
            {
              Argument_Reader reader(rocket::sref("<std.checksum.crc32_new()>.finish"), args);
              // Get the hasher.
              Reference_Modifier::S_object_key xmod = { rocket::sref("!h") };
              self.zoom_in(rocket::move(xmod));
              auto& h = dynamic_cast<CRC32::Hasher&>(self.open().mut_opaque().mut());
              // Parse arguments.
              if(reader.start().finish()) {
                Reference_Root::S_temporary xref = { h.finish() };
                return rocket::move(xref);
              }
              reader.throw_no_matching_function_call();
            }
        )
      ));
    return r;
  }

G_integer std_checksum_crc32(const G_string& data)
  {
    CRC32::Hasher h;
    h.write(data);
    return h.finish();
  }

    namespace {

    template<std::uint8_t valueT> struct Hexdigit : std::integral_constant<char, char((valueT < 10) ? ('0' + valueT) : ('A' + valueT - 10))>
      {
      };
    template<std::uint8_t valueT> constexpr std::array<char, 2> do_generate_hex_digits_for_byte() noexcept
      {
        return {{ Hexdigit<std::uint8_t(valueT / 16)>::value, Hexdigit<std::uint8_t(valueT % 16)>::value }};
      };
    template<std::size_t... indicesT> constexpr std::array<std::array<char, 2>, 256> do_generate_hexdigits_impl(const std::index_sequence<indicesT...>&) noexcept
      {
        return {{ do_generate_hex_digits_for_byte<std::uint8_t(indicesT)>()... }};
      }
    constexpr auto s_hexdigits = do_generate_hexdigits_impl(std::make_index_sequence<256>());

    template<bool bigendT, typename WordT> G_string& do_pdigits_impl(G_string& str, const WordT& ref)
      {
        static_assert(std::is_unsigned<WordT>::value, "??");
        std::array<std::uint8_t, sizeof(WordT)> stor_le;
        std::uint64_t word = static_cast<std::uint64_t>(ref);
        // Write the word in little-endian order.
        for(auto& byte : stor_le) {
          byte = word & 0xFF;
          word >>= 8;
        }
        // Append hexadecimal digits.
        if(bigendT) {
          std::for_each(stor_le.rbegin(), stor_le.rend(), [&](std::uint8_t b) { str.append(s_hexdigits[b].data(), 2);  });
        } else {
          std::for_each(stor_le.begin(), stor_le.end(), [&](std::uint8_t b) { str.append(s_hexdigits[b].data(), 2);  });
        }
        return str;
      }
    template<typename WordT> G_string& do_pdigits_be(G_string& str, const WordT& ref)
      {
        return do_pdigits_impl<1, WordT>(str, ref);
      }
    template<typename WordT> G_string& do_pdigits_le(G_string& str, const WordT& ref)
      {
        return do_pdigits_impl<0, WordT>(str, ref);
      }

    template<bool bigendT, typename WordT> WordT& do_load_impl(WordT& ref, const std::uint8_t* ptr)
      {
        static_assert(std::is_unsigned<WordT>::value, "??");
        std::array<std::uint8_t, sizeof(WordT)> stor_be;
        std::uint64_t word = 0;
        // Re-arrange bytes.
        if(bigendT) {
          std::copy_n(ptr, stor_be.size(), stor_be.begin());
        } else {
          std::copy_n(ptr, stor_be.size(), stor_be.rbegin());
        }
        // Assemble the word.
        for(const auto& byte : stor_be) {
          word <<= 8;
          word |= byte;
        }
        return ref = static_cast<WordT>(word);
      }
    template<typename WordT> WordT& do_load_be(WordT& ref, const std::uint8_t* ptr)
      {
        return do_load_impl<1, WordT>(ref, ptr);
      }
    template<typename WordT> WordT& do_load_le(WordT& ref, const std::uint8_t* ptr)
      {
        return do_load_impl<0, WordT>(ref, ptr);
      }

    template<typename WordT> constexpr WordT do_rotl(const WordT& ref, std::size_t bits)
      {
        static_assert(std::is_unsigned<WordT>::value, "??");
        // This is correct even when `bits` is zero.
        constexpr auto width = sizeof(WordT) * 8;
        auto sum = (ref << (+bits) % width) | (ref >> (-bits) % width);
        return static_cast<WordT>(sum);
      }

    template<typename WordT, std::size_t sizeT> inline std::array<WordT, sizeT>& do_padd(std::array<WordT, sizeT>& lhs, const std::array<WordT, sizeT>& rhs)
      {
        static_assert(std::is_unsigned<WordT>::value, "??");
        // Accumulate each element in parallel.
        for(std::size_t i = 0; i != sizeT; ++i) {
          lhs[i] += rhs[i];
        }
        return lhs;
      }

    }

    namespace {
    namespace MD5 {

    constexpr std::array<std::uint32_t, 4> s_init = {{
      0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476
    }};

    class Hasher : public Abstract_Opaque
      {
      private:
        std::array<std::uint32_t, 4> m_regs;
        std::uint64_t m_size;
        std::array<std::uint8_t, 64> m_chunk;

      public:
        Hasher() noexcept
          : m_regs(s_init),
            m_size(0)
          {
          }

      private:
        void do_consume_chunk(const std::uint8_t* p) noexcept
          {
            std::uint32_t f, g, w;
            // https://en.wikipedia.org/wiki/MD5
            auto update = [&](std::uint32_t i, auto&& specx,
                              std::uint32_t& a, std::uint32_t& b, std::uint32_t& c, std::uint32_t& d,
                              std::uint32_t k, std::uint8_t r)
              {
                specx(i, b, c, d);
                do_load_le(w, p + g * 4);
                w = a + f + k + w;
                a = b + do_rotl(w, r);
              };
            auto spec0 = [&](std::uint32_t i, std::uint32_t& b, std::uint32_t& c, std::uint32_t& d)
              {
                f = d ^ (b & (c ^ d));
                g = i;
              };
            auto spec1 = [&](std::uint32_t i, std::uint32_t& b, std::uint32_t& c, std::uint32_t& d)
              {
                f = c ^ (d & (b ^ c));
                g = (5 * i + 1) % 16;
              };
            auto spec2 = [&](std::uint32_t i, std::uint32_t& b, std::uint32_t& c, std::uint32_t& d)
              {
                f = b ^ c ^ d;
                g = (3 * i + 5) % 16;
              };
            auto spec3 = [&](std::uint32_t i, std::uint32_t& b, std::uint32_t& c, std::uint32_t& d)
              {
                f = c ^ (b | ~d);
                g = (7 * i) % 16;
              };
            // Unroll loops by hand.
            auto r = this->m_regs;
            //  0
            update(0x00, spec0, r[0], r[1], r[2], r[3], 0xD76AA478,  7);
            update(0x01, spec0, r[3], r[0], r[1], r[2], 0xE8C7B756, 12);
            update(0x02, spec0, r[2], r[3], r[0], r[1], 0x242070DB, 17);
            update(0x03, spec0, r[1], r[2], r[3], r[0], 0xC1BDCEEE, 22);
            update(0x04, spec0, r[0], r[1], r[2], r[3], 0xF57C0FAF,  7);
            update(0x05, spec0, r[3], r[0], r[1], r[2], 0x4787C62A, 12);
            update(0x06, spec0, r[2], r[3], r[0], r[1], 0xA8304613, 17);
            update(0x07, spec0, r[1], r[2], r[3], r[0], 0xFD469501, 22);
            update(0x08, spec0, r[0], r[1], r[2], r[3], 0x698098D8,  7);
            update(0x09, spec0, r[3], r[0], r[1], r[2], 0x8B44F7AF, 12);
            update(0x0A, spec0, r[2], r[3], r[0], r[1], 0xFFFF5BB1, 17);
            update(0x0B, spec0, r[1], r[2], r[3], r[0], 0x895CD7BE, 22);
            update(0x0C, spec0, r[0], r[1], r[2], r[3], 0x6B901122,  7);
            update(0x0D, spec0, r[3], r[0], r[1], r[2], 0xFD987193, 12);
            update(0x0E, spec0, r[2], r[3], r[0], r[1], 0xA679438E, 17);
            update(0x0F, spec0, r[1], r[2], r[3], r[0], 0x49B40821, 22);
            // 1
            update(0x10, spec1, r[0], r[1], r[2], r[3], 0xF61E2562,  5);
            update(0x11, spec1, r[3], r[0], r[1], r[2], 0xC040B340,  9);
            update(0x12, spec1, r[2], r[3], r[0], r[1], 0x265E5A51, 14);
            update(0x13, spec1, r[1], r[2], r[3], r[0], 0xE9B6C7AA, 20);
            update(0x14, spec1, r[0], r[1], r[2], r[3], 0xD62F105D,  5);
            update(0x15, spec1, r[3], r[0], r[1], r[2], 0x02441453,  9);
            update(0x16, spec1, r[2], r[3], r[0], r[1], 0xD8A1E681, 14);
            update(0x17, spec1, r[1], r[2], r[3], r[0], 0xE7D3FBC8, 20);
            update(0x18, spec1, r[0], r[1], r[2], r[3], 0x21E1CDE6,  5);
            update(0x19, spec1, r[3], r[0], r[1], r[2], 0xC33707D6,  9);
            update(0x1A, spec1, r[2], r[3], r[0], r[1], 0xF4D50D87, 14);
            update(0x1B, spec1, r[1], r[2], r[3], r[0], 0x455A14ED, 20);
            update(0x1C, spec1, r[0], r[1], r[2], r[3], 0xA9E3E905,  5);
            update(0x1D, spec1, r[3], r[0], r[1], r[2], 0xFCEFA3F8,  9);
            update(0x1E, spec1, r[2], r[3], r[0], r[1], 0x676F02D9, 14);
            update(0x1F, spec1, r[1], r[2], r[3], r[0], 0x8D2A4C8A, 20);
            // 2
            update(0x20, spec2, r[0], r[1], r[2], r[3], 0xFFFA3942,  4);
            update(0x21, spec2, r[3], r[0], r[1], r[2], 0x8771F681, 11);
            update(0x22, spec2, r[2], r[3], r[0], r[1], 0x6D9D6122, 16);
            update(0x23, spec2, r[1], r[2], r[3], r[0], 0xFDE5380C, 23);
            update(0x24, spec2, r[0], r[1], r[2], r[3], 0xA4BEEA44,  4);
            update(0x25, spec2, r[3], r[0], r[1], r[2], 0x4BDECFA9, 11);
            update(0x26, spec2, r[2], r[3], r[0], r[1], 0xF6BB4B60, 16);
            update(0x27, spec2, r[1], r[2], r[3], r[0], 0xBEBFBC70, 23);
            update(0x28, spec2, r[0], r[1], r[2], r[3], 0x289B7EC6,  4);
            update(0x29, spec2, r[3], r[0], r[1], r[2], 0xEAA127FA, 11);
            update(0x2A, spec2, r[2], r[3], r[0], r[1], 0xD4EF3085, 16);
            update(0x2B, spec2, r[1], r[2], r[3], r[0], 0x04881D05, 23);
            update(0x2C, spec2, r[0], r[1], r[2], r[3], 0xD9D4D039,  4);
            update(0x2D, spec2, r[3], r[0], r[1], r[2], 0xE6DB99E5, 11);
            update(0x2E, spec2, r[2], r[3], r[0], r[1], 0x1FA27CF8, 16);
            update(0x2F, spec2, r[1], r[2], r[3], r[0], 0xC4AC5665, 23);
            // 3
            update(0x30, spec3, r[0], r[1], r[2], r[3], 0xF4292244,  6);
            update(0x31, spec3, r[3], r[0], r[1], r[2], 0x432AFF97, 10);
            update(0x32, spec3, r[2], r[3], r[0], r[1], 0xAB9423A7, 15);
            update(0x33, spec3, r[1], r[2], r[3], r[0], 0xFC93A039, 21);
            update(0x34, spec3, r[0], r[1], r[2], r[3], 0x655B59C3,  6);
            update(0x35, spec3, r[3], r[0], r[1], r[2], 0x8F0CCC92, 10);
            update(0x36, spec3, r[2], r[3], r[0], r[1], 0xFFEFF47D, 15);
            update(0x37, spec3, r[1], r[2], r[3], r[0], 0x85845DD1, 21);
            update(0x38, spec3, r[0], r[1], r[2], r[3], 0x6FA87E4F,  6);
            update(0x39, spec3, r[3], r[0], r[1], r[2], 0xFE2CE6E0, 10);
            update(0x3A, spec3, r[2], r[3], r[0], r[1], 0xA3014314, 15);
            update(0x3B, spec3, r[1], r[2], r[3], r[0], 0x4E0811A1, 21);
            update(0x3C, spec3, r[0], r[1], r[2], r[3], 0xF7537E82,  6);
            update(0x3D, spec3, r[3], r[0], r[1], r[2], 0xBD3AF235, 10);
            update(0x3E, spec3, r[2], r[3], r[0], r[1], 0x2AD7D2BB, 15);
            update(0x3F, spec3, r[1], r[2], r[3], r[0], 0xEB86D391, 21);
            // Accumulate the result.
            do_padd(this->m_regs, r);
          }

      public:
        void describe(std::ostream& os) const override
          {
            os << "MD5 hasher";
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
            // There is nothing to do.
          }

        void write(const G_string& data) noexcept
          {
            auto bp = reinterpret_cast<const std::uint8_t*>(data.data());
            auto ep = bp + data.size();
            auto bc = this->m_chunk.begin() + static_cast<std::ptrdiff_t>(this->m_size % 64);
            auto ec = this->m_chunk.end();
            std::ptrdiff_t n;
            // If the last chunk was not empty, ...
            if(bc != this->m_chunk.begin()) {
              // ... append data to the last chunk, ...
              n = rocket::min(ep - bp, ec - bc);
              std::copy_n(bp, n, bc);
              this->m_size += n & 0xFF;
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
              bc = this->m_chunk.begin();
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
              std::copy_n(bp, n, bc);
              this->m_size += n & 0xFF;
              bp += n;
              bc += n;
            }
            ROCKET_ASSERT(bp == ep);
          }
        G_string finish() noexcept
          {
            // Finalize the hasher.
            auto bc = this->m_chunk.begin() + static_cast<std::ptrdiff_t>(this->m_size % 64);
            auto ec = this->m_chunk.end();
            std::ptrdiff_t n;
            // Append a `0x80` byte followed by zeroes.
            *(bc++) = 0x80;
            n = ec - bc;
            if(n < 8) {
              // Wrap.
              std::fill_n(bc, n, 0);
              this->do_consume_chunk(this->m_chunk.data());
              bc = this->m_chunk.begin();
            }
            n = ec - bc - 8;
            if(n > 0) {
              // Fill zeroes.
              std::fill_n(bc, n, 0);
              bc += n;
            }
            ROCKET_ASSERT(ec - bc == 8);
            // Write the number of bits in little-endian order.
            auto bits = this->m_size * 8;
            for(std::ptrdiff_t i = 0; i != 8; ++i) {
              bc[i] = bits & 0xFF;
              bits >>= 8;
            }
            this->do_consume_chunk(this->m_chunk.data());
            // Get the checksum.
            G_string ck;
            ck.reserve(32);
            rocket::for_each(this->m_regs, [&](std::uint32_t w) { do_pdigits_le(ck, w);  });
            // Reset internal states.
            this->m_regs = s_init;
            this->m_size = 0;
            return ck;
          }
      };

    }
    }

G_object std_checksum_md5_new()
  {
    G_object r;
    r.insert_or_assign(rocket::sref("!h"),  // details
      G_opaque(
        rocket::make_refcnt<MD5::Hasher>()
      ));
    r.insert_or_assign(rocket::sref("write"),
      G_function(
        make_simple_binding(
          // Description
          rocket::sref("<std.checksum.md5_new()>.write"),
          // Opaque parameter
          G_null(),
          // Definition
          [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& self, Cow_Vector<Reference>&& args) -> Reference
            {
              Argument_Reader reader(rocket::sref("<std.checksum.md5_new()>.write"), args);
              // Get the hasher.
              Reference_Modifier::S_object_key xmod = { rocket::sref("!h") };
              self.zoom_in(rocket::move(xmod));
              auto& h = dynamic_cast<MD5::Hasher&>(self.open().mut_opaque().mut());
              // Parse arguments.
              G_string data;
              if(reader.start().g(data).finish()) {
                h.write(data);
                return Reference_Root::S_null();
              }
              reader.throw_no_matching_function_call();
            }
        )
      ));
    r.insert_or_assign(rocket::sref("finish"),
      G_function(
        make_simple_binding(
          // Description
          rocket::sref("<std.checksum.md5_new()>.finish"),
          // Opaque parameter
          G_null(),
          // Definition
          [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& self, Cow_Vector<Reference>&& args) -> Reference
            {
              Argument_Reader reader(rocket::sref("<std.checksum.md5_new()>.finish"), args);
              // Get the hasher.
              Reference_Modifier::S_object_key xmod = { rocket::sref("!h") };
              self.zoom_in(rocket::move(xmod));
              auto& h = dynamic_cast<MD5::Hasher&>(self.open().mut_opaque().mut());
              // Parse arguments.
              if(reader.start().finish()) {
                Reference_Root::S_temporary xref = { h.finish() };
                return rocket::move(xref);
              }
              reader.throw_no_matching_function_call();
            }
        )
      ));
    return r;
  }

G_string std_checksum_md5(const G_string& data)
  {
    MD5::Hasher h;
    h.write(data);
    return h.finish();
  }

void create_bindings_checksum(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.checksum.crc32_new()`
    //===================================================================
    result.insert_or_assign(rocket::sref("crc32_new"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.checksum.crc32_new"), args);
            // Parse arguments.
            if(reader.start().finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_checksum_crc32_new() };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.checksum.crc32()`
    //===================================================================
    result.insert_or_assign(rocket::sref("crc32"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
            "        return h.finish();\n"
            "      };\n"
            "    ```\n"
            "\n"
            "    This function is expected to be both more efficient and easier\n"
            "    to use.\n"
            "\n"
            "  * Returns the CRC-32 checksum as an `integer`. The high-order 32\n"
            "    bits are always zeroes.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.checksum.crc32"), args);
            // Parse arguments.
            G_string data;
            if(reader.start().g(data).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_checksum_crc32(data) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.checksum.md5_new()`
    //===================================================================
    result.insert_or_assign(rocket::sref("md5_new"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.checksum.md5_new"), args);
            // Parse arguments.
            if(reader.start().finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_checksum_md5_new() };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.checksum.md5()`
    //===================================================================
    result.insert_or_assign(rocket::sref("md5"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
            "        return h.finish();\n"
            "      };\n"
            "    ```\n"
            "\n"
            "    This function is expected to be both more efficient and easier\n"
            "    to use.\n"
            "\n"
            "  * Returns the 2 checksum as a `string` of 32 hexadecimal digits\n"
            "    in uppercase.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.checksum.md5"), args);
            // Parse arguments.
            G_string data;
            if(reader.start().g(data).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_checksum_md5(data) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // End of `std.checksum`
    //===================================================================
  }

}  // namespace Asteria
