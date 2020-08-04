// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "json.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../compiler/token_stream.hpp"
#include "../compiler/parser_error.hpp"
#include "../compiler/enums.hpp"
#include "../utilities.hpp"

namespace asteria {
namespace {

class Indenter
  {
  public:
    virtual
    ~Indenter();

  public:
    virtual
    tinyfmt&
    break_line(tinyfmt& fmt)
    const
      = 0;

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
    const noexcept
      = 0;
  };

Indenter::
~Indenter()
  {
  }

class Indenter_none
final
  : public Indenter
  {
  public:
    explicit
    Indenter_none()
      = default;

  public:
    tinyfmt&
    break_line(tinyfmt& fmt)
    const override
      { return fmt;  }

    void
    increment_level()
    override
      { }

    void
    decrement_level()
    override
      { }

    bool
    has_indention()
    const noexcept override
      { return false;  }
  };

class Indenter_string
final
  : public Indenter
  {
  private:
    cow_string m_add;
    cow_string m_cur;

  public:
    explicit
    Indenter_string(const cow_string& add)
      : m_add(add), m_cur(::rocket::sref("\n"))
      { }

  public:
    tinyfmt&
    break_line(tinyfmt& fmt)
    const override
      { return fmt << this->m_cur;  }

    void
    increment_level()
    override
      { this->m_cur.append(this->m_add);  }

    void
    decrement_level()
    override
      { this->m_cur.pop_back(this->m_add.size());  }

    bool
    has_indention()
    const noexcept override
      { return this->m_add.size();  }
  };

class Indenter_spaces
final
  : public Indenter
  {
  private:
    size_t m_add;
    size_t m_cur;

  public:
    explicit
    Indenter_spaces(int64_t add)
      : m_add(static_cast<size_t>(::rocket::clamp(add, 0, 10))), m_cur(0)
      { }

  public:
    tinyfmt&
    break_line(tinyfmt& fmt)
    const override
      { return fmt << pwrap(this->m_add, this->m_cur);  }

    void
    increment_level()
    override
      { this->m_cur += this->m_add;  }

    void
    decrement_level()
    override
      { this->m_cur -= this->m_add;  }

    bool
    has_indention()
    const noexcept override
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
            fmt << ::rocket::sref(seq, 6);
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
      else if(::rocket::is_any_of(curp->second.vtype(),
                     { vtype_null, vtype_boolean, vtype_integer, vtype_real, vtype_string,
                       vtype_array, vtype_object }))
        return true;
      else
        ++curp;
  }

struct S_xformat_array
  {
    refp<const V_array> refa;
    V_array::const_iterator curp;
  };

struct S_xformat_object
  {
    refp<const V_object> refo;
    V_object::const_iterator curp;
  };

using Xformat = variant<S_xformat_array, S_xformat_object>;

V_string
do_format_nonrecursive(const Value& value, bool json5, Indenter& indent)
  {
    ::rocket::tinyfmt_str fmt;

    // Transform recursion to iteration using a handwritten stack.
    auto qvalue = ::rocket::ref(value);
    cow_vector<Xformat> stack;

    for(;;) {
      // Format a value. `qvalue` must always point to a valid value here.
      switch(weaken_enum(qvalue->vtype())) {
        case vtype_boolean:
          // Write `true` or `false`.
          fmt << qvalue->as_boolean();
          break;

        case vtype_integer:
          // Write the integer in decimal.
          fmt << static_cast<double>(qvalue->as_integer());
          break;

        case vtype_real: {
          double real = qvalue->as_real();
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

        case vtype_string:
          // Write the quoted string.
          do_quote_string(fmt, qvalue->as_string());
          break;

        case vtype_array: {
          const auto& array = qvalue->as_array();
          fmt << '[';

          // Open an array.
          S_xformat_array ctxa = { ::rocket::ref(array), array.begin() };
          if(ctxa.curp != array.end()) {
            indent.increment_level();
            indent.break_line(fmt);

            // Descend into the array.
            qvalue = ::rocket::ref(ctxa.curp[0]);
            stack.emplace_back(::std::move(ctxa));
            continue;
          }

          // Write an empty array.
          fmt << ']';
          break;
        }

        case vtype_object: {
          const auto& object = qvalue->as_object();
          fmt << '{';

          // Open an object.
          S_xformat_object ctxo = { ::rocket::ref(object), object.begin() };
          if(do_find_uncensored(ctxo.curp, object)) {
            indent.increment_level();
            indent.break_line(fmt);

            // Write the key followed by a colon.
            do_format_object_key(fmt, json5, indent, ctxo.curp->first);

            // Descend into the object.
            qvalue = ::rocket::ref(ctxo.curp->second);
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
            qvalue = ::rocket::ref(ctxa.curp[0]);
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
          if(do_find_uncensored(++(ctxo.curp), ctxo.refo)) {
            fmt << ',';
            indent.break_line(fmt);

            // Write the key followed by a colon.
            do_format_object_key(fmt, json5, indent, ctxo.curp->first);

            // Format the next value.
            qvalue = ::rocket::ref(ctxo.curp->second);
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

phsh_string
do_accept_object_key(Token_Stream& tstrm)
  {
    auto qtok = tstrm.peek_opt();
    if(!qtok)
      throw Parser_Error(parser_status_closed_brace_or_json5_key_expected, tstrm.next_sloc(),
                         tstrm.next_length());

    cow_string name;
    if(qtok->is_identifier()) {
      name = qtok->as_identifier();
    }
    else if(qtok->is_string_literal()) {
      name = qtok->as_string_literal();
    }
    else {
      throw Parser_Error(parser_status_closed_brace_or_json5_key_expected, tstrm.next_sloc(),
                         tstrm.next_length());
    }
    tstrm.shift();

    auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
    if(!kpunct)
      throw Parser_Error(parser_status_colon_expected, tstrm.next_sloc(), tstrm.next_length());
    return name;
  }

struct S_xparse_array
  {
    V_array array;
  };

struct S_xparse_object
  {
    V_object object;
    phsh_string key;
  };

using Xparse = variant<S_xparse_array, S_xparse_object>;

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
        throw Parser_Error(parser_status_expression_expected, tstrm.next_sloc(), tstrm.next_length());

      switch(weaken_enum(qtok->index())) {
        case Token::index_punctuator: {
          // Accept a `+`, `-`, `[` or `{`.
          auto punct = qtok->as_punctuator();
          switch(weaken_enum(punct)) {
            case punctuator_add:
            case punctuator_sub: {
              cow_string name;
              qtok = tstrm.peek_opt(1);
              if(qtok && qtok->is_identifier())
                name = qtok->as_identifier();

              // Only `Infinity` and `NaN` may follow.
              // Note that the tokenizer will have merged sign symbols into adjacent number literals.
              if(::rocket::is_none_of(name, { "Infinity", "NaN" }))
                throw Parser_Error(parser_status_expression_expected, tstrm.next_sloc(),
                                   tstrm.next_length());

              // Accept a special numeric value.
              double sign = (punct == punctuator_add) - 1;
              double real = (name[0] == 'I') ? ::std::numeric_limits<double>::infinity()
                                             : ::std::numeric_limits<double>::quiet_NaN();

              value = ::std::copysign(real, sign);
              tstrm.shift(2);
              break;
            }

            case punctuator_bracket_op: {
              tstrm.shift();

              // Open an array.
              auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
              if(!kpunct) {
                // Descend into the new array.
                S_xparse_array ctxa = { V_array() };
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
                S_xparse_object ctxo = { V_object(), do_accept_object_key(tstrm) };
                stack.emplace_back(::std::move(ctxo));
                continue;
              }

              // Accept an empty object.
              value = V_object();
              break;
            }

            default:
              throw Parser_Error(parser_status_expression_expected, tstrm.next_sloc(), tstrm.next_length());
          }
          break;
        }

        case Token::index_identifier: {
          // Accept a literal.
          const auto& name = qtok->as_identifier();
          if(::rocket::is_none_of(name, { "null", "true", "false", "Infinity", "NaN" }))
            throw Parser_Error(parser_status_expression_expected, tstrm.next_sloc(), tstrm.next_length());

          switch(name[0]) {
            case 'n':
              value = nullptr;
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
          throw Parser_Error(parser_status_expression_expected, tstrm.next_sloc(), tstrm.next_length());
      }

      // A complete value has been accepted. Insert it into its parent array or object.
      for(;;) {
        if(stack.empty())
          // Accept the root value.
          return value;

        if(stack.back().index() == 0) {
          auto& ctxa = stack.mut_back().as<0>();
          ctxa.array.emplace_back(::std::move(value));

          // Look for the next element.
          auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl, punctuator_comma });
          if(!kpunct)
            throw Parser_Error(parser_status_closed_bracket_or_comma_expected, tstrm.next_sloc(),
                               tstrm.next_length());

          // Check for termination of this array.
          if(*kpunct != punctuator_bracket_cl) {
            kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
            if(!kpunct) {
              // Look for the next element.
              break;
            }
          }

          // Close this array.
          value = ::std::move(ctxa.array);
        }
        else {
          auto& ctxo = stack.mut_back().as<1>();
          ctxo.object.insert_or_assign(::std::move(ctxo.key), ::std::move(value));

          // Look for the next element.
          auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl, punctuator_comma });
          if(!kpunct)
            throw Parser_Error(parser_status_closed_brace_or_comma_expected, tstrm.next_sloc(),
                               tstrm.next_length());

          // Check for termination of this array.
          if(*kpunct != punctuator_brace_cl) {
            kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
            if(!kpunct) {
              // Look for the next element.
              ctxo.key = do_accept_object_key(tstrm);
              break;
            }
          }

          // Close this object.
          value = ::std::move(ctxo.object);
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
    tstrm.reload(cbuf, ::rocket::sref("<JSON text>"));
    if(tstrm.empty())
      ASTERIA_THROW("Empty JSON string");

    // Parse a single value.
    auto value = do_json_parse_nonrecursive(tstrm);
    if(!tstrm.empty())
      ASTERIA_THROW("Excess text at end of JSON string");
    return value;
  }
  catch(Parser_Error& except) {
    ASTERIA_THROW("Invalid JSON string: $3 (line $1, offset $2)", except.line(), except.offset(),
                                                                  describe_parser_status(except.status()));
  }

}  // namespace

V_string
std_json_format(Value value, optV_string indent)
  {
    // No line break is inserted if `indent` is null or empty.
    return (!indent || indent->empty()) ? do_format_nonrecursive(value, false, Indenter_none())
                                        : do_format_nonrecursive(value, false, Indenter_string(*indent));
  }

V_string
std_json_format(Value value, V_integer indent)
  {
    // No line break is inserted if `indent` is non-positive.
    return (indent <= 0) ? do_format_nonrecursive(value, false, Indenter_none())
                         : do_format_nonrecursive(value, false, Indenter_spaces(indent));
  }

V_string
std_json_format5(Value value, optV_string indent)
  {
    // No line break is inserted if `indent` is null or empty.
    return (!indent || indent->empty()) ? do_format_nonrecursive(value, true, Indenter_none())
                                        : do_format_nonrecursive(value, true, Indenter_string(*indent));
  }

V_string
std_json_format5(Value value, V_integer indent)
  {
    // No line break is inserted if `indent` is non-positive.
    return (indent <= 0) ? do_format_nonrecursive(value, true, Indenter_none())
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
      ASTERIA_THROW("Could not open file '$2'\n"
                    "[`fopen()` failed: $1]",
                    format_errno(errno), path);

    // Parse characters from the file.
    ::setbuf(fp, nullptr);
    ::rocket::tinybuf_file cbuf(::std::move(fp));
    return do_json_parse(cbuf);
  }

void
create_bindings_json(V_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.json.format()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("format"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.json.format(value, [indent])`

  * Converts a value to a string in the JSON format, according to
    IETF RFC 7159. This function generates text that conforms to
    JSON strictly; values whose types cannot be represented in JSON
    are discarded if they are found in an object and censored to
    `null` otherwise. If `indent` is set to a string, it is used as
    each level of indention following a line break, unless it is
    empty, in which case no line break is inserted. If `indent` is
    set to an integer, it is clamped between `0` and `10`
    inclusively and this function behaves as if a string consisting
    of this number of spaces was set. Its default value is an empty
    string.

  * Returns the formatted text as a string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.json.format"));
    Argument_Reader::State state;
    // Parse arguments.
    Value value;
    optV_string sindent;
    if(reader.I().o(value).S(state).o(sindent).F()) {
      Reference_root::S_temporary xref = { std_json_format(::std::move(value), ::std::move(sindent)) };
      return self = ::std::move(xref);
    }
    V_integer nindent;
    if(reader.L(state).v(nindent).F()) {
      Reference_root::S_temporary xref = { std_json_format(::std::move(value), ::std::move(nindent)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.json.format5()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("format5"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.json.format5(value, [indent])`

  * Converts a value to a string in the JSON5 format. In addition
    to IETF RFC 7159, a few extensions in ECMAScript 5.1 have been
    introduced to make the syntax more human-readable. Infinities
    and NaNs are preserved. Object keys that are valid identifiers
    are not quoted. Trailing commas of array and object members are
    appended if `indent` is neither null nor zero nor an empty
    string. This function is otherwise identical to `format()`.

  * Returns the formatted text as a string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.json.format5"));
    Argument_Reader::State state;
    // Parse arguments.
    Value value;
    optV_string sindent;
    if(reader.I().o(value).S(state).o(sindent).F()) {
      Reference_root::S_temporary xref = { std_json_format5(::std::move(value), ::std::move(sindent)) };
      return self = ::std::move(xref);
    }
    V_integer nindent;
    if(reader.L(state).v(nindent).F()) {
      Reference_root::S_temporary xref = { std_json_format5(::std::move(value), ::std::move(nindent)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.json.parse()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("parse"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.json.parse(text)`

  * Parses a string containing data encoded in the JSON format and
    converts it to a value. This function reuses the tokenizer of
    Asteria and allows quite a few extensions, some of which are
    also supported by JSON5:

    * Single-line and multiple-line comments are allowed.
    * Binary and hexadecimal numbers are allowed.
    * Numbers can have binary exponents.
    * Infinities and NaNs are allowed.
    * Numbers can start with plus signs.
    * Strings and object keys may be single-quoted.
    * Escape sequences (including UTF-32) are allowed in strings.
    * Element lists of arrays and objects may end in commas.
    * Object keys may be unquoted if they are valid identifiers.

    Be advised that numbers are always parsed as reals.

  * Returns the parsed value.

  * Throws an exception if the string is invalid.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.json.parse"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_json_parse(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.json.parse_file()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("parse_file"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.json.parse_file(path)`

  * Parses the contents of the file denoted by `path` as a JSON
    string. This function behaves identical to `parse()` otherwise.

  * Returns the parsed value.

  * Throws an exception if a read error occurs, or if the string is
    invalid.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.json.parse_file"));
    // Parse arguments.
    V_string path;
    if(reader.I().v(path).F()) {
      Reference_root::S_temporary xref = { std_json_parse_file(::std::move(path)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));
  }

}  // namespace asteria
