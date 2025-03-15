// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "zlib.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/binding_generator.hpp"
#include "../runtime/global_context.hpp"
#include "../utils.hpp"
#define ZLIB_CONST 1
#include <zlib.h>
namespace asteria {
namespace {

void
do_zlib_throw_error(const char* fname, const ::z_stream* strm, int err)
  {
    const char* msg = "[no error message]";
    if(strm->msg)
      msg = strm->msg;
    else if(err == Z_MEM_ERROR)
      msg = "memory allocation failure";

    ASTERIA_THROW((
        "zlib error: $1\n[`$2()` returned `$3`]"),
        msg, fname, err);
  }

class Deflator
  :
    public Abstract_Opaque
  {
  private:
    ::z_stream m_strm[1] = { };

  public:
    Deflator(int wbits, int level)
      {
        int err = ::deflateInit2(this->m_strm, level, Z_DEFLATED, wbits, 9, 0);
        if(err != Z_OK)
          do_zlib_throw_error("deflateInit2", this->m_strm, err);
      }

    Deflator(const Deflator& other, int)
      {
        int err = ::deflateCopy(this->m_strm, const_cast<::z_stream*>(other.m_strm));
        if(err != Z_OK)
          do_zlib_throw_error("deflateCopy", this->m_strm, err);
      }

    ~Deflator()
      {
        ::deflateEnd(this->m_strm);
      }

    Deflator(const Deflator&) = delete;
    Deflator& operator=(const Deflator&) & = delete;

  private:
    int
    do_output_deflate(cow_string& out, int flush)
      {
        constexpr size_t size_add = 4096;
        auto ipos = out.insert(out.end(), size_add, '*');
        this->m_strm->avail_out = size_add;
        this->m_strm->next_out = reinterpret_cast<::Byte*>(&*ipos);

        int err = ::deflate(this->m_strm, flush);
        ipos += static_cast<ptrdiff_t>(size_add - this->m_strm->avail_out);
        out.erase(ipos, out.end());
        return err;
      }

  public:
    tinyfmt&
    describe(tinyfmt& fmt) const override
      {
        return format(fmt, "instance of `std.zlib.Deflator` at `$1`", this);
      }

    void
    collect_variables(Variable_HashMap&, Variable_HashMap&) const override
      {
      }

    Deflator*
    clone_opt(refcnt_ptr<Abstract_Opaque>& out) const override
      {
        auto ptr = new Deflator(*this, 42);
        out.reset(ptr);
        return ptr;
      }

    void
    clear() noexcept
      {
        ::deflateReset(this->m_strm);
      }

    void
    update(V_string& out, const void* data, size_t size)
      {
        auto& bptr = this->m_strm->next_in;
        bptr = static_cast<const ::Byte*>(data);
        auto eptr = bptr + size;

        for(;;) {
          ::uInt in = ::rocket::clamp_cast<::uInt>(eptr - bptr, 0, INT_MAX);
          this->m_strm->avail_in = in;
          if(in == 0)
            break;

          int err = this->do_output_deflate(out, Z_NO_FLUSH);
          if(err != Z_OK)
            do_zlib_throw_error("deflate", this->m_strm, err);
        }
      }

    void
    flush(V_string& out)
      {
        this->m_strm->next_in = nullptr;
        this->m_strm->avail_in = 0;

        for(;;) {
          int err = this->do_output_deflate(out, Z_SYNC_FLUSH);
          if(err == Z_BUF_ERROR)
            break;
          else if(err != Z_OK)
            do_zlib_throw_error("deflate", this->m_strm, err);
        }
      }

    void
    finish(V_string& out)
      {
        this->m_strm->next_in = nullptr;
        this->m_strm->avail_in = 0;

        for(;;) {
          int err = this->do_output_deflate(out, Z_FINISH);
          if(err == Z_STREAM_END)
            break;
          else if(err != Z_OK)
            do_zlib_throw_error("deflate", this->m_strm, err);
        }
      }
  };

void
do_construct_Deflator(V_object& result, V_string format, optV_integer level)
  {
    static constexpr auto s_private_uuid = &"{708e2b4c-c3d1-46bd-8492-e95df5e79dbf}";
    result.insert_or_assign(s_private_uuid, std_zlib_Deflator_private(format, level));
    result.insert_or_assign(&"output", V_string());

    result.insert_or_assign(&"update",
      ASTERIA_BINDING(
        "std.zlib.Deflator::update", "data",
        Reference&& self, Argument_Reader&& reader)
      {
        auto& self_obj = self.dereference_mutable().mut_object();
        auto& defl = self_obj.mut(s_private_uuid).mut_opaque();
        auto& output = self_obj.mut(&"output").mut_string();
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (void) std_zlib_Deflator_update(defl, output, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"flush",
      ASTERIA_BINDING(
        "std.zlib.Deflator::flush", "",
        Reference&& self, Argument_Reader&& reader)
      {
        auto& self_obj = self.dereference_mutable().mut_object();
        auto& defl = self_obj.mut(s_private_uuid).mut_opaque();
        auto& output = self_obj.mut(&"output").mut_string();

        reader.start_overload();
        if(reader.end_overload())
          return (void) std_zlib_Deflator_flush(defl, output);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"finish",
      ASTERIA_BINDING(
        "std.zlib.Deflator::finish", "",
        Reference&& self, Argument_Reader&& reader)
      {
        auto& self_obj = self.dereference_mutable().mut_object();
        auto& defl = self_obj.mut(s_private_uuid).mut_opaque();
        auto& output = self_obj.mut(&"output").mut_string();

        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_zlib_Deflator_finish(defl, output);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"clear",
      ASTERIA_BINDING(
        "std.zlib.Deflator::clear", "",
        Reference&& self, Argument_Reader&& reader)
      {
        auto& defl = self.dereference_mutable().mut_object().mut(s_private_uuid).mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (void) std_zlib_Deflator_clear(defl);

        reader.throw_no_matching_function_call();
      });
  }

class Inflator
  :
    public Abstract_Opaque
  {
  private:
    ::z_stream m_strm[1] = { };

  public:
    explicit Inflator(int wbits)
      {
        int err = ::inflateInit2(this->m_strm, wbits);
        if(err != Z_OK)
          do_zlib_throw_error("inflateInit2", this->m_strm, err);
      }

    Inflator(const Inflator& other, int)
      {
        int err = ::inflateCopy(this->m_strm, const_cast<::z_stream*>(other.m_strm));
        if(err != Z_OK)
          do_zlib_throw_error("inflateCopy", this->m_strm, err);
      }

    ~Inflator()
      {
        ::inflateEnd(this->m_strm);
      }

    Inflator(const Inflator&) = delete;
    Inflator& operator=(const Inflator&) & = delete;

  private:
    int
    do_output_inflate(cow_string& out, int flush)
      {
        constexpr size_t size_add = 4096;
        auto ipos = out.insert(out.end(), size_add, '*');
        this->m_strm->avail_out = size_add;
        this->m_strm->next_out = reinterpret_cast<::Byte*>(&*ipos);

        int err = ::inflate(this->m_strm, flush);
        ipos += static_cast<ptrdiff_t>(size_add - this->m_strm->avail_out);
        out.erase(ipos, out.end());
        return err;
      }

  public:
    tinyfmt&
    describe(tinyfmt& fmt) const override
      {
        return format(fmt, "instance of `std.zlib.Inflator` at `$1`", this);
      }

    void
    collect_variables(Variable_HashMap&, Variable_HashMap&) const override
      {
      }

    Inflator*
    clone_opt(refcnt_ptr<Abstract_Opaque>& out) const override
      {
        auto ptr = new Inflator(*this, 42);
        out.reset(ptr);
        return ptr;
      }

    void
    clear() noexcept
      {
        ::inflateReset(this->m_strm);
      }

    void
    update(V_string& out, const void* data, size_t size)
      {
        auto& bptr = this->m_strm->next_in;
        bptr = static_cast<const ::Byte*>(data);
        auto eptr = bptr + size;

        for(;;) {
          ::uInt in = ::rocket::clamp_cast<::uInt>(eptr - bptr, 0, INT_MAX);
          this->m_strm->avail_in = in;
          if(in == 0)
            break;

          int err = this->do_output_inflate(out, Z_NO_FLUSH);
          if(err == Z_STREAM_END)
            break;
          else if(err != Z_OK)
            do_zlib_throw_error("inflate", this->m_strm, err);
        }
      }

    void
    flush(V_string& out)
      {
        this->m_strm->next_in = nullptr;
        this->m_strm->avail_in = 0;

        for(;;) {
          int err = this->do_output_inflate(out, Z_SYNC_FLUSH);
          if((err == Z_BUF_ERROR) || (err == Z_STREAM_END))
            break;
          else if(err != Z_OK)
            do_zlib_throw_error("inflate", this->m_strm, err);
        }
      }

    void
    finish(V_string& out)
      {
        this->m_strm->next_in = nullptr;
        this->m_strm->avail_in = 0;

        for(;;) {
          int err = this->do_output_inflate(out, Z_FINISH);
          if(err == Z_STREAM_END)
            break;
          else if(err != Z_OK)
            do_zlib_throw_error("inflate", this->m_strm, err);
        }
      }
  };

void
do_construct_Inflator(V_object& result, V_string format)
  {
    static constexpr auto s_private_uuid = &"{49fcc6d1-74ac-47c4-9cd0-46c84b5e7718}";
    result.insert_or_assign(s_private_uuid, std_zlib_Inflator_private(format));
    result.insert_or_assign(&"output", V_string());

    result.insert_or_assign(&"update",
      ASTERIA_BINDING(
        "std.zlib.Inflator::update", "data",
        Reference&& self, Argument_Reader&& reader)
      {
        auto& self_obj = self.dereference_mutable().mut_object();
        auto& infl = self_obj.mut(s_private_uuid).mut_opaque();
        auto& output = self_obj.mut(&"output").mut_string();
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (void) std_zlib_Inflator_update(infl, output, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"flush",
      ASTERIA_BINDING(
        "std.zlib.Inflator::flush", "",
        Reference&& self, Argument_Reader&& reader)
      {
        auto& self_obj = self.dereference_mutable().mut_object();
        auto& infl = self_obj.mut(s_private_uuid).mut_opaque();
        auto& output = self_obj.mut(&"output").mut_string();

        reader.start_overload();
        if(reader.end_overload())
          return (void) std_zlib_Inflator_flush(infl, output);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"finish",
      ASTERIA_BINDING(
        "std.zlib.Inflator::finish", "",
        Reference&& self, Argument_Reader&& reader)
      {
        auto& self_obj = self.dereference_mutable().mut_object();
        auto& infl = self_obj.mut(s_private_uuid).mut_opaque();
        auto& output = self_obj.mut(&"output").mut_string();

        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_zlib_Inflator_finish(infl, output);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"clear",
      ASTERIA_BINDING(
        "std.zlib.Inflator::clear", "",
        Reference&& self, Argument_Reader&& reader)
      {
        auto& infl = self.dereference_mutable().mut_object().mut(s_private_uuid).mut_opaque();

        reader.start_overload();
        if(reader.end_overload())
          return (void) std_zlib_Inflator_clear(infl);

        reader.throw_no_matching_function_call();
      });
  }

int
do_wbits(const V_string& format)
  {
    if(format == "gzip")
      return 31;
    else if(format == "deflate")
      return 15;
    else if(format == "raw")
      return -15;
    else
      ASTERIA_THROW(("Invalid compression format `$1`"), format);
  }

int
do_level(const optV_integer& level)
  {
    if(!level)
      return Z_DEFAULT_COMPRESSION;

    if(*level < 0)
      ASTERIA_THROW(( "Negative compression level `$1`"), *level);

    return ::rocket::clamp_cast<int>(*level, Z_NO_COMPRESSION, Z_BEST_COMPRESSION);
  }

}  // namespace

V_object
std_zlib_Deflator(V_string format, optV_integer level)
  {
    V_object result;
    do_construct_Deflator(result, format, level);
    return result;
  }

V_opaque
std_zlib_Deflator_private(V_string format, optV_integer level)
  {
    return ::rocket::make_refcnt<Deflator>(do_wbits(format), do_level(level));
  }

void
std_zlib_Deflator_update(V_opaque& r, V_string& output, V_string data)
  {
    r.open<Deflator>().update(output, data.data(), data.size());
  }

void
std_zlib_Deflator_flush(V_opaque& r, V_string& output)
  {
    r.open<Deflator>().flush(output);
  }

V_string
std_zlib_Deflator_finish(V_opaque& r, V_string& output)
  {
    r.open<Deflator>().finish(output);
    return output;
  }

void
std_zlib_Deflator_clear(V_opaque& r)
  {
    r.open<Deflator>().clear();
  }

V_string
std_zlib_deflate(V_string data, optV_integer level)
  {
    Deflator defl(do_wbits(&"deflate"), do_level(level));
    V_string output;
    defl.update(output, data.data(), data.size());
    defl.finish(output);
    return output;
  }

V_string
std_zlib_gzip(V_string data, optV_integer level)
  {
    Deflator defl(do_wbits(&"gzip"), do_level(level));
    V_string output;
    defl.update(output, data.data(), data.size());
    defl.finish(output);
    return output;
  }

V_object
std_zlib_Inflator(V_string format)
  {
    V_object result;
    do_construct_Inflator(result, format);
    return result;
  }

V_opaque
std_zlib_Inflator_private(V_string format)
  {
    return ::rocket::make_refcnt<Inflator>(do_wbits(format));
  }

void
std_zlib_Inflator_update(V_opaque& r, V_string& output, V_string data)
  {
    r.open<Inflator>().update(output, data.data(), data.size());
  }

void
std_zlib_Inflator_flush(V_opaque& r, V_string& output)
  {
    r.open<Inflator>().flush(output);
  }

V_string
std_zlib_Inflator_finish(V_opaque& r, V_string& output)
  {
    r.open<Inflator>().finish(output);
    return output;
  }

void
std_zlib_Inflator_clear(V_opaque& r)
  {
    r.open<Inflator>().clear();
  }

V_string
std_zlib_inflate(V_string data)
  {
    Inflator infl(do_wbits(&"deflate"));
    V_string output;
    infl.update(output, data.data(), data.size());
    infl.finish(output);
    return output;
  }

V_string
std_zlib_gunzip(V_string data)
  {
    Inflator infl(do_wbits(&"gzip"));
    V_string output;
    infl.update(output, data.data(), data.size());
    infl.finish(output);
    return output;
  }

void
create_bindings_zlib(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(&"Deflator",
      ASTERIA_BINDING(
        "std.zlib.Deflator", "format, [level]",
        Argument_Reader&& reader)
      {
        V_string format;
        optV_integer level;

        reader.start_overload();
        reader.required(format);
        reader.optional(level);
        if(reader.end_overload())
          return (Value) std_zlib_Deflator(format, level);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"deflate",
      ASTERIA_BINDING(
        "std.zlib.deflate", "data, [level]",
        Argument_Reader&& reader)
      {
        V_string data;
        optV_integer level;

        reader.start_overload();
        reader.required(data);
        reader.optional(level);
        if(reader.end_overload())
          return (Value) std_zlib_deflate(data, level);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"gzip",
      ASTERIA_BINDING(
        "std.zlib.gzip", "data, [level]",
        Argument_Reader&& reader)
      {
        V_string data;
        optV_integer level;

        reader.start_overload();
        reader.required(data);
        reader.optional(level);
        if(reader.end_overload())
          return (Value) std_zlib_gzip(data, level);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"Inflator",
      ASTERIA_BINDING(
        "std.zlib.Inflator", "format",
        Argument_Reader&& reader)
      {
        V_string format;

        reader.start_overload();
        reader.required(format);
        if(reader.end_overload())
          return (Value) std_zlib_Inflator(format);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"inflate",
      ASTERIA_BINDING(
        "std.zlib.inflate", "data",
        Argument_Reader&& reader)
      {
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_zlib_inflate(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"gunzip",
      ASTERIA_BINDING(
        "std.zlib.gunzip", "data",
        Argument_Reader&& reader)
      {
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_zlib_gunzip(data);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
