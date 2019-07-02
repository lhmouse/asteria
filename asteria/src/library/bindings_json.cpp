// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_json.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/collector.hpp"
#include "../utilities.hpp"
#include "../compiler/token_stream.hpp"

namespace Asteria {

    namespace {

    class Indenter
      {
      public:
        virtual ~Indenter();

      public:
        virtual std::ostream& break_line(std::ostream& cstrm) const = 0;
        virtual void increment_level() = 0;
        virtual void decrement_level() = 0;
      };

    Indenter::~Indenter()
      {
      }

    class Indenter_none : public Indenter
      {
      public:
        explicit Indenter_none()
          {
          }

      public:
        std::ostream& break_line(std::ostream& cstrm) const override
          {
            return cstrm;
          }
        void increment_level() override
          {
          }
        void decrement_level() override
          {
          }
      };

    class Indenter_string : public Indenter
      {
      private:
        G_string m_add;
        G_string m_cur;

      public:
        explicit Indenter_string(G_string add)
          : m_add(rocket::move(add)), m_cur()
          {
          }

      public:
        std::ostream& break_line(std::ostream& cstrm) const override
          {
            cstrm << std::endl;
            cstrm << this->m_cur;
            return cstrm;
          }
        void increment_level() override
          {
            this->m_cur.append(this->m_add);
          }
        void decrement_level() override
          {
            this->m_cur.erase(this->m_cur.size() - this->m_add.size());
          }
      };

    class Indenter_spaces : public Indenter
      {
      private:
        std::size_t m_add;
        std::size_t m_cur;

      public:
        explicit Indenter_spaces(std::size_t add)
          : m_add(add), m_cur()
          {
          }

      public:
        std::ostream& break_line(std::ostream& cstrm) const override
          {
            cstrm << std::endl;
            if(this->m_cur != 0) {
              cstrm << std::setfill(' ') << std::setw(static_cast<int>(std::min<std::size_t>(this->m_cur, INT_MAX))) << ' ';
            }
            return cstrm;
          }
        void increment_level() override
          {
            this->m_cur += this->m_add;
          }
        void decrement_level() override
          {
            this->m_cur -= this->m_add;
          }
      };

    std::ostream& do_quote_string(std::ostream& cstrm, const G_string& str)
      {
        // Although JavaScript uses UCS-2 rather than UTF-16, the JSON specification adopts UTF-16.
        cstrm << '\"';
        std::size_t offset = 0;
        while(offset < str.size()) {
          // Convert UTF-8 to UTF-16.
          char32_t cp;
          if(!utf8_decode(cp, str, offset)) {
            // Invalid UTF-8 code units are replaced with the replacement character.
            cp = 0xFFFD;
          }
          // Escape double quotes, backslashes, and control characters.
          switch(cp) {
          case '\"':
            cstrm << "\\\"";
            break;
          case '\\':
            cstrm << "\\\\";
            break;
          case '\b':
            cstrm << "\\b";
            break;
          case '\f':
            cstrm << "\\f";
            break;
          case '\n':
            cstrm << "\\n";
            break;
          case '\r':
            cstrm << "\\r";
            break;
          case '\t':
            cstrm << "\\t";
            break;
          default:
            if((0x20 <= cp) && (cp <= 0x7E)) {
              // Write printable characters as is.
              cstrm << static_cast<char>(cp);
              break;
            }
            // Encode the character in UTF-16.
            char16_t ustr[2];
            char16_t* pos = ustr;
            utf16_encode(pos, cp);
            cstrm << std::hex << std::uppercase << std::setfill('0');
            std::for_each(ustr, pos, [&](unsigned uch) { cstrm << "\\u" << std::setw(4) << uch;  });
            break;
          }
        }
        cstrm << '\"';
        return cstrm;
      }

    bool do_can_format_pair(const G_object::value_type& pair)
      {
        return pair.second.is_null() || pair.second.is_boolean() || pair.second.is_integer() || pair.second.is_real() ||
               pair.second.is_string() || pair.second.is_array() || pair.second.is_object();
      }

