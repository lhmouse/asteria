// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "checksum.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/runtime_error.hpp"
#include "../runtime/global_context.hpp"
#include "../utils.hpp"
#include <sys/stat.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <zlib.h>

namespace asteria {
namespace {

class CRC32_Hasher final
  : public Abstract_Opaque
  {
  private:
    ::uLong m_reg;

  public:
    explicit
    CRC32_Hasher() noexcept
      { this->clear();  }

  public:
    tinyfmt&
    describe(tinyfmt& fmt) const override
      {
        return format(fmt, "instance of `std.checksum.CRC32` at `$1`", this);
      }

    void
    get_variables(Variable_HashMap&, Variable_HashMap&) const override
      { }

    CRC32_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& out) const override
      {
        auto ptr = new auto(*this);
        out.reset(ptr);
        return ptr;
      }

    void
    clear() noexcept
      {
        this->m_reg = 0;
      }

    void
    update(const void* data, size_t size) noexcept
      {
        auto bptr = static_cast<const ::Byte*>(data);
#if ZLIB_VERNUM < 0x1290
        // XXX: Why do we have to maintain this stupid piece of code for compatibility?
        const auto eptr = bptr + size;
        while(ptrdiff_t n = ::rocket::min(eptr - bptr, INT_MAX))
          this->m_reg = ::crc32(this->m_reg, (bptr += n) - n, static_cast<::uInt>(n));
#else
        // zlib >= 1.2.9
        this->m_reg = ::crc32_z(this->m_reg, bptr, size);
#endif
      }

