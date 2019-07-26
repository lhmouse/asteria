// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_json.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/collector.hpp"
#include "../compiler/token_stream.hpp"
#include "../utilities.hpp"

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
            if(this->m_cur.empty()) {
              return cstrm;
            }
            return cstrm << this->m_cur;
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
        G_integer m_add;
        G_integer m_cur;

      public:
        explicit Indenter_spaces(G_integer add)
          : m_add(add), m_cur()
          {
          }

      public:
        std::ostream& break_line(std::ostream& cstrm) const override
          {
            cstrm << std::endl;
            if(this->m_cur == 0) {
              return cstrm;
            }
            return cstrm << std::setfill(' ') << std::setw(static_cast<int>(rocket::min(this->m_cur, INT_MAX))) << ' ';
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
            std::for_each(ustr, pos, [&](char16_t uch) { cstrm << "\\u" << std::setw(4) << uch;  });
            break;
          }
        }
        cstrm << '\"';
        return cstrm;
      }

    G_object::const_iterator do_find_uncensored(const G_object& object, G_object::const_iterator from)
      {
        return std::find_if(from, object.end(),
                            [](const auto& pair) { return pair.second.is_boolean() || pair.second.is_integer() || pair.second.is_real() ||
                                                          pair.second.is_string() || pair.second.is_array() || pair.second.is_object() ||
                                                          pair.second.is_null();  });
      }

    std::ostream& do_format_scalar(std::ostream& cstrm, const Value& value)
      {
        if(value.is_boolean()){
          // Write `true` or `false`.
          return cstrm << std::boolalpha << std::nouppercase << value.as_boolean();
        }
        if(value.is_integer()) {
          // Write the integer in decimal.
          return cstrm << std::dec << value.as_integer();
        }
        if(value.is_real() && std::isfinite(value.as_real())) {
          // Write the real number in decimal.
          return cstrm << std::defaultfloat << std::nouppercase << std::setprecision(17) << value.as_real();
        }
        if(value.is_string()) {
          // Write the quoted string.
          return do_quote_string(cstrm, value.as_string());
        }
        // Anything else is censored to `null`.
        return cstrm << "null";
      }

    struct S_xformat_array
      {
        std::reference_wrapper<const G_array> array;
        G_array::const_iterator cur;
      };
    struct S_xformat_object
      {
        std::reference_wrapper<const G_object> object;
        G_object::const_iterator cur;
      };
    using Xformat = Variant<S_xformat_array, S_xformat_object>;

    G_string do_format_nonrecursive(const Value& value, Indenter& indent)
      {
        Cow_osstream cstrm;
        cstrm.imbue(std::locale::classic());
        // Transform recursion to iteration using a handwritten stack.
        auto qvalue = std::addressof(value);
        Cow_Vector<Xformat> stack;
        do {
          // Find a leaf value. `qvalue` must always point to a valid value here.
          if(qvalue->is_array()){
            const auto& array = qvalue->as_array();
            // Open an array.
            cstrm << '[';
            auto cur = array.begin();
            if(cur != array.end()) {
              // Indent the body.
              indent.increment_level();
              indent.break_line(cstrm);
              // Decend into the array.
              S_xformat_array ctxa = { std::ref(array), cur };
              stack.emplace_back(rocket::move(ctxa));
              qvalue = std::addressof(*cur);
              continue;
            }
            // Write an empty array.
            cstrm << ']';
          }
          else if(qvalue->is_object()) {
            const auto& object = qvalue->as_object();
            // Open an object.
            cstrm << '{';
            auto cur = do_find_uncensored(object, object.begin());
            if(cur != object.end()) {
              // Indent the body.
              indent.increment_level();
              indent.break_line(cstrm);
              // Write the key followed by a colon.
              do_quote_string(cstrm, cur->first);
              cstrm << ':';
              // Decend into the object.
              S_xformat_object ctxo = { std::ref(object), cur };
              stack.emplace_back(rocket::move(ctxo));
              qvalue = std::addressof(cur->second);
              continue;
            }
            // Write an empty object.
            cstrm << '}';
          }
          else {
            // Just write a scalar value which is never recursive.
            do_format_scalar(cstrm, *qvalue);
          }
          for(;;) {
            // Advance to the next element if any.
            if(stack.empty()) {
              // Finish the root value.
              return cstrm.extract_string();
            }
            if(stack.back().index() == 0) {
              auto& ctxa = stack.mut_back().as<0>();
              // Advance to the next element.
              auto cur = ++(ctxa.cur);
              if(cur != ctxa.array.get().end()) {
                // Add a separator between elements.
                cstrm << ',';
                indent.break_line(cstrm);
                // Format the next element.
                ctxa.cur = cur;
                qvalue = std::addressof(*cur);
                break;
              }
              // Unindent the body.
              indent.decrement_level();
              indent.break_line(cstrm);
              // Finish this array.
              cstrm << ']';
            }
            else {
              auto& ctxo = stack.mut_back().as<1>();
              // Advance to the next key-value pair.
              auto cur = do_find_uncensored(ctxo.object, ++(ctxo.cur));
              if(cur != ctxo.object.get().end()) {
                // Add a separator between elements.
                cstrm << ',';
                indent.break_line(cstrm);
                // Write the key followed by a colon.
                do_quote_string(cstrm, cur->first);
                cstrm << ':';
                // Format the next value.
                ctxo.cur = cur;
                qvalue = std::addressof(cur->second);
                break;
              }
              // Unindent the body.
              indent.decrement_level();
              indent.break_line(cstrm);
              // Finish this array.
              cstrm << '}';
            }
            stack.pop_back();
          }
        } while(true);
      }  // namespace
    G_string do_format_nonrecursive(const Value& value, Indenter&& indent)
      {
        return do_format_nonrecursive(value, indent);
      }

    }

