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

    // Expand arrays and objects by hand.
    // This blows the entire C++ object model up. Don't play with this at home!
    ::rocket::linear_buffer byte_stack;

  r:
    switch(this->type()) {
      case type_null:
      case type_boolean:
      case type_integer:
      case type_real:
        break;

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
          // Move raw bytes into `byte_stack`.
          char* adata = (char*) altr.mut_data();
          byte_stack.putn(adata, altr.size() * sizeof(*this));
          ::memset(adata, 0, altr.size() * sizeof(*this));
        }
        altr.~V_array();
        break;
      }

      case type_object: {
        auto& altr = this->m_stor.mut<type_object>();
        if(!altr.empty() && altr.unique()) {
          // Move raw bytes into `byte_stack`.
          for(auto it = altr.mut_begin();  it != altr.end();  ++it) {
            char* adata = (char*) &(it->second);
            byte_stack.putn(adata, sizeof(*this));
            ::memset(adata, 0, sizeof(*this));
          }
        }
        altr.~V_object();
        break;
      }

      default:
        ASTERIA_TERMINATE((
            "Invalid value type (type `$1`)"),
            this->type());
    }

    if(!byte_stack.empty()) {
      // Pop an element.
      ROCKET_ASSERT(byte_stack.size() >= sizeof(*this));
      ::memcpy(this->m_bytes, byte_stack.end() - sizeof(*this), sizeof(*this));
      byte_stack.unaccept(sizeof(*this));
      goto r;
    }

