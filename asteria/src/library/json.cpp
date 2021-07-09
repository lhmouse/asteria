// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "json.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/runtime_error.hpp"
#include "../runtime/global_context.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/compiler_error.hpp"
#include "../compiler/enums.hpp"
#include "../utils.hpp"

namespace asteria {
namespace {

struct Indenter
  {
    virtual
    ~Indenter();

    virtual tinyfmt&
    break_line(tinyfmt& fmt) const
      = 0;

    virtual void
    increment_level()
      = 0;

    virtual void
    decrement_level()
      = 0;

    virtual bool
    has_indention() const noexcept
      = 0;
  };

Indenter::
~Indenter()
  {
  }

class Indenter_none final
  : public Indenter
  {
  public:
    explicit
    Indenter_none()
      = default;

  public:
    tinyfmt&
    break_line(tinyfmt& fmt) const override
      { return fmt;  }

    void
    increment_level() override
      { }

    void
    decrement_level() override
      { }

    bool
    has_indention() const noexcept override
      { return false;  }
  };

class Indenter_string final
  : public Indenter
  {
  private:
    cow_string m_add;
    cow_string m_cur;

  public:
    explicit
    Indenter_string(const cow_string& add)
      : m_add(add), m_cur(sref("\n"))
      { }

  public:
    tinyfmt&
    break_line(tinyfmt& fmt) const override
      { return fmt << this->m_cur;  }

    void
    increment_level() override
      { this->m_cur.append(this->m_add);  }

    void
    decrement_level() override
      { this->m_cur.pop_back(this->m_add.size());  }

    bool
    has_indention() const noexcept override
      { return this->m_add.size();  }
  };

class Indenter_spaces final
  : public Indenter
  {
  private:
    size_t m_add;
    size_t m_cur;

  public:
    explicit
    Indenter_spaces(int64_t add)
      : m_add(::rocket::clamp_cast<size_t>(add, 0, 10)), m_cur(0)
      { }

  public:
    tinyfmt&
    break_line(tinyfmt& fmt) const override
      { return fmt << pwrap(this->m_add, this->m_cur);  }

    void
    increment_level() override
      { this->m_cur += this->m_add;  }

    void
    decrement_level() override
      { this->m_cur -= this->m_add;  }

    bool
    has_indention() const noexcept override
      { return this->m_add;  }
  };

tinyfmt&
do_quote_string(tinyfmt& fmt, const cow_string& str)
  {
    // Although JavaScript uses UCS-2 rather than UTF-16, the JSON specification adopts UTF-16.
    fmt << '\"';
    size_t offset = 0;
    while(offset < str.size()) {
      // Convert UTF-8 to UTF-16.
      char32_t cp;
      if(!utf8_decode(cp, str, offset))
        // Invalid UTF-8 code units are replaced with the replacement character.
        cp = 0xFFFD;

      // Escape double quotes, backslashes, and control characters.
      switch(cp) {
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

        default: {
          if((0x20 <= cp) && (cp <= 0x7E)) {
            // Write printable characters as is.
            fmt << static_cast<char>(cp);
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
            ::std::memcpy(seq + 2, nump.data() + 2, 4);
            fmt << sref(seq, 6);
          }
          break;
        }
      }
    }
    fmt << '\"';
    return fmt;
  }

tinyfmt&
do_format_object_key(tinyfmt& fmt, bool json5, const Indenter& indent, const cow_string& name)
  {
    // Write the key.
    if(json5 && name.size() && is_cctype(name[0], cctype_namei) &&
                ::std::all_of(name.begin() + 1, name.end(),
                              [](char c) { return is_cctype(c, cctype_namei | cctype_digit);  }))
      fmt << name;
    else
      do_quote_string(fmt, name);

    // Write the colon.
    if(indent.has_indention())
      fmt << ": ";
    else
      fmt << ':';

    return fmt;
  }

bool
do_find_uncensored(V_object::const_iterator& curp, const V_object& object)
  {
    for(;;)
      if(curp == object.end())
        return false;
      else if(::rocket::is_any_of(curp->second.type(),
                     { type_null, type_boolean, type_integer, type_real, type_string,
                       type_array, type_object }))
        return true;
      else
        ++curp;
  }

struct S_xformat_array
  {
    const V_array* refa;
    V_array::const_iterator curp;
  };

struct S_xformat_object
  {
    const V_object* refo;
    V_object::const_iterator curp;
  };

using Xformat = ::rocket::variant<S_xformat_array, S_xformat_object>;

V_string
do_format_nonrecursive(const Value& value, bool json5, Indenter& indent)
  {
    ::rocket::tinyfmt_str fmt;

    // Transform recursion to iteration using a handwritten stack.
    auto qval = &value;
    cow_vector<Xformat> stack;

    for(;;) {
      // Format a value. `qval` must always point to a valid value here.
      switch(weaken_enum(qval->type())) {
        case type_boolean:
          // Write `true` or `false`.
          fmt << qval->as_boolean();
          break;

        case type_integer:
          // Write the integer in decimal.
          fmt << static_cast<double>(qval->as_integer());
          break;

        case type_real: {
          double real = qval->as_real();
          if(::std::isfinite(real)) {
            // Write the real in decimal.
            fmt << real;
          }
          else if(!json5) {
            // Censor the value.
            fmt << "null";
          }
          else if(!::std::isnan(real)) {
            // JSON5 allows infinities in ECMAScript form.
            fmt << "Infinity";
          }
          else {
            // JSON5 allows NaNs in ECMAScript form.
            fmt << "NaN";
          }
          break;
        }

        case type_string:
          // Write the quoted string.
          do_quote_string(fmt, qval->as_string());
          break;

        case type_array: {
          const auto& array = qval->as_array();
          fmt << '[';

          // Open an array.
          S_xformat_array ctxa = { ::std::addressof(array), array.begin() };
          if(ctxa.curp != array.end()) {
            indent.increment_level();
            indent.break_line(fmt);

            // Descend into the array.
            qval = &(ctxa.curp[0]);
            stack.emplace_back(::std::move(ctxa));
            continue;
          }

          // Write an empty array.
          fmt << ']';
          break;
        }

        case type_object: {
          const auto& object = qval->as_object();
          fmt << '{';

          // Open an object.
          S_xformat_object ctxo = { ::std::addressof(object), object.begin() };
          if(do_find_uncensored(ctxo.curp, object)) {
            indent.increment_level();
            indent.break_line(fmt);

            // Write the key followed by a colon.
            do_format_object_key(fmt, json5, indent, ctxo.curp->first);

            // Descend into the object.
            qval = &(ctxo.curp->second);
            stack.emplace_back(::std::move(ctxo));
            continue;
          }

          // Write an empty object.
          fmt << '}';
          break;
        }

        default:
          // Anything else is censored to `null`.
          fmt << "null";
          break;
      }

      // A complete value has been written. Advance to the next element if any.
      for(;;) {
        if(stack.empty())
          // Finish the root value.
          return fmt.extract_string();

        // Advance to the next element.
        if(stack.back().index() == 0) {
          auto& ctxa = stack.mut_back().as<0>();
          if(++(ctxa.curp) != ctxa.refa->end()) {
            fmt << ',';
            indent.break_line(fmt);

            // Format the next element.
            qval = &(ctxa.curp[0]);
            break;
          }

          // Close this array.
          if(json5 && indent.has_indention())
            fmt << ',';

          indent.decrement_level();
          indent.break_line(fmt);
          fmt << ']';
        }
        else {
          auto& ctxo = stack.mut_back().as<1>();
          if(do_find_uncensored(++(ctxo.curp), *(ctxo.refo))) {
            fmt << ',';
            indent.break_line(fmt);

            // Write the key followed by a colon.
            do_format_object_key(fmt, json5, indent, ctxo.curp->first);

            // Format the next value.
            qval = &(ctxo.curp->second);
            break;
          }

          // Close this object.
          if(json5 && indent.has_indention())
            fmt << ',';

          indent.decrement_level();
          indent.break_line(fmt);
          fmt << '}';
        }
        stack.pop_back();
      }
    }
  }

V_string
do_format_nonrecursive(const Value& value, bool json5, Indenter&& indent)
  {
    return do_format_nonrecursive(value, json5, indent);
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

struct S_xparse_array
  {
    V_array arr;
  };

struct S_xparse_object
  {
    V_object obj;
    phsh_string key;
    Source_Location key_sloc;
  };

using Xparse = ::rocket::variant<S_xparse_array, S_xparse_object>;

void
do_accept_object_key(S_xparse_object& ctxo, Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_closed_brace_or_json5_key_expected, tstrm.next_sloc());

    switch(weaken_enum(qtok->index())) {
      case Token::index_identifier:
        ctxo.key = qtok->as_identifier();
        break;

      case Token::index_string_literal:
        ctxo.key = qtok->as_string_literal();
        break;

      default:
        throw Compiler_Error(Compiler_Error::M_status(),
                  compiler_status_closed_brace_or_json5_key_expected, tstrm.next_sloc());
    }
    ctxo.key_sloc = qtok->sloc();
    tstrm.shift();

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
    if(!kpunct)
      throw Compiler_Error(Compiler_Error::M_status(),
                compiler_status_colon_expected, tstrm.next_sloc());
  }

Value
do_json_parse_nonrecursive(Token_Stream& tstrm)
  {
    Value value;

    // Implement a non-recursive descent parser.
    cow_vector<Xparse> stack;

    for(;;) {
      // Accept a value. No other things such as closed brackets are allowed.
      auto qtok = tstrm.peek_opt();
      if(!qtok)
        throw Compiler_Error(Compiler_Error::M_format(),
                  compiler_status_expression_expected, tstrm.next_sloc(),
                  "value expected");

      switch(weaken_enum(qtok->index())) {
        case Token::index_punctuator: {
          // Accept an `[` or `{`.
          auto punct = qtok->as_punctuator();
          switch(weaken_enum(punct)) {
            case punctuator_bracket_op: {
              tstrm.shift();

              // Open an array.
              auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
              if(!kpunct) {
                // Descend into the new array.
                S_xparse_array ctxa = { };
                stack.emplace_back(::std::move(ctxa));
                continue;
              }

              // Accept an empty array.
              value = V_array();
              break;
            }

            case punctuator_brace_op: {
              tstrm.shift();

              // Open an object.
              auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
              if(!kpunct) {
                // Descend into the new object.
                S_xparse_object ctxo = { };
                do_accept_object_key(ctxo, tstrm);
                stack.emplace_back(::std::move(ctxo));
                continue;
              }

              // Accept an empty object.
              value = V_object();
              break;
            }

            default:
              throw Compiler_Error(Compiler_Error::M_format(),
                        compiler_status_expression_expected, tstrm.next_sloc(),
                        "value expected");
          }
          break;
        }

        case Token::index_identifier: {
          // Accept a literal.
          const auto& name = qtok->as_identifier();
          if(::rocket::is_none_of(name, { "null", "true", "false", "Infinity", "NaN" }))
            throw Compiler_Error(Compiler_Error::M_format(),
                      compiler_status_expression_expected, tstrm.next_sloc(),
                      "value expected");

          switch(name[0]) {
            case 'n':
              value = nullopt;
              break;

            case 't':
              value = true;
              break;

            case 'f':
              value = false;
              break;

            case 'I':
              value = ::std::numeric_limits<double>::infinity();
              break;

            case 'N':
              value = ::std::numeric_limits<double>::quiet_NaN();
              break;

            default:
              ROCKET_ASSERT(false);
          }
          tstrm.shift();
          break;
        }

        case Token::index_real_literal:
          // Accept a number.
          value = qtok->as_real_literal();
          tstrm.shift();
          break;

        case Token::index_string_literal:
          // Accept a UTF-8 string.
          value = qtok->as_string_literal();
          tstrm.shift();
          break;

        default:
          throw Compiler_Error(Compiler_Error::M_format(),
                    compiler_status_expression_expected, tstrm.next_sloc(),
                    "value expected");
      }

      // A complete value has been accepted. Insert it into its parent array or object.
      for(;;) {
        if(stack.empty())
          // Accept the root value.
          return value;

        if(stack.back().index() == 0) {
          auto& ctxa = stack.mut_back().as<0>();
          ctxa.arr.emplace_back(::std::move(value));

          // Look for the next element.
          auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl, punctuator_comma });
          if(!kpunct)
            throw Compiler_Error(Compiler_Error::M_status(),
                      compiler_status_closed_bracket_or_comma_expected, tstrm.next_sloc());

          // Check for termination of this array.
          if(*kpunct != punctuator_bracket_cl) {
            kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
            if(!kpunct) {
              // Look for the next element.
              break;
            }
          }

          // Close this array.
          value = ::std::move(ctxa.arr);
        }
        else {
          auto& ctxo = stack.mut_back().as<1>();
          auto pair = ctxo.obj.try_emplace(::std::move(ctxo.key), ::std::move(value));
          if(!pair.second)
            throw Compiler_Error(Compiler_Error::M_status(),
                      compiler_status_duplicate_key_in_object, ctxo.key_sloc);

          // Look for the next element.
          auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl, punctuator_comma });
          if(!kpunct)
            throw Compiler_Error(Compiler_Error::M_status(),
                      compiler_status_closed_brace_or_comma_expected, tstrm.next_sloc());

