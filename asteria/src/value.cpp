// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "value.hpp"
#include "util.hpp"

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
    switch(this->type()) {
      case type_null:
        return false;

      case type_boolean:
        return this->m_stor.as<type_boolean>();

      case type_integer:
        return this->m_stor.as<type_integer>() != 0;

      case type_real:
        return this->m_stor.as<type_real>() != 0;

      case type_string:
        return this->m_stor.as<type_string>().size() != 0;

      case type_opaque:
      case type_function:
        return true;

      case type_array:
        return this->m_stor.as<type_array>().size() != 0;

      case type_object:
        return true;

      default:
        ASTERIA_TERMINATE("invalid value type (type `$1`)", this->type());
    }
  }

Compare
Value::
compare(const Value& other)
  const noexcept
  {
    // Compare values of different types
    if(this->type() != other.type()) {
      // Compare operands that are both of arithmetic types.
      if(this->is_convertible_to_real() && other.is_convertible_to_real())
        return do_3way_compare_scalar(this->convert_to_real(), other.convert_to_real());

      // Otherwise, they are unordered.
      return compare_unordered;
    }

    // Compare values of the same type
    switch(this->type()) {
      case type_null:
        return compare_equal;

      case type_boolean:
        return do_3way_compare_scalar(this->m_stor.as<type_boolean>(), other.m_stor.as<type_boolean>());

      case type_integer:
        return do_3way_compare_scalar(this->m_stor.as<type_integer>(), other.m_stor.as<type_integer>());

      case type_real:
        return do_3way_compare_scalar(this->m_stor.as<type_real>(), other.m_stor.as<type_real>());

      case type_string:
        return do_3way_compare_scalar(this->m_stor.as<type_string>().compare(other.m_stor.as<type_string>()), 0);

      case type_opaque:
      case type_function:
        return compare_unordered;

      case type_array: {
        const auto& lhs = this->m_stor.as<type_array>();
        const auto& rhs = other.m_stor.as<type_array>();

        // Perform lexicographical comparison on the longest initial sequences of the same length.
        size_t rlen = ::rocket::min(lhs.size(), rhs.size());
        for(size_t i = 0;  i < rlen;  ++i) {
          auto cmp = lhs[i].compare(rhs[i]);
          if(cmp != compare_equal)
            return cmp;
        }
        return do_3way_compare_scalar(lhs.size(), rhs.size());
      }

      case type_object:
        return compare_unordered;

      default:
        ASTERIA_TERMINATE("invalid value type (type `$1`)", this->type());
    }
  }

bool
Value::
unique()
  const noexcept
  {
    switch(this->type()) {
      case type_null:
        return false;

      case type_boolean:
      case type_integer:
      case type_real:
        return true;

      case type_string:
        return this->m_stor.as<type_string>().unique();

      case type_opaque:
        return this->m_stor.as<type_opaque>().unique();

      case type_function:
        return this->m_stor.as<type_function>().unique();

      case type_array:
        return this->m_stor.as<type_array>().unique();

      case type_object:
        return this->m_stor.as<type_object>().unique();

      default:
        ASTERIA_TERMINATE("invalid value type (type `$1`)", this->type());
    }
  }

long
Value::
use_count()
  const noexcept
  {
    switch(this->type()) {
      case type_null:
        return 0;

      case type_boolean:
      case type_integer:
      case type_real:
        return 1;

      case type_string:
        return this->m_stor.as<type_string>().use_count();

      case type_opaque:
        return this->m_stor.as<type_opaque>().use_count();

      case type_function:
        return this->m_stor.as<type_function>().use_count();

      case type_array:
        return this->m_stor.as<type_array>().use_count();

      case type_object:
        return this->m_stor.as<type_object>().use_count();

      default:
        ASTERIA_TERMINATE("invalid value type (type `$1`)", this->type());
    }
  }

tinyfmt&
Value::
print(tinyfmt& fmt, bool escape)
  const
  {
    switch(this->type()) {
      case type_null:
        // null
        return fmt << "null";

      case type_boolean:
        // true
        return fmt << this->m_stor.as<type_boolean>();

      case type_integer:
        // 42
        return fmt << this->m_stor.as<type_integer>();

      case type_real:
        // 123.456
        return fmt << this->m_stor.as<type_real>();

      case type_string:
        if(!escape)
          // hello
          return fmt << this->m_stor.as<type_string>();
        else
          // "hello"
          return fmt << quote(this->m_stor.as<type_string>());

      case type_opaque:
        // #opaque [[my opaque]]
        return fmt << "(opaque) [[" << this->m_stor.as<type_opaque>() << "]]";

      case type_function:
        // *function [[my function]]
        return fmt << "(function) [[" << this->m_stor.as<type_function>() << "]]";

      case type_array: {
        const auto& altr = this->m_stor.as<type_array>();
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

      case type_object: {
        const auto& altr = this->m_stor.as<type_object>();
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
        ASTERIA_TERMINATE("invalid value type (type `$1`)", this->type());
    }
  }

tinyfmt&
Value::
dump(tinyfmt& fmt, size_t indent, size_t hanging)
  const
  {
    switch(this->type()) {
      case type_null:
        // null
        return fmt << "null";

      case type_boolean:
        // boolean true
        return fmt << "boolean " << this->m_stor.as<type_boolean>();

      case type_integer:
        // integer 42
        return fmt << "integer " << this->m_stor.as<type_integer>();

      case type_real:
        // real 123.456
        return fmt << "real " << this->m_stor.as<type_real>();

      case type_string: {
        const auto& altr = this->m_stor.as<type_string>();
        // string(5) "hello"
        fmt << "string(" << altr.size() << ") " << quote(altr);
        return fmt;
      }

      case type_opaque: {
        const auto& altr = this->m_stor.as<type_opaque>();
        // #opaque(0x123456) [[my opaque]]
        fmt << "opaque(" << altr.get_opt() << ") [[" << altr << "]]";
        return fmt;
      }

      case type_function: {
        const auto& altr = this->m_stor.as<type_function>();
        // *function [[my function]]
        fmt << "function [[" << altr << "]]";
        return fmt;
      }

      case type_array: {
        const auto& altr = this->m_stor.as<type_array>();
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

      case type_object: {
        const auto& altr = this->m_stor.as<type_object>();
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
        ASTERIA_TERMINATE("invalid value type (type `$1`)", this->type());
    }
  }

Variable_Callback&
Value::
enumerate_variables(Variable_Callback& callback)
  const
  {
    switch(this->type()) {
      case type_null:
      case type_boolean:
      case type_integer:
      case type_real:
      case type_string:
        return callback;

      case type_opaque:
        return this->m_stor.as<type_opaque>().enumerate_variables(callback);

      case type_function:
        return this->m_stor.as<type_function>().enumerate_variables(callback);

      case type_array:
        ::rocket::for_each(this->m_stor.as<type_array>(),
                     [&](const auto& val) { val.enumerate_variables(callback);  });
        return callback;

      case type_object:
        ::rocket::for_each(this->m_stor.as<type_object>(),
                     [&](const auto& pair) { pair.second.enumerate_variables(callback);  });
        return callback;

      default:
        ASTERIA_TERMINATE("invalid value type (type `$1`)", this->type());
    }
  }

}  // namespace asteria
