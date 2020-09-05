// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "value.hpp"
#include "utilities.hpp"

namespace asteria {
namespace {

template<typename ValT,
ROCKET_ENABLE_IF(::std::is_integral<ValT>::value)>
constexpr
Compare
do_3way_compare_scalar(const ValT& lhs, const ValT& rhs)
noexcept
  {
    return (lhs < rhs) ? compare_less
         : (lhs > rhs) ? compare_greater
                       : compare_equal;
  }

template<typename ValT,
ROCKET_ENABLE_IF(::std::is_floating_point<ValT>::value)>
constexpr
Compare
do_3way_compare_scalar(const ValT& lhs, const ValT& rhs)
noexcept
  {
    return ::std::isless(lhs, rhs)      ? compare_less
         : ::std::isgreater(lhs, rhs)   ? compare_greater
         : ::std::isunordered(lhs, rhs) ? compare_unordered
                                        : compare_equal;
  }

}  // namespace

bool
Value::
test()
const noexcept
  {
    switch(this->vtype()) {
      case vtype_null:
        return false;

      case vtype_boolean:
        return this->m_stor.as<vtype_boolean>();

      case vtype_integer:
        return this->m_stor.as<vtype_integer>() != 0;

      case vtype_real:
        return this->m_stor.as<vtype_real>() != 0;

      case vtype_string:
        return this->m_stor.as<vtype_string>().size() != 0;

      case vtype_opaque:
      case vtype_function:
        return true;

      case vtype_array:
        return this->m_stor.as<vtype_array>().size() != 0;

      case vtype_object:
        return true;

      default:
        ASTERIA_TERMINATE("invalid value type (vtype `$1`)", this->vtype());
    }
  }

Compare
Value::
compare(const Value& other)
const noexcept
  {
    // Compare values of different types
    if(this->vtype() != other.vtype()) {
      // Compare operands that are both of arithmetic types.
      if(this->is_convertible_to_real() && other.is_convertible_to_real())
        return do_3way_compare_scalar(this->convert_to_real(), other.convert_to_real());

      // Otherwise, they are unordered.
      return compare_unordered;
    }

    // Compare values of the same type
    switch(this->vtype()) {
      case vtype_null:
        return compare_equal;

      case vtype_boolean:
        return do_3way_compare_scalar(this->m_stor.as<vtype_boolean>(), other.m_stor.as<vtype_boolean>());

      case vtype_integer:
        return do_3way_compare_scalar(this->m_stor.as<vtype_integer>(), other.m_stor.as<vtype_integer>());

      case vtype_real:
        return do_3way_compare_scalar(this->m_stor.as<vtype_real>(), other.m_stor.as<vtype_real>());

      case vtype_string:
        return do_3way_compare_scalar(this->m_stor.as<vtype_string>().compare(other.m_stor.as<vtype_string>()), 0);

      case vtype_opaque:
      case vtype_function:
        return compare_unordered;

      case vtype_array: {
        const auto& lhs = this->m_stor.as<vtype_array>();
        const auto& rhs = other.m_stor.as<vtype_array>();
        // Perform lexicographical comparison on the longest initial sequences of the same length.
        size_t rlen = ::rocket::min(lhs.size(), rhs.size());
        for(size_t i = 0;  i < rlen;  ++i) {
          auto cmp = lhs[i].compare(rhs[i]);
          if(cmp != compare_equal)
            return cmp;
        }
        return do_3way_compare_scalar(lhs.size(), rhs.size());
      }

      case vtype_object:
        return compare_unordered;

      default:
        ASTERIA_TERMINATE("invalid value type (vtype `$1`)", this->vtype());
    }
  }

bool
Value::
unique()
const noexcept
  {
    switch(this->vtype()) {
      case vtype_null:
        return false;

      case vtype_boolean:
      case vtype_integer:
      case vtype_real:
        return true;

      case vtype_string:
        return this->m_stor.as<vtype_string>().unique();

      case vtype_opaque:
        return this->m_stor.as<vtype_opaque>().unique();

      case vtype_function:
        return this->m_stor.as<vtype_function>().unique();

      case vtype_array:
        return this->m_stor.as<vtype_array>().unique();

      case vtype_object:
        return this->m_stor.as<vtype_object>().unique();

      default:
        ASTERIA_TERMINATE("invalid value type (vtype `$1`)", this->vtype());
    }
  }

long
Value::
use_count()
const noexcept
  {
    switch(this->vtype()) {
      case vtype_null:
        return 0;

      case vtype_boolean:
      case vtype_integer:
      case vtype_real:
        return 1;

      case vtype_string:
        return this->m_stor.as<vtype_string>().use_count();

      case vtype_opaque:
        return this->m_stor.as<vtype_opaque>().use_count();

      case vtype_function:
        return this->m_stor.as<vtype_function>().use_count();

      case vtype_array:
        return this->m_stor.as<vtype_array>().use_count();

      case vtype_object:
        return this->m_stor.as<vtype_object>().use_count();

      default:
        ASTERIA_TERMINATE("invalid value type (vtype `$1`)", this->vtype());
    }
  }

long
Value::
gcref_split()
const noexcept
  {
    switch(this->vtype()) {
      case vtype_null:
      case vtype_boolean:
      case vtype_integer:
      case vtype_real:
      case vtype_string:
        return 0;

      case vtype_opaque:
        return this->m_stor.as<vtype_opaque>().use_count();

      case vtype_function:
        return this->m_stor.as<vtype_function>().use_count();

      case vtype_array:
        return this->m_stor.as<vtype_array>().use_count();

      case vtype_object:
        return this->m_stor.as<vtype_object>().use_count();

      default:
        ASTERIA_TERMINATE("invalid value type (vtype `$1`)", this->vtype());
    }
  }

tinyfmt&
Value::
print(tinyfmt& fmt, bool escape)
const
  {
    switch(this->vtype()) {
      case vtype_null:
        // null
        return fmt << "null";

      case vtype_boolean:
        // true
        return fmt << this->m_stor.as<vtype_boolean>();

      case vtype_integer:
        // 42
        return fmt << this->m_stor.as<vtype_integer>();

      case vtype_real:
        // 123.456
        return fmt << this->m_stor.as<vtype_real>();

      case vtype_string:
        if(!escape)
          // hello
          return fmt << this->m_stor.as<vtype_string>();
        else
          // "hello"
          return fmt << quote(this->m_stor.as<vtype_string>());

      case vtype_opaque:
        // #opaque [[`my opaque`]]
        return fmt << "#opaque [[`" << this->m_stor.as<vtype_opaque>() << "`]]";

      case vtype_function:
        // *function [[`my function`]]
        return fmt << "*function [[`" << this->m_stor.as<vtype_function>() << "`]]";

      case vtype_array: {
        const auto& altr = this->m_stor.as<vtype_array>();
        // [ 1, 2, 3, ]
        fmt << '[';
        for(size_t i = 0;  i < altr.size();  ++i) {
          fmt << ' ';
          altr[i].print(fmt, true);
          fmt << ',';
        }
        fmt << " ]";
        return fmt;
      }

      case vtype_object: {
        const auto& altr = this->m_stor.as<vtype_object>();
        // { "one" = 1, "two" = 2, "three" = 3, }
        fmt << '{';
        for(auto q = altr.begin();  q != altr.end();  ++q) {
          fmt << ' ' << quote(q->first) << " = ";
          q->second.print(fmt, true);
          fmt << ',';
        }
        fmt << " }";
        return fmt;
      }

      default:
        ASTERIA_TERMINATE("invalid value type (vtype `$1`)", this->vtype());
    }
  }

tinyfmt&
Value::
dump(tinyfmt& fmt, size_t indent, size_t hanging)
const
  {
    switch(this->vtype()) {
      case vtype_null:
        // null
        return fmt << "null";

      case vtype_boolean:
        // boolean true
        return fmt << "boolean " << this->m_stor.as<vtype_boolean>();

      case vtype_integer:
        // integer 42
        return fmt << "integer " << this->m_stor.as<vtype_integer>();

      case vtype_real:
        // real 123.456
        return fmt << "real " << this->m_stor.as<vtype_real>();

      case vtype_string: {
        const auto& altr = this->m_stor.as<vtype_string>();
        // string(5) "hello"
        fmt << "string(" << altr.size() << ") " << quote(altr);
        return fmt;
      }

      case vtype_opaque: {
        const auto& altr = this->m_stor.as<vtype_opaque>();
        // opaque(0x123456) [[`my opaque`]]
        fmt << "opaque(" << altr.ptr() << ") [[`" << altr << "`]]";
        return fmt;
      }

      case vtype_function: {
        const auto& altr = this->m_stor.as<vtype_function>();
        // function(0x123456) [[`my function`]]
        fmt << "function(" << altr.ptr() << ") [[`" << altr << "`]]";
        return fmt;
      }

      case vtype_array: {
        const auto& altr = this->m_stor.as<vtype_array>();
        // array(3) [
        //   0 = integer 1;
        //   1 = integer 2;
        //   2 = integer 3;
        // ]
        fmt << "array(" << altr.size() << ") [";
        for(size_t i = 0;  i < altr.size();  ++i) {
          fmt << pwrap(indent, hanging + indent) << i << " = ";
          altr[i].dump(fmt, indent, hanging + indent) << ';';
        }
        fmt << pwrap(indent, hanging) << ']';
        return fmt;
      }

      case vtype_object: {
        const auto& altr = this->m_stor.as<vtype_object>();
        // object(3) {
        //   "one" = integer 1;
        //   "two" = integer 2;
        //   "three" = integer 3;
        // }
        fmt << "object(" << altr.size() << ") {";
        for(auto q = altr.begin();  q != altr.end();  ++q) {
          fmt << pwrap(indent, hanging + indent) << quote(q->first) << " = ";
          q->second.dump(fmt, indent, hanging + indent) << ';';
        }
        fmt << pwrap(indent, hanging) << '}';
        return fmt;
      }

      default:
        ASTERIA_TERMINATE("invalid value type (vtype `$1`)", this->vtype());
    }
  }

Variable_Callback&
Value::
enumerate_variables(Variable_Callback& callback)
const
  {
    switch(this->vtype()) {
      case vtype_null:
      case vtype_boolean:
      case vtype_integer:
      case vtype_real:
      case vtype_string:
        return callback;

      case vtype_opaque:
        return this->m_stor.as<vtype_opaque>().enumerate_variables(callback);

      case vtype_function:
        return this->m_stor.as<vtype_function>().enumerate_variables(callback);

      case vtype_array:
        ::rocket::for_each(this->m_stor.as<vtype_array>(),
                     [&](const auto& elem) { elem.enumerate_variables(callback);  });
        return callback;

      case vtype_object:
        ::rocket::for_each(this->m_stor.as<vtype_object>(),
                     [&](const auto& pair) { pair.second.enumerate_variables(callback);  });
        return callback;

      default:
        ASTERIA_TERMINATE("invalid value type (vtype `$1`)", this->vtype());
    }
  }

}  // namespace asteria
