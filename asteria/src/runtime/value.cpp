// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "value.hpp"
#include "../utilities.hpp"

namespace Asteria {

const char* Value::get_gtype_name(Gtype gtype) noexcept
  {
    switch(gtype) {
    case gtype_null:
      {
        return "null";
      }
    case gtype_boolean:
      {
        return "boolean";
      }
    case gtype_integer:
      {
        return "integer";
      }
    case gtype_real:
      {
        return "real";
      }
    case gtype_string:
      {
        return "string";
      }
    case gtype_opaque:
      {
        return "opaque";
      }
    case gtype_function:
      {
        return "function";
      }
    case gtype_array:
      {
        return "array";
      }
    case gtype_object:
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
    switch(static_cast<Gtype>(this->m_stor.index())) {
    case gtype_null:
      {
        return false;
      }
    case gtype_boolean:
      {
        return this->m_stor.as<gtype_boolean>();
      }
    case gtype_integer:
      {
        return this->m_stor.as<gtype_integer>() != 0;
      }
    case gtype_real:
      {
        return std::fpclassify(this->m_stor.as<gtype_integer>()) != FP_ZERO;
      }
    case gtype_string:
      {
        return this->m_stor.as<gtype_string>().size() != 0;
      }
    case gtype_opaque:
    case gtype_function:
      {
        return true;
      }
    case gtype_array:
      {
        return this->m_stor.as<gtype_array>().size() != 0;
      }
    case gtype_object:
      {
        return true;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

Value::Compare Value::compare(const Value& other) const noexcept
  {
    // Define a utility function with respect to partial ordering.
    auto compare_arithmetic = [&](const auto& lhs, const auto& rhs)
      {
        static_assert(std::is_arithmetic<decltype(+lhs)>::value, "??");
        // Floating-point numbers may be unordered.
        if(std::is_floating_point<decltype(+lhs)>::value && std::isunordered(lhs, rhs)) {
          return compare_unordered;
        }
        // Guarantee total ordering otherwise.
        if(lhs < rhs) {
          return compare_less;
        }
        if(lhs > rhs) {
          return compare_greater;
        }
        return compare_equal;
      };
    ///////////////////////////////////////////////////////////////////////////
    // Compare values of different types
    ///////////////////////////////////////////////////////////////////////////
    if(this->m_stor.index() != other.m_stor.index()) {
      // Compare operands that are both of arithmetic types.
      if(this->is_convertible_to_real() && other.is_convertible_to_real()) {
        return compare_arithmetic(this->convert_to_real(), other.convert_to_real());
      }
      // Otherwise, they are unordered.
      return compare_unordered;
    }
    ///////////////////////////////////////////////////////////////////////////
    // Compare values of the same type
    ///////////////////////////////////////////////////////////////////////////
    switch(static_cast<Gtype>(this->m_stor.index())) {
    case gtype_null:
      {
        return compare_equal;
      }
    case gtype_boolean:
      {
        return compare_arithmetic(this->m_stor.as<gtype_boolean>(), other.m_stor.as<gtype_boolean>());
      }
    case gtype_integer:
      {
        return compare_arithmetic(this->m_stor.as<gtype_integer>(), other.m_stor.as<gtype_integer>());
      }
    case gtype_real:
      {
        return compare_arithmetic(this->m_stor.as<gtype_real>(), other.m_stor.as<gtype_real>());
      }
    case gtype_string:
      {
        int cmp = this->m_stor.as<gtype_string>().compare(other.m_stor.as<gtype_string>());
        return compare_arithmetic(cmp, 0);
      }
    case gtype_opaque:
    case gtype_function:
      {
        return compare_unordered;
      }
    case gtype_array:
      {
        // Perform lexicographical comparison of array elements.
        const auto& lhs = this->m_stor.as<gtype_array>();
        const auto& rhs = other.m_stor.as<gtype_array>();
        // Compare the longest initial sequences.
        // If they are equal, the longer array is greater than the shorter one.
        auto rlen = rocket::min(lhs.size(), rhs.size());
        for(std::size_t i = 0; i < rlen; ++i) {
          auto cmp = lhs[i].compare(rhs[i]);
          if(cmp != compare_equal) {
            return cmp;
          }
        }
        return compare_arithmetic(lhs.size(), rhs.size());
      }
    case gtype_object:
      {
        return compare_unordered;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

void Value::print(std::ostream& os, bool quote_strings) const
  {
    switch(static_cast<Gtype>(this->m_stor.index())) {
    case gtype_null:
      {
        // null
        os << "null";
        return;
      }
    case gtype_boolean:
      {
        // true
        os << std::boolalpha << std::nouppercase << this->m_stor.as<gtype_boolean>();
        return;
      }
    case gtype_integer:
      {
        // 42
        os << std::dec << this->m_stor.as<gtype_integer>();
        return;
      }
    case gtype_real:
      {
        // 123.456
        os << std::dec << std::nouppercase << std::setprecision(DECIMAL_DIG) << this->m_stor.as<gtype_real>();
        return;
      }
    case gtype_string:
      {
        if(quote_strings) {
          // "hello"
          os << quote(this->m_stor.as<gtype_string>());
          return;
        }
        // hello
        os << this->m_stor.as<gtype_string>();
        return;
      }
    case gtype_opaque:
      {
        // <opaque> [[`my opaque`]]
        os << "<opaque> [[`" << this->m_stor.as<gtype_opaque>() << "`]]";
        return;
      }
    case gtype_function:
      {
        // <function> [[`my function`]]
        os << "<function> [[`" << this->m_stor.as<gtype_function>() << "`]]";
        return;
      }
    case gtype_array:
      {
        // [ 1; 2; 3; ]
        os << '[';
        for(const auto& elem : this->m_stor.as<gtype_array>()) {
          os << ' ';
          elem.print(os, true);
          os << ',';
        }
        os << ' ' << ']';
        return;
      }
    case gtype_object:
      {
        // { "one" = 1; "two" = 2; "three" = 3; }
        os << '{';
        for(const auto& pair : this->m_stor.as<gtype_object>()) {
          os << ' ' << quote(pair.first) << " = ";
          pair.second.print(os, true);
          os << ',';
        }
        os << ' ' << '}';
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

    namespace {

    std::ostream& do_auto_indent(std::ostream& os, std::size_t indent_increment, std::size_t indent_next)
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

    }

void Value::dump(std::ostream& os, std::size_t indent_increment, std::size_t indent_next) const
  {
    switch(static_cast<Gtype>(this->m_stor.index())) {
    case gtype_null:
      {
        // null
        os << "null";
        return;
      }
    case gtype_boolean:
      {
        // boolean true
        os << "boolean " << std::boolalpha << std::nouppercase << this->m_stor.as<gtype_boolean>();
        return;
      }
    case gtype_integer:
      {
        // integer 42
        os << "integer " << std::dec << this->m_stor.as<gtype_integer>();
        return;
      }
    case gtype_real:
      {
        // real 123.456
        os << "real " << std::dec << std::nouppercase << std::setprecision(DECIMAL_DIG) << this->m_stor.as<gtype_real>();
        return;
      }
    case gtype_string:
      {
        // string(5) "hello"
        const auto& altr = this->m_stor.as<gtype_string>();
        os << "string(" << std::dec << altr.size() << ") " << quote(altr);
        return;
      }
    case gtype_opaque:
      {
        // opaque("typeid") [[`my opaque`]]
        const auto& altr = this->m_stor.as<gtype_opaque>();
        os << "opaque(" << quote(typeid(altr.get()).name()) << ") [[`" << altr << "`]]";
        return;
      }
    case gtype_function:
      {
        // function("typeid") [[`my function`]]
        const auto& altr = this->m_stor.as<gtype_function>();
        os << "function(" << quote(typeid(altr.get()).name()) << ") [[`" << altr << "`]]";
        return;
      }
    case gtype_array:
      {
        // array(3) =
        //  [
        //   0 = integer 1;
        //   1 = integer 2;
        //   2 = integer 3;
        //  ]
        const auto& altr = this->m_stor.as<gtype_array>();
        os << "array(" << std::dec << altr.size() << ")";
        do_auto_indent(os, indent_increment, indent_next + 1);
        os << '[';
        for(const auto& elem : altr) {
          do_auto_indent(os, indent_increment, indent_next + indent_increment);
          os << std::dec << (std::addressof(elem) - altr.data()) << " = ";
          elem.dump(os, indent_increment, indent_next + indent_increment);
          os << ';';
        }
        do_auto_indent(os, indent_increment, indent_next + 1);
        os << ']';
        return;
      }
    case gtype_object:
      {
        // object(3) =
        //  {
        //   "one" = integer 1;
        //   "two" = integer 2;
        //   "three" = integer 3;
        //  }
        const auto& altr = this->m_stor.as<gtype_object>();
        os << "object(" << std::dec << altr.size() << ")";
        do_auto_indent(os, indent_increment, indent_next + 1);
        os << '{';
        for(const auto& pair : altr) {
          do_auto_indent(os, indent_increment, indent_next + indent_increment);
          os << quote(pair.first) << " = ";
          pair.second.dump(os, indent_increment, indent_next + indent_increment);
          os << ';';
        }
        do_auto_indent(os, indent_increment, indent_next + 1);
        os << '}';
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

bool Value::unique() const noexcept
  {
    switch(static_cast<Gtype>(this->m_stor.index())) {
    case gtype_null:
      {
        return false;
      }
    case gtype_boolean:
    case gtype_integer:
    case gtype_real:
      {
        return true;
      }
    case gtype_string:
      {
        return this->m_stor.as<gtype_string>().unique();
      }
    case gtype_opaque:
      {
        return this->m_stor.as<gtype_opaque>().unique();
      }
    case gtype_function:
      {
        return this->m_stor.as<gtype_function>().unique();
      }
    case gtype_array:
      {
        return this->m_stor.as<gtype_array>().unique();
      }
    case gtype_object:
      {
        return this->m_stor.as<gtype_object>().unique();
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

long Value::use_count() const noexcept
  {
    switch(static_cast<Gtype>(this->m_stor.index())) {
    case gtype_null:
      {
        return 0;
      }
    case gtype_boolean:
    case gtype_integer:
    case gtype_real:
      {
        return 1;
      }
    case gtype_string:
      {
        return this->m_stor.as<gtype_string>().use_count();
      }
    case gtype_opaque:
      {
        return this->m_stor.as<gtype_opaque>().use_count();
      }
    case gtype_function:
      {
        return this->m_stor.as<gtype_function>().use_count();
      }
    case gtype_array:
      {
        return this->m_stor.as<gtype_array>().use_count();
      }
    case gtype_object:
      {
        return this->m_stor.as<gtype_object>().use_count();
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

void Value::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    switch(static_cast<Gtype>(this->m_stor.index())) {
    case gtype_null:
    case gtype_boolean:
    case gtype_integer:
    case gtype_real:
    case gtype_string:
      {
        return;
      }
    case gtype_opaque:
      {
        this->m_stor.as<gtype_opaque>()->enumerate_variables(callback);
        return;
      }
    case gtype_function:
      {
        this->m_stor.as<gtype_function>()->enumerate_variables(callback);
        return;
      }
    case gtype_array:
      {
        rocket::for_each(this->m_stor.as<gtype_array>(), [&](const auto& elem) { elem.enumerate_variables(callback);  });
        return;
      }
    case gtype_object:
      {
        rocket::for_each(this->m_stor.as<gtype_object>(), [&](const auto& pair) { pair.second.enumerate_variables(callback);  });
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->m_stor.index(), "` has been encountered.");
    }
  }

}  // namespace Asteria