#ifdef ROCKET_DEBUG
    ::std::memset(this->m_bytes, 0xEB, sizeof(*this));
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
do_get_variables_slow(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    // Expand recursion by hand with a stack.
    auto qval = this;
    cow_vector<Rbr_Element> stack;

  r:
    switch(qval->type()) {
      case type_null:
      case type_boolean:
      case type_integer:
      case type_real:
      case type_string:
        break;

      case type_opaque:
        if(!staged.insert(qval, nullptr))
          break;

        // Collect variables recursively.
        qval->m_stor.as<V_opaque>().get_variables(staged, temp);
        break;

      case type_function:
        if(!staged.insert(qval, nullptr))
          break;

        // Collect variables recursively.
        qval->m_stor.as<V_function>().get_variables(staged, temp);
        break;

      case type_array: {
        if(!staged.insert(qval, nullptr))
          break;

        // Push this element onto `stack`.
        const auto& altr = qval->m_stor.as<V_array>();
        Rbr_array elema = { &altr, altr.begin() };
        if(elema.curp != altr.end()) {
          // Open an array.
          qval = &*(elema.curp);
          stack.emplace_back(::std::move(elema));
          goto r;
        }

        break;
      }

      case type_object: {
        if(!staged.insert(qval, nullptr))
          break;

        // Push this element onto `stack`.
        const auto& altr = qval->m_stor.as<V_object>();
        Rbr_object elemo = { &altr, altr.begin() };
        if(elemo.curp != altr.end()) {
          // Open an object.
          qval = &(elemo.curp->second);
          stack.emplace_back(::std::move(elemo));
          goto r;
        }

        break;
      }

      default:
        ASTERIA_TERMINATE((
            "Invalid value type (type `$1`)"),
            qval->type());
    }

    while(stack.size()) {
      // Advance to the next element.
      auto& elem = stack.mut_back();
      switch(elem.index()) {
        case 0: {
          auto& elema = elem.mut<0>();
          ++ elema.curp;
          if(elema.curp != elema.refa->end()) {
            qval = &*(elema.curp);
            goto r;
          }

          // Close this array.
          break;
        }

        case 1: {
          auto& elemo = elem.mut<1>();
          ++ elemo.curp;
          if(elemo.curp != elemo.refo->end()) {
            qval = &(elemo.curp->second);
            goto r;
          }

          // Close this object.
          break;
        }

        default:
          ROCKET_ASSERT(false);
      }

      stack.pop_back();
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

  r:
    switch(qval->type_mask() | qoth->type_mask()) {
      case M_null:
        break;

      case M_boolean: {
        ROCKET_ASSERT(qval->m_stor.index() == type_boolean);
        ROCKET_ASSERT(qoth->m_stor.index() == type_boolean);
        V_boolean lhs = qval->m_stor.as<V_boolean>();
        V_boolean rhs = qoth->m_stor.as<V_boolean>();

        do_compare_3way_(lhs, rhs);
        break;
      }

      case M_integer: {
        ROCKET_ASSERT(qval->m_stor.index() == type_integer);
        ROCKET_ASSERT(qoth->m_stor.index() == type_integer);
        V_integer lhs = qval->m_stor.as<V_integer>();
        V_integer rhs = qoth->m_stor.as<V_integer>();

        do_compare_3way_(lhs, rhs);
        break;
      }

      case M_real | M_integer:
      case M_real: {
        ROCKET_ASSERT(qval->is_real());
        ROCKET_ASSERT(qoth->is_real());
        V_real lhs = qval->as_real();
        V_real rhs = qoth->as_real();

        if(::std::isunordered(lhs, rhs))
          return compare_unordered;

        do_compare_3way_(lhs, rhs);
        break;
      }

      case M_string: {
        ROCKET_ASSERT(qval->m_stor.index() == type_string);
        ROCKET_ASSERT(qoth->m_stor.index() == type_string);
        const V_string& lhs = qval->m_stor.as<V_string>();
        const V_string& rhs = qoth->m_stor.as<V_string>();

        int cmp = lhs.compare(rhs);
        do_compare_3way_(cmp, 0);
        break;
      }

      case M_array: {
        ROCKET_ASSERT(qval->m_stor.index() == type_array);
        ROCKET_ASSERT(qoth->m_stor.index() == type_array);
        const V_array& lhs = qval->m_stor.as<V_array>();
        const V_array& rhs = qoth->m_stor.as<V_array>();

        if(!lhs.empty() && !rhs.empty()) {
          // Open a pair of arrays.
          Rbr_array lelem = { &lhs, lhs.begin() };
          qval = &*(lelem.curp);
          Rbr_array relem = { &rhs, rhs.begin() };
          qoth = &*(relem.curp);
          stack.emplace_back(::std::move(lelem), ::std::move(relem));
          goto r;
        }

        do_compare_3way_(lhs.size(), rhs.size());
        break;
      }

      default:
        return compare_unordered;
    }

    while(stack.size()) {
      // Advance to the next elements.
      auto& r = stack.mut_back();
      ++ r.first.curp;
      ++ r.second.curp;
      if((r.first.curp != r.first.refa->end()) && (r.second.curp != r.second.refa->end())) {
        qval = &*(r.first.curp);
        qoth = &*(r.second.curp);
        goto r;
      }

      // Close these arrays.
      do_compare_3way_(r.first.refa->size(), r.second.refa->size());

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
    switch(qval->type()) {
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
        fmt << '\"';
        c_quote(fmt, qval->m_stor.as<V_string>().data(), qval->m_stor.as<V_string>().size());
        fmt << '\"';
        break;

      case type_opaque:
        fmt << "(opaque) [[" << qval->m_stor.as<V_opaque>() << "]]";
        break;

      case type_function:
        fmt << "(function) [[" << qval->m_stor.as<V_function>() << "]]";
        break;

      case type_array: {
        const auto& altr = qval->m_stor.as<V_array>();

        Rbr_array elema = { &altr, altr.begin() };
        if(elema.curp != altr.end()) {
          // Open an array.
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

        Rbr_object elemo = { &altr, altr.begin() };
        if(elemo.curp != altr.end()) {
          // Open an object.
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
        ASTERIA_TERMINATE((
            "Invalid value type (type `$1`)"),
            qval->type());
    }

    while(stack.size()) {
      // Advance to the next element.
      auto& elem = stack.mut_back();
      switch(elem.index()) {
        case 0: {
          auto& elema = elem.mut<0>();
          ++ elema.curp;
          if(elema.curp != elema.refa->end()) {
            fmt << ", ";
            qval = &*(elema.curp);
            goto r;
          }

          // Close this array.
          fmt << " ]";
          break;
        }

        case 1: {
          auto& elemo = elem.mut<1>();
          ++ elemo.curp;
          if(elemo.curp != elemo.refo->end()) {
            fmt << ", \"";
            c_quote(fmt, elemo.curp->first.data(), elemo.curp->first.size());
            fmt << "\": ";
            qval = &(elemo.curp->second);
            goto r;
          }

          // Close this object.
          fmt << " }";
          break;
        }

        default:
          ROCKET_ASSERT(false);
      }

      stack.pop_back();
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
        fmt << "boolean " << qval->m_stor.as<V_boolean>() << ';';
        break;

      case type_integer:
        fmt << "integer " << qval->m_stor.as<V_integer>() << ';';
        break;

      case type_real:
        fmt << "real " << qval->m_stor.as<V_real>() << ';';
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
        fmt << "array(" << altr.size() << ") ";

        Rbr_array elema = { &altr, altr.begin() };
        if(elema.curp != altr.end()) {
          // Open an array.
          fmt << '[';
          details_value::do_break_line(fmt, indent, hanging + indent * (stack.size() + 1));
          fmt << (elema.curp - altr.begin()) << " = ";
          qval = &*(elema.curp);
          stack.emplace_back(::std::move(elema));
          goto r;
        }

        fmt << "[ ];";
        break;
      }

      case type_object: {
        const auto& altr = qval->m_stor.as<V_object>();
        fmt << "object(" << altr.size() << ") ";

        Rbr_object elemo = { &altr, altr.begin() };
        if(elemo.curp != altr.end()) {
          // Open an object.
          fmt << '{';
          details_value::do_break_line(fmt, indent, hanging + indent * (stack.size() + 1));
          fmt << '\"';
          c_quote(fmt, elemo.curp->first.data(), elemo.curp->first.size());
          fmt << "\" = ";
          qval = &(elemo.curp->second);
          stack.emplace_back(::std::move(elemo));
          goto r;
        }

        fmt << "{ };";
        break;
      }

      default:
        ASTERIA_TERMINATE((
            "Invalid value type (type `$1`)"),
            qval->type());
    }

    while(stack.size()) {
      // Advance to the next element.
      auto& elem = stack.mut_back();
      switch(elem.index()) {
        case 0: {
          auto& elema = elem.mut<0>();
          ++ elema.curp;
          if(elema.curp != elema.refa->end()) {
            details_value::do_break_line(fmt, indent, hanging + indent * stack.size());
            fmt << (elema.curp - elema.refa->begin()) << " = ";
            qval = &*(elema.curp);
            goto r;
          }

          // Close this array.
          details_value::do_break_line(fmt, indent, hanging + indent * (stack.size() - 1));
          fmt << "];";
          break;
        }

        case 1: {
          auto& elemo = elem.mut<1>();
          ++ elemo.curp;
          if(elemo.curp != elemo.refo->end()) {
            details_value::do_break_line(fmt, indent, hanging + indent * stack.size());
            fmt << '\"';
            c_quote(fmt, elemo.curp->first.data(), elemo.curp->first.size());
            fmt << "\" = ";
            qval = &(elemo.curp->second);
            goto r;
          }

          // Close this object.
          details_value::do_break_line(fmt, indent, hanging + indent * (stack.size() - 1));
          fmt << "};";
          break;
        }

        default:
          ROCKET_ASSERT(false);
      }

      stack.pop_back();
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
