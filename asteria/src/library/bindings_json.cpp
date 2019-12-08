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
        virtual tinyfmt& break_line(tinyfmt& fmt) const = 0;
        virtual void increment_level() = 0;
        virtual void decrement_level() = 0;
      };

    Indenter::~Indenter()
      {
      }

    class Indenter_none final : public Indenter
      {
      public:
        explicit Indenter_none()
          {
          }

      public:
        tinyfmt& break_line(tinyfmt& fmt) const override
          {
            return fmt;
          }
        void increment_level() override
          {
          }
        void decrement_level() override
          {
          }
      };

    class Indenter_string final : public Indenter
      {
      private:
        cow_string m_add;
        cow_string m_cur;

      public:
        explicit Indenter_string(const cow_string& add)
          :
            m_add(add), m_cur(rocket::sref("\n"))
          {
          }

      public:
        tinyfmt& break_line(tinyfmt& fmt) const override
          {
            return fmt << this->m_cur;
          }
        void increment_level() override
          {
            this->m_cur.append(this->m_add);
          }
        void decrement_level() override
          {
            this->m_cur.pop_back(this->m_add.size());
          }
      };

    class Indenter_spaces final : public Indenter
      {
      private:
        size_t m_add;
        size_t m_cur;

      public:
        explicit Indenter_spaces(size_t add)
          :
            m_add(add), m_cur(0)
          {
          }

      public:
        tinyfmt& break_line(tinyfmt& fmt) const override
          {
            return fmt << pwrap(this->m_add, this->m_cur);
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

    tinyfmt& do_quote_string(tinyfmt& fmt, aref<G_string> str)
      {
        // Although JavaScript uses UCS-2 rather than UTF-16, the JSON specification adopts UTF-16.
        fmt << '\"';
        size_t offset = 0;
        while(offset < str.size()) {
          // Convert UTF-8 to UTF-16.
          char32_t cp;
          if(!utf8_decode(cp, str, offset)) {
            // Invalid UTF-8 code units are replaced with the replacement character.
            cp = 0xFFFD;
          }
          // Escape double quotes, backslashes, and control characters.
          switch(cp) {
            {{
          case '\"':
              fmt << "\\\"";
              break;
            }{
          case '\\':
              fmt << "\\\\";
              break;
            }{
          case '\b':
              fmt << "\\b";
              break;
            }{
          case '\f':
              fmt << "\\f";
              break;
            }{
          case '\n':
              fmt << "\\n";
              break;
            }{
          case '\r':
              fmt << "\\r";
              break;
            }{
          case '\t':
              fmt << "\\t";
              break;
            }{
          default:
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
              rocket::ascii_numput nump;
              for(auto p = ustr; p != epos; ++p) {
                nump.put_XU(*p, 4);
                char seq[8] = { "\\u" };
                std::memcpy(seq + 2, nump.data() + 2, 4);
                fmt << rocket::sref(seq, 6);
              }
              break;
            }}
          }
        }
        fmt << '\"';
        return fmt;
      }

    G_object::const_iterator do_find_uncensored(aref<G_object> object, G_object::const_iterator from)
      {
        return std::find_if(from, object.end(),
                            [](const auto& p) { return p.second.is_boolean() || p.second.is_integer() || p.second.is_real() ||
                                                       p.second.is_string() || p.second.is_array() || p.second.is_object() ||
                                                       p.second.is_null();  });
      }

    tinyfmt& do_format_scalar(tinyfmt& fmt, const Value& value)
      {
        if(value.is_boolean()){
          // Write `true` or `false`.
          return fmt << value.as_boolean();
        }
        if(value.is_integer()) {
          // Write the integer in decimal.
          return fmt << value.as_integer();
        }
        if(value.is_real() && std::isfinite(value.as_real())) {
          // Write the real in decimal.
          return fmt << value.as_real();
        }
        if(value.is_string()) {
          // Write the quoted string.
          return do_quote_string(fmt, value.as_string());
        }
        // Anything else is censored to `null`.
        return fmt << "null";
      }

    struct S_xformat_array
      {
        ref_to<const G_array> refa;
        G_array::const_iterator curp;
      };
    struct S_xformat_object
      {
        ref_to<const G_object> refo;
        G_object::const_iterator curp;
      };
    using Xformat = variant<S_xformat_array, S_xformat_object>;

    G_string do_format_nonrecursive(const Value& value, Indenter& indent)
      {
        tinyfmt_str fmt;
        // Transform recursion to iteration using a handwritten stack.
        auto qvalue = std::addressof(value);
        cow_vector<Xformat> stack;
        do {
          // Find a leaf value. `qvalue` must always point to a valid value here.
          if(qvalue->is_array()){
            const auto& array = qvalue->as_array();
            // Open an array.
            fmt << '[';
            auto cur = array.begin();
            if(cur != array.end()) {
              // Indent the body.
              indent.increment_level();
              indent.break_line(fmt);
              // Decend into the array.
              S_xformat_array ctxa = { rocket::ref(array), cur };
              stack.emplace_back(rocket::move(ctxa));
              qvalue = std::addressof(*cur);
              continue;
            }
            // Write an empty array.
            fmt << ']';
          }
          else if(qvalue->is_object()) {
            const auto& object = qvalue->as_object();
            // Open an object.
            fmt << '{';
            auto cur = do_find_uncensored(object, object.begin());
            if(cur != object.end()) {
              // Indent the body.
              indent.increment_level();
              indent.break_line(fmt);
              // Write the key followed by a colon.
              do_quote_string(fmt, cur->first);
              fmt << ':';
              // Decend into the object.
              S_xformat_object ctxo = { rocket::ref(object), cur };
              stack.emplace_back(rocket::move(ctxo));
              qvalue = std::addressof(cur->second);
              continue;
            }
            // Write an empty object.
            fmt << '}';
          }
          else {
            // Just write a scalar value which is never recursive.
            do_format_scalar(fmt, *qvalue);
          }
          for(;;) {
            // Advance to the next element if any.
            if(stack.empty()) {
              // Finish the root value.
              return fmt.extract_string();
            }
            if(stack.back().index() == 0) {
              auto& ctxa = stack.mut_back().as<0>();
              // Advance to the next element.
              auto curp = ++(ctxa.curp);
              if(curp != ctxa.refa->end()) {
                // Add a separator between elements.
                fmt << ',';
                indent.break_line(fmt);
                // Format the next element.
                ctxa.curp = curp;
                qvalue = std::addressof(*curp);
                break;
              }
              // Unindent the body.
              indent.decrement_level();
              indent.break_line(fmt);
              // Finish this array.
              fmt << ']';
            }
            else {
              auto& ctxo = stack.mut_back().as<1>();
              // Advance to the next key-value pair.
              auto curp = do_find_uncensored(ctxo.refo, ++(ctxo.curp));
              if(curp != ctxo.refo->end()) {
                // Add a separator between elements.
                fmt << ',';
                indent.break_line(fmt);
                // Write the key followed by a colon.
                do_quote_string(fmt, curp->first);
                fmt << ':';
                // Format the next value.
                ctxo.curp = curp;
                qvalue = std::addressof(curp->second);
                break;
              }
              // Unindent the body.
              indent.decrement_level();
              indent.break_line(fmt);
              // Finish this array.
              fmt << '}';
            }
            stack.pop_back();
          }
        } while(true);
      }
    G_string do_format_nonrecursive(const Value& value, Indenter&& indent)
      {
        return do_format_nonrecursive(value, indent);
      }

    }  // namespace

G_string std_json_format(const Value& value, aopt<G_string> indent)
  {
    // No line break is inserted if `indent` is null or empty.
    return (!indent || indent->empty()) ? do_format_nonrecursive(value, Indenter_none())
                                        : do_format_nonrecursive(value, Indenter_string(*indent));
  }

G_string std_json_format(const Value& value, aref<G_integer> indent)
  {
    // No line break is inserted if `indent` is non-positive.
    return (indent <= 0) ? do_format_nonrecursive(value, Indenter_none())
                         : do_format_nonrecursive(value, Indenter_spaces(static_cast<size_t>(rocket::min(indent, 10))));
  }

    namespace {

    opt<G_string> do_accept_identifier_opt(Token_Stream& tstrm, std::initializer_list<const char*> accept)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::clear;
        }
        if(qtok->is_identifier()) {
          auto name = qtok->as_identifier();
          if(rocket::is_any_of(name, accept)) {
            tstrm.shift();
            // A match has been found.
            return rocket::move(name);
          }
        }
        return rocket::clear;
      }

    opt<Punctuator> do_accept_punctuator_opt(Token_Stream& tstrm, std::initializer_list<Punctuator> accept)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::clear;
        }
        if(qtok->is_punctuator()) {
          auto punct = qtok->as_punctuator();
          if(rocket::is_any_of(punct, accept)) {
            tstrm.shift();
            // A match has been found.
            return punct;
          }
        }
        return rocket::clear;
      }

    opt<G_real> do_accept_number_opt(Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::clear;
        }
        if(qtok->is_integer_literal()) {
          auto val = qtok->as_integer_literal();
          tstrm.shift();
          // Convert integers to reals, as JSON does not have an integral type.
          return G_real(val);
        }
        if(qtok->is_real_literal()) {
          auto val = qtok->as_real_literal();
          tstrm.shift();
          // This real can be copied as is.
          return G_real(val);
        }
        if(qtok->is_punctuator()) {
          auto punct = qtok->as_punctuator();
          if(rocket::is_any_of(punct, { punctuator_add, punctuator_sub })) {
            G_real sign = (punct == punctuator_sub) ? -1.0 : +1.0;
            // Only `Infinity` and `NaN` are allowed.
            // Please note that the tokenizer will have merged sign symbols into adjacent number literals.
            qtok = tstrm.peek_opt(1);
            if(!qtok) {
              return rocket::clear;
            }
            if(!qtok->is_identifier()) {
              return rocket::clear;
            }
            const auto& name = qtok->as_identifier();
            if(name == "Infinity") {
              tstrm.shift(2);
              // Accept a signed `Infinity`.
              return std::copysign(std::numeric_limits<G_real>::infinity(), sign);
            }
            if(name == "NaN") {
              tstrm.shift(2);
              // Accept a signed `NaN`.
              return std::copysign(std::numeric_limits<G_real>::quiet_NaN(), sign);
            }
          }
        }
        return rocket::clear;
      }

    opt<G_string> do_accept_string_opt(Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::clear;
        }
        if(qtok->is_string_literal()) {
          auto val = qtok->as_string_literal();
          tstrm.shift();
          // This string literal can be copied as is in UTF-8.
          return G_string(rocket::move(val));
        }
        return rocket::clear;
      }

    opt<Value> do_accept_scalar_opt(Token_Stream& tstrm)
      {
        auto qnum = do_accept_number_opt(tstrm);
        if(qnum) {
          // Accept a `Number`.
          return *qnum;
        }
        auto qstr = do_accept_string_opt(tstrm);
        if(qstr) {
          // Accept a `String`.
          return rocket::move(*qstr);
        }
        qstr = do_accept_identifier_opt(tstrm, { "true", "false", "Infinity", "NaN", "null" });
        if(qstr) {
          if(qstr->front() == 't') {
            // Accept a `Boolean` of value `true`.
            return true;
          }
          if(qstr->front() == 'f') {
            // Accept a `Boolean` of value `false`.
            return false;
          }
          if(qstr->front() == 'I') {
            // Accept a `Number` of value `Infinity`.
            return std::numeric_limits<G_real>::infinity();
          }
          if(qstr->front() == 'N') {
            // Accept a `Number` of value `NaN`.
            return std::numeric_limits<G_real>::quiet_NaN();
          }
          // Accept an explicit `null`.
          return nullptr;
        }
        return rocket::clear;
      }

    opt<G_string> do_accept_key_opt(Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return rocket::clear;
        }
        if(qtok->is_identifier()) {
          auto name = qtok->as_identifier();
          tstrm.shift();
          // Identifiers are allowed unquoted in JSON5.
          return G_string(rocket::move(name));
        }
        if(qtok->is_string_literal()) {
          auto val = qtok->as_string_literal();
          tstrm.shift();
          // This string literal can be copied as is in UTF-8.
          return G_string(rocket::move(val));
        }
        return rocket::clear;
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
    using Xparse = variant<S_xparse_array, S_xparse_object>;

    opt<Value> do_json_parse_nonrecursive_opt(Token_Stream& tstrm)
      {
        Value value;
        // Implement a recursive descent parser without recursion.
        cow_vector<Xparse> stack;
        do {
          // Accept a leaf value. No other things such as closed brackets are allowed.
          auto kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_op, punctuator_brace_op });
          if(kpunct == punctuator_bracket_op) {
            // An open bracket has been accepted.
            kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
            if(!kpunct) {
              // Descend into the new array.
              S_xparse_array ctxa = { rocket::clear };
              stack.emplace_back(rocket::move(ctxa));
              continue;
            }
            // Accept an empty array.
            value = G_array();
          }
          else if(kpunct == punctuator_brace_op) {
            // An open brace has been accepted.
            kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
            if(!kpunct) {
              // A key followed by a colon is expected.
              auto qkey = do_accept_key_opt(tstrm);
              if(!qkey) {
                return rocket::clear;
              }
              kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
              if(!kpunct) {
                return rocket::clear;
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
              return rocket::clear;
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
              kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl, punctuator_comma });
              if(!kpunct) {
                return rocket::clear;
              }
              if(*kpunct == punctuator_comma) {
                kpunct = do_accept_punctuator_opt(tstrm, { punctuator_bracket_cl });
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
              kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl, punctuator_comma });
              if(!kpunct) {
                return rocket::clear;
              }
              if(*kpunct == punctuator_comma) {
                kpunct = do_accept_punctuator_opt(tstrm, { punctuator_brace_cl });
                if(!kpunct) {
                  // The next key is expected to follow the comma.
                  auto qkey = do_accept_key_opt(tstrm);
                  if(!qkey) {
                    return rocket::clear;
                  }
                  kpunct = do_accept_punctuator_opt(tstrm, { punctuator_colon });
                  if(!kpunct) {
                    return rocket::clear;
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

Value std_json_parse(aref<G_string> text)
  {
    // We reuse the lexer of Asteria here, allowing quite a few extensions e.g. binary numeric literals and comments.
    Compiler_Options opts = { };
    opts.escapable_single_quotes = true;
    opts.keywords_as_identifiers = true;
    opts.integers_as_reals = true;
    // Tokenize the source string.
    Token_Stream tstrm;
    try {
      tinybuf_str cbuf;
      cbuf.set_string(text, tinybuf::open_read);
      tstrm.reload(cbuf, rocket::sref("<JSON text>"), opts);
    }
    catch(Parser_Error& except) {
      return nullptr;
    }
    // Parse tokens.
    auto qvalue = do_json_parse_nonrecursive_opt(tstrm);
    if(!qvalue) {
      return nullptr;
    }
    // Check whether all tokens have been consumed.
    auto qtok = tstrm.peek_opt();
    if(qtok) {
      return nullptr;
    }
    return rocket::move(*qvalue);
  }

void create_bindings_json(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.json.format()`
    //===================================================================
    result.insert_or_assign(rocket::sref("format"),
      G_function(rocket::make_refcnt<Simple_Binding_Wrapper>(
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
        [](const Value& /*opaque*/, const Global_Context& /*global*/,
                   Reference&& /*self*/, cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.json.format"), rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          Value value;
          opt<G_string> sindent;
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
      G_function(rocket::make_refcnt<Simple_Binding_Wrapper>(
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
        [](const Value& /*opaque*/, const Global_Context& /*global*/,
                   Reference&& /*self*/, cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.json.parse"), rocket::ref(args));
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