          // Check for termination of this array.
          if(*kpunct != punctuator_brace_cl) {
            kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
            if(!kpunct) {
              // Look for the next element.
              do_accept_object_key(ctxo, tstrm);
              break;
            }
          }

          // Close this object.
          value = ::std::move(ctxo.obj);
        }
        stack.pop_back();
      }
    }
  }

Value
do_json_parse(tinybuf& cbuf)
  try {
    // We reuse the lexer of Asteria here, allowing quite a few extensions e.g. binary numeric
    // literals and comments.
    Compiler_Options opts;
    opts.escapable_single_quotes = true;
    opts.keywords_as_identifiers = true;
    opts.integers_as_reals = true;

    Token_Stream tstrm(opts);
    tstrm.reload(sref("[JSON text]"), 1, cbuf);
    if(tstrm.empty())
      ASTERIA_THROW_RUNTIME_ERROR("empty JSON string");

    // Parse a single value.
    auto value = do_json_parse_nonrecursive(tstrm);
    if(!tstrm.empty())
      ASTERIA_THROW_RUNTIME_ERROR("excess text at end of JSON string");
    return value;
  }
  catch(Compiler_Error& except) {
    ASTERIA_THROW_RUNTIME_ERROR("invalid JSON string: $3 (line $1, column $2)",
                  except.line(), except.column(), describe_compiler_status(except.status()));
  }

}  // namespace

