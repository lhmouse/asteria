// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "value.hpp"
#include "../utilities.hpp"

namespace Asteria {

const char* Value::gtype_name_of(Gtype gtype) noexcept
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

    namespace {

    // These bytes should make up a `Value` that equals `null` and shall never be destroyed.
    // Don't play with this at home.
    constexpr std::aligned_union<0, Value>::type s_null[1] = { };

    }  // namespace

const Value& Value::null() noexcept
  {
    // This two-step cast is necessary to eliminate warnings when strict aliasing is in effect.
    return static_cast<const Value*>(static_cast<const void*>(s_null))[0];
  }

bool Value::test() const noexcept
  {
    switch(this->gtype()) {
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
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->gtype(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

    namespace {

    template<typename XvalueT, ROCKET_ENABLE_IF(std::is_integral<XvalueT>::value)> constexpr Value::Compare do_3way_compare(const XvalueT& lhs,
                                                                                                                            const XvalueT& rhs) noexcept
      {
        return (lhs < rhs) ? Value::compare_less
                           : (lhs > rhs) ? Value::compare_greater
                                         : Value::compare_equal;
      }
    template<typename XvalueT, ROCKET_ENABLE_IF(std::is_floating_point<XvalueT>::value)> constexpr Value::Compare do_3way_compare(const XvalueT& lhs,
                                                                                                                                  const XvalueT& rhs) noexcept
      {
        return std::isless(lhs, rhs) ? Value::compare_less
                                     : std::isgreater(lhs, rhs) ? Value::compare_greater
                                                                : std::isunordered(lhs, rhs) ? Value::compare_unordered
                                                                                             : Value::compare_equal;
      }

    }  // namespace

Value::Compare Value::compare(const Value& other) const noexcept
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
    case gtype_null:
      {
        return compare_equal;
      }
    case gtype_boolean:
      {
        return do_3way_compare(this->m_stor.as<gtype_boolean>(), other.m_stor.as<gtype_boolean>());
      }
    case gtype_integer:
      {
        return do_3way_compare(this->m_stor.as<gtype_integer>(), other.m_stor.as<gtype_integer>());
      }
    case gtype_real:
      {
        return do_3way_compare(this->m_stor.as<gtype_real>(), other.m_stor.as<gtype_real>());
      }
    case gtype_string:
      {
        return do_3way_compare(this->m_stor.as<gtype_string>().compare(other.m_stor.as<gtype_string>()), 0);
      }
    case gtype_opaque:
    case gtype_function:
      {
        return compare_unordered;
      }
    case gtype_array:
      {
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
      }
    case gtype_object:
      {
        return compare_unordered;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->gtype(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

bool Value::unique() const noexcept
  {
    switch(this->gtype()) {
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
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->gtype(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

long Value::use_count() const noexcept
  {
    switch(this->gtype()) {
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
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->gtype(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

long Value::gcref_split() const noexcept
  {
    switch(this->gtype()) {
    case gtype_null:
    case gtype_boolean:
    case gtype_integer:
    case gtype_real:
    case gtype_string:
      {
        return 0;
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
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->gtype(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

std::ostream& Value::print(std::ostream& os, bool escape) const
  {
    switch(this->gtype()) {
    case gtype_null:
      {
        // null
        return os << "null";
      }
    case gtype_boolean:
      {
        // true
        return os << std::boolalpha << std::nouppercase << this->m_stor.as<gtype_boolean>();
      }
    case gtype_integer:
      {
        // 42
        return os << std::dec << this->m_stor.as<gtype_integer>();
      }
    case gtype_real:
      {
        // 123.456
        return os << std::defaultfloat << std::nouppercase << std::setprecision(17) << this->m_stor.as<gtype_real>();
      }
    case gtype_string:
      {
        const auto& altr = this->m_stor.as<gtype_string>();
        if(!escape) {
          // hello
          os << altr;
        }
        else {
          // "hello"
          os << quote(altr);
        }
        return os;
      }
    case gtype_opaque:
      {
        const auto& altr = this->m_stor.as<gtype_opaque>();
        // <opaque> [[`my opaque`]]
        os << "<opaque> [[`" << altr << "`]]";
        return os;
      }
    case gtype_function:
      {
        const auto& altr = this->m_stor.as<gtype_function>();
        // <function> [[`my function`]]
        os << "<function> [[`" << altr << "`]]";
        return os;
      }
    case gtype_array:
      {
        const auto& altr = this->m_stor.as<gtype_array>();
        // [ 1, 2, 3, ]
        os << '[';
        for(size_t i = 0; i < altr.size(); ++i) {
          os << ' ';
          altr[i].print(os, true);
          os << ',';
        }
        os << " ]";
        return os;
      }
    case gtype_object:
      {
        const auto& altr = this->m_stor.as<gtype_object>();
        G_string kbuf;
        // { "one" = 1, "two" = 2, "three" = 3, }
        os << '{';
        for(auto q = altr.begin(); q != altr.end(); ++q) {
          os << ' ' << quote(kbuf, q->first) << " = ";
          q->second.print(os, true);
          os << ',';
        }
        os << " }";
        return os;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->gtype(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

std::ostream& Value::dump(std::ostream& os, size_t indent, size_t hanging) const
  {
    switch(this->gtype()) {
    case gtype_null:
      {
        // null
        return os << "null";
      }
    case gtype_boolean:
      {
        // boolean true
        return os << "boolean " << std::boolalpha << std::nouppercase << this->m_stor.as<gtype_boolean>();
      }
    case gtype_integer:
      {
        // integer 42
        return os << "integer " << std::dec << this->m_stor.as<gtype_integer>();
      }
    case gtype_real:
      {
        // real 123.456
        return os << "real " << std::defaultfloat << std::nouppercase << std::setprecision(17) << this->m_stor.as<gtype_real>();
      }
    case gtype_string:
      {
        const auto& altr = this->m_stor.as<gtype_string>();
        // string(5) "hello"
        os << "string(" << std::dec << altr.size() << ") " << quote(altr);
        return os;
      }
    case gtype_opaque:
      {
        const auto& altr = this->m_stor.as<gtype_opaque>();
        // opaque("typeid") [[`my opaque`]]
        os << "opaque(" << quote(typeid(*altr).name()) << ") [[`" << altr << "`]]";
        return os;
      }
    case gtype_function:
      {
        const auto& altr = this->m_stor.as<gtype_function>();
        // function("typeid") [[`my function`]]
        os << "function(" << quote(typeid(*altr).name()) << ") [[`" << altr << "`]]";
        return os;
      }
    case gtype_array:
      {
        const auto& altr = this->m_stor.as<gtype_array>();
        G_string ibuf;
        // array(3) =
        //  [
        //   0 = integer 1;
        //   1 = integer 2;
        //   2 = integer 3;
        //  ]
        os << "array(" << std::dec << altr.size() << ")";
        os << pwrapln(ibuf, indent, hanging + 1) << '[';
        for(size_t i = 0; i < altr.size(); ++i) {
          os << pwrapln(ibuf, indent, hanging + indent) << std::dec << i << " = ";
          altr[i].dump(os, indent, hanging + indent) << ';';
        }
        os << pwrapln(ibuf, indent, hanging + 1) << ']';
        return os;
      }
    case gtype_object:
      {
        const auto& altr = this->m_stor.as<gtype_object>();
        G_string ibuf, kbuf;
        // object(3) =
        //  {
        //   "one" = integer 1;
        //   "two" = integer 2;
        //   "three" = integer 3;
        //  }
        os << "object(" << std::dec << altr.size() << ")";
        os << pwrapln(ibuf, indent, hanging + 1) << '{';
        for(auto q = altr.begin(); q != altr.end(); ++q) {
          os << pwrapln(ibuf, indent, hanging + indent) << quote(kbuf, q->first) << " = ";
          q->second.dump(os, indent, hanging + indent) << ';';
        }
        os << pwrapln(ibuf, indent, hanging + 1) << '}';
        return os;
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->gtype(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

void Value::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    switch(this->gtype()) {
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
        return this->m_stor.as<gtype_opaque>()->enumerate_variables(callback);
      }
    case gtype_function:
      {
        return this->m_stor.as<gtype_function>()->enumerate_variables(callback);
      }
    case gtype_array:
      {
        return rocket::for_each(this->m_stor.as<gtype_array>(), [&](const auto& elem) { elem.enumerate_variables(callback);  });
      }
    case gtype_object:
      {
        return rocket::for_each(this->m_stor.as<gtype_object>(), [&](const auto& pair) { pair.second.enumerate_variables(callback);  });
      }
    default:
      ASTERIA_TERMINATE("An unknown value type enumeration `", this->gtype(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
