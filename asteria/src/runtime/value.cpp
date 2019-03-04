// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "value.hpp"
#include "../utilities.hpp"

namespace Asteria {

const char * Value::get_type_name(Value_Type etype) noexcept
  {
    switch(etype) {
    case type_null:
      {
        return "null";
      }
    case type_boolean:
      {
        return "boolean";
      }
    case type_integer:
      {
        return "integer";
      }
    case type_real:
      {
        return "real";
      }
    case type_string:
      {
        return "string";
      }
    case type_opaque:
      {
        return "opaque";
      }
    case type_function:
      {
        return "function";
      }
    case type_array:
      {
        return "array";
      }
    case type_object:
      {
        return "object";
      }
    default:
      return "<unknown type>";
    }
  }

    namespace {

    alignas(Value) struct
      {
        char bytes[sizeof(Value)];
      }
    const s_null = { 0 };  // Don't play with this at home.

    }

const Value & Value::get_null() noexcept
  {
    return *static_cast<const Value *>(static_cast<const void *>(s_null.bytes));
  }

bool Value::test() const noexcept
  {
    switch(this->type()) {
    case type_null:
      {
        return false;
      }
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
        return alt.size() != 0;
      }
    case type_opaque:
    case type_function:
      {
        return true;
      }
    case type_array:
      {
        const auto &alt = this->check<D_array>();
        return alt.size() != 0;
      }
    case type_object:
      {
        return true;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->type(), "` has been encountered.");
    }
  }

    namespace {

    template<typename ElementT> Value::Compare do_three_way_compare(const ElementT &lhs, const ElementT &rhs)
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

    }

