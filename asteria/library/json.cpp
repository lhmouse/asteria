// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "json.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/binding_generator.hpp"
#include "../runtime/global_context.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/compiler_error.hpp"
#include "../compiler/enums.hpp"
#include "../utils.hpp"
#include <algorithm>
namespace asteria {
namespace {

struct Indenter
  {
    virtual ~Indenter();

    virtual
    void
    break_line(tinyfmt& fmt)
      const = 0;

    virtual
    void
    increment_level()
      = 0;

    virtual
    void
    decrement_level()
      = 0;

    virtual
    bool
    has_indention()
      const noexcept = 0;
  };

Indenter::
~Indenter()
  {
  }

struct Indenter_none : Indenter
  {
    Indenter_none()
      {
      }

    void
    break_line(tinyfmt& /*fmt*/)
      const override
      {
      }

    void
    increment_level()
      override
      {
      }

    void
    decrement_level()
      override
      {
      }

    bool
    has_indention()
      const noexcept override
      {
        return false;
      }
  };

struct Indenter_string : Indenter
  {
    cow_string add;
    cow_string cur;

    explicit Indenter_string(const cow_string& xadd)
      {
        this->add = xadd;
        this->cur = &"\n";
      }

    void
    break_line(tinyfmt& fmt)
      const override
      {
        fmt << this->cur;
      }

    void
    increment_level()
      override
      {
        this->cur.append(this->add);
      }

    void
    decrement_level()
      override
      {
        this->cur.pop_back(this->add.size());
      }

    bool
    has_indention()
      const noexcept override
      {
        return this->add.size() != 0;
      }
  };

struct Indenter_spaces : Indenter
  {
    size_t add;
    size_t cur;

    explicit Indenter_spaces(int64_t xadd)
      {
        this->add = ::rocket::clamp_cast<size_t>(xadd, 0, 10);
        this->cur = 0;
      }

    void
    break_line(tinyfmt& fmt)
      const override
      {
        static constexpr char s_spaces[] =
          {
#define do_spaces_8_   ' ',' ',' ',' ',' ',' ',' ',' ',
#define do_spaces_32_  do_spaces_8_ do_spaces_8_ do_spaces_8_ do_spaces_8_
            do_spaces_32_ do_spaces_32_ do_spaces_32_ do_spaces_32_
            do_spaces_32_ do_spaces_32_ do_spaces_32_ do_spaces_32_
          };

        // When `step` is zero, separate fields with a single space.
        if(ROCKET_EXPECT(this->add == 0)) {
          fmt << s_spaces[0];
          return;
        }

        // Otherwise, terminate the current line, and indent the next.
        fmt << '\n';
        size_t nrem = this->cur;
        while(nrem != 0) {
          size_t nslen = ::rocket::min(nrem, sizeof(s_spaces));
          nrem -= nslen;
          fmt.putn(s_spaces, nslen);
        }
      }

    void
    increment_level()
      override
      {
        this->cur += this->add;
      }

    void
    decrement_level()
      override
      {
        this->cur -= this->add;
      }