    std::ostream& do_json_format(std::ostream& cstrm, const Value& value, Indenter& indent)
      {
        if(value.is_null()) {
          // Write `"null"`.
          return cstrm << "null";
        }
        if(value.is_boolean()) {
          // Write `"true"` or `"false"`.
          return cstrm << std::boolalpha << std::nouppercase << value.as_boolean();
        }
        if(value.is_integer()) {
          // Write the integer in decimal.
          return cstrm << std::dec << value.as_integer();
        }
        if(value.is_real()) {
          // Infinities and NaNs are censored to null. All other values are safe.
          if(!std::isfinite(value.as_real())) {
            return cstrm << "null";
          }
          return cstrm << std::defaultfloat << std::nouppercase << std::setprecision(17) << value.as_real();
        }
        if(value.is_string()) {
          // All values are safe.
          return do_quote_string(cstrm, value.as_string());
        }
        if(value.is_array()) {
          // All values are safe.
          const auto& altr = value.as_array();
          cstrm << '[';
          auto qcur = altr.begin();
          if(qcur != altr.end()) {
            // Indent the body.
            indent.increment_level();
            indent.break_line(cstrm);
            // Write elements.
            for(;;) {
              do_json_format(cstrm, *qcur, indent);
              if(++qcur == altr.end()) {
                break;
              }
              cstrm << ',';
              indent.break_line(cstrm);
            }
            // Unindent the body.
            indent.decrement_level();
            indent.break_line(cstrm);
          }
          cstrm << ']';
          return cstrm;
        }
        if(value.is_object()) {
          // All values are safe.
          const auto& altr = value.as_object();
          cstrm << '{';
          auto qcur = std::find_if(altr.begin(), altr.end(), do_can_format_pair);
          if(qcur != altr.end()) {
            // Indent the body.
            indent.increment_level();
            indent.break_line(cstrm);
            // Write key-value pairs.
            for(;;) {
              do_quote_string(cstrm, qcur->first);
              cstrm << ':';
              do_json_format(cstrm, qcur->second, indent);
              if((qcur = std::find_if(++qcur, altr.end(), do_can_format_pair)) == altr.end()) {
                break;
              }
              cstrm << ',';
              indent.break_line(cstrm);
            }
            // Unindent the body.
            indent.decrement_level();
            indent.break_line(cstrm);
          }
          cstrm << '}';
          return cstrm;
        }
        // Functions and opaque values are discarded.
        return cstrm << "null";
      }
    std::ostream& do_json_format(std::ostream& cstrm, const Value& value, Indenter&& indent)
      {
        return do_json_format(cstrm, value, indent);
      }

    }

G_string std_json_format(const Value& value, const Opt<G_string>& indent)
  {
    // No line break is inserted if `indent` is null or empty.
    Cow_osstream cstrm;
    cstrm.imbue(std::locale::classic());
    if(!indent || indent->empty()) {
      do_json_format(cstrm, value, Indenter_none());
    } else {
      do_json_format(cstrm, value, Indenter_string(*indent));
    }
    return cstrm.extract_string();
  }

G_string std_json_format(const Value& value, const G_integer& indent)
  {
    // No line break is inserted if `indent` is non-positive.
    Cow_osstream cstrm;
    cstrm.imbue(std::locale::classic());
    if(indent <= 0) {
      do_json_format(cstrm, value, Indenter_none());
    } else {
      do_json_format(cstrm, value, Indenter_spaces(static_cast<std::size_t>(rocket::min(indent, 10))));
    }
    return cstrm.extract_string();
  }

    namespace {

