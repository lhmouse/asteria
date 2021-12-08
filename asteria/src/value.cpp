// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "value.hpp"
#include "utils.hpp"

namespace asteria {
namespace {

template<typename ValT,
ROCKET_ENABLE_IF(::std::is_integral<ValT>::value)>
constexpr Compare
do_3way_compare_scalar(const ValT& lhs, const ValT& rhs)
  {
    return (lhs < rhs) ? compare_less
         : (lhs > rhs) ? compare_greater
                       : compare_equal;
  }

template<typename ValT,
ROCKET_ENABLE_IF(::std::is_floating_point<ValT>::value)>
constexpr Compare
do_3way_compare_scalar(const ValT& lhs, const ValT& rhs)
  {
    return ::std::isless(lhs, rhs)      ? compare_less
         : ::std::isgreater(lhs, rhs)   ? compare_greater
         : ::std::isunordered(lhs, rhs) ? compare_unordered
                                        : compare_equal;
  }

}  // namespace

void
Value::
do_get_variables_slow(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    switch(this->type()) {
      case type_null:
      case type_boolean:
      case type_integer:
      case type_real:
      case type_string:
        return;

      case type_opaque:
        this->m_stor.as<V_opaque>().get_variables(staged, temp);
        return;

      case type_function:
        this->m_stor.as<V_function>().get_variables(staged, temp);
        return;

      case type_array:
        ::rocket::for_each(this->m_stor.as<V_array>(),
            [&](const auto& val) { val.get_variables(staged, temp);  });
        return;

      case type_object:
        ::rocket::for_each(this->m_stor.as<V_object>(),
            [&](const auto& pair) { pair.second.get_variables(staged, temp);  });
        return;

      default:
        ASTERIA_TERMINATE("invalid value type (type `$1`)", this->type());
    }
  }

bool
Value::
do_test_slow() const noexcept
  {
    switch(this->type()) {
      case type_null:
        return false;

      case type_boolean:
        return this->m_stor.as<V_boolean>();

      case type_integer:
        return this->m_stor.as<V_integer>() != 0;

      case type_real:
        return this->m_stor.as<V_real>() != 0;

      case type_string:
        return this->m_stor.as<V_string>().empty() == false;

      case type_opaque:
      case type_function:
        return true;

      case type_array:
        return this->m_stor.as<V_array>().empty() == false;

      case type_object:
        return true;

      default:
        ASTERIA_TERMINATE("invalid value type (type `$1`)", this->type());
    }
  }

Compare
Value::
do_compare_slow(const Value& lhs, const Value& rhs) noexcept
  {
    // Compare values of different types.
    if(lhs.type() != rhs.type()) {
      // If they both have arithmeteic types, convert them to `real`.
      if(lhs.is_real() && rhs.is_real())
        return do_3way_compare_scalar(lhs.as_real(), rhs.as_real());

      // Otherwise, they are unordered.
      return compare_unordered;
    }

    // Compare values of the same type
    switch(lhs.type()) {
      case type_null:
        return compare_equal;

      case type_boolean:
        return do_3way_compare_scalar(
                  lhs.m_stor.as<V_boolean>(), rhs.m_stor.as<V_boolean>());

      case type_integer:
        return do_3way_compare_scalar(
                  lhs.m_stor.as<V_integer>(), rhs.m_stor.as<V_integer>());

      case type_real:
        return do_3way_compare_scalar(
                  lhs.m_stor.as<V_real>(), rhs.m_stor.as<V_real>());

      case type_string:
        return do_3way_compare_scalar(
            lhs.m_stor.as<V_string>().compare(rhs.m_stor.as<V_string>()), 0);

      case type_opaque:
      case type_function:
        return compare_unordered;

      case type_array: {
        // Perform lexicographical comparison on the longest initial sequences
        // of the same length.
        const auto& la = lhs.m_stor.as<V_array>();
        const auto& ra = rhs.m_stor.as<V_array>();
        size_t rlen = ::rocket::min(la.size(), ra.size());

        Compare comp;
        for(size_t k = 0;  k != rlen;  ++k)
          if((comp = do_compare_slow(la[k], ra[k])) != compare_equal)
            return comp;

        // If the initial sequences compare equal, the longer array is greater
        // than the shorter.
        comp = do_3way_compare_scalar(la.size(), ra.size());
        return comp;
      }

      case type_object:
        return compare_unordered;

      default:
        ASTERIA_TERMINATE("invalid value type (type `$1`)", lhs.type());
    }
  }

void
Value::
do_throw_type_mismatch(const char* desc) const
  {
    ::rocket::sprintf_and_throw<::std::invalid_argument>(
          "Value: type mismatch (expecting %s, but value had type `%s`)",
          desc, describe_type(this->type()));
  }

tinyfmt&
Value::
print(tinyfmt& fmt, bool escape) const
  {
    switch(this->type()) {
      case type_null:
        return fmt << "null";

      case type_boolean:
        return fmt << this->m_stor.as<V_boolean>();

      case type_integer:
        return fmt << this->m_stor.as<V_integer>();

      case type_real:
        return fmt << this->m_stor.as<V_real>();

      case type_string:
        if(!escape)
          return fmt << this->m_stor.as<V_string>();
        else
          return fmt << quote(this->m_stor.as<V_string>());

      case type_opaque:
        return fmt << "(opaque) [[" << this->m_stor.as<V_opaque>() << "]]";

      case type_function:
        return fmt << "(function) [[" << this->m_stor.as<V_function>() << "]]";

      case type_array: {
        const auto& altr = this->m_stor.as<V_array>();
        fmt << "[";
        auto it = altr.begin();
        if(it != altr.end()) {
          for(;;) {
            fmt << ' ';
            it->print(fmt, true);
            if(++it == altr.end())
              break;
            fmt << ',';
          }
        }
        fmt << " ]";
        return fmt;
      }

      case type_object: {
        const auto& altr = this->m_stor.as<V_object>();
        fmt << "{";
        auto it = altr.begin();
        if(it != altr.end()) {
          for(;;) {
            fmt << ' ';
            fmt << quote(it->first) << " = ";
            it->second.print(fmt, true);
            if(++it == altr.end())
              break;
            fmt << ',';
          }
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
dump(tinyfmt& fmt, size_t indent, size_t hanging) const
  {
    switch(this->type()) {
      case type_null:
        return fmt << "null";

      case type_boolean:
        return fmt << "boolean " << this->m_stor.as<V_boolean>();

      case type_integer:
        return fmt << "integer " << this->m_stor.as<V_integer>();

      case type_real:
        return fmt << "real " << this->m_stor.as<V_real>();

      case type_string: {
        const auto& altr = this->m_stor.as<V_string>();
        fmt << "string(" << altr.size() << ") " << quote(altr);
        return fmt;
      }

      case type_opaque: {
        const auto& altr = this->m_stor.as<V_opaque>();
        fmt << "opaque [[" << altr << "]]";
        return fmt;
      }

      case type_function: {
        const auto& altr = this->m_stor.as<V_function>();
        fmt << "function [[" << altr << "]]";
        return fmt;
      }

      case type_array: {
        const auto& altr = this->m_stor.as<V_array>();
        fmt << "array(" << altr.size() << ") [";
        for(size_t i = 0;  i < altr.size();  ++i) {
          fmt << pwrap(indent, hanging + indent) << i << " = ";
          altr[i].dump(fmt, indent, hanging + indent) << ';';
        }
        fmt << pwrap(indent, hanging) << ']';
        return fmt;
      }

      case type_object: {
        const auto& altr = this->m_stor.as<V_object>();
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

}  // namespace asteria
