// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "value.hpp"
#include "utilities.hpp"

namespace Asteria {

const char * Value::get_type_name(Value::Type type) noexcept
  {
    switch(type) {
    case type_null:
      return "null";
    case type_boolean:
      return "boolean";
    case type_integer:
      return "integer";
    case type_real:
      return "real";
    case type_string:
      return "string";
    case type_opaque:
      return "opaque";
    case type_function:
      return "function";
    case type_array:
      return "array";
    case type_object:
      return "object";
    default:
      return "<unknown>";
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
    case type_null:
      return false;
    case type_boolean:
      {
        const auto &alt = this->check<D_boolean>();
        return alt;
      }
    case type_integer:
      {
        const auto &alt = this->check<D_integer>();
        return alt != 0;
      }
    case type_real:
      {
        const auto &alt = this->check<D_real>();
        return std::fpclassify(alt) != FP_ZERO;
      }
    case type_string:
      {
        const auto &alt = this->check<D_string>();
        return alt.empty() == false;
      }
    case type_opaque:
    case type_function:
      return true;
    case type_array:
      {
        const auto &alt = this->check<D_array>();
        return alt.empty() == false;
      }
    case type_object:
      {
        const auto &alt = this->check<D_object>();
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
      if(this->type() == type_null) {
        return Value::compare_less;
      }
      if(other.type() == type_null) {
        return Value::compare_greater;
      }
      return Value::compare_unordered;
    }
    // If both operands have the same type, perform normal comparison.
    switch(this->type()) {
    case type_null:
      return Value::compare_equal;
    case type_boolean:
      {
        const auto &alt_lhs = this->check<D_boolean>();
        const auto &alt_rhs = other.check<D_boolean>();
        if(alt_lhs < alt_rhs) {
          return Value::compare_less;
        }
        if(alt_lhs > alt_rhs) {
          return Value::compare_greater;
        }
        return Value::compare_equal;
      }
    case type_integer:
      {
        const auto &alt_lhs = this->check<D_integer>();
        const auto &alt_rhs = other.check<D_integer>();
        if(alt_lhs < alt_rhs) {
          return Value::compare_less;
        }
        if(alt_lhs > alt_rhs) {
          return Value::compare_greater;
        }
        return Value::compare_equal;
      }
    case type_real:
      {
        const auto &alt_lhs = this->check<D_real>();
        const auto &alt_rhs = other.check<D_real>();
        if(std::isunordered(alt_lhs, alt_rhs)) {
          return Value::compare_unordered;
        }
        if(std::isless(alt_lhs, alt_rhs)) {
          return Value::compare_less;
        }
        if(std::isgreater(alt_lhs, alt_rhs)) {
          return Value::compare_greater;
        }
        return Value::compare_equal;
      }
    case type_string:
      {
        const auto &alt_lhs = this->check<D_string>();
        const auto &alt_rhs = other.check<D_string>();
        const int cmp = alt_lhs.compare(alt_rhs);
        if(cmp < 0) {
          return Value::compare_less;
        }
        if(cmp > 0) {
          return Value::compare_greater;
        }
        return Value::compare_equal;
      }
    case type_opaque:
    case type_function:
      return Value::compare_unordered;
    case type_array:
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
    case type_object:
      return Value::compare_unordered;
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->type(), "` has been encountered.");
    }
  }

namespace {

  inline Indent do_indent_or_space(Size indent_increment, Size indent_next)
    {
      return (indent_increment != 0) ? Indent('\n', indent_next) : Indent(' ', 0);
    }

}

void Value::dump(std::ostream &os, Size indent_increment, Size indent_next) const
  {
    switch(this->type()) {
    case type_null:
      // null
      os <<"null";
      return;
    case type_boolean:
      {
        const auto &alt = this->check<D_boolean>();
        // boolean true
        os <<"boolean " <<std::boolalpha <<std::nouppercase <<alt;
        return;
      }
    case type_integer:
      {
        const auto &alt = this->check<D_integer>();
        // integer 42
        os <<"integer " <<std::dec <<alt;
        return;
      }
    case type_real:
      {
        const auto &alt = this->check<D_real>();
        // real 123.456
        os <<"real " <<std::dec <<std::nouppercase <<std::setprecision(std::numeric_limits<D_real>::max_digits10) <<alt;
        return;
      }
    case type_string:
      {
        const auto &alt = this->check<D_string>();
        // string(5) "hello"
        os <<"string(" <<std::dec <<alt.size() <<") \"" <<escape(alt) <<"\"";
        return;
      }
    case type_opaque:
      {
        const auto &alt = this->check<D_opaque>();
        // opaque("typeid") "my opaque"
        os <<"opaque(\"" <<typeid(*alt).name() <<"\") \"" <<escape(alt->describe()) <<"\"";
        return;
      }
    case type_function:
      {
        const auto &alt = this->check<D_function>();
        // function("typeid") "my function"
        os <<"function(\"" <<typeid(*alt).name() <<"\") \"" <<escape(alt->describe()) <<"\"";
        return;
      }
    case type_array:
      {
        const auto &alt = this->check<D_array>();
        // array(3) = [
        //   0 = integer 1,
        //   1 = integer 2,
        //   2 = integer 3,
        // ]
        os <<"array(" <<std::dec <<alt.size() <<") [";
        for(auto it = alt.begin(); it != alt.end(); ++it) {
          os <<do_indent_or_space(indent_increment, indent_next + indent_increment) <<std::dec <<(it - alt.begin()) <<" = ";
          it->dump(os, indent_increment, indent_next + indent_increment);
          os <<',';
        }
        os <<do_indent_or_space(indent_increment, indent_next) <<']';
        return;
      }
    case type_object:
      {
        const auto &alt = this->check<D_object>();
        // object(3) = {
        //   "one" = integer 1,
        //   "two" = integer 2,
        //   "three" = integer 3,
        // }
        os <<"object(" <<std::dec <<alt.size() <<") {";
        for(auto it = alt.begin(); it != alt.end(); ++it) {
          os <<do_indent_or_space(indent_increment, indent_next + indent_increment) <<"\"" <<escape(it->first) <<"\" = ";
          it->second.dump(os, indent_increment, indent_next + indent_increment);
          os <<',';
        }
        os <<do_indent_or_space(indent_increment, indent_next) <<'}';
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->type(), "` has been encountered.");
    }
  }

}