G_string std_json_format(const Value& value, const Opt<G_string>& indent)
  {
    // No line break is inserted if `indent` is null or empty.
    return (!indent || indent->empty()) ? do_format_nonrecursive(value, Indenter_none())
                                        : do_format_nonrecursive(value, Indenter_string(*indent));
  }

G_string std_json_format(const Value& value, const G_integer& indent)
  {
    // No line break is inserted if `indent` is non-positive.
    return (indent <= 0) ? do_format_nonrecursive(value, Indenter_none())
                         : do_format_nonrecursive(value, Indenter_spaces(rocket::min(indent, 10)));
  }

    namespace {

    Opt<G_string> do_accept_identifier_opt(Token_Stream& tstrm, std::initializer_list<const char*> accept)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        if(qtok->is_identifier()) {
          auto name = qtok->as_identifier();
          if(rocket::is_any_of(name, accept)) {
            tstrm.shift();
            // A match has been found.
            return rocket::move(name);
          }
        }
        return rocket::nullopt;
      }

    Opt<Token::Punctuator> do_accept_punctuator_opt(Token_Stream& tstrm, std::initializer_list<Token::Punctuator> accept)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        if(qtok->is_punctuator()) {
          auto punct = qtok->as_punctuator();
          if(rocket::is_any_of(punct, accept)) {
            tstrm.shift();
            // A match has been found.
            return punct;
          }
        }
        return rocket::nullopt;
      }

    Opt<G_real> do_accept_number_opt(Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        if(qtok->is_integer_literal()) {
          auto value = qtok->as_integer_literal();
          tstrm.shift();
          // Convert integers to real numbers, as JSON does not have an integral type.
          return G_real(value);
        }
        if(qtok->is_real_literal()) {
          auto value = qtok->as_real_literal();
          tstrm.shift();
          // This real number can be copied as is.
          return G_real(value);
        }
        if(qtok->is_punctuator()) {
          auto punct = qtok->as_punctuator();
          if(rocket::is_any_of(punct, { Token::punctuator_add, Token::punctuator_sub })) {
            // Only `Infinity` and `NaN` are allowed.
            // Please note that the tokenizer will have merged sign symbols into adjacent number literals.
            qtok = tstrm.peek_opt(1);
            if(!qtok) {
              return rocket::nullopt;
            }
            if(!qtok->is_identifier()) {
              return rocket::nullopt;
            }
            const auto& name = qtok->as_identifier();
            bool rneg = punct == Token::punctuator_sub;
            if(name == "Infinity") {
              tstrm.shift(2);
              // Accept a signed `Infinity`.
              return G_real(std::copysign(INFINITY, -rneg));
            }
            if(name == "NaN") {
              tstrm.shift(2);
              // Accept a signed `NaN`.
              return G_real(std::copysign(NAN, -rneg));
            }
          }
        }
        return rocket::nullopt;
      }

    Opt<G_string> do_accept_string_opt(Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        if(qtok->is_string_literal()) {
          auto value = qtok->as_string_literal();
          tstrm.shift();
          // This string literal can be copied as is in UTF-8.
          return G_string(rocket::move(value));
        }
        return rocket::nullopt;
      }

    Opt<Value> do_accept_scalar_opt(Token_Stream& tstrm)
      {
        auto qnum = do_accept_number_opt(tstrm);
        if(qnum) {
          // Accept a `Number`.
          return G_real(*qnum);
        }
        auto qstr = do_accept_string_opt(tstrm);
        if(qstr) {
          // Accept a `String`.
          return G_string(rocket::move(*qstr));
        }
        qstr = do_accept_identifier_opt(tstrm, { "true", "false", "Infinity", "NaN", "null" });
        if(qstr) {
          if(qstr->front() == 't') {
            // Accept a `Boolean` of value `true`.
            return G_boolean(true);
          }
          if(qstr->front() == 'f') {
            // Accept a `Boolean` of value `false`.
            return G_boolean(false);
          }
          if(qstr->front() == 'I') {
            // Accept a `Number` of value `Infinity`.
            return G_real(INFINITY);
          }
          if(qstr->front() == 'N') {
            // Accept a `Number` of value `NaN`.
            return G_real(NAN);
          }
          // Accept an explicit `null`.
          return G_null();
        }
        return rocket::nullopt;
      }

    Opt<G_string> do_accept_key_opt(Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::nullopt;
        }
        if(qtok->is_identifier()) {
          auto name = qtok->as_identifier();
          tstrm.shift();
          // Identifiers are allowed unquoted in JSON5.
          return G_string(rocket::move(name));
        }
        if(qtok->is_string_literal()) {
          auto value = qtok->as_string_literal();
          tstrm.shift();
          // This string literal can be copied as is in UTF-8.
          return G_string(rocket::move(value));
        }
        return rocket::nullopt;
      }

    struct S_xparse_array
      {
        G_array array;
      };
    struct S_xparse_object
      {
        G_object object;
        G_string key;
      };
    using Xparse = Variant<S_xparse_array, S_xparse_object>;

    Opt<Value> do_json_parse_nonrecursive_opt(Token_Stream& tstrm)
      {
        Value value;
        // Implement a recursive descent parser without recursion.
        Cow_Vector<Xparse> stack;
        do {
          // Accept a leaf value. No other things such as closed brackets are allowed.
          auto kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_bracket_op, Token::punctuator_brace_op });
          if(kpunct == Token::punctuator_bracket_op) {
            // An open bracket has been accepted.
            kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_bracket_cl });
            if(!kpunct) {
              // Descend into the new array.
              S_xparse_array ctxa = { rocket::clear };
              stack.emplace_back(rocket::move(ctxa));
              continue;
            }
            // Accept an empty array.
            value = G_array();
          }
          else if(kpunct == Token::punctuator_brace_op) {
            // An open brace has been accepted.
            kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_brace_cl });
            if(!kpunct) {
              // A key followed by a colon is expected.
              auto qkey = do_accept_key_opt(tstrm);
              if(!qkey) {
                return rocket::nullopt;
              }
              kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_colon });
              if(!kpunct) {
                return rocket::nullopt;
              }
              // Descend into a new object.
              S_xparse_object ctxo = { rocket::clear, rocket::move(*qkey) };
              stack.emplace_back(rocket::move(ctxo));
              continue;
            }
            // Accept an empty object.
            value = G_object();
          }
          else {
            // Just accept a scalar value which is never recursive.
            auto qvalue = do_accept_scalar_opt(tstrm);
            if(!qvalue) {
              return rocket::nullopt;
            }
            value = rocket::move(*qvalue);
          }
          // Insert the value into its parent array or object.
          for(;;) {
            if(stack.empty()) {
              // Accept the root value.
              return rocket::move(value);
            }
            if(stack.back().index() == 0) {
              auto& ctxa = stack.mut_back().as<0>();
              // Append the value to its parent array.
              ctxa.array.emplace_back(rocket::move(value));
              // Look for the next element.
              kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_bracket_cl, Token::punctuator_comma });
              if(!kpunct) {
                return rocket::nullopt;
              }
              if(*kpunct == Token::punctuator_comma) {
                kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_bracket_cl });
                if(!kpunct) {
                  // The next element is expected to follow the comma.
                  break;
                }
                // An extra comma is allowed in JSON5.
              }
              // Pop the array.
              value = rocket::move(ctxa.array);
            }
            else {
              auto& ctxo = stack.mut_back().as<1>();
              // Insert the value into its parent object.
              ctxo.object.insert_or_assign(rocket::move(ctxo.key), rocket::move(value));
              // Look for the next element.
              kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_brace_cl, Token::punctuator_comma });
              if(!kpunct) {
                return rocket::nullopt;
              }
              if(*kpunct == Token::punctuator_comma) {
                kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_brace_cl });
                if(!kpunct) {
                  // The next key is expected to follow the comma.
                  auto qkey = do_accept_key_opt(tstrm);
                  if(!qkey) {
                    return rocket::nullopt;
                  }
                  kpunct = do_accept_punctuator_opt(tstrm, { Token::punctuator_colon });
                  if(!kpunct) {
                    return rocket::nullopt;
                  }
                  ctxo.key = rocket::move(*qkey);
                  // The next value is expected to follow the colon.
                  break;
                }
                // An extra comma is allowed in JSON5.
              }
              // Pop the object.
              value = rocket::move(ctxo.object);
            }
            stack.pop_back();
          }
        } while(true);
      }

    }  // namespace