    bool
    has_indention()
      const noexcept override
      {
        return this->add != 0;
      }
  };

void
do_quote_string(tinyfmt& fmt, const cow_string& str)
  {
    // Although JavaScript uses UCS-2 rather than UTF-16, the JSON
    // specification adopts UTF-16.
    fmt << '\"';
    size_t offset = 0;
    while(offset < str.size()) {
      // Convert UTF-8 to UTF-16.
      char32_t cp;
      if(!utf8_decode(cp, str, offset)) {
        cp = 0xFFFD;
        offset ++;
      }

      // Escape double quotes, backslashes, and control characters.
      switch(cp)
        {
        case '\"':
          fmt << "\\\"";
          break;

        case '\\':
          fmt << "\\\\";
          break;

        case '\b':
          fmt << "\\b";
          break;

        case '\f':
          fmt << "\\f";
          break;

        case '\n':
          fmt << "\\n";
          break;

        case '\r':
          fmt << "\\r";
          break;

        case '\t':
          fmt << "\\t";
          break;

        default:
          {
            if((0x20 <= cp) && (cp <= 0x7E)) {
              // Write printable characters as is.
              fmt << (char) cp;
              break;
            }

            // Encode the character in UTF-16.
            char16_t ustr[2];
            char16_t* epos = ustr;
            utf16_encode(epos, cp);

            // Write code units.
            ::rocket::ascii_numput nump;
            for(auto p = ustr;  p != epos;  ++p) {
              nump.put_XU(*p, 4);
              char seq[8] = { "\\u" };
              ::memcpy(seq + 2, nump.data() + 2, 4);
              fmt.putn(seq, 6);
            }
            break;
          }
      }
    }
    fmt << '\"';
  }

void
do_format_object_key(tinyfmt& fmt, const Indenter& indent, const cow_string& name)
  {
    // Write the key.
    if(false && name.size() && is_cmask(name[0], cmask_namei)
         && ::std::all_of(name.begin() + 1, name.end(),
                          [](char c) { return is_cmask(c, cmask_name);  }))
      fmt << name;
    else
      do_quote_string(fmt, name);

    // Write the colon.
    if(indent.has_indention())
      fmt << ": ";
    else
      fmt << ':';
  }

bool
do_find_uncensored(V_object::const_iterator& curp, const V_object& object)
  {
    for(;;)
      if(curp == object.end())
        return false;
      else if(::rocket::is_none_of(curp->second.type(),
                 { type_null, type_boolean, type_integer, type_real, type_string,
                   type_array, type_object }))
        ++ curp;
      else
        return true;
  }

struct Xformat_array
  {
    const V_array* refa;
    V_array::const_iterator curp;
  };

struct Xformat_object
  {
    const V_object* refo;
    V_object::const_iterator curp;
  };

using Xformat = ::rocket::variant<Xformat_array, Xformat_object>;

void
do_format_nonrecursive(::rocket::tinyfmt& fmt, const Value& value, Indenter&& indent)
  {
    // Transform recursion to iteration using a handwritten stack.
    auto qval = &value;
    cow_vector<Xformat> stack;

    // Format a value. `qval` must always point to a valid value here.
  format_next:
    if(qval->is_boolean()) {
      // Write `true` or `false`.
      fmt << qval->as_boolean();
    }
    else if(qval->is_real()) {
      // Write the real number in decimal. JSON5 allows infinities and NaN;
      // otherwise they are replaced with nulls.
      double rval = qval->as_real();
      int cls = ::std::fpclassify(rval);
      if((cls == FP_ZERO) || (cls == FP_NORMAL) || (cls == FP_SUBNORMAL))
        fmt << rval;
      else if((cls == FP_NAN) && false)
        fmt << "NaN";
      else if((cls == FP_INFINITE) && false)
        fmt << ("-Infinity" + !::std::signbit(rval));
      else
        fmt << "null";
    }
    else if(qval->is_string()) {
      // Write the string in double quotes.
      do_quote_string(fmt, qval->as_string());
    }
    else if(qval->is_array()) {
      const auto& array = qval->as_array();
      Xformat_array ctxa = { &array, array.begin() };
      if(ctxa.curp != array.end()) {
        // Open an array.
        fmt << '[';
        indent.increment_level();
        indent.break_line(fmt);

        qval = &*(ctxa.curp);
        stack.emplace_back(move(ctxa));
        goto format_next;
      }
      fmt << "[]";
    }
    else if(qval->is_object()) {
      const auto& object = qval->as_object();
      Xformat_object ctxo = { &object, object.begin() };
      if(do_find_uncensored(ctxo.curp, object)) {
        // Open an object.
        fmt << '{';
        indent.increment_level();
        indent.break_line(fmt);
        do_format_object_key(fmt, indent, ctxo.curp->first);

        qval = &(ctxo.curp->second);
        stack.emplace_back(move(ctxo));
        goto format_next;
      }
      fmt << "{}";
    }
    else
      fmt << "null";

    while(stack.size()) {
      // Advance to the next element.
      auto& ctx = stack.mut_back();
      switch(ctx.index())
        {
        case 0:
          {
            auto& ctxa = ctx.mut<0>();
            ++ ctxa.curp;
            if(ctxa.curp != ctxa.refa->end()) {
              fmt << ',';
              indent.break_line(fmt);

              // Format the next element.
              qval = &*(ctxa.curp);
              goto format_next;
            }

            // Close this array.
            if(false && indent.has_indention())
              fmt << ',';

            indent.decrement_level();
            indent.break_line(fmt);
            fmt << ']';
            break;
          }

        case 1:
          {
            auto& ctxo = ctx.mut<1>();
            if(do_find_uncensored(++(ctxo.curp), *(ctxo.refo))) {
              fmt << ',';
              indent.break_line(fmt);
              do_format_object_key(fmt, indent, ctxo.curp->first);

              // Format the next value.
              qval = &(ctxo.curp->second);
              goto format_next;
            }

            // Close this object.
            if(false && indent.has_indention())
              fmt << ',';

            indent.decrement_level();
            indent.break_line(fmt);
            fmt << '}';
            break;
          }

        default:
          ROCKET_ASSERT(false);
      }

      stack.pop_back();
    }
  }

opt<Punctuator>
do_accept_punctuator_opt(Token_Stream& tstrm, initializer_list<Punctuator> accept)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      return nullopt;