    Parser_Error do_make_parser_error(const Token_Stream& tstrm, Parser_Error::Code code)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return Parser_Error(UINT32_MAX, SIZE_MAX, 0, code);
        }
        return Parser_Error(qtok->line(), qtok->offset(), qtok->length(), code);
      }

    Value do_json_parse(Token_Stream& tstrm)
      {
  return { };
/*        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          // The error code doesn't really matter. The exception itself is caught, and that is all.
          throw do_make_parser_error(tstrm, Parser_Error::code_no_data_loaded);
        }
        if(qtok->is_identifier()) {
          auto name = qtok->as_identifier();
          if(name == "null") {
            // Accept a `null`.
            tstrm.shift();
            return G_null();
          }
          if(name == "true") {
            // Accept a `Boolean` of value `true`.
            tstrm.shift();
            return G_boolean(true);
          }
          if(name == "false") {
            // Accept a `Boolean` of value `false`.
            tstrm.shift();
            return G_boolean(false);
          }
          if(name == "Infinity") {
            // Accept a `Number` of value `Infinity`. This is specific to JSON5.
            tstrm.shift();
            return G_real(INFINITY);
          }
          if(name == "NaN") {
            // Accept a `Number` of value `NaN`. This is specific to JSON5.
            tstrm.shift();
            return G_real(NAN);
          }
          // Any other identifiers are invalid.
          throw do_make_parser_error(tstrm, Parser_Error::code_no_data_loaded);
        }
        if(qtok->is_real_literal()) {
          auto value = qtok->as_real_literal();
          // Accept a `Number`.
          tstrm.shift();
          return G_real(rocket::move(value));
        }
        if(qtok->is_string_literal()) {
          auto value = qtok->as_string_literal();
          // Accept a `String`.
          tstrm.shift();
          return G_string(rocket::move(value));
        }
        if(qtok->is_punctuator()) {
          auto punct = qtok->as_punctuator();
          if((punct == Token::punctuator_add) || (punct == Token::punctuator_sub)) {
            bool rneg = (punct == Token::punctuator_sub);
            // Note that the tokenizer shall have merged sign symbols into adjacent numeric literals.
            // Therefore, the only things we would like to care about here are infinities and NaNs.
            tstrm.shift();
            qtok = tstrm.peek_opt();
            if(!qtok) {
              throw do_make_parser_error(tstrm, Parser_Error::code_no_data_loaded);
            }
            if(!qtok->is_identifier()) {
              throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
            }
            auto name = qtok->as_identifier();
            if(name == "Infinity") {
              // Accept a `Number` of value `Infinity`. This is specific to JSON5.
              tstrm.shift();
              return G_real(std::copysign(INFINITY, -rneg));
            }
            if(name == "NaN") {
              // Accept a `Number` of value `NaN`. This is specific to JSON5.
              tstrm.shift();
              return G_real(std::copysign(NAN, -rneg));
            }
            // Any other identifiers are invalid.
            throw do_make_parser_error(tstrm, Parser_Error::code_no_data_loaded);
          }
          if(punct == Token::punctuator_bracket_op) {
            // Get array elements.
            G_array array;
            // A value is expected.
            tstrm.shift();
            qtok = tstrm.peek_opt();
            if(!qtok) {
              throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
            }
            for(;;) {
              auto elem = do_accept_value(tstrm);
              array.emplace_back(rocket::move(elem));
              // The value shall be terminated by a closed bracket or comma.
              qtok = tstrm.peek_opt();
              if(!qtok) {
                throw do_make_parser_error(tstrm, Parser_Error::code_closed_bracket_expected);
              }
              if(!qtok->is_punctuator()) {
                throw do_make_parser_error(tstrm, Parser_Error::code_closed_bracket_expected);
              }
              punct = qtok->as_punctuator();
              if(punct == Token::punctuator_bracket_cl) {
                break;
              }
              if(punct != Token::punctuator_comma) {
                throw do_make_parser_error(tstrm, Parser_Error::code_closed_bracket_expected);
              }
              tstrm.shift();
              qtok = tstrm.peek_opt();
              if(!qtok) {
                throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
              }
              if(!qtok->is_punctuator()) {
              
              if(punct == Token::punctuator_comma) {
                // Get the next element unless the token is a punctuator.
                qtok = tstrm.peek_opt();
                if(!qtok) {
                  throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
                }
                if(!qtok->is_punctuator()) {
                  continue;
                }
                punct = qtok->as_punctuator();
              }
            }
              
              // In the former case, the comma is deleted.
              


            


auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_bracket_op });
if(!kpunct) {
  return false;
}
std::size_t nelems = 0;
for(;;) {
  bool succ = do_accept_expression(units, tstrm);
  if(!succ) {
    break;
  }
  nelems += 1;
  // Look for the separator.
  kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_comma, Token::punctuator_semicol });
  if(!kpunct) {
    break;
  }
}
kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_bracket_cl });
if(!kpunct) {
  throw do_make_parser_error(tstrm, Parser_Error::code_closed_bracket_expected);
}
Xprunit::S_unnamed_array xunit = { nelems };
units.emplace_back(rocket::move(xunit));
return true;
        }
        // Any other tokens are invalid.
        throw do_make_parser_error(tstrm, Parser_Error::code_no_data_loaded);
*/      }

    }