    V_integer
    finish() noexcept
      {
        uint32_t val = static_cast<uint32_t>(this->m_reg);
        this->clear();
        return val;
      }
  };

void
do_construct_CRC32(V_object& result)
  {
    static constexpr auto s_uuid = sref("{2C78B9D8-A8F4-4CE9-36E7-12B9EE14AD3D}");
    result.insert_or_assign(s_uuid, std_checksum_CRC32_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING(
        "std.checksum.CRC32::update", "data",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (void) std_checksum_CRC32_update(hasher, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING(
        "std.checksum.CRC32::finish", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_CRC32_finish(hasher);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("clear"),
      ASTERIA_BINDING(
        "std.checksum.CRC32::clear", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (void) std_checksum_CRC32_clear(hasher);

        reader.throw_no_matching_function_call();
      });
  }

class FNV1a32_Hasher final
  : public Abstract_Opaque
  {
  private:
    uint32_t m_reg;

  public:
    explicit
    FNV1a32_Hasher() noexcept
      { this->clear();  }

  public:
    tinyfmt&
    describe(tinyfmt& fmt) const override
      {
        return format(fmt, "instance of `std.checksum.FNV1a32` at `$1`", this);
      }

    void
    get_variables(Variable_HashMap&, Variable_HashMap&) const override
      { }

    FNV1a32_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& out) const override
      {
        auto ptr = new auto(*this);
        out.reset(ptr);
        return ptr;
      }

    void
    clear() noexcept
      {
        this->m_reg = 2166136261;
      }

    void
    update(const void* data, size_t size) noexcept
      {
        auto bptr = static_cast<const uint8_t*>(data);
        auto eptr = bptr + size;
        while(bptr != eptr)
          this->m_reg = (this->m_reg ^ *(bptr++)) * 16777619;
      }

    V_integer
    finish() noexcept
      {
        uint32_t val = static_cast<uint32_t>(this->m_reg);
        this->clear();
        return val;
      }
  };

void
do_construct_FNV1a32(V_object& result)
  {
    static constexpr auto s_uuid = sref("{2C79571C-5D7B-4674-056A-6C0D075A82FC}");
    result.insert_or_assign(s_uuid, std_checksum_FNV1a32_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING(
        "std.checksum.FNV1a32::update", "data",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (void) std_checksum_FNV1a32_update(hasher, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING(
        "std.checksum.FNV1a32::finish", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_FNV1a32_finish(hasher);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("clear"),
      ASTERIA_BINDING(
        "std.checksum.FNV1a32::clear", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (void) std_checksum_FNV1a32_clear(hasher);

        reader.throw_no_matching_function_call();
      });
  }

template<size_t N>
V_string
do_copy_SHA_result(const ::std::array<unsigned char, N>& bytes)
  {
    V_string str;
    auto wpos = str.insert(str.begin(), N * 2, '-');

    uint32_t ch;
    for(uint32_t value : bytes)
      for(int k = 1;  k >= 0;  --k)
        ch = (value >> k * 4) & 0x0F,
          *(wpos++) = static_cast<char>('0' + ch + ((9 - ch) >> 29));

    return str;
  }

class MD5_Hasher final
  : public Abstract_Opaque
  {
  private:
    ::MD5_CTX m_reg[1];

  public:
    explicit
    MD5_Hasher() noexcept
      { this->clear();  }

  public:
    tinyfmt&
    describe(tinyfmt& fmt) const override
      {
        return format(fmt, "instance of `std.checksum.MD5` at `$1`", this);
      }

    void
    get_variables(Variable_HashMap&, Variable_HashMap&) const override
      { }

    MD5_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& out) const override
      {
        auto ptr = new auto(*this);
        out.reset(ptr);
        return ptr;
      }

    void
    clear() noexcept
      {
        ::MD5_Init(this->m_reg);
      }

    void
    update(const void* data, size_t size) noexcept
      {
        ::MD5_Update(this->m_reg, data, size);
      }

    V_string
    finish() noexcept
      {
        ::std::array<unsigned char, MD5_DIGEST_LENGTH> val;
        ::MD5_Final(val.data(), this->m_reg);
        this->clear();
        return do_copy_SHA_result(val);
      }
  };

void
do_construct_MD5(V_object& result)
  {
    static constexpr auto s_uuid = sref("{2C795808-7290-4675-056A-D3825905F8E1}");
    result.insert_or_assign(s_uuid, std_checksum_MD5_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING(
        "std.checksum.MD5::update", "data",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (void) std_checksum_MD5_update(hasher, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING(
        "std.checksum.MD5::finish", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_MD5_finish(hasher);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("clear"),
      ASTERIA_BINDING(
        "std.checksum.MD5::clear", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (void) std_checksum_MD5_clear(hasher);

        reader.throw_no_matching_function_call();
      });
  }

class SHA1_Hasher final
  : public Abstract_Opaque
  {
  private:
    ::SHA_CTX m_reg[1];

  public:
    explicit
    SHA1_Hasher() noexcept
      { this->clear();  }

  public:
    tinyfmt&
    describe(tinyfmt& fmt) const override
      {
        return format(fmt, "instance of `std.checksum.SHA1` at `$1`", this);
      }

    void
    get_variables(Variable_HashMap&, Variable_HashMap&) const override
      { }

    SHA1_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& out) const override
      {
        auto ptr = new auto(*this);
        out.reset(ptr);
        return ptr;
      }

    void
    clear() noexcept
      {
        ::SHA1_Init(this->m_reg);
      }

    void
    update(const void* data, size_t size) noexcept
      {
        ::SHA1_Update(this->m_reg, data, size);
      }

    V_string
    finish() noexcept
      {
        ::std::array<unsigned char, SHA_DIGEST_LENGTH> val;
        ::SHA1_Final(val.data(), this->m_reg);
        this->clear();
        return do_copy_SHA_result(val);
      }
  };

void
do_construct_SHA1(V_object& result)
  {
    static constexpr auto s_uuid = sref("{2D242315-AF9A-4EDC-0612-CBBBCBBB75BB}");
    result.insert_or_assign(s_uuid, std_checksum_SHA1_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING(
        "std.checksum.SHA1::update", "data",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (void) std_checksum_SHA1_update(hasher, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING(
        "std.checksum.SHA1::finish", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_SHA1_finish(hasher);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("clear"),
      ASTERIA_BINDING(
        "std.checksum.SHA1::clear", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (void) std_checksum_SHA1_clear(hasher);

        reader.throw_no_matching_function_call();
      });
  }

class SHA224_Hasher final
  : public Abstract_Opaque
  {
  private:
    ::SHA256_CTX m_reg[1];

  public:
    explicit
    SHA224_Hasher() noexcept
      { this->clear();  }

  public:
    tinyfmt&
    describe(tinyfmt& fmt) const override
      {
        return format(fmt, "instance of `std.checksum.SHA224` at `$1`", this);
      }

    void
    get_variables(Variable_HashMap&, Variable_HashMap&) const override
      { }

    SHA224_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& out) const override
      {
        auto ptr = new auto(*this);
        out.reset(ptr);
        return ptr;
      }

    void
    clear() noexcept
      {
        ::SHA224_Init(this->m_reg);
      }

    void
    update(const void* data, size_t size) noexcept
      {
        ::SHA224_Update(this->m_reg, data, size);
      }

    V_string
    finish() noexcept
      {
        ::std::array<unsigned char, SHA224_DIGEST_LENGTH> val;
        ::SHA224_Final(val.data(), this->m_reg);
        this->clear();
        return do_copy_SHA_result(val);
      }
  };

void
do_construct_SHA224(V_object& result)
  {
    static constexpr auto s_uuid = sref("{2D24231A-8D6F-4EDC-0612-C448C44886E4}");
    result.insert_or_assign(s_uuid, std_checksum_SHA224_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING(
        "std.checksum.SHA224::update", "data",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (void) std_checksum_SHA224_update(hasher, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING(
        "std.checksum.SHA224::finish", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_SHA224_finish(hasher);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("clear"),
      ASTERIA_BINDING(
        "std.checksum.SHA224::clear", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (void) std_checksum_SHA224_clear(hasher);

        reader.throw_no_matching_function_call();
      });
  }

class SHA256_Hasher final
  : public Abstract_Opaque
  {
  private:
    ::SHA256_CTX m_reg[1];

  public:
    explicit
    SHA256_Hasher() noexcept
      { this->clear();  }

  public:
    tinyfmt&
    describe(tinyfmt& fmt) const override
      {
        return format(fmt, "instance of `std.checksum.SHA256` at `$1`", this);
      }

    void
    get_variables(Variable_HashMap&, Variable_HashMap&) const override
      { }

    SHA256_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& out) const override
      {
        auto ptr = new auto(*this);
        out.reset(ptr);
        return ptr;
      }

    void
    clear() noexcept
      {
        ::SHA256_Init(this->m_reg);
      }

    void
    update(const void* data, size_t size) noexcept
      {
        ::SHA256_Update(this->m_reg, data, size);
      }

    V_string
    finish() noexcept
      {
        ::std::array<unsigned char, SHA256_DIGEST_LENGTH> val;
        ::SHA256_Final(val.data(), this->m_reg);
        this->clear();
        return do_copy_SHA_result(val);
      }
  };

void
do_construct_SHA256(V_object& result)
  {
    static constexpr auto s_uuid = sref("{2D24231C-F3D7-4EDC-0612-551055107FE2}");
    result.insert_or_assign(s_uuid, std_checksum_SHA256_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING(
        "std.checksum.SHA256::update", "data",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (void) std_checksum_SHA256_update(hasher, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING(
        "std.checksum.SHA256::finish", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_SHA256_finish(hasher);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("clear"),
      ASTERIA_BINDING(
        "std.checksum.SHA256::clear", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (void) std_checksum_SHA256_clear(hasher);

        reader.throw_no_matching_function_call();
      });
  }

class SHA384_Hasher final
  : public Abstract_Opaque
  {
  private:
    ::SHA512_CTX m_reg[1];

  public:
    explicit
    SHA384_Hasher() noexcept
      { this->clear();  }

  public:
    tinyfmt&
    describe(tinyfmt& fmt) const override
      {
        return format(fmt, "instance of `std.checksum.SHA384` at `$1`", this);
      }

    void
    get_variables(Variable_HashMap&, Variable_HashMap&) const override
      { }

    SHA384_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& out) const override
      {
        auto ptr = new auto(*this);
        out.reset(ptr);
        return ptr;
      }

    void
    clear() noexcept
      {
        ::SHA384_Init(this->m_reg);
      }

    void
    update(const void* data, size_t size) noexcept
      {
        ::SHA384_Update(this->m_reg, data, size);
      }

    V_string
    finish() noexcept
      {
        ::std::array<unsigned char, SHA384_DIGEST_LENGTH> val;
        ::SHA384_Final(val.data(), this->m_reg);
        this->clear();
        return do_copy_SHA_result(val);
      }
  };

void
do_construct_SHA384(V_object& result)
  {
    static constexpr auto s_uuid = sref("{2D24231E-B48F-4EDC-0612-145E145E6F29}");
    result.insert_or_assign(s_uuid, std_checksum_SHA384_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING(
        "std.checksum.SHA384::update", "data",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (void) std_checksum_SHA384_update(hasher, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING(
        "std.checksum.SHA384::finish", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_SHA384_finish(hasher);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("clear"),
      ASTERIA_BINDING(
        "std.checksum.SHA384::clear", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (void) std_checksum_SHA384_clear(hasher);

        reader.throw_no_matching_function_call();
      });
  }

class SHA512_Hasher final
  : public Abstract_Opaque
  {
  private:
    ::SHA512_CTX m_reg[1];

  public:
    explicit
    SHA512_Hasher() noexcept
      { this->clear();  }

  public:
    tinyfmt&
    describe(tinyfmt& fmt) const override
      {
        return format(fmt, "instance of `std.checksum.SHA512` at `$1`", this);
      }

    void
    get_variables(Variable_HashMap&, Variable_HashMap&) const override
      { }

    SHA512_Hasher*
    clone_opt(rcptr<Abstract_Opaque>& out) const override
      {
        auto ptr = new auto(*this);
        out.reset(ptr);
        return ptr;
      }

    void
    clear() noexcept
      {
        ::SHA512_Init(this->m_reg);
      }

    void
    update(const void* data, size_t size) noexcept
      {
        ::SHA512_Update(this->m_reg, data, size);
      }

    V_string
    finish() noexcept
      {
        ::std::array<unsigned char, SHA512_DIGEST_LENGTH> val;
        ::SHA512_Final(val.data(), this->m_reg);
        this->clear();
        return do_copy_SHA_result(val);
      }
  };

void
do_construct_SHA512(V_object& result)
  {
    static constexpr auto s_uuid = sref("{2D242320-7A94-4EDC-0612-8851885187F8}");
    result.insert_or_assign(s_uuid, std_checksum_SHA512_private());

    result.insert_or_assign(sref("update"),
      ASTERIA_BINDING(
        "std.checksum.SHA512::update", "data",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (void) std_checksum_SHA512_update(hasher, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("finish"),
      ASTERIA_BINDING(
        "std.checksum.SHA512::finish", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_SHA512_finish(hasher);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("clear"),
      ASTERIA_BINDING(
        "std.checksum.SHA512::clear", "",
        Reference&& self, Argument_Reader&& reader)
      {
        self.push_modifier_object_key(s_uuid);
        auto& hasher = self.dereference_mutable().mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (void) std_checksum_SHA512_clear(hasher);

        reader.throw_no_matching_function_call();
      });
  }

template<typename HasherT>
decltype(::std::declval<HasherT&>().finish())
do_hash_bytes(const V_string& data)
  {
    HasherT h;
    h.update(data.data(), data.size());
    return h.finish();
  }

template<typename HasherT>
decltype(::std::declval<HasherT&>().finish())
do_hash_file(const V_string& path)
  {
    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), O_RDONLY));
    if(!fd)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Could not open file '$2'\n[`open()` failed: $1]"),
          format_errno(), path);

    // Get the file mode and preferred I/O block size.
    struct ::stat stb;
    if(::fstat(fd, &stb) != 0)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Could not get information about source file '$2'",
          "[`fstat()` failed: $1]"),
          format_errno(), path);

    // Allocate the I/O buffer.
    ::rocket::unique_ptr<char, void (void*)> pbuf(::operator delete);
    size_t nbuf = static_cast<size_t>(stb.st_blksize | 0x1000);
    pbuf.reset(static_cast<char*>(::operator new(nbuf)));

    // Read bytes from the file and hash them.
    HasherT h;
    for(;;) {
      ::ssize_t nread = ::read(fd, pbuf, nbuf);
      if(nread <= 0) {
        // Check for end of file.
        if(nread == 0)
          break;

        ASTERIA_THROW_RUNTIME_ERROR((
            "Error reading file '$2'\n[`read()` failed: $1]"),
            format_errno(), path);
      }
      h.update(pbuf, static_cast<size_t>(nread));
    }
    return h.finish();
  }

}  // namespace

V_object
std_checksum_CRC32()
  {
    V_object result;
    do_construct_CRC32(result);
    return result;
  }

V_opaque
std_checksum_CRC32_private()
  {
    return ::rocket::make_refcnt<CRC32_Hasher>();
  }

void
std_checksum_CRC32_update(V_opaque& h, V_string data)
  {
    h.open<CRC32_Hasher>().update(data.data(), data.size());
  }

V_integer
std_checksum_CRC32_finish(V_opaque& h)
  {
    return h.open<CRC32_Hasher>().finish();
  }

void
std_checksum_CRC32_clear(V_opaque& h)
  {
    h.open<CRC32_Hasher>().clear();
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

V_object
std_checksum_FNV1a32()
  {
    V_object result;
    do_construct_FNV1a32(result);
    return result;
  }

V_opaque
std_checksum_FNV1a32_private()
  {
    return ::rocket::make_refcnt<FNV1a32_Hasher>();
  }

void
std_checksum_FNV1a32_update(V_opaque& h, V_string data)
  {
    h.open<FNV1a32_Hasher>().update(data.data(), data.size());
  }

V_integer
std_checksum_FNV1a32_finish(V_opaque& h)
  {
    return h.open<FNV1a32_Hasher>().finish();
  }

void
std_checksum_FNV1a32_clear(V_opaque& h)
  {
    h.open<FNV1a32_Hasher>().clear();
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

V_object
std_checksum_MD5()
  {
    V_object result;
    do_construct_MD5(result);
    return result;
  }

V_opaque
std_checksum_MD5_private()
  {
    return ::rocket::make_refcnt<MD5_Hasher>();
  }

void
std_checksum_MD5_update(V_opaque& h, V_string data)
  {
    h.open<MD5_Hasher>().update(data.data(), data.size());
  }

V_string
std_checksum_MD5_finish(V_opaque& h)
  {
    return h.open<MD5_Hasher>().finish();
  }

void
std_checksum_MD5_clear(V_opaque& h)
  {
    h.open<MD5_Hasher>().clear();
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

V_object
std_checksum_SHA1()
  {
    V_object result;
    do_construct_SHA1(result);
    return result;
  }

V_opaque
std_checksum_SHA1_private()
  {
    return ::rocket::make_refcnt<SHA1_Hasher>();
  }

void
std_checksum_SHA1_update(V_opaque& h, V_string data)
  {
    h.open<SHA1_Hasher>().update(data.data(), data.size());
  }

V_string
std_checksum_SHA1_finish(V_opaque& h)
  {
    return h.open<SHA1_Hasher>().finish();
  }

void
std_checksum_SHA1_clear(V_opaque& h)
  {
    h.open<SHA1_Hasher>().clear();
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

V_object
std_checksum_SHA224()
  {
    V_object result;
    do_construct_SHA224(result);
    return result;
  }

V_opaque
std_checksum_SHA224_private()
  {
    return ::rocket::make_refcnt<SHA224_Hasher>();
  }

void
std_checksum_SHA224_update(V_opaque& h, V_string data)
  {
    h.open<SHA224_Hasher>().update(data.data(), data.size());
  }

V_string
std_checksum_SHA224_finish(V_opaque& h)
  {
    return h.open<SHA224_Hasher>().finish();
  }

void
std_checksum_SHA224_clear(V_opaque& h)
  {
    h.open<SHA224_Hasher>().clear();
  }

V_string
std_checksum_sha224(V_string data)
  {
    return do_hash_bytes<SHA224_Hasher>(data);
  }

V_string
std_checksum_sha224_file(V_string path)
  {
    return do_hash_file<SHA224_Hasher>(path);
  }

V_object
std_checksum_SHA256()
  {
    V_object result;
    do_construct_SHA256(result);
    return result;
  }

V_opaque
std_checksum_SHA256_private()
  {
    return ::rocket::make_refcnt<SHA256_Hasher>();
  }

void
std_checksum_SHA256_update(V_opaque& h, V_string data)
  {
    h.open<SHA256_Hasher>().update(data.data(), data.size());
  }

V_string
std_checksum_SHA256_finish(V_opaque& h)
  {
    return h.open<SHA256_Hasher>().finish();
  }

void
std_checksum_SHA256_clear(V_opaque& h)
  {
    h.open<SHA256_Hasher>().clear();
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

V_object
std_checksum_SHA384()
  {
    V_object result;
    do_construct_SHA384(result);
    return result;
  }

V_opaque
std_checksum_SHA384_private()
  {
    return ::rocket::make_refcnt<SHA384_Hasher>();
  }

void
std_checksum_SHA384_update(V_opaque& h, V_string data)
  {
    h.open<SHA384_Hasher>().update(data.data(), data.size());
  }

V_string
std_checksum_SHA384_finish(V_opaque& h)
  {
    return h.open<SHA384_Hasher>().finish();
  }

void
std_checksum_SHA384_clear(V_opaque& h)
  {
    h.open<SHA384_Hasher>().clear();
  }

V_string
std_checksum_sha384(V_string data)
  {
    return do_hash_bytes<SHA384_Hasher>(data);
  }

V_string
std_checksum_sha384_file(V_string path)
  {
    return do_hash_file<SHA384_Hasher>(path);
  }

V_object
std_checksum_SHA512()
  {
    V_object result;
    do_construct_SHA512(result);
    return result;
  }

V_opaque
std_checksum_SHA512_private()
  {
    return ::rocket::make_refcnt<SHA512_Hasher>();
  }

void
std_checksum_SHA512_update(V_opaque& h, V_string data)
  {
    h.open<SHA512_Hasher>().update(data.data(), data.size());
  }

V_string
std_checksum_SHA512_finish(V_opaque& h)
  {
    return h.open<SHA512_Hasher>().finish();
  }

void
std_checksum_SHA512_clear(V_opaque& h)
  {
    h.open<SHA512_Hasher>().clear();
  }

V_string
std_checksum_sha512(V_string data)
  {
    return do_hash_bytes<SHA512_Hasher>(data);
  }

V_string
std_checksum_sha512_file(V_string path)
  {
    return do_hash_file<SHA512_Hasher>(path);
  }

void
create_bindings_checksum(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("CRC32"),
      ASTERIA_BINDING(
        "std.checksum.CRC32", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_CRC32();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("crc32"),
      ASTERIA_BINDING(
        "std.checksum.crc32", "data",
        Argument_Reader&& reader)
      {
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_checksum_crc32(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("crc32_file"),
      ASTERIA_BINDING(
        "std.checksum.crc32_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_checksum_crc32_file(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("FNV1a32"),
      ASTERIA_BINDING(
        "std.checksum.FNV1a32", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_FNV1a32();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("fnv1a32"),
      ASTERIA_BINDING(
        "std.checksum.fnv1a32", "data",
        Argument_Reader&& reader)
      {
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_checksum_fnv1a32(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("fnv1a32_file"),
      ASTERIA_BINDING(
        "std.checksum.fnv1a32_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_checksum_fnv1a32_file(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("MD5"),
      ASTERIA_BINDING(
        "std.checksum.MD5", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_MD5();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("md5"),
      ASTERIA_BINDING(
        "std.checksum.md5", "data",
        Argument_Reader&& reader)
      {
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_checksum_md5(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("md5_file"),
      ASTERIA_BINDING(
        "std.checksum.md5_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_checksum_md5_file(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("SHA1"),
      ASTERIA_BINDING(
        "std.checksum.SHA1", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_SHA1();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sha1"),
      ASTERIA_BINDING(
        "std.checksum.sha1", "data",
        Argument_Reader&& reader)
      {
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_checksum_sha1(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sha1_file"),
      ASTERIA_BINDING(
        "std.checksum.sha1_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_checksum_sha1_file(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("SHA224"),
      ASTERIA_BINDING(
        "std.checksum.SHA224", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_SHA224();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sha224"),
      ASTERIA_BINDING(
        "std.checksum.sha224", "data",
        Argument_Reader&& reader)
      {
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_checksum_sha224(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sha224_file"),
      ASTERIA_BINDING(
        "std.checksum.sha224_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_checksum_sha224_file(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("SHA256"),
      ASTERIA_BINDING(
        "std.checksum.SHA256", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_SHA256();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sha256"),
      ASTERIA_BINDING(
        "std.checksum.sha256", "data",
        Argument_Reader&& reader)
      {
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_checksum_sha256(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sha256_file"),
      ASTERIA_BINDING(
        "std.checksum.sha256_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_checksum_sha256_file(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("SHA384"),
      ASTERIA_BINDING(
        "std.checksum.SHA384", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_SHA384();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sha384"),
      ASTERIA_BINDING(
        "std.checksum.sha384", "data",
        Argument_Reader&& reader)
      {
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_checksum_sha384(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sha384_file"),
      ASTERIA_BINDING(
        "std.checksum.sha384_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_checksum_sha384_file(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("SHA512"),
      ASTERIA_BINDING(
        "std.checksum.SHA512", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_checksum_SHA512();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sha512"),
      ASTERIA_BINDING(
        "std.checksum.sha512", "data",
        Argument_Reader&& reader)
      {
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_checksum_sha512(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sha512_file"),
      ASTERIA_BINDING(
        "std.checksum.sha512_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_checksum_sha512_file(path);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
