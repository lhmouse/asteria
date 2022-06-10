// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

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
    ::rocket::linear_buffer bytes;

    do {
      switch(this->type()) {
        case type_null:
        case type_boolean:
        case type_integer:
        case type_real:
          break;

        case type_string:
          this->m_stor.as<type_string>().~V_string();
          break;

        case type_opaque:
          this->m_stor.as<type_opaque>().~V_opaque();
          break;

        case type_function:
          this->m_stor.as<type_function>().~V_function();
          break;

        case type_array: {
          auto& altr = this->m_stor.as<type_array>();
          if(altr.unique() && !altr.empty()) {
            // Move raw bytes into `bytes`.
            for(auto it = altr.mut_begin();  it != altr.end();  ++it) {
              bytes.putn(it->m_bytes, sizeof(storage));
              ::std::memset(it->m_bytes, 0, sizeof(storage));
            }
          }
          altr.~V_array();
          break;
        }

        case type_object: {
          auto& altr = this->m_stor.as<type_object>();
          if(altr.unique() && !altr.empty()) {
            // Move raw bytes into `bytes`.
            for(auto it = altr.mut_begin();  it != altr.end();  ++it) {
              bytes.putn(it->second.m_bytes, sizeof(storage));
              ::std::memset(it->second.m_bytes, 0, sizeof(storage));
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
    }
    while(bytes.getn(this->m_bytes, sizeof(storage)) != 0);

    ROCKET_ASSERT(bytes.empty());
    ::std::memset(this->m_bytes, 0xEB, sizeof(storage));
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

        qval->as_opaque().get_variables(staged, temp);
        break;

      case type_function:
        if(!staged.insert(qval, nullptr))
          break;

        qval->as_function().get_variables(staged, temp);
        break;

      case type_array: {
        if(!staged.insert(qval, nullptr))
          break;

        const auto& altr = qval->as_array();
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

        const auto& altr = qval->as_object();
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
          auto& elema = elem.as<0>();
          if(++(elema.curp) != elema.refa->end()) {
            qval = &*(elema.curp);
            goto r;
          }

          // Close this array.
          break;
        }

        case 1: {
          auto& elemo = elem.as<1>();
          if(++(elemo.curp) != elemo.refo->end()) {
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

Compare
Value::
do_compare_slow(const Value& other) const noexcept
  {
    // Expand recursion by hand with a stack.
    static_assert(compare_greater == 1);
    static_assert(compare_less == 2);
    static_assert(compare_equal == 3);

    auto qlhs = this;
    auto qrhs = &other;
    cow_bivector<Rbr_array, Rbr_array> stack;
    Compare comp;

  r:
    if(qlhs->is_real() && qrhs->is_real()) {
      // Convert integers to reals and compare them.
      comp = static_cast<Compare>(
               ::std::islessequal(qlhs->as_real(), qrhs->as_real()) * 2 +
               ::std::isgreaterequal(qlhs->as_real(), qrhs->as_real()));
    }
    else {
      // Non-arithmetic values of different types can't be compared.
      if(qlhs->type() != qrhs->type())
        return compare_unordered;

      // Compare values of the same type.
      comp = compare_equal;

      switch(qlhs->type()) {
        case type_null:
          break;

        case type_boolean:
          comp = details_value::do_3way_compare<int>(
                           qlhs->as_boolean(), qrhs->as_boolean());
          break;

        case type_integer:
          comp = details_value::do_3way_compare<V_integer>(
                           qlhs->as_integer(), qrhs->as_integer());
          break;

        case type_real:
          ROCKET_ASSERT(false);

        case type_string:
          comp = details_value::do_3way_compare<int>(
                        qlhs->as_string().compare(qrhs->as_string()), 0);
          break;

        case type_opaque:
        case type_function:
          return compare_unordered;

        case type_array: {
          const auto& altr1 = qlhs->as_array();
          const auto& altr2 = qrhs->as_array();

          if(altr1.empty() != altr2.empty())
            return details_value::do_3way_compare<size_t>(
                                       altr1.size(), altr2.size());

          if(!altr1.empty()) {
            // Open a pair of arrays.
            Rbr_array elem1 = { &altr1, altr1.begin() };
            qlhs = &*(elem1.curp);
            Rbr_array elem2 = { &altr2, altr2.begin() };
            qrhs = &*(elem2.curp);
            stack.emplace_back(::std::move(elem1), ::std::move(elem2));
            goto r;
          }

          break;
        }

        case type_object:
          return compare_unordered;

        default:
          ASTERIA_TERMINATE((
              "Invalid value type (type `$1`)"),
              qlhs->type());
      }
    }

    if(comp != compare_equal)
      return comp;

    while(stack.size()) {
      // Advance to the next element.
      auto& elem = stack.mut_back();
      bool cont1 = ++(elem.first.curp) != elem.first.refa->end();
      bool cont2 = ++(elem.second.curp) != elem.second.refa->end();

      if(cont1 != cont2)
        return details_value::do_3way_compare<size_t>(
                     elem.first.refa->size(), elem.second.refa->size());

      if(cont1) {
        qlhs = &*(elem.first.curp);
        qrhs = &*(elem.second.curp);
        goto r;
      }

      // Close these arrays.
      stack.pop_back();
    }

    return compare_equal;
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
    // Expand recursion by hand with a stack.
    auto qval = this;
    cow_vector<Rbr_Element> stack;

  r:
    switch(qval->type()) {
      case type_null:
        fmt << "null";
        break;

      case type_boolean:
        fmt << qval->as_boolean();
        break;

      case type_integer:
        fmt << qval->as_integer();
        break;

      case type_real:
        fmt << qval->as_real();
        break;

      case type_string:
        if(escape) {
          // Escape the string.
          fmt << quote(qval->as_string());
        }
        else {
          // Write the string verbatim.
          fmt << qval->as_string();
        }
        break;

      case type_opaque:
        fmt << "(opaque) [[" << qval->as_opaque() << "]]";
        break;

      case type_function:
        fmt << "(function) [[" << qval->as_function() << "]]";
        break;

      case type_array: {
        const auto& altr = qval->as_array();

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
        const auto& altr = qval->as_object();

        Rbr_object elemo = { &altr, altr.begin() };
        if(elemo.curp != altr.end()) {
          // Open an object.
          fmt << "{ " << quote(elemo.curp->first) << ": ";
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
          auto& elema = elem.as<0>();
          if(++(elema.curp) != elema.refa->end()) {
            fmt << ", ";
            qval = &*(elema.curp);
            goto r;
          }

          // Close this array.
          fmt << " ]";
          break;
        }

        case 1: {
          auto& elemo = elem.as<1>();
          if(++(elemo.curp) != elemo.refo->end()) {
            fmt << ", " << quote(elemo.curp->first) << ": ";
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
print_to_stderr(bool escape) const
  {
    ::rocket::tinyfmt_file fmt(stderr, nullptr);
    this->print(fmt, escape);
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
        fmt << "boolean " << qval->as_boolean() << ';';
        break;

      case type_integer:
        fmt << "integer " << qval->as_integer() << ';';
        break;

      case type_real:
        fmt << "real " << qval->as_real() << ';';
        break;

      case type_string: {
        const auto& altr = qval->as_string();
        fmt << "string(" << altr.size() << ") " << quote(altr) << ';';
        break;
      }

      case type_opaque: {
        const auto& altr = qval->as_opaque();
        fmt << "opaque [[" << altr << "]];";
        break;
      }

      case type_function: {
        const auto& altr = qval->as_function();
        fmt << "function [[" << altr << "]];";
        break;
      }

      case type_array: {
        const auto& altr = qval->as_array();
        fmt << "array(" << altr.size() << ") ";

        Rbr_array elema = { &altr, altr.begin() };
        if(elema.curp != altr.end()) {
          // Open an array.
          fmt << '[';
          fmt << pwrap(indent, hanging + indent * (stack.size() + 1));
          fmt << (elema.curp - altr.begin()) << " = ";
          qval = &*(elema.curp);
          stack.emplace_back(::std::move(elema));
          goto r;
        }

        fmt << "[ ];";
        break;
      }

      case type_object: {
        const auto& altr = qval->as_object();
        fmt << "object(" << altr.size() << ") ";

        Rbr_object elemo = { &altr, altr.begin() };
        if(elemo.curp != altr.end()) {
          // Open an object.
          fmt << '{';
          fmt << pwrap(indent, hanging + indent * (stack.size() + 1));
          fmt << quote(elemo.curp->first) << " = ";
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
          auto& elema = elem.as<0>();
          if(++(elema.curp) != elema.refa->end()) {
            fmt << pwrap(indent, hanging + indent * stack.size());
            fmt << (elema.curp - elema.refa->begin()) << " = ";
            qval = &*(elema.curp);
            goto r;
          }

          // Close this array.
          fmt << pwrap(indent, hanging + indent * (stack.size() - 1));
          fmt << "];";
          break;
        }

        case 1: {
          auto& elemo = elem.as<1>();
          if(++(elemo.curp) != elemo.refo->end()) {
            fmt << pwrap(indent, hanging + indent * stack.size());
            fmt << quote(elemo.curp->first) << " = ";
            qval = &(elemo.curp->second);
            goto r;
          }

          // Close this object.
          fmt << pwrap(indent, hanging + indent * (stack.size() - 1));
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