Value::Compare Value::compare(const Value &other) const noexcept
  {
    // Values of different types can only be compared if either of them is `null`.
    if(this->type() != other.type()) {
      // `null` is considered to be equal to `null` and less than anything else.
      if(this->type() == type_null) {
        return Value::compare_less;
      }
      if(other.type() == type_null) {
        return Value::compare_greater;
      }
      return Value::compare_unordered;
    }
    // If both values have the same type, perform normal comparison.
    switch(this->type()) {
    case type_null:
      {
        return Value::compare_equal;
      }
    case type_boolean:
      {
        const auto &lhs = this->check<D_boolean>();
        const auto &rhs = other.check<D_boolean>();
        return do_three_way_compare(lhs, rhs);
      }
    case type_integer:
      {
        const auto &lhs = this->check<D_integer>();
        const auto &rhs = other.check<D_integer>();
        return do_three_way_compare(lhs, rhs);
      }
    case type_real:
      {
        const auto &lhs = this->check<D_real>();
        const auto &rhs = other.check<D_real>();
        if(std::isunordered(lhs, rhs)) {
          return Value::compare_unordered;
        }
        return do_three_way_compare(lhs, rhs);
      }
    case type_string:
      {
        const auto &lhs = this->check<D_string>();
        const auto &rhs = other.check<D_string>();
        return do_three_way_compare(lhs.compare(rhs), 0);
      }
    case type_opaque:
    case type_function:
      {
        return Value::compare_unordered;
      }
    case type_array:
      {
        const auto &lhs = this->check<D_array>();
        const auto &rhs = other.check<D_array>();
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
    case type_object:
      {
        return Value::compare_unordered;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->type(), "` has been encountered.");
    }
  }

    namespace {

    constexpr Indent do_indent_or_space(std::size_t indent_increment, std::size_t indent_next)
      {
        return (indent_increment != 0) ? indent('\n', indent_next) : indent(' ', 0);
      }

    }

void Value::print(std::ostream &os, std::size_t indent_increment, std::size_t indent_next) const
  {
    switch(this->type()) {
    case type_null:
      {
        // null
        os << "null";
        return;
      }
    case type_boolean:
      {
        const auto &alt = this->check<D_boolean>();
        // boolean true
        os << "boolean " << std::boolalpha << std::nouppercase << alt;
        return;
      }
    case type_integer:
      {
        const auto &alt = this->check<D_integer>();
        // integer 42
        os << "integer " << std::dec << alt;
        return;
      }
    case type_real:
      {
        const auto &alt = this->check<D_real>();
        // real 123.456
        os << "real " << std::dec << std::nouppercase << std::setprecision(DECIMAL_DIG) << alt;
        return;
      }
    case type_string:
      {
        const auto &alt = this->check<D_string>();
        // string(5) "hello"
        os << "string(" << std::dec << alt.size() << ") " << quote(alt);
        return;
      }
    case type_opaque:
      {
        const auto &alt = this->check<D_opaque>();
        // opaque("typeid") "my opaque"
        os << "opaque(" << quote(typeid(alt.get()).name()) << ") [[`" << alt.get() << "`]]";
        return;
      }
    case type_function:
      {
        const auto &alt = this->check<D_function>();
        // function("typeid") "my function"
        os << "function(" << quote(typeid(alt.get()).name()) << ") [[`" << alt.get() << "`]]";
        return;
      }
    case type_array:
      {
        const auto &alt = this->check<D_array>();
        // array(3) =
        //  [
        //   0 = integer 1;
        //   1 = integer 2;
        //   2 = integer 3;
        //  ]
        os << "array(" << std::dec << alt.size() << ")";
        os << do_indent_or_space(indent_increment, indent_next + 1) << '[';
        for(auto it = alt.begin(); it != alt.end(); ++it) {
          os << do_indent_or_space(indent_increment, indent_next + indent_increment) << std::dec << (it - alt.begin()) << " = ";
          it->print(os, indent_increment, indent_next + indent_increment);
          os << ';';
        }
        os << do_indent_or_space(indent_increment, indent_next + 1) << ']';
        return;
      }
    case type_object:
      {
        const auto &alt = this->check<D_object>();
        // object(3) =
        //  {
        //   "one" = integer 1;
        //   "two" = integer 2;
        //   "three" = integer 3;
        //  }
        os << "object(" << std::dec << alt.size() << ")";
        os << do_indent_or_space(indent_increment, indent_next + 1) << '{';
        for(auto it = alt.begin(); it != alt.end(); ++it) {
          os << do_indent_or_space(indent_increment, indent_next + indent_increment) << quote(it->first) << " = ";
          it->second.print(os, indent_increment, indent_next + indent_increment);
          os << ';';
        }
        os << do_indent_or_space(indent_increment, indent_next + 1) << '}';
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->type(), "` has been encountered.");
    }
  }

bool Value::unique() const noexcept
  {
    switch(this->type()) {
    case type_null:
    case type_boolean:
    case type_integer:
    case type_real:
      {
        return true;
      }
    case type_string:
      {
        const auto &alt = this->check<D_string>();
        return alt.unique();
      }
    case type_opaque:
      {
        const auto &alt = this->check<D_opaque>();
        return alt.unique();
      }
    case type_function:
      {
        const auto &alt = this->check<D_function>();
        return alt.unique();
      }
    case type_array:
      {
        const auto &alt = this->check<D_array>();
        return alt.unique();
      }
    case type_object:
      {
        const auto &alt = this->check<D_object>();
        return alt.unique();
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->type(), "` has been encountered.");
    }
  }

long Value::use_count() const noexcept
  {
    switch(this->type()) {
    case type_null:
      {
        return 0;
      }
    case type_boolean:
    case type_integer:
    case type_real:
      {
        return 1;
      }
    case type_string:
      {
        const auto &alt = this->check<D_string>();
        return alt.use_count();
      }
    case type_opaque:
      {
        const auto &alt = this->check<D_opaque>();
        return alt.use_count();
      }
    case type_function:
      {
        const auto &alt = this->check<D_function>();
        return alt.use_count();
      }
    case type_array:
      {
        const auto &alt = this->check<D_array>();
        return alt.use_count();
      }
    case type_object:
      {
        const auto &alt = this->check<D_object>();
        return alt.use_count();
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->type(), "` has been encountered.");
    }
  }

void Value::enumerate_variables(const Abstract_Variable_Callback &callback) const
  {
    switch(this->type()) {
    case type_null:
    case type_boolean:
    case type_integer:
    case type_real:
    case type_string:
      {
        return;
      }
    case type_opaque:
      {
        const auto &alt = this->check<D_opaque>();
        alt->enumerate_variables(callback);
        return;
      }
    case type_function:
      {
        const auto &alt = this->check<D_function>();
        alt->enumerate_variables(callback);
        return;
      }
    case type_array:
      {
        const auto &alt = this->check<D_array>();
        rocket::for_each(alt, [&](const D_array::value_type &elem) { elem.enumerate_variables(callback);  });
        return;
      }
    case type_object:
      {
        const auto &alt = this->check<D_object>();
        rocket::for_each(alt, [&](const D_object::value_type &pair) { pair.second.enumerate_variables(callback);  });
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->type(), "` has been encountered.");
    }
  }

}