Value std_json_parse(const G_string& text)
  {
    // Tokenize source data.
    // We reuse the lexer of Asteria here, allowing quite a few extensions e.g. binary numeric literals and comments.
    Compiler_Options options = { };
    options.escapable_single_quote_string = true;
    options.keyword_as_identifier = true;
    options.integer_as_real = true;
    // Use a `streambuf` rather than an `istream` to minimize overheads.
    Cow_stringbuf cbuf(text, std::ios_base::in);
    Token_Stream tstrm;
    if(!tstrm.load(cbuf, rocket::sref("<JSON text>"), options)) {
      ASTERIA_DEBUG_LOG("Could not tokenize JSON text: ", text);
      return G_null();
    }
    auto qvalue = do_json_parse_nonrecursive_opt(tstrm);
    if(!qvalue) {
      ASTERIA_DEBUG_LOG("Invalid or empty JSON text: ", text);
      return G_null();
    }
    if(!tstrm.empty()) {
      ASTERIA_DEBUG_LOG("Excess token: ", text);
      return G_null();
    }
    return rocket::move(*qvalue);
  }

void create_bindings_json(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.json.format()`
    //===================================================================
    result.insert_or_assign(rocket::sref("format"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
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
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
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
        })
      ));
    //===================================================================
    // `std.json.parse()`
    //===================================================================
    result.insert_or_assign(rocket::sref("parse"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
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
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
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
        })
      ));
    //===================================================================
    // End of `std.json`
    //===================================================================
  }

}  // namespace Asteria
