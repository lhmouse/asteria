// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "value.hpp"
#include "../utilities.hpp"

namespace Asteria {

const char* Value::get_type_name(Dtype etype) noexcept
  {
    switch(etype) {
    case dtype_null:
      {
        return "null";
      }
    case dtype_boolean:
      {
        return "boolean";
      }
    case dtype_integer:
      {
        return "integer";
      }
    case dtype_real:
      {
        return "real";
      }
    case dtype_string:
      {
        return "string";
      }
    case dtype_opaque:
      {
        return "opaque";
      }
    case dtype_function:
      {
        return "function";
      }
    case dtype_array:
      {
        return "array";
      }
    case dtype_object:
      {
        return "object";
      }
    default:
      return "<unknown type>";
    }
  }

const Value& Value::get_null() noexcept
  {
    // Don't play with this at home.
    static constexpr std::aligned_union<0, Value>::type s_null = { };
    // This two-step conversion is necessary to eliminate warnings when strict aliasing is in effect.
    const void* pv = std::addressof(s_null);
    return *(static_cast<const Value*>(pv));
  }

bool Value::test() const noexcept
  {
    switch(this->dtype()) {
    case dtype_null:
      {
        return false;
      }
    case dtype_boolean:
      {
        return this->check<D_boolean>();
      }
    case dtype_integer:
      {
        return this->check<D_integer>() != 0;
      }
    case dtype_real:
      {
        return std::fpclassify(this->check<D_real>()) != FP_ZERO;
      }
    case dtype_string:
      {
        return this->check<D_string>().size() != 0;
      }
    case dtype_opaque:
    case dtype_function:
      {
        return true;
      }
    case dtype_array:
      {
        return this->check<D_array>().size() != 0;
      }
    case dtype_object:
      {
        return true;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->dtype(), "` has been encountered.");
    }
  }

Value::Compare Value::do_compare_partial(const Value& other) const noexcept
  {
    if(this->dtype() == dtype_null) {
      // `null` compares equal with `null` and less than anything else.
      if(other.dtype() == dtype_null) {
        return compare_equal;
      }
      return compare_less;
    }
    if((this->dtype() == dtype_boolean) && (other.dtype() == dtype_boolean)) {
      // Compare `boolean` values as integers.
      const auto& lhs = this->check<D_boolean>();
      const auto& rhs = other.check<D_boolean>();
      if(lhs == rhs) {
        return compare_equal;
      }
      return (lhs < rhs) ? compare_less : compare_greater;
    }
    if((this->dtype() == dtype_integer) && (other.dtype() == dtype_integer)) {
      // Compare `integer` values.
      const auto& lhs = this->check<D_integer>();
      const auto& rhs = other.check<D_integer>();
      if(lhs == rhs) {
        return compare_equal;
      }
      return (lhs < rhs) ? compare_less : compare_greater;
    }
    if((this->dtype() == dtype_integer) && (other.dtype() == dtype_real)) {
      // Compare an `integer` value and a `real` value.
      const auto& lhs = this->check<D_integer>();
      const auto& rhs = other.check<D_real>();
      // Cast the `integer` to type `real` and compare the result with the other.
      auto rlhs = D_real(lhs);
      if(std::isunordered(rlhs, rhs)) {
        return compare_unordered;
      }
      if(rlhs == rhs) {
        return compare_equal;
      }
      return (rlhs < rhs) ? compare_less : compare_greater;
    }
    if((this->dtype() == dtype_real) && (other.dtype() == dtype_real)) {
      // Compare `real` values.
      const auto& lhs = this->check<D_real>();
      const auto& rhs = other.check<D_real>();
      if(std::isunordered(lhs, rhs)) {
        return compare_unordered;
      }
      if(lhs == rhs) {
        return compare_equal;
      }
      return (lhs < rhs) ? compare_less : compare_greater;
    }
    if((this->dtype() == dtype_string) && (other.dtype() == dtype_string)) {
      // Compare `string` values.
      const auto& lhs = this->check<D_string>();
      const auto& rhs = other.check<D_string>();
      // Make use of the three-way comparison result of `D_string::compare()`.
      auto res = lhs.compare(rhs);
      if(res == 0) {
        return compare_equal;
      }
      return (res < 0) ? compare_less : compare_greater;
    }
    if((this->dtype() == dtype_array) && (other.dtype() == dtype_array)) {
      // Compare `array` values.
      const auto& lhs = this->check<D_array>();
      const auto& rhs = other.check<D_array>();
      // Perform lexicographical comparison of array elements.
      auto nlimit = rocket::min(lhs.size(), rhs.size());
      for(std::size_t i = 0; i < nlimit; ++i) {
        auto res = lhs[i].compare(rhs[i]);
        if(res != compare_equal) {
          return res;
        }
      }
      if(lhs.size() == rhs.size()) {
        return compare_equal;
      }
      return (lhs.size() < rhs.size()) ? compare_less : compare_greater;
    }
    // Anything not defined here is unordered.
    return compare_unordered;
  }

Value::Compare Value::compare(const Value& other) const noexcept
  {
    if(this->dtype() <= other.dtype()) {
      // Compare them in the natural order.
      return this->do_compare_partial(other);
    }
    // Swap the operands and compare them, then translate `compare_{less,greater}` to the other but preserve the other results.
    int cmp = other.do_compare_partial(*this);
    cmp ^= cmp >> 3;
    return static_cast<Compare>(cmp);
  }

void Value::print(std::ostream& os, bool quote_strings) const
  {
    switch(this->dtype()) {
    case dtype_null:
      {
        // null
        os << "null";
        return;
      }
    case dtype_boolean:
      {
        const auto& alt = this->check<D_boolean>();
        // true
        os << std::boolalpha << std::nouppercase << alt;
        return;
      }
    case dtype_integer:
      {
        const auto& alt = this->check<D_integer>();
        // 42
        os << std::dec << alt;
        return;
      }
    case dtype_real:
      {
        const auto& alt = this->check<D_real>();
        // 123.456
        os << std::dec << std::nouppercase << std::setprecision(DECIMAL_DIG) << alt;
        return;
      }
    case dtype_string:
      {
        const auto& alt = this->check<D_string>();
        if(quote_strings) {
          // "hello"
          os << quote(alt);
          return;
        }
        // hello
        os << alt;
        return;
      }
    case dtype_opaque:
      {
        const auto& alt = this->check<D_opaque>();
        // <opaque> [[`my opaque`]]
        os << "<opaque> [[`" << alt.get() << "`]]";
        return;
      }
    case dtype_function:
      {
        const auto& alt = this->check<D_function>();
        // <function> [[`my function`]]
        os << "<function> [[`" << alt.get() << "`]]";
        return;
      }
    case dtype_array:
      {
        const auto& alt = this->check<D_array>();
        // [ 1; 2; 3; ]
        os << '[';
        for(auto it = alt.begin(); it != alt.end(); ++it) {
          os << ' ';
          it->print(os, true);
          os << ';';
        }
        os << ' ' << ']';
        return;
      }
    case dtype_object:
      {
        const auto& alt = this->check<D_object>();
        // { "one" = 1; "two" = 2; "three" = 3; }
        os << '{';
        for(auto it = alt.begin(); it != alt.end(); ++it) {
          os << ' ';
          os << quote(it->first);
          os << " = ";
          it->second.print(os, true);
          os << ';';
        }
        os << ' ' << '}';
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->dtype(), "` has been encountered.");
    }
  }

std::ostream& Value::do_auto_indent(std::ostream& os, std::size_t indent_increment, std::size_t indent_next) const
  {
    if(indent_increment == 0) {
      // Output everything in a single line. Characters are separated by spaces.
      return os << ' ';
    }
    // Terminate the current line and indent it accordingly.
    os << std::endl;
    os.width(static_cast<std::streamsize>(indent_next));
    return os << "";
  }

void Value::dump(std::ostream& os, std::size_t indent_increment, std::size_t indent_next) const
  {
    switch(this->dtype()) {
    case dtype_null:
      {
        // null
        os << "null";
        return;
      }
    case dtype_boolean:
      {
        const auto& alt = this->check<D_boolean>();
        // boolean true
        os << "boolean " << std::boolalpha << std::nouppercase << alt;
        return;
      }
    case dtype_integer:
      {
        const auto& alt = this->check<D_integer>();
        // integer 42
        os << "integer " << std::dec << alt;
        return;
      }
    case dtype_real:
      {
        const auto& alt = this->check<D_real>();
        // real 123.456
        os << "real " << std::dec << std::nouppercase << std::setprecision(DECIMAL_DIG) << alt;
        return;
      }
    case dtype_string:
      {
        const auto& alt = this->check<D_string>();
        // string(5) "hello"
        os << "string(" << std::dec << alt.size() << ") " << quote(alt);
        return;
      }
    case dtype_opaque:
      {
        const auto& alt = this->check<D_opaque>();
        // opaque("typeid") [[`my opaque`]]
        os << "opaque(" << quote(typeid(alt.get()).name()) << ") [[`" << alt.get() << "`]]";
        return;
      }
    case dtype_function:
      {
        const auto& alt = this->check<D_function>();
        // function("typeid") [[`my function`]]
        os << "function(" << quote(typeid(alt.get()).name()) << ") [[`" << alt.get() << "`]]";
        return;
      }
    case dtype_array:
      {
        const auto& alt = this->check<D_array>();
        // array(3) =
        //  [
        //   0 = integer 1;
        //   1 = integer 2;
        //   2 = integer 3;
        //  ]
        os << "array(" << std::dec << alt.size() << ")";
        this->do_auto_indent(os, indent_increment, indent_next + 1);
        os << '[';
        for(auto it = alt.begin(); it != alt.end(); ++it) {
          this->do_auto_indent(os, indent_increment, indent_next + indent_increment);
          os << std::dec << (it - alt.begin()) << " = ";
          it->dump(os, indent_increment, indent_next + indent_increment);
          os << ';';
        }
        this->do_auto_indent(os, indent_increment, indent_next + 1);
        os << ']';
        return;
      }
    case dtype_object:
      {
        const auto& alt = this->check<D_object>();
        // object(3) =
        //  {
        //   "one" = integer 1;
        //   "two" = integer 2;
        //   "three" = integer 3;
        //  }
        os << "object(" << std::dec << alt.size() << ")";
        this->do_auto_indent(os, indent_increment, indent_next + 1);
        os << '{';
        for(auto it = alt.begin(); it != alt.end(); ++it) {
          this->do_auto_indent(os, indent_increment, indent_next + indent_increment);
          os << quote(it->first) << " = ";
          it->second.dump(os, indent_increment, indent_next + indent_increment);
          os << ';';
        }
        this->do_auto_indent(os, indent_increment, indent_next + 1);
        os << '}';
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->dtype(), "` has been encountered.");
    }
  }

bool Value::unique() const noexcept
  {
    switch(this->dtype()) {
    case dtype_null:
    case dtype_boolean:
    case dtype_integer:
    case dtype_real:
      {
        return true;
      }
    case dtype_string:
      {
        return this->check<D_string>().unique();
      }
    case dtype_opaque:
      {
        return this->check<D_opaque>().unique();
      }
    case dtype_function:
      {
        return this->check<D_function>().unique();
      }
    case dtype_array:
      {
        return this->check<D_array>().unique();
      }
    case dtype_object:
      {
        return this->check<D_object>().unique();
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->dtype(), "` has been encountered.");
    }
  }

long Value::use_count() const noexcept
  {
    switch(this->dtype()) {
    case dtype_null:
      {
        return 0;
      }
    case dtype_boolean:
    case dtype_integer:
    case dtype_real:
      {
        return 1;
      }
    case dtype_string:
      {
        return this->check<D_string>().use_count();
      }
    case dtype_opaque:
      {
        return this->check<D_opaque>().use_count();
      }
    case dtype_function:
      {
        return this->check<D_function>().use_count();
      }
    case dtype_array:
      {
        return this->check<D_array>().use_count();
      }
    case dtype_object:
      {
        return this->check<D_object>().use_count();
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->dtype(), "` has been encountered.");
    }
  }

void Value::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    switch(this->dtype()) {
    case dtype_null:
    case dtype_boolean:
    case dtype_integer:
    case dtype_real:
    case dtype_string:
      {
        return;
      }
    case dtype_opaque:
      {
        this->check<D_opaque>()->enumerate_variables(callback);
        return;
      }
    case dtype_function:
      {
        this->check<D_function>()->enumerate_variables(callback);
        return;
      }
    case dtype_array:
      {
        rocket::for_each(this->check<D_array>(), [&](const auto& elem) { elem.enumerate_variables(callback);  });
        return;
      }
    case dtype_object:
      {
        rocket::for_each(this->check<D_object>(), [&](const auto& pair) { pair.second.enumerate_variables(callback);  });
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->dtype(), "` has been encountered.");
    }
  }

}  // namespace Asteria