Value std_json_parse(const G_string& text)
  {
    // Tokenize source data.
    // We reuse the lexer of Asteria here, allowing quite a few extensions e.g. binary numeric literals and comments.
    Parser_Options options = { };
    options.escapable_single_quote_string = true;
    options.keyword_as_identifier = true;
    options.integer_as_real = true;
    // Use a `streambuf` rather than an `istream` to minimize overloads.
    Cow_stringbuf cbuf(text, std::istream::in);
    Token_Stream tstrm;
    if(!tstrm.load(cbuf, rocket::sref("<JSON text>"), options)) {
      ASTERIA_DEBUG_LOG("Could not tokenize source text: ", tstrm.get_parser_error());
      return G_null();
    }
    Value value;
    try {
      value = do_json_parse(tstrm);
    } catch(Parser_Error& err) {  // `Parser_Error` is not derived from `std::exception`. Don't play with this at home.
      ASTERIA_DEBUG_LOG("Caught `Parser_Error`: ", err);
      return G_null();
    }
    if(!tstrm.empty()) {
      ASTERIA_DEBUG_LOG("Excess token: ", *(tstrm.peek_opt()));
      return G_null();
    }
    return value;
  }

void create_bindings_json(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.json.format()`
    //===================================================================
    result.insert_or_assign(rocket::sref("format"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.json.format(value, [indent])`\n"
            "\n"
            "  * Converts a value to a `string` in the JSON format, according to\n"
            "    IETF RFC 7159. This function generates text that conforms to\n"
            "    JSON strictly; values whose types cannot be represented in JSON\n"
            "    are discarded if they are found in an `object` and censored to\n"
            "    `null` otherwise. If `indent` is set to a `string`, it is used\n"
            "    as each level of indention following a line break, unless it is\n"
            "    empty, in which case no line break is inserted. If `indent` is\n"
            "    set to an `integer`, it is clamped between `0` and `10`\n"
            "    inclusively and this function behaves as if a `string`\n"
            "    consisting of this number of spaces was set. Its default value\n"
            "    is an empty `string`.\n"
            "\n"
            "  * Returns the formatted text as a `string`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.json.format"), args);
            Argument_Reader::State state;
            // Parse arguments.
            Value value;
            Opt<G_string> sindent;
            if(reader.start().g(value).save(state).g(sindent).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_json_format(value, sindent) };
              return rocket::move(xref);
            }
            G_integer nindent;
            if(reader.load(state).g(nindent).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_json_format(value, nindent) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.json.parse()`
    //===================================================================
    result.insert_or_assign(rocket::sref("parse"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.json.parse(text)`\n"
            "\n"
            "  * Parses a `string` containing data encoded in the JSON format\n"
            "    and converts it to a value. This function reuses the tokenizer\n"
            "    of Asteria and allows quite a few extensions, some of which\n"
            "    are also supported by JSON5:\n"
            "\n"
            "    * Single-line and multiple-line comments are allowed.\n"
            "    * Binary and hexadecimal numbers are allowed.\n"
            "    * Numbers can have binary exponents.\n"
            "    * Infinities and NaNs are allowed.\n"
            "    * Numbers can start with plus signs.\n"
            "    * Strings and object keys may be single-quoted.\n"
            "    * Escape sequences (including UTF-32) are allowed in strings.\n"
            "    * Element lists of arrays and objects may end in commas.\n"
            "    * Object keys may be unquoted if they are valid identifiers.\n"
            "\n"
            "    Be advised that numbers are always parsed as `real`s.\n"
            "\n"
            "  * Returns the parsed value. If `text` is empty or is not a valid\n"
            "    JSON string, `null` is returned. There is no way to tell empty\n"
            "    or explicit `\"null\"` inputs from failures.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.json.parse"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_json_parse(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // End of `std.json`
    //===================================================================
  }

}