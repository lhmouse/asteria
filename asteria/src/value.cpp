// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "value.hpp"
#include "utilities.hpp"

namespace Asteria {

bool Value::test() const noexcept
  {
    switch(this->gtype()) {
      {{
    case gtype_null:
        return false;
      }{
    case gtype_boolean:
        return this->m_stor.as<gtype_boolean>();
      }{
    case gtype_integer:
        return this->m_stor.as<gtype_integer>() != 0;
      }{
    case gtype_real:
        return std::fpclassify(this->m_stor.as<gtype_integer>()) != FP_ZERO;
      }{
    case gtype_string:
        return this->m_stor.as<gtype_string>().size() != 0;
      }{
    case gtype_opaque:
    case gtype_function:
        return true;
      }{
    case gtype_array:
        return this->m_stor.as<gtype_array>().size() != 0;
      }{
    case gtype_object:
        return true;
      }}
    default:
      ASTERIA_TERMINATE("invalid value type (gtype `", this->gtype(), "`)");
    }
  }

    namespace {

    template<typename ValT, ROCKET_ENABLE_IF(std::is_integral<ValT>::value)> Compare do_3way_compare(const ValT& lhs, const ValT& rhs) noexcept
      {
        if(lhs < rhs) {
          return compare_less;
        }
        if(lhs > rhs) {
          return compare_greater;
        }
        return compare_equal;
      }
    template<typename ValT, ROCKET_ENABLE_IF(std::is_floating_point<ValT>::value)> Compare do_3way_compare(const ValT& lhs, const ValT& rhs) noexcept
      {
        if(std::isless(lhs, rhs)) {
          return compare_less;
        }
        if(std::isgreater(lhs, rhs)) {
          return compare_greater;
        }
        if(std::isunordered(lhs, rhs)) {
          return compare_unordered;
        }
        return compare_equal;
      }

    }  // namespace

Compare Value::compare(const Value& other) const noexcept
  {
    ///////////////////////////////////////////////////////////////////////////
    // Compare values of different types
    ///////////////////////////////////////////////////////////////////////////
    if(this->gtype() != other.gtype()) {
      // Compare operands that are both of arithmetic types.
      if(this->is_convertible_to_real() && other.is_convertible_to_real()) {
        return do_3way_compare(this->convert_to_real(), other.convert_to_real());
      }
      // Otherwise, they are unordered.
      return compare_unordered;
    }
    ///////////////////////////////////////////////////////////////////////////
    // Compare values of the same type
    ///////////////////////////////////////////////////////////////////////////
    switch(this->gtype()) {
      {{
    case gtype_null:
        return compare_equal;
      }{
    case gtype_boolean:
        return do_3way_compare(this->m_stor.as<gtype_boolean>(), other.m_stor.as<gtype_boolean>());
      }{
    case gtype_integer:
        return do_3way_compare(this->m_stor.as<gtype_integer>(), other.m_stor.as<gtype_integer>());
      }{
    case gtype_real:
        return do_3way_compare(this->m_stor.as<gtype_real>(), other.m_stor.as<gtype_real>());
      }{
    case gtype_string:
        return do_3way_compare(this->m_stor.as<gtype_string>().compare(other.m_stor.as<gtype_string>()), 0);
      }{
    case gtype_opaque:
    case gtype_function:
        return compare_unordered;
      }{
    case gtype_array:
        const auto& lhs = this->m_stor.as<gtype_array>();
        const auto& rhs = other.m_stor.as<gtype_array>();
        // Perform lexicographical comparison on the longest initial sequences of the same length.
        auto cmp = compare_equal;
        std::mismatch(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                      [&](const Value& x, const Value& y) { return (cmp = x.compare(y)) == compare_equal;  });
        if(cmp != compare_equal) {
          return cmp;
        }
        // If they are equal, the longer array is greater than the shorter one.
        return do_3way_compare(lhs.size(), rhs.size());
      }{
    case gtype_object:
        return compare_unordered;
      }}
    default:
      ASTERIA_TERMINATE("invalid value type (gtype `", this->gtype(), "`)");
    }
  }

bool Value::unique() const noexcept
  {
    switch(this->gtype()) {
      {{
    case gtype_null:
        return false;
      }{
    case gtype_boolean:
    case gtype_integer:
    case gtype_real:
        return true;
      }{
    case gtype_string:
        return this->m_stor.as<gtype_string>().unique();
      }{
    case gtype_opaque:
        return this->m_stor.as<gtype_opaque>().get().unique();
      }{
    case gtype_function:
        return this->m_stor.as<gtype_function>().get().unique();
      }{
    case gtype_array:
        return this->m_stor.as<gtype_array>().unique();
      }{
    case gtype_object:
        return this->m_stor.as<gtype_object>().unique();
      }}
    default:
      ASTERIA_TERMINATE("invalid value type (gtype `", this->gtype(), "`)");
    }
  }

long Value::use_count() const noexcept
  {
    switch(this->gtype()) {
      {{
    case gtype_null:
        return 0;
      }{
    case gtype_boolean:
    case gtype_integer:
    case gtype_real:
        return 1;
      }{
    case gtype_string:
        return this->m_stor.as<gtype_string>().use_count();
      }{
    case gtype_opaque:
        return this->m_stor.as<gtype_opaque>().get().use_count();
      }{
    case gtype_function:
        return this->m_stor.as<gtype_function>().get().use_count();
      }{
    case gtype_array:
        return this->m_stor.as<gtype_array>().use_count();
      }{
    case gtype_object:
        return this->m_stor.as<gtype_object>().use_count();
      }}
    default:
      ASTERIA_TERMINATE("invalid value type (gtype `", this->gtype(), "`)");
    }
  }

long Value::gcref_split() const noexcept
  {
    switch(this->gtype()) {
      {{
    case gtype_null:
    case gtype_boolean:
    case gtype_integer:
    case gtype_real:
    case gtype_string:
        return 0;
      }{
    case gtype_opaque:
        return this->m_stor.as<gtype_opaque>().get().use_count();
      }{
    case gtype_function:
        return this->m_stor.as<gtype_function>().get().use_count();
      }{
    case gtype_array:
        return this->m_stor.as<gtype_array>().use_count();
      }{
    case gtype_object:
        return this->m_stor.as<gtype_object>().use_count();
      }}
    default:
      ASTERIA_TERMINATE("invalid value type (gtype `", this->gtype(), "`)");
    }
  }

tinyfmt& Value::print(tinyfmt& fmt, bool escape) const
  {
    switch(this->gtype()) {
      {{
    case gtype_null:
        // null
        return fmt << "null";
      }{
    case gtype_boolean:
        // true
        return fmt << this->m_stor.as<gtype_boolean>();
      }{
    case gtype_integer:
        // 42
        return fmt << this->m_stor.as<gtype_integer>();
      }{
    case gtype_real:
        // 123.456
        return fmt << this->m_stor.as<gtype_real>();
      }{
    case gtype_string:
        const auto& altr = this->m_stor.as<gtype_string>();
        if(!escape)
          // hello
          return fmt << altr;
        else
          // "hello"
          return fmt << quote(altr);
      }{
    case gtype_opaque:
        const auto& altr = this->m_stor.as<gtype_opaque>();
        // <opaque> [[`my opaque`]]
        fmt << "<opaque> [[`" << *altr << "`]]";
        return fmt;
      }{
    case gtype_function:
        const auto& altr = this->m_stor.as<gtype_function>();
        // <function> [[`my function`]]
        fmt << "<function> [[`" << *altr << "`]]";
        return fmt;
      }{
    case gtype_array:
        const auto& altr = this->m_stor.as<gtype_array>();
        // [ 1, 2, 3, ]
        fmt << '[';
        for(size_t i = 0; i < altr.size(); ++i) {
          fmt << ' ';
          altr[i].print(fmt, true);
          fmt << ',';
        }
        fmt << " ]";
        return fmt;
      }{
    case gtype_object:
        const auto& altr = this->m_stor.as<gtype_object>();
        // { "one" = 1, "two" = 2, "three" = 3, }
        fmt << '{';
        for(auto q = altr.begin(); q != altr.end(); ++q) {
          fmt << ' ' << quote(q->first) << " = ";
          q->second.print(fmt, true);
          fmt << ',';
        }
        fmt << " }";
        return fmt;
      }}
    default:
      ASTERIA_TERMINATE("invalid value type (gtype `", this->gtype(), "`)");
    }
  }

tinyfmt& Value::dump(tinyfmt& fmt, size_t indent, size_t hanging) const
  {
    switch(this->gtype()) {
      {{
    case gtype_null:
        // null
        return fmt << "null";
      }{
    case gtype_boolean:
        // boolean true
        return fmt << "boolean " << this->m_stor.as<gtype_boolean>();
      }{
    case gtype_integer:
        // integer 42
        return fmt << "integer " << this->m_stor.as<gtype_integer>();
      }{
    case gtype_real:
        // real 123.456
        return fmt << "real " << this->m_stor.as<gtype_real>();
      }{
    case gtype_string:
        const auto& altr = this->m_stor.as<gtype_string>();
        // string(5) "hello"
        fmt << "string(" << altr.size() << ") " << quote(altr);
        return fmt;
      }{
    case gtype_opaque:
        const auto& altr = this->m_stor.as<gtype_opaque>();
        // opaque("typeid") [[`my opaque`]]
        fmt << "opaque(" << quote(typeid(*altr).name()) << ") [[`" << *altr << "`]]";
        return fmt;
      }{
    case gtype_function:
        const auto& altr = this->m_stor.as<gtype_function>();
        // function("typeid") [[`my function`]]
        fmt << "function(" << quote(typeid(*altr).name()) << ") [[`" << *altr << "`]]";
        return fmt;
      }{
    case gtype_array:
        const auto& altr = this->m_stor.as<gtype_array>();
        // array(3) =
        //  [
        //   0 = integer 1;
        //   1 = integer 2;
        //   2 = integer 3;
        //  ]
        fmt << "array(" << altr.size() << ")";
        fmt << pwrap(indent, hanging + 1) << '[';
        for(size_t i = 0; i < altr.size(); ++i) {
          fmt << pwrap(indent, hanging + indent) << i << " = ";
          altr[i].dump(fmt, indent, hanging + indent) << ';';
        }
        fmt << pwrap(indent, hanging + 1) << ']';
        return fmt;
      }{
    case gtype_object:
        const auto& altr = this->m_stor.as<gtype_object>();
        // object(3) =
        //  {
        //   "one" = integer 1;
        //   "two" = integer 2;
        //   "three" = integer 3;
        //  }
        fmt << "object(" << altr.size() << ")";
        fmt << pwrap(indent, hanging + 1) << '{';
        for(auto q = altr.begin(); q != altr.end(); ++q) {
          fmt << pwrap(indent, hanging + indent) << quote(q->first) << " = ";
          q->second.dump(fmt, indent, hanging + indent) << ';';
        }
        fmt << pwrap(indent, hanging + 1) << '}';
        return fmt;
      }}
    default:
      ASTERIA_TERMINATE("invalid value type (gtype `", this->gtype(), "`)");
    }
  }

Variable_Callback& Value::enumerate_variables(Variable_Callback& callback) const
  {
    switch(this->gtype()) {
      {{
    case gtype_null:
    case gtype_boolean:
    case gtype_integer:
    case gtype_real:
    case gtype_string:
        return callback;
      }{
    case gtype_opaque:
        return this->m_stor.as<gtype_opaque>()->enumerate_variables(callback);
      }{
    case gtype_function:
        return this->m_stor.as<gtype_function>()->enumerate_variables(callback);
      }{
    case gtype_array:
        return rocket::for_each(this->m_stor.as<gtype_array>(), [&](const auto& e) { e.enumerate_variables(callback);  }), callback;
      }{
    case gtype_object:
        return rocket::for_each(this->m_stor.as<gtype_object>(), [&](const auto& p) { p.second.enumerate_variables(callback);  }), callback;
      }}
    default:
      ASTERIA_TERMINATE("invalid value type (gtype `", this->gtype(), "`)");
    }
  }

}  // namespace Asteria
