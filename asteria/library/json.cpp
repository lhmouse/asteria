// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "json.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/binding_generator.hpp"
#include "../utils.hpp"
#include <algorithm>
namespace asteria {
namespace {

struct Parser_Context
  {
    int32_t c;
    bool eof;
    int64_t offset;
    int64_t saved_offset;
  };

constexpr ROCKET_ALWAYS_INLINE
bool
is_within(int c, int lo, int hi)
  {
    return (c >= lo) && (c <= hi);
  }

template<typename... Ts>
constexpr ROCKET_ALWAYS_INLINE
bool
is_any(int c, Ts... accept_set)
  {
    return (... || (c == accept_set));
  }

ROCKET_ALWAYS_INLINE
void
do_err(Parser_Context& ctx, const char* error)
  {
    ctx.c = -1;
    ctx.offset = ctx.saved_offset;

    if(error)
      ASTERIA_THROW((
          "Could not parse JSON string: $1 at offset `$2`"),
          error, ctx.offset);
  }

struct Memory_Source
  {
    const char* bptr;
    const char* sptr;
    const char* eptr;

    constexpr
    Memory_Source()
      noexcept
      : bptr(), sptr(), eptr()  { }

    constexpr
    Memory_Source(const char* s, size_t n)
      noexcept
      : bptr(s), sptr(s), eptr(s + n)  { }

    int
    getc()
      noexcept
      {
        int r = -1;
        if(this->sptr != this->eptr) {
          r = static_cast<unsigned char>(*(this->sptr));
          this->sptr ++;
        }
        return r;
      }

    size_t
    getn(char* s, size_t n)
      noexcept
      {
        size_t r = ::std::min(static_cast<size_t>(this->eptr - this->sptr), n);
        if(r != 0) {
          ::memcpy(s, this->sptr, r);
          this->sptr += r;
        }
        return r;
      }

    int64_t
    tell()
      const noexcept
      {
        return this->sptr - this->bptr;
      }
  };

struct Unified_Source
  {
    tinyfmt* fmt = nullptr;
    Memory_Source* mem = nullptr;
    ::std::FILE* fp = nullptr;

    Unified_Source(tinyfmt* b)
      noexcept
      : fmt(b)  { }

    Unified_Source(Memory_Source* m)
      noexcept
      : mem(m)  { }

    Unified_Source(::std::FILE* f)
      noexcept
      : fp(f)  { }

    int
    getc()
      const
      {
        if(this->mem)
          return this->mem->getc();
        else if(this->fp)
          return ::fgetc(this->fp);
        else
          return this->fmt->getc();
      }

    size_t
    getn(char* s, size_t n)
      const
      {
        if(this->mem)
          return this->mem->getn(s, n);
        else if(this->fp)
          return ::fread(s, 1, n, this->fp);
        else
          return this->fmt->getn(s, n);
      }

    int64_t
    tell()
      const
      {
        if(this->mem)
          return this->mem->tell();
        else if(this->fp)
          return ::ftello(this->fp);
        else
          return this->fmt->tell();
      }
  };

struct Unified_Sink
  {
    tinyfmt* fmt = nullptr;
    cow_string* str = nullptr;
    ::std::FILE* fp = nullptr;

    Unified_Sink(tinyfmt* b)
      noexcept
      : fmt(b)  { }

    Unified_Sink(cow_string* s)
      noexcept
      : str(s)  { }

    Unified_Sink(::std::FILE* f)
      noexcept
      : fp(f)  { }

    void
    putc(char c)
      const
      {
        if(this->str)
          this->str->push_back(c);
        else if(this->fp)
          ::fputc(c, this->fp);
        else
          this->fmt->putc(c);
      }

    void
    putn(const char* s, size_t n)
      const
      {
        if(this->str)
          this->str->append(s, n);
        else if(this->fp)
          ::fwrite(s, 1, n, this->fp);
        else
          this->fmt->putn(s, n);
      }
  };

struct Indent
  {
    cow_string cur;
    cow_string add;

    Indent(const cow_string& xadd)
      {
        if(xadd.empty())
          return;

        this->cur = &"\n";
        this->add = xadd;
      }