V_string
std_json_format(Value value, Opt_string indent)
  {
    // No line break is inserted if `indent` is null or empty.
    return (!indent || indent->empty())
               ? do_format_nonrecursive(value, false, Indenter_none())
               : do_format_nonrecursive(value, false, Indenter_string(*indent));
  }

V_string
std_json_format(Value value, V_integer indent)
  {
    // No line break is inserted if `indent` is non-positive.
    return (indent <= 0)
               ? do_format_nonrecursive(value, false, Indenter_none())
               : do_format_nonrecursive(value, false, Indenter_spaces(indent));
  }

V_string
std_json_format5(Value value, Opt_string indent)
  {
    // No line break is inserted if `indent` is null or empty.
    return (!indent || indent->empty())
               ? do_format_nonrecursive(value, true, Indenter_none())
               : do_format_nonrecursive(value, true, Indenter_string(*indent));
  }

V_string
std_json_format5(Value value, V_integer indent)
  {
    // No line break is inserted if `indent` is non-positive.
    return (indent <= 0)
               ? do_format_nonrecursive(value, true, Indenter_none())
               : do_format_nonrecursive(value, true, Indenter_spaces(indent));
  }

Value
std_json_parse(V_string text)
  {
    // Parse characters from the string.
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(text, tinybuf::open_read);
    return do_json_parse(cbuf);
  }

