// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "value.hpp"
#include "utils.hpp"
#include "llds/variable_hashmap.hpp"
#include "../rocket/linear_buffer.hpp"
#include "../rocket/tinyfmt_file.hpp"
namespace asteria {
namespace {

// Recursion breaker
struct Rbr_array
  {
    const V_array* refa;
    V_array::const_iterator curp;
  };

struct Rbr_object
  {
    const V_object* refo;
    V_object::const_iterator curp;
  };

using Rbr_Element = ::rocket::variant<Rbr_array, Rbr_object>;

}  // namespace

ROCKET_NEVER_INLINE void
Value::
do_destroy_variant_slow() noexcept
  try {
#ifdef ROCKET_DEBUG
    // Attempt to run out of stack in a rather stupid way.
    static char* volatile s_stupid_begin;
    static char* volatile s_stupid_end;

    char stupid[1000] = { };
    s_stupid_begin = stupid;
    s_stupid_end = stupid + sizeof(s_stupid_end);

    s_stupid_begin[0] = 1;
    s_stupid_end[-1] = 2;
#endif

    // Expand arrays and objects by hand. Don't play with this at home!
    cow_vector<bytes_type> stack;

  r:
    if(this->m_stor.index() >= type_string)
      switch(this->m_stor.index()) {
        case type_string:
          this->m_stor.mut<type_string>().~V_string();
          break;

        case type_opaque:
          this->m_stor.mut<type_opaque>().~V_opaque();
          break;

        case type_function:
          this->m_stor.mut<type_function>().~V_function();
          break;

        case type_array: {
          auto& altr = this->m_stor.mut<type_array>();
          if(!altr.empty() && altr.unique()) {
            for(auto it = altr.mut_begin();  it != altr.end();  ++it) {
              // Move raw bytes into `stack`.
              stack.push_back(it->m_bytes);
              bfill(it->m_bytes, 0);
            }
          }
          altr.~V_array();
          break;
        }

        case type_object: {
          auto& altr = this->m_stor.mut<type_object>();
          if(!altr.empty() && altr.unique()) {
            for(auto it = altr.mut_begin();  it != altr.end();  ++it) {
              // Move raw bytes into `stack`.
              stack.push_back(it->second.m_bytes);
              bfill(it->second.m_bytes, 0);
            }
          }
          altr.~V_object();
          break;
        }

        default:
          ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
      }

    if(!stack.empty()) {
      this->m_bytes = stack.back();
      stack.pop_back();
      goto r;
    }

#ifdef ROCKET_DEBUG
    bfill(this->m_bytes, 0xEB);
#endif
  }
  catch(exception& stdex) {
    ::fprintf(stderr,
        "WARNING: An unusual exception that was thrown from the destructor "
        "of a value has been caught and ignored. Some resources might have "
        "leaked. If this issue persists, please file a bug report.\n"
        "\n"
        "  exception class: %s\n"
        "  what(): %s\n",
        typeid(stdex).name(), stdex.what());
  }

void
Value::
do_collect_variables_slow(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    // Expand recursion by hand with a stack.
    auto qval = this;
    cow_vector<Rbr_Element> stack;
    const refcnt_ptr<Variable> null_var_ptr;

  r:
    if(staged.insert(qval, null_var_ptr))  // mark once
      switch(qval->m_stor.index()) {
        case type_opaque:
          qval->m_stor.as<V_opaque>().collect_variables(staged, temp);
          break;

        case type_function:
          qval->m_stor.as<V_function>().collect_variables(staged, temp);
          break;

        case type_array: {
          const auto& altr = qval->m_stor.as<V_array>();
          if(!altr.empty()) {
            Rbr_array elema = { &altr, altr.begin() };
            qval = &*(elema.curp);
            stack.emplace_back(move(elema));
            goto r;
          }
          break;
        }

        case type_object: {
          const auto& altr = qval->m_stor.as<V_object>();
          if(!altr.empty()) {
            Rbr_object elemo = { &altr, altr.begin() };
            qval = &(elemo.curp->second);
            stack.emplace_back(move(elemo));
            goto r;
          }
          break;
        }
      }

    while(stack.size())
      switch(stack.back().index()) {
        case 0: {
          Rbr_array& elema = stack.mut_back().mut<0>();
          ++ elema.curp;
          if(elema.curp != elema.refa->end()) {
            qval = &*(elema.curp);
            goto r;
          }
          stack.pop_back();
          break;
        }

        case 1: {
          Rbr_object& elemo = stack.mut_back().mut<1>();
          ++ elemo.curp;
          if(elemo.curp != elemo.refo->end()) {
            qval = &(elemo.curp->second);
            goto r;
          }
          stack.pop_back();
          break;
        }

        default:
          ROCKET_ASSERT(false);
      }
  }

void
Value::
do_throw_type_mismatch(const char* expecting) const
  {
    ::rocket::sprintf_and_throw<::std::invalid_argument>(
          "asteria::Value: type mismatch (expecting %s, got `%s`)",
          expecting, describe_type(this->type()));
  }

void
Value::
do_throw_uncomparable_with(const Value& other) const
  {
    ::rocket::sprintf_and_throw<::std::invalid_argument>(
          "asteria::Value: `%s` and `%s` are not comparable",
          describe_type(this->type()), describe_type(other.type()));
  }

Compare
Value::
compare_numeric_partial(V_integer other) const noexcept
  {
    if(this->m_stor.index() == type_integer) {
      // total
      V_integer value = this->m_stor.as<V_integer>();

      if(value < other)
        return compare_less;
      else if(value > other)
        return compare_greater;
      else
        return compare_equal;
    }

    if(this->m_stor.index() == type_real) {
      // partial
      V_real value = this->m_stor.as<V_real>();

      // assert `otest <= oreal <= other`
      ::std::fesetround(FE_DOWNWARD);
      V_real oreal = static_cast<V_real>(other);
      V_integer otest = static_cast<V_integer>(oreal);
      ::std::fesetround(FE_TONEAREST);

      if(::std::isless(value, oreal))
        return compare_less;
      else if(::std::isgreater(value, oreal))
        return compare_greater;
      else if(::std::isunordered(value, oreal))
        return compare_unordered;
      else if(otest != other)
        return compare_less;
      else
        return compare_equal;
    }

    // incomparable
    return compare_unordered;
  }

Compare
Value::
compare_numeric_partial(V_real other) const noexcept
  {
    if(this->m_stor.index() == type_integer) {
      // partial
      V_integer value = this->m_stor.as<V_integer>();

      // assert `vtest <= vreal <= value`
      ::std::fesetround(FE_DOWNWARD);
      V_real vreal = static_cast<V_real>(value);
      V_integer vtest = static_cast<V_integer>(vreal);
      ::std::fesetround(FE_TONEAREST);

      if(::std::isless(vreal, other))
        return compare_less;
      else if(::std::isgreater(vreal, other))
        return compare_greater;
      else if(::std::isunordered(vreal, other))
        return compare_unordered;
      else if(vtest != value)
        return compare_greater;
      else
        return compare_equal;
    }

    if(this->m_stor.index() == type_real) {
      // partial
      V_real value = this->m_stor.as<V_real>();

      if(::std::isless(value, other))
        return compare_less;
      else if(::std::isgreater(value, other))
        return compare_greater;
      else if(::std::isunordered(value, other))
        return compare_unordered;
      else
        return compare_equal;
    }

    // incomparable
    return compare_unordered;
  }

Compare
Value::
compare_partial(const Value& other) const
  {
    // Expand recursion by hand with a stack.
    auto qval = this;
    auto qoth = &other;
    cow_bivector<Rbr_array, Rbr_array> stack;

  r:
    if(qoth->m_stor.index() == type_integer) {
      // specialized
      auto cmp = qval->compare_numeric_partial(qoth->m_stor.as<V_integer>());
      if(cmp != compare_equal)
        return cmp;
    }
    else if(qoth->m_stor.index() == type_real) {
      // specialized
      auto cmp = qval->compare_numeric_partial(qoth->m_stor.as<V_real>());
      if(cmp != compare_equal)
        return cmp;
    }
    else if(qval->m_stor.index() != qoth->m_stor.index()) {
      // incomparable
      return compare_unordered;
    }
    else if(qval->m_stor.index() == type_boolean) {
      // specialized
      if(qval->m_stor.as<V_boolean>() < qoth->m_stor.as<V_boolean>())
        return compare_less;
      else if(qval->m_stor.as<V_boolean>() > qoth->m_stor.as<V_boolean>())
        return compare_greater;
    }
    else if(qval->m_stor.index() == type_string) {
      // specialized
      int r = qval->m_stor.as<V_string>().compare(qoth->m_stor.as<V_string>());
      if(r < 0)
        return compare_less;
      else if(r > 0)
        return compare_greater;
    }
    else if(qval->m_stor.index() == type_array) {
      // recursive
      const auto& altr_val = qval->m_stor.as<V_array>();
      const auto& altr_oth = qoth->m_stor.as<V_array>();
      if(!altr_val.empty() && !altr_oth.empty()) {
        Rbr_array elem_val = { &altr_val, altr_val.begin() };
        Rbr_array elem_oth = { &altr_oth, altr_oth.begin() };
        qval = &*(elem_val.curp);
        qoth = &*(elem_oth.curp);
        stack.emplace_back(move(elem_val), move(elem_oth));
        goto r;
      }

      // longer one wins.
      if(altr_val.size() < altr_oth.size())
        return compare_less;
      else if(altr_val.size() > altr_oth.size())
        return compare_greater;
    }
    else if(qval->m_stor.index() != type_null) {
      // functions, opaques, objects etc. are not comparable.
      return compare_unordered;
    }

    while(stack.size()) {
      Rbr_array& elem_val = stack.mut_back().first;
      Rbr_array& elem_oth = stack.mut_back().second;
      ++ elem_val.curp;
      ++ elem_oth.curp;
      if((elem_val.curp != elem_val.refa->end()) && (elem_oth.curp != elem_oth.refa->end())) {
        qval = &*(elem_val.curp);
        qoth = &*(elem_oth.curp);
        goto r;
      }

      if(elem_val.refa->size() < elem_oth.refa->size())
        return compare_less;
      else if(elem_val.refa->size() > elem_oth.refa->size())
        return compare_greater;

      stack.pop_back();
    }

    return compare_equal;
  }

tinyfmt&
Value::
print(tinyfmt& fmt) const
  {
    // Expand recursion by hand with a stack.
    auto qval = this;
    cow_vector<Rbr_Element> stack;

  r:
    switch(qval->m_stor.index()) {
      case type_null:
        fmt << "null";
        break;

      case type_boolean:
        fmt << qval->m_stor.as<V_boolean>();
        break;

      case type_integer:
        fmt << qval->m_stor.as<V_integer>();
        break;

      case type_real:
        fmt << qval->m_stor.as<V_real>();
        break;

      case type_string:
        fmt << "\"";
        c_quote(fmt, qval->m_stor.as<V_string>());
        fmt << "\"";
        break;

      case type_opaque:
        fmt << "(opaque) [[" << qval->m_stor.as<V_opaque>() << "]]";
        break;

      case type_function:
        fmt << "(function) [[" << qval->m_stor.as<V_function>() << "]]";
        break;

      case type_array: {
        const auto& altr = qval->m_stor.as<V_array>();
        if(!altr.empty()) {
          Rbr_array elema = { &altr, altr.begin() };
          fmt << "[ ";
          qval = &*(elema.curp);
          stack.emplace_back(move(elema));
          goto r;
        }
        fmt << "[ ]";
        break;
      }

      case type_object: {
        const auto& altr = qval->m_stor.as<V_object>();
        if(!altr.empty()) {
          Rbr_object elemo = { &altr, altr.begin() };
          fmt << "{ \"";
          c_quote(fmt, elemo.curp->first);
          fmt << "\": ";
          qval = &(elemo.curp->second);
          stack.emplace_back(move(elemo));
          goto r;
        }
        fmt << "{ }";
        break;
      }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), this->m_stor.index());
    }

    while(stack.size())
      switch(stack.back().index()) {
        case 0: {
          Rbr_array& elema = stack.mut_back().mut<0>();
          ++ elema.curp;
          if(elema.curp != elema.refa->end()) {
            fmt << ", ";
            qval = &*(elema.curp);
            goto r;
          }
          fmt << " ]";
          stack.pop_back();
          break;
        }

        case 1: {
          Rbr_object& elemo = stack.mut_back().mut<1>();
          ++ elemo.curp;
          if(elemo.curp != elemo.refo->end()) {
            fmt << ", \"";
            c_quote(fmt, elemo.curp->first);
            fmt << "\": ";
            qval = &(elemo.curp->second);
            goto r;
          }
          fmt << " }";
          stack.pop_back();
          break;
        }

        default:
          ROCKET_ASSERT(false);
      }

    return fmt;
  }