    Indent(int64_t xadd)
      {
        if(xadd <= 0)
          return;

        this->cur = &"\n";
        this->add.append(::rocket::clamp_cast<size_t>(xadd, 0, 40), ' ');
      }
  };

void
do_load_next(Parser_Context& ctx, const Unified_Source& usrc)
  {
    ctx.c = usrc.getc();
    if(ctx.c < 0) {
      ctx.eof = true;
      return do_err(ctx, nullptr);
    }

    if(is_within(ctx.c, 0x80, 0xBF))
      return do_err(ctx, "Invalid UTF-8 byte");
    else if(ROCKET_UNEXPECT(ctx.c > 0x7F)) {
      // Parse a multibyte Unicode character.
      uint32_t u8len = ::rocket::lzcnt32((static_cast<uint32_t>(ctx.c) << 24) ^ UINT32_MAX);
      ctx.c &= (1 << (7 - u8len)) - 1;

      char tbytes[4];
      if(usrc.getn(tbytes, u8len - 1) != u8len - 1) {
        ctx.eof = true;
        return do_err(ctx, "Incomplete UTF-8 sequence");
      }

      for(uint32_t k = 0;  k != u8len - 1;  ++k)
        if(!is_within(static_cast<uint8_t>(tbytes[k]), 0x80, 0xBF))
          return do_err(ctx, "Invalid UTF-8 sequence");
        else {
          ctx.c <<= 6;
          ctx.c |= tbytes[k] & 0x3F;
        }

      if((ctx.c < 0x80)  // overlong
          || (ctx.c < (1 << (u8len * 5 - 4)))  // overlong
          || is_within(ctx.c, 0xD800, 0xDFFF)  // surrogates
          || (ctx.c > 0x10FFFF))
        return do_err(ctx, "Invalid Unicode character");
    }
  }

ROCKET_FLATTEN
void
do_token(cow_string& token, Parser_Context& ctx, const Unified_Source& usrc)
  {
    // Clear the current token and skip whitespace.
    token.clear();

    if(ctx.c < 0) {
      ctx.saved_offset = usrc.tell();
      do_load_next(ctx, usrc);
      if(ctx.c < 0)
        return;

      // Skip the UTF-8 BOM, if any.
      if((ctx.saved_offset == 0) && (ctx.c == 0xFEFF)) {
        ctx.saved_offset = usrc.tell();
        do_load_next(ctx, usrc);
        if(ctx.c < 0)
          return;
      }
    }

    while(is_any(ctx.c, ' ', '\t', '\r', '\n')) {
      if(usrc.mem) {
        auto tptr = usrc.mem->sptr;
        while(usrc.mem->eptr != tptr) {
          if(!is_any(*tptr, ' ', '\t', '\r', '\n'))
            break;
          ++ tptr;
        }

        usrc.mem->sptr = tptr;
      }
      else if(usrc.fp) {
        (void)! ::fscanf(usrc.fp, "%*[ \t\r\n]");
      }

      ctx.saved_offset = usrc.tell();
      do_load_next(ctx, usrc);
      if(ctx.c < 0)
        return;
    }

    switch(ctx.c)
      {
      case '[':
      case ']':
      case '{':
      case '}':
      case ':':
      case ',':
        // Take each of these characters as a single token; do not attempt to get
        // the next character, as some of these tokens may terminate the input,
        // and the stream may be blocking but we can't really know whether there
        // are more data.
        token.push_back(static_cast<char>(ctx.c));
        ctx.c = -1;
        break;

      case '_':
      case '$':
      case 'A' ... 'Z':
      case 'a' ... 'z':
        // Take an identifier. As in JavaScript, we accept dollar signs in
        // identifiers as an extension.
        do {
          token.push_back(static_cast<char>(ctx.c));
          do_load_next(ctx, usrc);
        }
        while(is_any(ctx.c, '_', '$') || is_within(ctx.c, 'A', 'Z')
              || is_within(ctx.c, 'a', 'z') || is_within(ctx.c, '0', '9'));

        // If the end of input has been reached, `ctx.error` may be set. We will
        // not return an error, so clear it.
        ROCKET_ASSERT(token.size() != 0);
        ctx.eof = false;
        break;

      case '+':
      case '-':
        // Take a floating-point number. Strictly, JSON doesn't allow plus signs
        // or leading zeroes, but we accept them as extensions.
        token.push_back(static_cast<char>(ctx.c));
        do_load_next(ctx, usrc);

        if(!is_within(ctx.c, '0', '9'))
          return do_err(ctx, "Invalid number");

        // fallthrough
      case '0' ... '9':
        do {
          token.push_back(static_cast<char>(ctx.c));
          do_load_next(ctx, usrc);
        } while(is_within(ctx.c, '0', '9'));

        if(ctx.c == '.') {
          token.push_back(static_cast<char>(ctx.c));
          do_load_next(ctx, usrc);

          if(!is_within(ctx.c, '0', '9'))
            return do_err(ctx, "Invalid number");

          do {
            token.push_back(static_cast<char>(ctx.c));
            do_load_next(ctx, usrc);
          } while(is_within(ctx.c, '0', '9'));
        }

        if(is_any(ctx.c, 'e', 'E')) {
          token.push_back(static_cast<char>(ctx.c));
          do_load_next(ctx, usrc);

          if(is_any(ctx.c, '+', '-')) {
            token.push_back(static_cast<char>(ctx.c));
            do_load_next(ctx, usrc);
          }

          if(!is_within(ctx.c, '0', '9'))
            return do_err(ctx, "Invalid number");

          do {
            token.push_back(static_cast<char>(ctx.c));
            do_load_next(ctx, usrc);
          } while(is_within(ctx.c, '0', '9'));
        }

        // If the end of input has been reached, `ctx.error` may be set. We will
        // not return an error, so clear it.
        ROCKET_ASSERT(token.size() != 0);
        ctx.eof = false;
        break;

      case '\"':
        // Take a double-quoted string. When stored in `token`, it shall start
        // with a double-quote character, followed by the decoded string. No
        // terminating double-quote character is appended.
        token.push_back('\"');
        for(;;) {
          if(usrc.mem) {
            auto tptr = usrc.mem->sptr;
            while(usrc.mem->eptr != tptr) {
              if(is_any(*tptr, '\\', '\"') || !is_within(*tptr, 0x20, 0x7E))
                break;
              ++ tptr;
            }

            if(tptr != usrc.mem->sptr)
              token.append(usrc.mem->sptr, static_cast<size_t>(tptr - usrc.mem->sptr));
            usrc.mem->sptr = tptr;
          }
          else if(usrc.fp) {
            char temp[256];
            size_t len;
            while(::fscanf(usrc.fp, "%255[]-~ !#-[]%zn", temp, &len) == 1) {
              token.append(temp, len);
              if(len < 256)
                break;
            }
          }

          do_load_next(ctx, usrc);
          if(ctx.eof)
            return do_err(ctx, "String not terminated properly");
          else if((ctx.c <= 0x1F) || (ctx.c == 0x7F))
            return do_err(ctx, "Control character not allowed in string");
          else if(ctx.c == '\"')
            break;

          if(ROCKET_UNEXPECT(ctx.c == '\\')) {
            // Read an escape sequence.
            int next = usrc.getc();
            if(next < 0) {
              ctx.eof = true;
              return do_err(ctx, "Incomplete escape sequence");
            }

            switch(next)
              {
              case '\\':
              case '\"':
              case '/':
                ctx.c = next;
                break;

              case 'b':
                ctx.c = '\b';
                break;

              case 'f':
                ctx.c = '\f';
                break;

              case 'n':
                ctx.c = '\n';
                break;

              case 'r':
                ctx.c = '\r';
                break;

              case 't':
                ctx.c = '\t';
                break;

              case 'u':
                {
                  // Read the first UTF-16 code unit.
                  char temp[16];
                  if(usrc.getn(temp, 4) != 4)
                    return do_err(ctx, "Invalid escape sequence");

                  ctx.c = 0;
                  for(uint32_t k = 0;  k != 4;  ++k) {
                    ctx.c <<= 4;
                    int c = static_cast<uint8_t>(temp[k]);
                    if(is_within(c, '0', '9'))
                      ctx.c |= c - '0';
                    else if(is_within(c, 'A', 'F'))
                      ctx.c |= c - 'A' + 10;
                    else if(is_within(c, 'a', 'f'))
                      ctx.c |= c - 'a' + 10;
                    else
                      return do_err(ctx, "Invalid hexadecimal digit");
                  }

                  if(is_within(ctx.c, 0xDC00, 0xDFFF))
                    return do_err(ctx, "Dangling UTF-16 trailing surrogate");
                  else if(is_within(ctx.c, 0xD800, 0xDBFF)) {
                    // The character was a UTF-16 leading surrogate. Now look for
                    // a trailing one, which must be another `\uXXXX` sequence.
                    if(usrc.getn(temp, 6) != 6)
                      return do_err(ctx, "Missing UTF-16 trailing surrogate");

                    if((temp[0] != '\\') || (temp[1] != 'u'))
                      return do_err(ctx, "Missing UTF-16 trailing surrogate");

                    int ls_value = ctx.c - 0xD800;
                    ctx.c = 0;
                    for(uint32_t k = 2;  k != 6;  ++k) {
                      ctx.c <<= 4;
                      int c = static_cast<uint8_t>(temp[k]);
                      if(is_within(c, '0', '9'))
                        ctx.c |= c - '0';
                      else if(is_within(c, 'A', 'F'))
                        ctx.c |= c - 'A' + 10;
                      else if(is_within(c, 'a', 'f'))
                        ctx.c |= c - 'a' + 10;
                      else
                        return do_err(ctx, "Invalid hexadecimal digit");
                    }

                    if(!is_within(ctx.c, 0xDC00, 0xDFFF))
                      return do_err(ctx, "Missing UTF-16 trailing surrogate");

                    ctx.c = 0x10000 + (ls_value << 10) + (ctx.c - 0xDC00);
                  }
                }
                break;

              default:
                return do_err(ctx, "Invalid escape sequence");
              }
          }

          // Move the unescaped character into the token.
          if(ROCKET_EXPECT(ctx.c <= 0x7F))
            token.push_back(static_cast<char>(ctx.c));
          else {
            char mbs[4];
            char* eptr = mbs;
            utf8_encode(eptr, static_cast<char32_t>(ctx.c));
            token.append(mbs, eptr);
          }
        }

        // Drop the terminating quotation mark for simplicity; do not attempt to
        // get the next character, as the stream may be blocking but we can't
        // really know whether there are more data.
        ROCKET_ASSERT(token.size() != 0);
        ctx.c = -1;
        break;

      default:
        return do_err(ctx, "Invalid character");
      }
  }

void
do_parse_from(Value& root, const Unified_Source& usrc)
  {
    Parser_Context ctx = { };
    ctx.c = -1;

    // Break deep recursion with a handwritten stack.
    struct xFrame
      {
        Value* target;
        V_array* psa;
        V_object* pso;
      };

    cow_vector<xFrame> stack;
    cow_string token;
    ::rocket::ascii_numget numg;
    Value* pstor = &root;

    do_token(token, ctx, usrc);
    if(token.empty())
      return do_err(ctx, "Blank input");

  do_pack_value_loop_:
    if(stack.size() > 32)
      return do_err(ctx, "Nesting limit exceeded");

    if(token[0] == '[') {
      // array
      do_token(token, ctx, usrc);
      if(ctx.eof)
        return do_err(ctx, "Array not terminated properly");

      if(token[0] != ']') {
        // open
        auto& frm = stack.emplace_back();
        frm.target = pstor;
        frm.psa = &(pstor->open_array());

        // first
        pstor = &(frm.psa->emplace_back());
        goto do_pack_value_loop_;
      }

      // empty
      pstor->open_array();
    }
    else if(token[0] == '{') {
      // object
      do_token(token, ctx, usrc);
      if(ctx.eof)
        return do_err(ctx, "Object not terminated properly");

      if(token[0] != '}') {
        // open
        auto& frm = stack.emplace_back();
        frm.target = pstor;
        frm.pso = &(pstor->open_object());

        // We are inside an object, so this token must be a key string, followed
        // by a colon, followed by its value.
        if(token[0] != '\"')
          return do_err(ctx, "Missing key string");

        auto emr = frm.pso->try_emplace(token.substr(1));
        ROCKET_ASSERT(emr.second);

        do_token(token, ctx, usrc);
        if(token[0] != ':')
          return do_err(ctx, "Missing colon");

        do_token(token, ctx, usrc);
        if(ctx.eof)
          return do_err(ctx, "Missing value");

        // first
        pstor = &(emr.first->second);
        goto do_pack_value_loop_;
      }

      // empty
      pstor->open_object();
    }
    else if(is_any(token[0], '+', '-') || is_within(token[0], '0', '9')) {
      // number
      size_t n = numg.parse_DD(token.data(), token.size());
      ROCKET_ASSERT(n == token.size());
      numg.cast_D(pstor->open_real(), -DBL_MAX, DBL_MAX);
      if(numg.overflowed())
        return do_err(ctx, "Number value out of range");
    }
    else if(token[0] == '\"') {
      // string
      pstor->open_string().assign(token.data() + 1, token.size() - 1);
    }
    else if(token == "null")
      *pstor = nullopt;
    else if(token == "true")
      *pstor = true;
    else if(token == "false")
      *pstor = false;
    else
      return do_err(ctx, "Invalid token");

    while(!stack.empty()) {
      const auto& frm = stack.back();
      if(frm.psa) {
        // array
        do_token(token, ctx, usrc);
        if(ctx.eof)
          return do_err(ctx, "Array not terminated properly");

        if(token[0] != ']') {
          if(token[0] != ',')
            return do_err(ctx, "Missing comma or closed bracket");

          do_token(token, ctx, usrc);
          if(ctx.eof)
            return do_err(ctx, "Missing value");

          if(token[0] != ']') {
            // next
            pstor = &(frm.psa->emplace_back());
            goto do_pack_value_loop_;
          }
        }
      }
      else {
        // object
        do_token(token, ctx, usrc);
        if(ctx.eof)
          return do_err(ctx, "Object not terminated properly");

        if(token[0] != '}') {
          if(token[0] != ',')
            return do_err(ctx, "Missing comma or closed brace");

          do_token(token, ctx, usrc);
          if(ctx.eof)
            return do_err(ctx, "Missing key string");

          if(token[0] != '}') {
            // We are inside an object, so this token must be a key string,
            // followed by a colon, followed by its value.
            if(token[0] != '\"')
              return do_err(ctx, "Missing key string");

            auto emr = frm.pso->try_emplace(token.substr(1));
            if(!emr.second)
              return do_err(ctx, "Duplicate key string");

            do_token(token, ctx, usrc);
            if(token[0] != ':')
              return do_err(ctx, "Missing colon");

            do_token(token, ctx, usrc);
            if(ctx.eof)
              return do_err(ctx, "Missing value");

            // next
            pstor = &(emr.first->second);
            goto do_pack_value_loop_;
          }
        }
      }

      // close
      pstor = frm.target;
      stack.pop_back();
    }

    do_token(token, ctx, usrc);
    if(token != "")
      return do_err(ctx, "Excess data after value");
  }

ROCKET_FLATTEN
void
do_escape_string_utf16(const Unified_Sink& usink, const cow_string& str)
  {
    auto bptr = str.data();
    const auto eptr = str.data() + str.size();
    for(;;) {
      // Get a sequence of characters that require no escaping.
      auto tptr = bptr;
      while(eptr != tptr) {
        if(is_any(*tptr, '\\', '\"', '/') || !is_within(*tptr, 0x20, 0x7E))
          break;
        ++ tptr;
      }

      if(tptr != bptr)
        usink.putn(bptr, static_cast<size_t>(tptr - bptr));
      bptr = tptr;

      if(bptr == eptr)
        break;

      int next = static_cast<uint8_t>(*(bptr ++));
      switch(next)
        {
        case '\\':
        case '\"':
        case '/':
          {
            char temp[2] = { '\\', *tptr };
            usink.putn(temp, 2);
          }
          break;

        case '\b':
          usink.putn("\\b", 2);
          break;

        case '\f':
          usink.putn("\\f", 2);
          break;

        case '\n':
          usink.putn("\\n", 2);
          break;

        case '\r':
          usink.putn("\\r", 2);
          break;

        case '\t':
          usink.putn("\\t", 2);
          break;

        default:
          if(is_within(next, 0x20, 0x7E))
            usink.putc(*tptr);
          else {
            // Read a UTF-8 character.
            char32_t c32;
            if(!utf8_decode(c32, tptr, static_cast<size_t>(eptr - tptr))) {
              // The input string is invalid. Consume one byte anyway, but print
              // a replacement character.
              usink.putn("\\uFFFD", 6);
            }
            else {
              // Re-encode the character in UTF-16. This will not fail.
              char16_t c16s[2];
              char16_t* c16ptr = c16s;
              utf16_encode(c16ptr, c32);

              char temp[16] = "\\u1234\\u5678zzz";
              uint32_t tlen = 6;
              for(uint32_t k = 2;  k != 6;  ++k) {
                int c = c16s[0] >> 12;
                c16s[0] = static_cast<uint16_t>(c16s[0] << 4);
                if(c < 10)
                  temp[k] = static_cast<char>(c + '0');
                else
                  temp[k] = static_cast<char>(c - 10 + 'A');
              }

              if(c16ptr - c16s > 1) {
                // Write a trailing surrogate.
                tlen = 12;
                for(uint32_t k = 8;  k != 12;  ++k) {
                  int c = c16s[1] >> 12;
                  c16s[1] = static_cast<uint16_t>(c16s[1] << 4);
                  if(c < 10)
                    temp[k] = static_cast<char>(c + '0');
                  else
                    temp[k] = static_cast<char>(c - 10 + 'A');
                }
              }

              usink.putn(temp, tlen);
              bptr = tptr;
            }
          }
        }
    }
  }

void
do_print_to(const Unified_Sink& usink, Indent& indent, const Value& root)
  {
    // Break deep recursion with a handwritten stack.
    struct xFrame
      {
        const V_array* psa;
        V_array::const_iterator ita;
        const V_object* pso;
        V_object::const_iterator ito;
      };

    cow_vector<xFrame> stack;
    ::rocket::ascii_numput nump;
    const Value* pstor = &root;

  do_unpack_loop_:
    switch(static_cast<uint32_t>(pstor->type()))
      {
      case type_null:
        usink.putn("null", 4);
        break;

      case type_array:
        if(!pstor->as_array().empty()) {
          // open
          auto& frm = stack.emplace_back();
          frm.psa = &(pstor->as_array());
          frm.ita = frm.psa->begin();
          usink.putc('[');
          indent.cur.append(indent.add);
          usink.putn(indent.cur.data(), indent.cur.size());

          pstor = &*(frm.ita);
          goto do_unpack_loop_;
        }

        usink.putn("[]", 2);
        break;

      case type_object:
        if(!pstor->as_object().empty()) {
          // open
          auto& frm = stack.emplace_back();
          frm.pso = &(pstor->as_object());
          frm.ito = frm.pso->begin();
          usink.putc('{');
          indent.cur.append(indent.add);
          usink.putn(indent.cur.data(), indent.cur.size());

          usink.putc('\"');
          do_escape_string_utf16(usink, frm.ito->first.rdstr());
          usink.putn("\": ", indent.cur.empty() ? 2U : 3U);
          pstor = &(frm.ito->second);
          goto do_unpack_loop_;
        }

        usink.putn("{}", 2);
        break;

      case type_boolean:
        nump.put_TB(pstor->as_boolean());
        usink.putn(nump.data(), nump.size());
        break;

      case type_integer:
        // as floating-point number; inaccurate for large values
        nump.put_DD(static_cast<V_real>(pstor->as_integer()));
        usink.putn(nump.data(), nump.size());
        break;

      case type_real:
        if(::std::isfinite(pstor->as_real())) {
          // finite; unquoted
          nump.put_DD(pstor->as_real());
          usink.putn(nump.data(), nump.size());
        }
        else {
          // invalid; nullified
          usink.putn("null", 4);
        }
        break;

      case type_string:
        // general; quoted
        usink.putc('\"');
        do_escape_string_utf16(usink, pstor->as_string());
        usink.putc('\"');
        break;

      default:
        // invalid; nullified
        usink.putn("null", 4);
      }

    while(!stack.empty()) {
      auto& frm = stack.mut_back();
      if(frm.psa) {
        // array
        if(++ frm.ita != frm.psa->end()) {
          // next
          usink.putc(',');
          usink.putn(indent.cur.data(), indent.cur.size());

          pstor = &*(frm.ita);
          goto do_unpack_loop_;
        }

        // end
        indent.cur.pop_back(indent.add.size());
        usink.putn(indent.cur.data(), indent.cur.size());
        usink.putc(']');
      }
      else {
        // object
        if(++ frm.ito != frm.pso->end()) {
          // next
          usink.putc(',');
          usink.putn(indent.cur.data(), indent.cur.size());

          usink.putc('\"');
          do_escape_string_utf16(usink, frm.ito->first.rdstr());
          usink.putn("\": ", indent.cur.empty() ? 2U : 3U);
          pstor = &(frm.ito->second);
          goto do_unpack_loop_;
        }

        // end
        indent.cur.pop_back(indent.add.size());
        usink.putn(indent.cur.data(), indent.cur.size());
        usink.putc('}');
      }

      // close
      stack.pop_back();
    }
  }

void
do_print_to(const Unified_Sink& usink, Indent&& indent, const Value& root)
  {
    do_print_to(usink, indent, root);
  }

}  // namespace

V_string
std_json_format(Value value, optV_string indent)
  {
    V_string str;
    do_print_to(&str, indent.value_or(&""), value);
    return str;
  }

V_string
std_json_format(Value value, V_integer indent)
  {
    V_string str;
    do_print_to(&str, indent, value);
    return str;
  }

void
std_json_format_to_file(V_string path, Value value, optV_string indent)
  {
    ::rocket::tinyfmt_file fmt;
    fmt.open(path.safe_c_str(), tinyfmt::open_write | tinyfmt::open_binary);
    do_print_to(fmt.get_handle(), indent.value_or(&""), value);
    fmt.flush();
  }

void
std_json_format_to_file(V_string path, Value value, V_integer indent)
  {
    ::rocket::tinyfmt_file fmt;
    fmt.open(path.safe_c_str(), tinyfmt::open_write | tinyfmt::open_binary);
    do_print_to(fmt.get_handle(), indent, value);
    fmt.flush();
  }

Value
std_json_parse(V_string text)
  {
    Value value;
    Memory_Source msrc(text.data(), text.size());
    do_parse_from(value, &msrc);
    return value;
  }

Value
std_json_parse_file(V_string path)
  {
    Value value;
    ::rocket::tinyfmt_file fmt;
    fmt.open(path.safe_c_str(), tinyfmt::open_read | tinyfmt::open_binary);
    do_parse_from(value, fmt.get_handle());
    return value;
  }

void
create_bindings_json(V_object& result, API_Version version)
  {
    result.insert_or_assign(&"format",
      ASTERIA_BINDING(
        "std.json.format", "[value], [indent]",
        Argument_Reader&& reader)
      {
        Value value;
        optV_string sind;
        V_integer iind;

        reader.start_overload();
        reader.optional(value);
        reader.save_state(0);
        reader.optional(sind);
        if(reader.end_overload())
          return (Value) std_json_format(value, sind);

        reader.load_state(0);
        reader.required(iind);
        if(reader.end_overload())
          return (Value) std_json_format(value, iind);

        reader.throw_no_matching_function_call();
      });

    if(version >= api_version_0002_0000)
      result.insert_or_assign(&"format_to_file",
        ASTERIA_BINDING(
          "std.json.format_to_file", "path, [value], [indent]",
          Argument_Reader&& reader)
        {
          V_string path;
          Value value;
          optV_string sind;
          V_integer iind;

          reader.start_overload();
          reader.required(path);
          reader.optional(value);
          reader.save_state(0);
          reader.optional(sind);
          if(reader.end_overload())
            return (void) std_json_format_to_file(path, value, sind);

          reader.load_state(0);
          reader.required(iind);
          if(reader.end_overload())
            return (void) std_json_format_to_file(path, value, iind);

          reader.throw_no_matching_function_call();
        });

    result.insert_or_assign(&"parse",
      ASTERIA_BINDING(
        "std.json.parse", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_json_parse(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"parse_file",
      ASTERIA_BINDING(
        "std.json.parse_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_json_parse_file(path);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
