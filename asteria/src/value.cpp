// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "value.hpp"
#include "utilities.hpp"

namespace Asteria {

Value::Value(const Value &) noexcept
  = default;
Value & Value::operator=(const Value &) noexcept
  = default;
Value::Value(Value &&) noexcept
  = default;
Value & Value::operator=(Value &&) noexcept
  = default;
Value::~Value()
  = default;

const char * get_type_name(Value::Type type) noexcept
  {
    switch(type) {
    case Value::type_null:
      return "null";
    case Value::type_boolean:
      return "boolean";
    case Value::type_integer:
      return "integer";
    case Value::type_double:
      return "double";
    case Value::type_string:
      return "string";
    case Value::type_opaque:
      return "opaque";
    case Value::type_function:
      return "function";
    case Value::type_array:
      return "array";
    case Value::type_object:
      return "object";
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", type, "` has been encountered.");
    }
  }

bool test_value(const Value &value)
  {
    switch(value.type()) {
    case Value::type_null:
      return false;
    case Value::type_boolean:
      {
        const auto &alt = value.as<D_boolean>();
        return alt;
      }
    case Value::type_integer:
      {
        const auto &alt = value.as<D_integer>();
        return alt != 0;
      }
    case Value::type_double:
      {
        const auto &alt = value.as<D_double>();
        return std::fpclassify(alt) != FP_ZERO;
      }
    case Value::type_string:
      {
        const auto &alt = value.as<D_string>();
        return alt.empty() == false;
      }
    case Value::type_opaque:
    case Value::type_function:
      return true;
    case Value::type_array:
      {
        const auto &alt = value.as<D_array>();
        return alt.empty() == false;
      }
    case Value::type_object:
      {
        const auto &alt = value.as<D_object>();
        return alt.empty() == false;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", value.type(), "` has been encountered.");
    }
  }

Value::Compare compare_values(const Value &lhs, const Value &rhs) noexcept
  {
    // `null` is considered to be equal to `null` and less than anything else.
    if(lhs.type() != rhs.type()) {
      if(lhs.type() == Value::type_null) {
        return Value::compare_less;
      }
      if(rhs.type() == Value::type_null) {
        return Value::compare_greater;
      }
      return Value::compare_unordered;
    }
    // If both operands have the same type, perform normal comparison.
    switch(lhs.type()) {
    case Value::type_null:
      return Value::compare_equal;
    case Value::type_boolean:
      {
        const auto &cand_lhs = lhs.as<D_boolean>();
        const auto &cand_rhs = rhs.as<D_boolean>();
        if(cand_lhs < cand_rhs) {
          return Value::compare_less;
        }
        if(cand_lhs > cand_rhs) {
          return Value::compare_greater;
        }
        return Value::compare_equal;
      }
    case Value::type_integer:
      {
        const auto &cand_lhs = lhs.as<D_integer>();
        const auto &cand_rhs = rhs.as<D_integer>();
        if(cand_lhs < cand_rhs) {
          return Value::compare_less;
        }
        if(cand_lhs > cand_rhs) {
          return Value::compare_greater;
        }
        return Value::compare_equal;
      }
    case Value::type_double:
      {
        const auto &cand_lhs = lhs.as<D_double>();
        const auto &cand_rhs = rhs.as<D_double>();
        if(std::isunordered(cand_lhs, cand_rhs)) {
          return Value::compare_unordered;
        }
        if(std::isless(cand_lhs, cand_rhs)) {
          return Value::compare_less;
        }
        if(std::isgreater(cand_lhs, cand_rhs)) {
          return Value::compare_greater;
        }
        return Value::compare_equal;
      }
    case Value::type_string:
      {
        const auto &cand_lhs = lhs.as<D_string>();
        const auto &cand_rhs = rhs.as<D_string>();
        const int cmp = cand_lhs.compare(cand_rhs);
        if(cmp < 0) {
          return Value::compare_less;
        }
        if(cmp > 0) {
          return Value::compare_greater;
        }
        return Value::compare_equal;
      }
    case Value::type_opaque:
    case Value::type_function:
      return Value::compare_unordered;
    case Value::type_array:
      {
        const auto &array_lhs = lhs.as<D_array>();
        const auto &array_rhs = rhs.as<D_array>();
        const auto rlen = std::min(array_lhs.size(), array_rhs.size());
        for(std::size_t i = 0; i < rlen; ++i) {
          const auto res = compare_values(array_lhs[i], array_rhs[i]);
          if(res != Value::compare_equal) {
            return res;
          }
        }
        if(array_lhs.size() < array_rhs.size()) {
          return Value::compare_less;
        }
        if(array_lhs.size() > array_rhs.size()) {
          return Value::compare_greater;
        }
        return Value::compare_equal;
      }
    case Value::type_object:
      return Value::compare_unordered;
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", lhs.type(), "` has been encountered.");
    }
  }

namespace {

  class Indent
    {
    private:
      std::size_t m_num;

    public:
      explicit constexpr Indent(std::size_t xnum) noexcept
        : m_num(xnum)
        {
        }

    public:
      std::size_t num() const noexcept
        {
          return m_num;
        }
    };

  inline std::ostream & operator<<(std::ostream &os, const Indent &indent)
    {
      const std::ostream::sentry sentry(os);
      if(!sentry) {
        return os;
      }
      using traits_type = std::ostream::traits_type;
      auto state = std::ios_base::goodbit;
      try {
        const auto num = indent.num();
        for(auto i = std::size_t(0); i != num; ++i) {
          // Write a space character.
          if(traits_type::eq_int_type(os.rdbuf()->sputc(' '), traits_type::eof())) {
            state |= std::ios_base::failbit;
            break;
          }
        }
      } catch(...) {
        // XXX: Relying on a private function is evil.
        rocket::details_cow_string::handle_io_exception(os);
        state &= ~std::ios_base::badbit;
      }
      if(state != std::ios_base::goodbit) {
        os.setstate(state);
      }
      os.width(0);
      return os;
    }

  class Quote
    {
    private:
      String m_str;

    public:
      explicit Quote(String xstr) noexcept
        : m_str(std::move(xstr))
        {
        }

    public:
      const String & str() const noexcept
        {
          return m_str;
        }
    };

  inline std::ostream & operator<<(std::ostream &os, const Quote &quote)
    {
      const std::ostream::sentry sentry(os);
      if(!sentry) {
        return os;
      }
      using traits_type = std::ostream::traits_type;
      auto state = std::ios_base::goodbit;
      try {
        const auto len = quote.str().length();
        for(auto i = std::size_t(-1); i != len + 1; ++i) {
          if((i == std::size_t(-1)) || (i == len)) {
            // Output double quote characters at both ends.
            if(traits_type::eq_int_type(os.rdbuf()->sputc('\"'), traits_type::eof())) {
              state |= std::ios_base::failbit;
              break;
            }
            continue;
          }
          const int ch = quote.str()[i] & 0xFF;
          if((0x20 <= ch) && (ch <= 0x7E)) {
            // Output an unescaped character.
            if(traits_type::eq_int_type(os.rdbuf()->sputc(static_cast<char>(ch)), traits_type::eof())) {
              state |= std::ios_base::failbit;
              break;
            }
            continue;
          }
          // Output an escape sequence.
          std::streamsize nwritten;
          switch(ch) {
          case '\"':
            nwritten = os.rdbuf()->sputn("\\\"", 2);
            break;
          case '\'':
            nwritten = os.rdbuf()->sputn("\\\'", 2);
            break;
          case '\\':
            nwritten = os.rdbuf()->sputn("\\\\", 2);
            break;
          case '\a':
            nwritten = os.rdbuf()->sputn("\\a", 2);
            break;
          case '\b':
            nwritten = os.rdbuf()->sputn("\\b", 2);
            break;
          case '\f':
            nwritten = os.rdbuf()->sputn("\\f", 2);
            break;
          case '\n':
            nwritten = os.rdbuf()->sputn("\\n", 2);
            break;
          case '\r':
            nwritten = os.rdbuf()->sputn("\\r", 2);
            break;
          case '\t':
            nwritten = os.rdbuf()->sputn("\\t", 2);
            break;
          case '\v':
            nwritten = os.rdbuf()->sputn("\\v", 2);
            break;
          default:
            {
              static constexpr char s_hex_table[] = "0123456789ABCDEF";
              char temp[4] = "\\x";
              temp[2] = s_hex_table[(ch >> 4) & 0x0F];
              temp[3] = s_hex_table[(ch >> 0) & 0x0F];
              nwritten = os.rdbuf()->sputn(temp, 4);
            }
            break;
          }
          if(nwritten == 0) {
            state |= std::ios_base::failbit;
            break;
          }
        }
      } catch(...) {
        // XXX: Relying on a private function is evil.
        rocket::details_cow_string::handle_io_exception(os);
        state &= ~std::ios_base::badbit;
      }
      if(state != std::ios_base::goodbit) {
        os.setstate(state);
      }
      os.width(0);
      return os;
    }

}

void dump_value(std::ostream &os, const Value &value, std::size_t indent_next, std::size_t indent_increment)
  {
    const auto type = value.type();
    switch(type) {
    case Value::type_null:
      // null
      os <<"null";
      return;
    case Value::type_boolean:
      {
        const auto &alt = value.as<D_boolean>();
        // boolean true
        os <<"boolean " <<std::boolalpha <<std::nouppercase <<alt;
        return;
      }
    case Value::type_integer:
      {
        const auto &alt = value.as<D_integer>();
        // integer 42
        os <<"integer " <<std::dec <<alt;
        return;
      }
    case Value::type_double:
      {
        const auto &alt = value.as<D_double>();
        // double 123.456
        os <<"double " <<std::dec <<std::nouppercase <<std::setprecision(std::numeric_limits<D_double>::max_digits10) <<alt;
        return;
      }
    case Value::type_string:
      {
        const auto &alt = value.as<D_string>();
        // string(5) "hello"
        os <<"string(" <<std::dec <<alt.size() <<") " <<Quote(alt);
        return;
      }
    case Value::type_opaque:
      {
        const auto &alt = value.as<D_opaque>();
        // opaque("typeid") "my opaque"
        os <<"opaque(\"" <<typeid(*alt).name() <<"\") " <<Quote(alt->describe());
        return;
      }
    case Value::type_function:
      {
        const auto &alt = value.as<D_function>();
        // function("typeid") "my function"
        os <<"function(\"" <<typeid(*alt).name() <<"\") " <<Quote(alt->describe());
        return;
      }
    case Value::type_array:
      {
        const auto &alt = value.as<D_array>();
        // array(3) = [
        //   0 = integer 1,
        //   1 = integer 2,
        //   2 = integer 3,
        // ]
        os <<"array(" <<std::dec <<alt.size() <<") [";
        for(auto it = alt.begin(); it != alt.end(); ++it) {
          os <<std::endl <<Indent(indent_next + indent_increment);
          os <<std::dec <<(it - alt.begin());
          os <<" = ";
          dump_value(os, *it, indent_next + indent_increment, indent_increment);
          os <<',';
        }
        os <<std::endl <<Indent(indent_next) <<']';
        return;
      }
    case Value::type_object:
      {
        const auto &alt = value.as<D_object>();
        // object(3) = {
        //   "one" = integer 1,
        //   "two" = integer 2,
        //   "three" = integer 3,
        // }
        os <<"object(" <<std::dec <<alt.size() <<") {";
        for(auto it = alt.begin(); it != alt.end(); ++it) {
          os <<std::endl <<Indent(indent_next + indent_increment);
          os <<Quote(it->first);
          os <<" = ";
          dump_value(os, it->second, indent_next + indent_increment, indent_increment);
          os <<',';
        }
        os <<std::endl <<Indent(indent_next) <<'}';
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", type, "` has been encountered.");
    }
  }

std::ostream & operator<<(std::ostream &os, const Value &value)
  {
    dump_value(os, value);
    return os;
  }

}
