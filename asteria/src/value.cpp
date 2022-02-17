// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "value.hpp"
#include "utils.hpp"
#include "llds/variable_hashmap.hpp"
#include "../rocket/linear_buffer.hpp"
#include "../rocket/tinyfmt_file.hpp"

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
    return ::std::islessequal(lhs, rhs)
         ? (::std::isless(lhs, rhs)    ? compare_less
                                       : compare_equal)
         : (::std::isgreater(lhs, rhs) ? compare_greater
                                       : compare_unordered);
  }

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

void
Value::
do_get_variables_slow(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    // Expand recursion by hand with a stack.
    auto qval = this;
    cow_vector<Rbr_Element> stack;

    do {
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
          const auto& altr = qval->as_array();
          if(altr.empty())
            break;

          if(!staged.insert(qval, nullptr))
            break;

          // Open an array.
          Rbr_array elem = { &altr, altr.begin() };
          stack.emplace_back(::std::move(elem));

          qval = &*(elem.curp);
          continue;
        }

        case type_object: {
          const auto& altr = qval->as_object();
          if(altr.empty())
            break;

          if(!staged.insert(qval, nullptr))
            break;

          // Open an object.
          Rbr_object elem = { &altr, altr.begin() };
          stack.emplace_back(::std::move(elem));

          qval = &(elem.curp->second);
          continue;
        }

        default:
          ASTERIA_TERMINATE("invalid value type (type `$1`)", qval->type());
      }

      // Advance to the next element.
      while(stack.size())
        if(stack.back().index() == 0) {
          auto& elem = stack.mut_back().as<0>();
          if(++(elem.curp) != elem.refa->end()) {
            qval = &*(elem.curp);
            break;
          }

          // Close this array.
          stack.pop_back();
        }
        else if(stack.back().index() == 1) {
          auto& elem = stack.mut_back().as<1>();
          if(++(elem.curp) != elem.refo->end()) {
            qval = &(elem.curp->second);
            break;
          }

          // Close this object.
          stack.pop_back();
        }
        else
          ROCKET_ASSERT(false);
    }
    while(stack.size());
  }

bool
Value::
do_test_slow() const noexcept
  {
    switch(this->type()) {
      case type_null:
        return false;

      case type_boolean:
        return this->as_boolean();

      case type_integer:
        return this->as_integer() != 0;

      case type_real:
        return this->as_real() != 0;

      case type_string:
        return this->as_string().empty() == false;

      case type_opaque:
      case type_function:
        return true;

      case type_array:
        return this->as_array().empty() == false;

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
                  lhs.as_boolean(), rhs.as_boolean());

      case type_integer:
        return do_3way_compare_scalar(
                  lhs.as_integer(), rhs.as_integer());

      case type_real:
        return do_3way_compare_scalar(
                  lhs.as_real(), rhs.as_real());

      case type_string:
        return do_3way_compare_scalar(
            lhs.as_string().compare(rhs.as_string()), 0);

      case type_opaque:
      case type_function:
        return compare_unordered;

      case type_array: {
        // Perform lexicographical comparison on the longest initial sequences
        // of the same length.
        const auto& la = lhs.as_array();
        const auto& ra = rhs.as_array();
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
          ASTERIA_TERMINATE("invalid value type (type `$1`)", this->type());
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

tinyfmt&
Value::
print(tinyfmt& fmt, bool escape) const
  {
    // Expand recursion by hand with a stack.
    auto qval = this;
    cow_vector<Rbr_Element> stack;

    do {
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
          if(altr.empty()) {
            fmt << "[ ]";
            break;
          }

          // Open an array.
          Rbr_array elem = { &altr, altr.begin() };
          stack.emplace_back(::std::move(elem));

          fmt << "[ ";
          qval = &*(elem.curp);
          continue;
        }

        case type_object: {
          const auto& altr = qval->as_object();
          if(altr.empty()) {
            fmt << "{ }";
            break;
          }

          // Open an object.
          Rbr_object elem = { &altr, altr.begin() };
          stack.emplace_back(::std::move(elem));

          fmt << "{ " << quote(elem.curp->first) << ": ";
          qval = &(elem.curp->second);
          continue;
        }

        default:
          ASTERIA_TERMINATE("invalid value type (type `$1`)", qval->type());
      }

      // Advance to the next element.
      while(stack.size())
        if(stack.back().index() == 0) {
          auto& elem = stack.mut_back().as<0>();
          if(++(elem.curp) != elem.refa->end()) {
            fmt << ", ";
            qval = &*(elem.curp);
            break;
          }

          // Close this array.
          stack.pop_back();
          fmt << " ]";
        }
        else if(stack.back().index() == 1) {
          auto& elem = stack.mut_back().as<1>();
          if(++(elem.curp) != elem.refo->end()) {
            fmt << ", " << quote(elem.curp->first) << ": ";
            qval = &(elem.curp->second);
            break;
          }

          // Close this object.
          stack.pop_back();
          fmt << " }";
        }
        else
          ROCKET_ASSERT(false);
    }
    while(stack.size());

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

    do {
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
          if(altr.empty()) {
            fmt << "[ ];";
            break;
          }

          // Open an array.
          Rbr_array elem = { &altr, altr.begin() };
          stack.emplace_back(::std::move(elem));

          fmt << '[';
          fmt << pwrap(indent, hanging + indent * stack.size());
          fmt << (elem.curp - altr.begin()) << " = ";
          qval = &*(elem.curp);
          continue;
        }

        case type_object: {
          const auto& altr = qval->as_object();
          fmt << "object(" << altr.size() << ") ";
          if(altr.empty()) {
            fmt << "{ };";
            break;
          }

          // Open an object.
          Rbr_object elem = { &altr, altr.begin() };
          stack.emplace_back(::std::move(elem));

          fmt << '{';
          fmt << pwrap(indent, hanging + indent * stack.size());
          fmt << quote(elem.curp->first) << " = ";
          qval = &(elem.curp->second);
          continue;
        }

        default:
          ASTERIA_TERMINATE("invalid value type (type `$1`)", qval->type());
      }

      // Advance to the next element.
      while(stack.size())
        if(stack.back().index() == 0) {
          auto& elem = stack.mut_back().as<0>();
          if(++(elem.curp) != elem.refa->end()) {
            fmt << pwrap(indent, hanging + indent * stack.size());
            fmt << (elem.curp - elem.refa->begin()) << " = ";
            qval = &*(elem.curp);
            break;
          }

          // Close this array.
          stack.pop_back();
          fmt << pwrap(indent, hanging + indent * stack.size());
          fmt << "];";
        }
        else if(stack.back().index() == 1) {
          auto& elem = stack.mut_back().as<1>();
          if(++(elem.curp) != elem.refo->end()) {
            fmt << pwrap(indent, hanging + indent * stack.size());
            fmt << quote(elem.curp->first) << " = ";
            qval = &(elem.curp->second);
            break;
          }

          // Close this object.
          stack.pop_back();
          fmt << pwrap(indent, hanging + indent * stack.size());
          fmt << "};";
        }
        else
          ROCKET_ASSERT(false);
    }
    while(stack.size());

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