bool
Value::
print_to_stderr() const
  {
    ::rocket::tinyfmt_file fmt(stderr, nullptr);
    this->print(fmt);
    return ::ferror(fmt.get_handle());
  }

tinyfmt&
Value::
dump(tinyfmt& fmt, size_t indent, size_t hanging) const
  {
    // Expand recursion by hand with a stack.
    auto qval = this;
    cow_vector<Rbr_Element> stack;

  r:
    switch(qval->type()) {
      case type_null:
        fmt << "null;";
        break;

      case type_boolean:
        fmt << "boolean " << qval->m_stor.as<V_boolean>() << ";";
        break;

      case type_integer:
        fmt << "integer " << qval->m_stor.as<V_integer>() << ";";
        break;

      case type_real:
        fmt << "real " << qval->m_stor.as<V_real>() << ";";
        break;

      case type_string: {
        const auto& altr = qval->m_stor.as<V_string>();
        fmt << "string(" << altr.size() << ") \"";
        c_quote(fmt, altr);
        fmt << "\";";
        break;
      }

      case type_opaque: {
        const auto& altr = qval->m_stor.as<V_opaque>();
        fmt << "opaque [[" << altr << "]];";
        break;
      }

      case type_function: {
        const auto& altr = qval->m_stor.as<V_function>();
        fmt << "function [[" << altr << "]];";
        break;
      }

      case type_array: {
        const auto& altr = qval->m_stor.as<V_array>();
        if(!altr.empty()) {
          Rbr_array elema = { &altr, altr.begin() };
          fmt << "array(" << altr.size() << ") [";
          details_value::do_break_line(fmt, indent, hanging + indent * (stack.size() + 1));
          fmt << (elema.curp - altr.begin()) << " = ";
          qval = &*(elema.curp);
          stack.emplace_back(move(elema));
          goto r;
        }
        fmt << "array(0) [ ];";
        break;
      }

      case type_object: {
        const auto& altr = qval->m_stor.as<V_object>();
        if(!altr.empty()) {
          Rbr_object elemo = { &altr, altr.begin() };
          fmt << "object(" << altr.size() << ") {";
          details_value::do_break_line(fmt, indent, hanging + indent * (stack.size() + 1));
          fmt << "\"";
          c_quote(fmt, elemo.curp->first);
          fmt << "\" = ";
          qval = &(elemo.curp->second);
          stack.emplace_back(move(elemo));
          goto r;
        }
        fmt << "object(0) { };";
        break;
      }

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), qval->type());
    }

    while(stack.size())
      switch(stack.back().index()) {
        case 0: {
          Rbr_array& elema = stack.mut_back().mut<0>();
          ++ elema.curp;
          if(elema.curp != elema.refa->end()) {
            details_value::do_break_line(fmt, indent, hanging + indent * stack.size());
            fmt << (elema.curp - elema.refa->begin()) << " = ";
            qval = &*(elema.curp);
            goto r;
          }
          details_value::do_break_line(fmt, indent, hanging + indent * (stack.size() - 1));
          fmt << "];";
          stack.pop_back();
          break;
        }

        case 1: {
          Rbr_object& elemo = stack.mut_back().mut<1>();
          ++ elemo.curp;
          if(elemo.curp != elemo.refo->end()) {
            details_value::do_break_line(fmt, indent, hanging + indent * stack.size());
            fmt << "\"";
            c_quote(fmt, elemo.curp->first);
            fmt << "\" = ";
            qval = &(elemo.curp->second);
            goto r;
          }
          details_value::do_break_line(fmt, indent, hanging + indent * (stack.size() - 1));
          fmt << "};";
          stack.pop_back();
          break;
        }

        default:
          ROCKET_ASSERT(false);
      }

    return fmt;
  }

bool
Value::
dump_to_stderr(size_t indent, size_t hanging) const
  {
    ::rocket::tinyfmt_file fmt(stderr, nullptr);
    this->dump(fmt, indent, hanging);
    return ::ferror(fmt.get_handle());
  }

}  // namespace asteria

template class ::rocket::variant<::asteria::V_null, ::asteria::V_boolean,
    ::asteria::V_integer, ::asteria::V_real, ::asteria::V_string, ::asteria::V_opaque,
    ::asteria::V_function, ::asteria::V_array,  ::asteria::V_object>;
template class ::rocket::cow_vector<::asteria::Value>;
template class ::rocket::cow_hashmap<::rocket::prehashed_string,
    ::asteria::Value, ::rocket::prehashed_string::hash>;
template class ::rocket::optional<::asteria::V_boolean>;
template class ::rocket::optional<::asteria::V_integer>;
template class ::rocket::optional<::asteria::V_real>;
template class ::rocket::optional<::asteria::V_string>;
template class ::rocket::optional<::asteria::V_opaque>;
template class ::rocket::optional<::asteria::V_function>;
template class ::rocket::optional<::asteria::V_array>;
template class ::rocket::optional<::asteria::V_object>;