    if(!qtok->is_punctuator())
      return nullopt;

    auto punct = qtok->as_punctuator();
    if(::rocket::is_none_of(punct, accept))
      return nullopt;

    tstrm.shift();
    return punct;
  }

struct Xparse_array
  {
    V_array arr;
  };

struct Xparse_object
  {
    V_object obj;
    phcow_string key;
    Source_Location key_sloc;
  };

using Xparse = ::rocket::variant<Xparse_array, Xparse_object>;

void
do_accept_object_key(Xparse_object& ctxo, Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      ASTERIA_THROW(("`}` or key expected at '$1'"), tstrm.next_sloc());

    if(qtok->is_identifier())
      ctxo.key = qtok->as_identifier();
    else if(qtok->is_string_literal())
      ctxo.key = qtok->as_string_literal();
    else
      ASTERIA_THROW(("identifier or string expected at '$1'"), tstrm.next_sloc());

    ctxo.key_sloc = qtok->sloc();
    tstrm.shift();

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
    if(!kpunct)
      ASTERIA_THROW(("`:` expected at '$1'"), tstrm.next_sloc());
  }

Value
do_parse_nonrecursive(Token_Stream& tstrm)
  {
    // Implement a non-recursive descent parser.
    Value value;
    cow_vector<Xparse> stack;

    // Accept a value. No other things such as closed brackets are allowed.
  parse_next:
    if(stack.size() > 32)
      ASTERIA_THROW(("Nesting limit exceeded at '$1'"), tstrm.next_sloc());

    auto qtok = tstrm.peek_opt();
    if(!qtok)
      ASTERIA_THROW(("Value expected at '$1'"), tstrm.next_sloc());

    if(qtok->is_punctuator()) {
      // Accept an `[` or `{`.
      if(qtok->as_punctuator() == punctuator_bracket_op) {
        tstrm.shift();

        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
        if(!kpunct) {
          stack.emplace_back(Xparse_array());
          goto parse_next;
        }

        // Accept an empty array.
        value = V_array();
      }
      else if(qtok->as_punctuator() == punctuator_brace_op) {
        tstrm.shift();

        auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
        if(!kpunct) {
          stack.emplace_back(Xparse_object());
          do_accept_object_key(stack.mut_back().mut<Xparse_object>(), tstrm);
          goto parse_next;
        }

        // Accept an empty object.
        value = V_object();
      }
      else
        ASTERIA_THROW(("Value expected at '$1'"), tstrm.next_sloc());
    }
    else if(qtok->is_identifier()) {
      // Accept a literal.
      if(qtok->as_identifier() == "null") {
        tstrm.shift();
        value = nullopt;
      }
      else if(qtok->as_identifier() == "true") {
        tstrm.shift();
        value = true;
      }
      else if(qtok->as_identifier() == "false") {
        tstrm.shift();
        value = false;
      }
      else if(qtok->as_identifier() == "Infinity") {
        tstrm.shift();
        value = ::std::numeric_limits<double>::infinity();
      }
      else if(qtok->as_identifier() == "NaN") {
        tstrm.shift();
        value = ::std::numeric_limits<double>::quiet_NaN();
      }
      else
        ASTERIA_THROW(("Value expected at '$1'"), tstrm.next_sloc());
    }
    else if(qtok->is_real_literal()) {
      // Accept a number.
      value = qtok->as_real_literal();
      tstrm.shift();
    }
    else if(qtok->is_string_literal()) {
      // Accept a UTF-8 string.
      value = qtok->as_string_literal();
      tstrm.shift();
    }
    else
      ASTERIA_THROW(("Value expected at '$1'"), tstrm.next_sloc());

    while(stack.size()) {
      // Advance to the next element.
      auto& ctx = stack.mut_back();
      switch(ctx.index())
        {
        case 0:
          {
            auto& ctxa = ctx.mut<Xparse_array>();
            ctxa.arr.emplace_back(move(value));

            // Look for the next element.
            auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl, punctuator_comma });
            if(!kpunct)
              ASTERIA_THROW(("`]` or `,` expected at '$1'"), tstrm.next_sloc());

            if(*kpunct == punctuator_comma) {
              // A closing bracket may still follow.
              kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
              if(!kpunct)
                goto parse_next;
            }

            // Close this array.
            value = move(ctxa.arr);
            break;
          }

        case 1:
          {
            auto& ctxo = ctx.mut<Xparse_object>();
            auto pair = ctxo.obj.try_emplace(move(ctxo.key), move(value));
            if(!pair.second)
              ASTERIA_THROW(("Duplicate key encountered at '$1'"), tstrm.next_sloc());

            // Look for the next element.
            auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl, punctuator_comma });
            if(!kpunct)
              ASTERIA_THROW(("`}` or `,` expected at '$1'"), tstrm.next_sloc());

