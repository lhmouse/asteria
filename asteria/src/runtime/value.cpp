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
    static constexpr std::aligned_union<0, Value> s_null = { };
    // This two-step conversion is necessary to eliminate warnings when strict aliasing is in effect.
    const void* pv = std::addressof(s_null);
    return *(static_cast<const Value*>(pv));
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

    namespace {

    template<typename ElementT> Value::Compare do_three_way_compare(const ElementT& lhs, const ElementT& rhs)
      {
        if(lhs < rhs) {
          return Value::compare_less;
        }
        if(rhs < lhs) {
          return Value::compare_greater;
        }
        return Value::compare_equal;
      }

    template<typename IteratorT> Value::Compare do_lexicographical_compare(IteratorT s1, IteratorT s2, std::size_t n)
      {
        auto p1 = rocket::move(s1);
        auto e1 = p1 + static_cast<typename std::iterator_traits<IteratorT>::difference_type>(n);
        auto p2 = rocket::move(s2);
        for(;;) {
          if(p1 == e1) {
            break;
          }
          auto res = p1->compare(*p2);
          if(res != Value::compare_equal) {
            return res;
          }
          ++p1;
          ++p2;
        }
        return Value::compare_equal;
      }

    }  // namespace

Value::Compare Value::compare(const Value& other) const noexcept
  {
    // Values of different types can only be compared if either of them is `null`.
    if(this->dtype() != other.dtype()) {
      // `null` is considered to be equal to `null` and less than anything else.
      if(this->dtype() == dtype_null) {
        return Value::compare_less;
      }
      if(other.dtype() == dtype_null) {
        return Value::compare_greater;
      }
      return Value::compare_unordered;
    }
    // If both values have the same type, perform normal comparison.
    switch(this->dtype()) {
    case dtype_null:
      {
        return Value::compare_equal;
      }
    case dtype_boolean:
      {
        const auto& lhs = this->check<D_boolean>();
        const auto& rhs = other.check<D_boolean>();
        return do_three_way_compare(lhs, rhs);
      }
    case dtype_integer:
      {
        const auto& lhs = this->check<D_integer>();
        const auto& rhs = other.check<D_integer>();
        return do_three_way_compare(lhs, rhs);
      }
    case dtype_real:
      {
        const auto& lhs = this->check<D_real>();
        const auto& rhs = other.check<D_real>();
        if(std::isunordered(lhs, rhs)) {
          return Value::compare_unordered;
        }
        return do_three_way_compare(lhs, rhs);
      }
    case dtype_string:
      {
        const auto& lhs = this->check<D_string>();
        const auto& rhs = other.check<D_string>();
        return do_three_way_compare(lhs.compare(rhs), 0);
      }
    case dtype_opaque:
    case dtype_function:
      {
        return Value::compare_unordered;
      }
    case dtype_array:
      {
        const auto& lhs = this->check<D_array>();
        const auto& rhs = other.check<D_array>();
        // Compare elements lexicographically.
        auto nlhs = lhs.size();
        auto nrhs = rhs.size();
        if(nlhs < nrhs) {
          auto res = do_lexicographical_compare(lhs.begin(), rhs.begin(), nlhs);
          if(res != compare_equal) {
            return res;
          }
          return compare_less;
        }
        if(nlhs > nrhs) {
          auto res = do_lexicographical_compare(lhs.begin(), rhs.begin(), nrhs);
          if(res != compare_equal) {
            return res;
          }
          return compare_greater;
        }
        return do_lexicographical_compare(lhs.begin(), rhs.begin(), nlhs);
      }
    case dtype_object:
      {
        return Value::compare_unordered;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->dtype(), "` has been encountered.");
    }
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