Value
std_json_parse_file(V_string path)
  {
    // Try opening the file.
    ::rocket::unique_posix_file fp(::fopen(path.safe_c_str(), "rb"), ::fclose);
    if(!fp)
      ASTERIA_THROW_RUNTIME_ERROR("could not open file '$2'\n"
                    "[`fopen()` failed: $1]",
                    format_errno(errno), path);

    // Parse characters from the file.
    ::rocket::tinybuf_file cbuf(::std::move(fp));
    return do_json_parse(cbuf);
  }

void
create_bindings_json(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("format"),
      ASTERIA_BINDING_BEGIN("std.json.format", self, global, reader) {
        Value value;
        Opt_string sind;
        V_integer iind;

        reader.start_overload();
        reader.optional(value);   // [value]
        reader.save_state(0);
        reader.optional(sind);    // [indent]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_json_format, value, sind);

        reader.load_state(0);     // [value]
        reader.required(iind);    // indent
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_json_format, value, iind);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("format5"),
      ASTERIA_BINDING_BEGIN("std.json.format5", self, global, reader) {
        Value value;
        Opt_string sind;
        V_integer iind;

        reader.start_overload();
        reader.optional(value);   // [value]
        reader.save_state(0);
        reader.optional(sind);    // [indent]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_json_format5, value, sind);

        reader.load_state(0);     // [value]
        reader.required(iind);    // indent
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_json_format5, value, iind);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("parse"),
      ASTERIA_BINDING_BEGIN("std.json.parse", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // text
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_json_parse, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("parse_file"),
      ASTERIA_BINDING_BEGIN("std.json.parse_file", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);    // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_json_parse_file, path);
      }
      ASTERIA_BINDING_END);
  }

}  // namespace asteria