            if(*kpunct == punctuator_comma) {
              // A closing brace may still follow.
              kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
              if(!kpunct) {
                do_accept_object_key(stack.mut_back().mut<Xparse_object>(), tstrm);
                goto parse_next;
              }
            }

            // Close this object.
            value = move(ctxo.obj);
            break;
          }

        default:
          ROCKET_ASSERT(false);
      }

      stack.pop_back();
    }

    return value;
  }

Value
do_parse(tinyfmt& cbuf)
  {
    // We reuse the lexer of Asteria here, allowing quite a few extensions e.g. binary numeric
    // literals and comments.
    Compiler_Options opts;
    opts.escapable_single_quotes = true;
    opts.keywords_as_identifiers = true;
    opts.integers_as_reals = true;

    Token_Stream tstrm(opts);
    try {
      tstrm.reload(&"[JSON text]", 1, move(cbuf));
    }
    catch(Compiler_Error& error) {
      ASTERIA_THROW(("Invalid token at '$1'"), error.sloc());
    }

    if(tstrm.empty())
      ASTERIA_THROW(("Empty JSON string"));

    // Parse a single value.
    auto value = do_parse_nonrecursive(tstrm);
    if(!tstrm.empty())
      ASTERIA_THROW(("Excess text at end of JSON string"));

    return value;
  }

}  // namespace

V_string
std_json_format(Value value, optV_string indent)
  {
    ::rocket::tinyfmt_str fmt;
    if(indent && (indent->length() != 0))
      do_format_nonrecursive(fmt, value, Indenter_string(*indent));
    else
      do_format_nonrecursive(fmt, value, Indenter_none());
    return fmt.extract_string();
  }

V_string
std_json_format(Value value, V_integer indent)
  {
    ::rocket::tinyfmt_str fmt;
    if(indent > 0)
      do_format_nonrecursive(fmt, value, Indenter_spaces(indent));
    else
      do_format_nonrecursive(fmt, value, Indenter_none());
    return fmt.extract_string();
  }

void
std_json_format_to_file(V_string path, Value value, optV_string indent)
  {
    ::rocket::tinyfmt_file fmt;
    fmt.open(path.safe_c_str(), tinyfmt::open_write);
    if(indent && (indent->length() != 0))
      do_format_nonrecursive(fmt, value, Indenter_string(*indent));
    else
      do_format_nonrecursive(fmt, value, Indenter_none());
    fmt.flush();
  }

void
std_json_format_to_file(V_string path, Value value, V_integer indent)
  {
    ::rocket::tinyfmt_file fmt;
    fmt.open(path.safe_c_str(), tinyfmt::open_write);
    if(indent > 0)
      do_format_nonrecursive(fmt, value, Indenter_spaces(indent));
    else
      do_format_nonrecursive(fmt, value, Indenter_none());
    fmt.flush();
  }

Value
std_json_parse(V_string text)
  {
    ::rocket::tinyfmt_str cbuf;
    cbuf.set_string(text, tinyfmt::open_read);
    return do_parse(cbuf);
  }

Value
std_json_parse_file(V_string path)
  {
    ::rocket::tinyfmt_file cbuf;
    cbuf.open(path.safe_c_str(), tinyfmt::open_read);
    return do_parse(cbuf);
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
