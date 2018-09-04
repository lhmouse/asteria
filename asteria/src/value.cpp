// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "value.hpp"
#include "utilities.hpp"

namespace Asteria {

const char * Value::get_type_name(Value::Type type) noexcept
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

Value::~Value()
  {
  }

Value::Value(const Value &) noexcept
  = default;
Value & Value::operator=(const Value &) noexcept
  = default;
Value::Value(Value &&) noexcept
  = default;
Value & Value::operator=(Value &&) noexcept
  = default;

bool Value::test() const noexcept
  {
    switch(this->type()) {
    case Value::type_null:
      return false;
    case Value::type_boolean:
      {
        const auto &alt = this->m_stor.as<D_boolean>();
        return alt;
      }
    case Value::type_integer:
      {
        const auto &alt = this->m_stor.as<D_integer>();
        return alt != 0;
      }
    case Value::type_double:
      {
        const auto &alt = this->m_stor.as<D_double>();
        return std::fpclassify(alt) != FP_ZERO;
      }
    case Value::type_string:
      {
        const auto &alt = this->m_stor.as<D_string>();
        return alt.empty() == false;
      }
    case Value::type_opaque:
    case Value::type_function:
      return true;
    case Value::type_array:
      {
        const auto &alt = this->m_stor.as<D_array>();
        return alt.empty() == false;
      }
    case Value::type_object:
      {
        const auto &alt = this->m_stor.as<D_object>();
        return alt.empty() == false;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->type(), "` has been encountered.");
    }
  }

Value::Compare Value::compare(const Value &other) const noexcept
  {
    // `null` is considered to be equal to `null` and less than anything else.
    if(this->type() != other.type()) {
      if(this->type() == Value::type_null) {
        return Value::compare_less;
      }
      if(other.type() == Value::type_null) {
        return Value::compare_greater;
      }
      return Value::compare_unordered;
    }
    // If both operands have the same type, perform normal comparison.
    switch(this->type()) {
    case Value::type_null:
      return Value::compare_equal;
    case Value::type_boolean:
      {
        const auto &cand_lhs = this->check<D_boolean>();
        const auto &cand_rhs = other.check<D_boolean>();
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
        const auto &cand_lhs = this->check<D_integer>();
        const auto &cand_rhs = other.check<D_integer>();
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
        const auto &cand_lhs = this->check<D_double>();
        const auto &cand_rhs = other.check<D_double>();
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
        const auto &cand_lhs = this->check<D_string>();
        const auto &cand_rhs = other.check<D_string>();
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
        const auto &array_lhs = this->check<D_array>();
        const auto &array_rhs = other.check<D_array>();
        const auto rlen = rocket::min(array_lhs.size(), array_rhs.size());
        for(Size i = 0; i < rlen; ++i) {
          const auto res = array_lhs[i].compare(array_rhs[i]);
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
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->type(), "` has been encountered.");
    }
  }

namespace {

  class Indent
    {
    private:
      bool m_nl;
      Size m_cnt;

    public:
      constexpr Indent(bool nl, Size cnt) noexcept
        : m_nl(nl), m_cnt(cnt)
        {
        }

    public:
      bool newline() const noexcept
        {
          return this->m_nl;
        }
      Size count() const noexcept
        {
          return this->m_cnt;
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
        if(indent.newline()) {
          // Write a line feed.
          if(traits_type::eq_int_type(os.rdbuf()->sputc('\n'), traits_type::eof())) {
            state |= std::ios_base::failbit;
            goto done;
          }
          for(auto i = indent.count(); i != 0; --i) {
            // Write space characters.
            if(traits_type::eq_int_type(os.rdbuf()->sputc(' '), traits_type::eof())) {
              state |= std::ios_base::failbit;
              goto done;
            }
          }
        } else {
          // Write a space character.
          if(traits_type::eq_int_type(os.rdbuf()->sputc(' '), traits_type::eof())) {
            state |= std::ios_base::failbit;
            goto done;
          }
        }
      } catch(...) {
        rocket::handle_ios_exception(os);
        state &= ~std::ios_base::badbit;
      }
    done:
      if(state) {
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
      explicit Quote(String str) noexcept
        : m_str(std::move(str))
        {
        }

    public:
      const char * begin() const noexcept
        {
          return this->m_str.data();
        }
      const char * end() const noexcept
        {
          return this->m_str.data() + this->m_str.size();
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
        // Output double quote characters at the beginning.
        if(traits_type::eq_int_type(os.rdbuf()->sputc('\"'), traits_type::eof())) {
          state |= std::ios_base::failbit;
          goto done;
        }
        // Output the string body.
        for(const char ch : quote) {
          if((0x20 <= ch) && (ch <= 0x7E)) {
            // Output an unescaped character.
            if(traits_type::eq_int_type(os.rdbuf()->sputc(ch), traits_type::eof())) {
              state |= std::ios_base::failbit;
              goto done;
            }
          } else {
            // Output an escape sequence.
            bool failed;
            switch(ch) {
            case '\"':
              failed = os.rdbuf()->sputn("\\\"", 2) < 2;
              break;
            case '\'':
              failed = os.rdbuf()->sputn("\\\'", 2) < 2;
              break;
            case '\\':
              failed = os.rdbuf()->sputn("\\\\", 2) < 2;
              break;
            case '\a':
              failed = os.rdbuf()->sputn("\\a", 2) < 2;
              break;
            case '\b':
              failed = os.rdbuf()->sputn("\\b", 2) < 2;
              break;
            case '\f':
              failed = os.rdbuf()->sputn("\\f", 2) < 2;
              break;
            case '\n':
              failed = os.rdbuf()->sputn("\\n", 2) < 2;
              break;
            case '\r':
              failed = os.rdbuf()->sputn("\\r", 2) < 2;
              break;
            case '\t':
              failed = os.rdbuf()->sputn("\\t", 2) < 2;
              break;
            case '\v':
              failed = os.rdbuf()->sputn("\\v", 2) < 2;
              break;
            default:
              {
                static constexpr char s_hex_table[] = "0123456789ABCDEF";
                char temp[4] = "\\x";
                temp[2] = s_hex_table[(ch >> 4) & 0x0F];
                temp[3] = s_hex_table[(ch >> 0) & 0x0F];
                failed = os.rdbuf()->sputn(temp, 4) < 4;
              }
              break;
            }
            if(failed) {
              state |= std::ios_base::failbit;
              goto done;
            }
          }
        }
        // Output double quote characters at the end.
        if(traits_type::eq_int_type(os.rdbuf()->sputc('\"'), traits_type::eof())) {
          state |= std::ios_base::failbit;
          goto done;
        }
      } catch(...) {
        rocket::handle_ios_exception(os);
        state &= ~std::ios_base::badbit;
      }
    done:
      if(state) {
        os.setstate(state);
      }
      os.width(0);
      return os;
    }

}

void Value::dump(std::ostream &os, Size indent_increment, Size indent_next) const
  {
    switch(this->type()) {
    case Value::type_null:
      // null
      os <<"null";
      return;
    case Value::type_boolean:
      {
        const auto &alt = this->m_stor.as<D_boolean>();
        // boolean true
        os <<"boolean " <<std::boolalpha <<std::nouppercase <<alt;
        return;
      }
    case Value::type_integer:
      {
        const auto &alt = this->m_stor.as<D_integer>();
        // integer 42
        os <<"integer " <<std::dec <<alt;
        return;
      }
    case Value::type_double:
      {
        const auto &alt = this->m_stor.as<D_double>();
        // double 123.456
        os <<"double " <<std::dec <<std::nouppercase <<std::setprecision(std::numeric_limits<D_double>::max_digits10) <<alt;
        return;
      }
    case Value::type_string:
      {
        const auto &alt = this->m_stor.as<D_string>();
        // string(5) "hello"
        os <<"string(" <<std::dec <<alt.size() <<") " <<Quote(alt);
        return;
      }
    case Value::type_opaque:
      {
        const auto &alt = this->m_stor.as<D_opaque>();
        // opaque("typeid") "my opaque"
        os <<"opaque(\"" <<typeid(*alt).name() <<"\") " <<Quote(alt->describe());
        return;
      }
    case Value::type_function:
      {
        const auto &alt = this->m_stor.as<D_function>();
        // function("typeid") "my function"
        os <<"function(\"" <<typeid(*alt).name() <<"\") " <<Quote(alt->describe());
        return;
      }
    case Value::type_array:
      {
        const auto &alt = this->m_stor.as<D_array>();
        // array(3) = [
        //   0 = integer 1,
        //   1 = integer 2,
        //   2 = integer 3,
        // ]
        os <<"array(" <<std::dec <<alt.size() <<") [";
        for(auto it = alt.begin(); it != alt.end(); ++it) {
          os <<Indent(indent_increment != 0, indent_next + indent_increment) <<std::dec <<(it - alt.begin()) <<" = ";
          it->dump(os, indent_increment, indent_next + indent_increment);
          os <<',';
        }
        os <<Indent(indent_increment != 0, indent_next) <<']';
        return;
      }
    case Value::type_object:
      {
        const auto &alt = this->m_stor.as<D_object>();
        // object(3) = {
        //   "one" = integer 1,
        //   "two" = integer 2,
        //   "three" = integer 3,
        // }
        os <<"object(" <<std::dec <<alt.size() <<") {";
        for(auto it = alt.begin(); it != alt.end(); ++it) {
          os <<Indent(indent_increment != 0, indent_next + indent_increment) <<Quote(it->first) <<" = ";
          it->second.dump(os, indent_increment, indent_next + indent_increment);
          os <<',';
        }
        os <<Indent(indent_increment != 0, indent_next) <<'}';
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->type(), "` has been encountered.");
    }
  }

}
