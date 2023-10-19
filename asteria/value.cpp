// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "precompiled.ipp"
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
    ::rocket::cow_vector<bytes_type> stack;

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
              ::memset(&(it->m_bytes), 0, sizeof(bytes_type));
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
              ::memset(&(it->second.m_bytes), 0, sizeof(bytes_type));
            }
          }
          altr.~V_object();
          break;
        }

        default:
          ASTERIA_TERMINATE(("Invalid value type (type `$1`)"), this->type());
      }

    if(!stack.empty()) {
      this->m_bytes = stack.back();
      stack.pop_back();
      goto r;
    }

#ifdef ROCKET_DEBUG
    ::std::memset(&(this->m_bytes), 0xEB, sizeof(bytes_type));
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
    const refcnt_ptr<Variable> null_variable_ptr;

  r:
    if(staged.insert(qval, null_variable_ptr))  // mark once
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
            stack.emplace_back(::std::move(elema));
            goto r;
          }
          break;
        }

        case type_object: {
          const auto& altr = qval->m_stor.as<V_object>();
          if(!altr.empty()) {
            Rbr_object elemo = { &altr, altr.begin() };
            qval = &(elemo.curp->second);
            stack.emplace_back(::std::move(elemo));
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
do_throw_type_mismatch(const char* desc) const
  {
    ASTERIA_THROW((
        "Value type mismatch (expecting `$1`, got `$2`)"),
        desc, describe_type(this->type()));
  }

Compare
Value::
compare(const Value& other) const noexcept
  {
    // Expand recursion by hand with a stack.
    auto qval = this;
    auto qoth = &other;
    cow_bivector<Rbr_array, Rbr_array> stack;

#define do_compare_3way_(xx, yy)  \
    if((xx) < (yy))  \
      return compare_less;  \
    else if((xx) > (yy))  \
      return compare_greater;  \
    else  \
      ((void) 0)  // no semicolon

#define do_check_unordered_(xx, yy)  \
    if(::std::isunordered((xx), (yy)))  \
      return compare_unordered;  \
    else  \
      ((void) 0)  // no semicolon

  r:
    if((qval->m_stor.index() == type_integer) && (qoth->m_stor.index() == type_real)) {
      // integer <=> real
      do_check_unordered_(qoth->m_stor.as<V_real>(), 0.0);
      do_compare_3way_(static_cast<V_real>(qval->m_stor.as<V_integer>()), qoth->m_stor.as<V_real>());
    }
    else if((qval->m_stor.index() == type_real) && (qoth->m_stor.index() == type_integer)) {
      // real <=> integer
      do_check_unordered_(qval->m_stor.as<V_real>(), 0.0);
      do_compare_3way_(qval->m_stor.as<V_real>(), static_cast<V_real>(qoth->m_stor.as<V_integer>()));
    }
    else if(qval->m_stor.index() != qoth->m_stor.index()) {
      // incomparable
      return compare_unordered;
    }
    else
      switch(qval->m_stor.index()) {
        case type_null:
          break;

        case type_boolean:
          do_compare_3way_(qval->m_stor.as<V_boolean>(), qoth->m_stor.as<V_boolean>());
          break;

        case type_integer:
          do_compare_3way_(qval->m_stor.as<V_integer>(), qoth->m_stor.as<V_integer>());
          break;

        case type_real:
          do_check_unordered_(qval->m_stor.as<V_real>(), qoth->m_stor.as<V_real>());
          do_compare_3way_(qval->m_stor.as<V_real>(), qoth->m_stor.as<V_real>());
          break;

        case type_string:
          do_compare_3way_(qval->m_stor.as<V_string>().compare(qoth->m_stor.as<V_string>()), 0);
          break;

        case type_array: {
          const auto& altr_val = qval->m_stor.as<V_array>();
          const auto& altr_oth = qoth->m_stor.as<V_array>();
          if(!altr_val.empty() && !altr_oth.empty()) {
            Rbr_array elem_val = { &altr_val, altr_val.begin() };
            Rbr_array elem_oth = { &altr_oth, altr_oth.begin() };
            qval = &*(elem_val.curp);
            qoth = &*(elem_oth.curp);
            stack.emplace_back(::std::move(elem_val), ::std::move(elem_oth));
            goto r;
          }
          do_compare_3way_(altr_val.size(), altr_oth.size());
          break;
        }

        default:
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
      do_compare_3way_(elem_val.refa->size(), elem_oth.refa->size());
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
        c_quote(fmt, qval->m_stor.as<V_string>().data(), qval->m_stor.as<V_string>().size());
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
          stack.emplace_back(::std::move(elema));
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
          c_quote(fmt, elemo.curp->first.data(), elemo.curp->first.size());
          fmt << "\": ";
          qval = &(elemo.curp->second);
          stack.emplace_back(::std::move(elemo));
          goto r;
        }
        fmt << "{ }";
        break;
      }

      default:
        ASTERIA_TERMINATE(("Invalid value type (type `$1`)"), this->type());
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
            c_quote(fmt, elemo.curp->first.data(), elemo.curp->first.size());
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
        c_quote(fmt, altr.data(), altr.size());
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
          stack.emplace_back(::std::move(elema));
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
          c_quote(fmt, elemo.curp->first.data(), elemo.curp->first.size());
          fmt << "\" = ";
          qval = &(elemo.curp->second);
          stack.emplace_back(::std::move(elemo));
          goto r;
        }
        fmt << "object(0) { };";
        break;
      }

      default:
        ASTERIA_TERMINATE(("Invalid value type `$1`"), qval->type());
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
            c_quote(fmt, elemo.curp->first.data(), elemo.curp->first.size());
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
