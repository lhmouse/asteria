// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef __FAST_MATH__
#  error Please turn off `-ffast-math`.
#endif

#include <initializer_list>  // ::std::initializer_list<>
#include <utility>  // ::std::pair<>, ::std::move(), ::std::forward(), ::std::integer_sequence<>
#include <stdexcept>
#include <typeinfo>
#include <cstddef>
#include <cstdint>
#include <climits>
#include "../rocket/preprocessor_utilities.h"
#include "../rocket/cow_string.hpp"
#include "../rocket/cow_vector.hpp"
#include "../rocket/cow_hashmap.hpp"
#include "../rocket/static_vector.hpp"
#include "../rocket/prehashed_string.hpp"
#include "../rocket/unique_handle.hpp"
#include "../rocket/unique_ptr.hpp"
#include "../rocket/refcnt_ptr.hpp"
#include "../rocket/variant.hpp"
#include "../rocket/optional.hpp"
#include "../rocket/array.hpp"
#include "../rocket/reference_wrapper.hpp"
#include "../rocket/tinyfmt.hpp"

namespace Asteria {

// Macros
#define ASTERIA_SFINAE_CONSTRUCT(T, ...)    ROCKET_ENABLE_IF(::std::is_constructible<T, __VA_ARGS__>::value)
#define ASTERIA_SFINAE_ASSIGN(T, ...)       ROCKET_ENABLE_IF(::rocket::is_lvalue_assignable<T, __VA_ARGS__>::value)
#define ASTERIA_SFINAE_CONVERT(T, ...)      ROCKET_ENABLE_IF(::std::is_convertible<T, __VA_ARGS__>::value)

#define ASTERIA_VARIANT_CONSTRUCTOR(C, V, T, t)   template<typename T, ASTERIA_SFINAE_CONSTRUCT(V, T&&)> C(T&& t)
#define ASTERIA_VARIANT_ASSIGNMENT(C, V, T, t)    template<typename T, ASTERIA_SFINAE_ASSIGN(V, T&&)> C& operator=(T&& t)

#define ASTERIA_INCOMPLET(T)       template<typename T##_1_ = T, typename T = T##_1_>

#define ASTERIA_VOID_T(...)        typename ::rocket::make_void<__VA_ARGS__>::type
#define ASTERIA_VOID_OF(...)       typename ::rocket::make_void<decltype(__VA_ARGS__)>::type

// `using`-directives
using ::std::initializer_list;
using ::std::integer_sequence;
using ::std::index_sequence;
using ::std::nullptr_t;
using ::std::max_align_t;
using ::std::int8_t;
using ::std::uint8_t;
using ::std::int16_t;
using ::std::uint16_t;
using ::std::int32_t;
using ::std::uint32_t;
using ::std::int64_t;
using ::std::uint64_t;
using ::std::intptr_t;
using ::std::uintptr_t;
using ::std::intmax_t;
using ::std::uintmax_t;
using ::std::ptrdiff_t;
using ::std::size_t;
using ::std::wint_t;
using ::std::exception;
using ::std::type_info;

using ::rocket::cbegin;
using ::rocket::cend;
using ::rocket::begin;
using ::rocket::end;
using ::rocket::xswap;
using ::rocket::swap;
using ::rocket::nullopt;

// Aliases
using cow_string = ::rocket::cow_string;
using cow_wstring = ::rocket::cow_wstring;
using cow_u16string = ::rocket::cow_u16string;
using cow_u32string = ::rocket::cow_u32string;
using phsh_string = ::rocket::prehashed_string;
using tinybuf = ::rocket::tinybuf;
using tinyfmt = ::rocket::tinyfmt;

template<typename E, typename D = ::std::default_delete<const E>> using uptr = ::rocket::unique_ptr<E, D>;
template<typename E> using rcptr = ::rocket::refcnt_ptr<E>;
template<typename E> using cow_vector = ::rocket::cow_vector<E>;
template<typename E> using cow_dictionary = ::rocket::cow_hashmap<::rocket::prehashed_string, E,
                                                ::rocket::prehashed_string::hash, ::std::equal_to<void>>;
template<typename E, size_t k> using sso_vector = ::rocket::static_vector<E, k>;
template<typename... P> using variant = ::rocket::variant<P...>;
template<typename T> using opt = ::rocket::optional<T>;
template<typename F, typename S> using pair = ::std::pair<F, S>;
template<typename F, typename S> using cow_bivector = ::rocket::cow_vector<::std::pair<F, S>>;
template<typename E, size_t... k> using array = ::rocket::array<E, k...>;
template<typename E> using ref_to = ::rocket::reference_wrapper<E>;

// Core
class Value;
class Source_Location;
class Recursion_Sentry;

// Low-level data structures
class Variable_HashSet;
class Reference_Dictionary;
class AVMC_Queue;

// Runtime
enum AIR_Status : uint8_t;
enum PTC_Aware : uint8_t;
class Abstract_Hooks;
class Runtime_Error;
class Reference_root;
class Reference_modifier;
class Reference;
class Evaluation_Stack;
class Variable;
class Variable_Callback;
class PTC_Arguments;
class Collector;
class Abstract_Context;
class Analytic_Context;
class Executive_Context;
class Global_Context;
class Random_Number_Generator;
class Generational_Collector;
class Variadic_Arguer;
class Instantiated_Function;
class AIR_Node;
class Backtrace_Frame;
class Simple_Script;
class Argument_Reader;

// Compiler
enum Punctuator : uint8_t;
enum Keyword : uint8_t;
enum Jump_Target : uint8_t;
enum Precedence : uint8_t;
enum Xop : uint8_t;
class Parser_Error;
class Token;
class Token_Stream;
class Expression_Unit;
class Statement;
class Infix_Element;
class Statement_Sequence;

// Type erasure
struct Rcbase : virtual ::rocket::refcnt_base<Rcbase>
  {
    virtual ~Rcbase();
  };

template<typename RealT> struct Rcfwd : Rcbase
  {
  };

template<typename RealT> using rcfwdp = rcptr<Rcfwd<RealT>>;

template<typename RealT> constexpr rcptr<RealT> unerase_cast(const rcfwdp<RealT>& ptr) noexcept
  {
    return ::rocket::static_pointer_cast<RealT>(ptr);
  }

// Opaque (user-defined) type support
struct Abstract_Opaque : public Rcbase
  {
    ~Abstract_Opaque() override;

    // This function is called to convert this object to a human-readble string.
    virtual tinyfmt& describe(tinyfmt& fmt) const = 0;

    // This function is called during garbage collection to mark variables that are not
    // directly reachable. Although strong exception safety is guaranteed, it is discouraged
    // to throw exceptions in this function, as it prevents garbage collection from running.
    virtual Variable_Callback& enumerate_variables(Variable_Callback& callback) const = 0;

    // This function is called when a mutable reference is requested and the current instance
    // is shared. If this function returns a null pointer, the shared instance is used. If this
    // function returns a non-null pointer, it replaces the current value. Derived classes that
    // are not copyable should throw an exception in this function.
    virtual Abstract_Opaque* clone_opt(rcptr<Abstract_Opaque>& output) const = 0;
  };

inline tinyfmt& operator<<(tinyfmt& fmt, const Abstract_Opaque& opaq)
  {
    return opaq.describe(fmt);
  }

struct Abstract_Function : public Rcbase
  {
    ~Abstract_Function() override;

    // This function is called to convert this object to a human-readble string.
    virtual tinyfmt& describe(tinyfmt& fmt) const = 0;

    // This function is called during garbage collection to mark variables that are not
    // directly reachable. Although strong exception safety is guaranteed, it is discouraged
    // to throw exceptions in this function, as it prevents garbage collection from running.
    virtual Variable_Callback& enumerate_variables(Variable_Callback& callback) const = 0;

    // This function may return a proper tail call wrapper.
    virtual Reference& invoke_ptc_aware(Reference& self, Global_Context& global, cow_vector<Reference>&& args) const = 0;
  };

inline tinyfmt& operator<<(tinyfmt& fmt, const Abstract_Function& func)
  {
    return func.describe(fmt);
  }

class cow_opaque
  {
  private:
    rcptr<Abstract_Opaque> m_sptr;

  public:
    constexpr cow_opaque(nullptr_t = nullptr) noexcept
      {
      }
    template<typename OpaqueT> constexpr cow_opaque(rcptr<OpaqueT> sptr) noexcept
      :
        m_sptr(::rocket::move(sptr))
      {
      }

  private:
    [[noreturn]] void do_throw_null_pointer() const;

  public:
    bool unique() const noexcept
      {
        return this->m_sptr.unique();
      }
    long use_count() const noexcept
      {
        return this->m_sptr.use_count();
      }
    cow_opaque& reset() noexcept
      {
        this->m_sptr = nullptr;
        return *this;
      }

    explicit constexpr operator bool () const noexcept
      {
        return bool(this->m_sptr);
      }
    const type_info& type() const noexcept
      {
        return typeid(*(this->m_sptr.get()));
      }

    tinyfmt& describe(tinyfmt& fmt) const
      {
        auto ptr = this->m_sptr.get();
        if(!ptr) {
          return fmt << "<null opaque pointer>";
        }
        return ptr->describe(fmt);
      }
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const
      {
        auto ptr = this->m_sptr.get();
        if(!ptr) {
          return callback;
        }
        return ptr->enumerate_variables(callback);
      }

    template<typename OpaqueT> rcptr<const OpaqueT> cast_opt() const
      {
        auto ptr = this->m_sptr.get();
        if(!ptr) {
          this->do_throw_null_pointer();
        }
        auto tsptr = ::rocket::dynamic_pointer_cast<const OpaqueT>(this->m_sptr);
        return tsptr;
      }
    template<typename OpaqueT> rcptr<OpaqueT> open_opt()
      {
        auto ptr = this->m_sptr.get();
        if(!ptr) {
          this->do_throw_null_pointer();
        }
        auto tsptr = ::rocket::dynamic_pointer_cast<OpaqueT>(this->m_sptr);
        if(!tsptr) {
          return tsptr;
        }
        // Clone the existent instance if it is shared.
        // If the overriding function returns a null pointer, the shared instance is used.
        // Note the covariance of the return type of `clone_opt()`.
        rcptr<Abstract_Opaque> sptr2;
        auto tptr2 = tsptr->clone_opt(sptr2);
        if(!tptr2) {
          return tsptr;
        }
        // Take ownership of the clone.
        // Don't introduce a dependent name here.
        ROCKET_ASSERT(tptr2 == sptr2.get());
        this->m_sptr.swap(sptr2);
        ptr = tptr2;
        ptr->add_reference();
        tsptr.reset(tptr2);
        return tsptr;
      }
  };

inline tinyfmt& operator<<(tinyfmt& fmt, const cow_opaque& opaq)
  {
    return opaq.describe(fmt);
  }

// Function type support
using simple_function_3 = Value (cow_vector<Reference>&& args,  // required if takes arguments
                                 Reference&& self,              // required if is a member function
                                 Global_Context& global);       // required if contains static data

using simple_function_0 = Value ();
using simple_function_1 = Value (cow_vector<Reference>&&);
using simple_function_2 = Value (cow_vector<Reference>&&, Reference&&);

class cow_function
  {
  private:
    simple_function_3* m_fptr = nullptr;
    const char* m_desc = nullptr;
    rcptr<const Abstract_Function> m_sptr;

  public:
    constexpr cow_function(nullptr_t = nullptr) noexcept
      {
      }
    constexpr cow_function(simple_function_3* fptr, const char* desc) noexcept
      :
        m_fptr(fptr), m_desc(desc)
      {
      }
    cow_function(simple_function_0* fptr, const char* desc) noexcept
      :
        m_fptr((simple_function_3*)(intptr_t)fptr), m_desc(desc)
      {
      }
    cow_function(simple_function_1* fptr, const char* desc) noexcept
      :
        m_fptr((simple_function_3*)(intptr_t)fptr), m_desc(desc)
      {
      }
    cow_function(simple_function_2* fptr, const char* desc) noexcept
      :
        m_fptr((simple_function_3*)(intptr_t)fptr), m_desc(desc)
      {
      }
    template<typename FunctionT> constexpr cow_function(rcptr<FunctionT> sptr) noexcept
      :
        m_sptr(::rocket::move(sptr))
      {
      }

  private:
    [[noreturn]] void do_throw_null_pointer() const;

  public:
    bool unique() const noexcept
      {
        return this->m_sptr.unique();
      }
    long use_count() const noexcept
      {
        return this->m_sptr.use_count();
      }
    cow_function& reset() noexcept
      {
        this->m_fptr = nullptr;
        this->m_desc = nullptr;
        this->m_sptr = nullptr;
        return *this;
      }

    explicit constexpr operator bool () const noexcept
      {
        auto fptr = this->m_fptr;
        if(fptr) {
          return true;  // static
        }
        return bool(this->m_sptr);  // dynamic
      }
    const type_info& type() const noexcept
      {
        auto fptr = this->m_fptr;
        if(fptr) {
          return typeid(*fptr);  // static
        }
        return typeid(*(this->m_sptr.get()));  // dynamic
      }

    tinyfmt& describe(tinyfmt& fmt) const
      {
        auto fptr = this->m_fptr;
        if(fptr) {
          return fmt << this->m_desc << " (native function @ " << (void*)(intptr_t)fptr << ")";  // static
        }
        auto ptr = this->m_sptr.get();
        if(!ptr) {
          return fmt << "<null function pointer>";
        }
        return ptr->describe(fmt);  // dynamic
      }
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const
      {
        auto fptr = this->m_fptr;
        if(fptr) {
          return callback;  // static - nothing to do
        }
        auto ptr = this->m_sptr.get();
        if(!ptr) {
          return callback;
        }
        return ptr->enumerate_variables(callback);  // dynamic
      }

    Reference& invoke_ptc_aware(Reference& self, Global_Context& global, cow_vector<Reference>&& args = { }) const;
    Reference& invoke(Reference& self, Global_Context& global, cow_vector<Reference>&& args = { }) const;
    Reference invoke(Global_Context& global, cow_vector<Reference>&& args = { }) const;
  };

inline tinyfmt& operator<<(tinyfmt& fmt, const cow_function& func)
  {
    return func.describe(fmt);
  }

// Fundamental types
using V_null      = nullptr_t;
using V_boolean   = bool;
using V_integer   = int64_t;
using V_real      = double;
using V_string    = cow_string;
using V_opaque    = cow_opaque;
using V_function  = cow_function;
using V_array     = cow_vector<Value>;
using V_object    = cow_dictionary<Value>;

// Indices of fundamental types
enum Vtype : uint8_t
  {
    vtype_null      = 0,
    vtype_boolean   = 1,
    vtype_integer   = 2,
    vtype_real      = 3,
    vtype_string    = 4,
    vtype_opaque    = 5,
    vtype_function  = 6,
    vtype_array     = 7,
    vtype_object    = 8,
  };

ROCKET_PURE_FUNCTION extern const char* describe_vtype(Vtype vtype) noexcept;

// Value comparison results
enum Compare : uint8_t
  {
    compare_unordered  = 0,  // The LHS operand is unordered with the RHS operand.
    compare_less       = 1,  // The LHS operand is less than the RHS operand.
    compare_equal      = 2,  // The LHS operand is equal to the RHS operand.
    compare_greater    = 3,  // The LHS operand is greater than the RHS operand.
  };

// Stack frame types
enum Frame_Type : uint8_t
  {
    frame_type_native  = 0,  // The frame could not be determined.
    frame_type_throw   = 1,  // An exception was thrown here.
    frame_type_catch   = 2,  // An exception was caught here.
    frame_type_call    = 3,  // An exception propagated across a function call.
    frame_type_func    = 4,  // An exception propagated across a function boundary.
    frame_type_defer   = 5,  // A new exception was thrown here.
  };

ROCKET_PURE_FUNCTION extern const char* describe_frame_type(Frame_Type type) noexcept;

// Garbage collection generations
enum GC_Generation : uint8_t
  {
    gc_generation_newest  = 0,
    gc_generation_middle  = 1,
    gc_generation_oldest  = 2,
  };

// Parser status codes
enum Parser_Status : uint32_t
  {
    parser_status_success                                    =    0,
    parser_status_utf8_sequence_invalid                      = 1001,
    parser_status_utf8_sequence_incomplete                   = 1002,
    parser_status_utf_code_point_invalid                     = 1003,
    parser_status_null_character_disallowed                  = 1004,
    parser_status_token_character_unrecognized               = 2001,
    parser_status_string_literal_unclosed                    = 2002,
    parser_status_escape_sequence_unknown                    = 2003,
    parser_status_escape_sequence_incomplete                 = 2004,
    parser_status_escape_sequence_invalid_hex                = 2005,
    parser_status_escape_utf_code_point_invalid              = 2006,
    parser_status_numeric_literal_invalid                    = 2007,
    parser_status_integer_literal_overflow                   = 2008,
    parser_status_integer_literal_inexact                    = 2009,
    parser_status_real_literal_overflow                      = 2010,
    parser_status_real_literal_underflow                     = 2011,
    parser_status_numeric_literal_suffix_invalid             = 2012,
    parser_status_block_comment_unclosed                     = 2014,
    parser_status_digit_separator_following_nondigit         = 2015,
    parser_status_identifier_expected                        = 3002,
    parser_status_semicolon_expected                         = 3003,
    parser_status_string_literal_expected                    = 3004,
    parser_status_statement_expected                         = 3005,
    parser_status_equals_sign_expected                       = 3006,
    parser_status_expression_expected                        = 3007,
    parser_status_open_brace_expected                        = 3008,
    parser_status_closed_brace_or_statement_expected         = 3009,
    parser_status_open_parenthesis_expected                  = 3010,
    parser_status_parameter_or_ellipsis_expected             = 3011,
    parser_status_closed_parenthesis_expected                = 3012,
    parser_status_colon_expected                             = 3013,
    parser_status_closed_brace_or_switch_clause_expected     = 3014,
    parser_status_keyword_while_expected                     = 3015,
    parser_status_keyword_catch_expected                     = 3016,
    parser_status_comma_expected                             = 3017,
    parser_status_for_statement_initializer_expected         = 3018,
    parser_status_semicolon_or_expression_expected           = 3019,
    parser_status_closed_brace_expected                      = 3020,
    parser_status_too_many_elements                          = 3021,
    parser_status_closed_bracket_expected                    = 3022,
    parser_status_open_brace_or_equal_initializer_expected   = 3023,
    parser_status_equals_sign_or_colon_expected              = 3024,
    parser_status_closed_bracket_or_comma_expected           = 3025,
    parser_status_closed_brace_or_comma_expected             = 3026,
    parser_status_closed_bracket_or_expression_expected      = 3027,
    parser_status_closed_brace_or_json5_key_expected         = 3028,
  };

ROCKET_PURE_FUNCTION extern const char* describe_parser_status(Parser_Status status) noexcept;

// API versioning of the standard library
enum API_Version : uint32_t
  {
    api_version_none       = 0x00000000,  // no standard library
    api_version_0001_0000  = 0x00010000,  // version 1.0
    api_version_sentinel /* auto inc */,  // [subtract one to get the maximum version number]
    api_version_latest     = 0xFFFFFFFF,  // everything
  };

// Options for source parsing and code generation
template<uint32_t... paramsT> struct Compiler_Options_template;
template<uint32_t fragmentT> struct Compiler_Options_fragment;

template<uint32_t versionT>
    struct Compiler_Options_template<versionT> : Compiler_Options_template<versionT, versionT>
  {
  };
template<uint32_t versionT, uint32_t fragmentT>
    struct Compiler_Options_template<versionT, fragmentT> : Compiler_Options_template<versionT, fragmentT - 1>,
                                                            Compiler_Options_fragment<fragmentT>
  {
    // All members from `Compiler_Options_fragment<fragmentT>` have been brought in.
    // This struct does not define anything by itself.
  };
template<uint32_t versionT> struct Compiler_Options_template<versionT, 0>
  {
    // This member exists in all derived structs.
    // We would like these structs to be trivial, hence virtual bases are not an option.
    uint8_t version = versionT;
  };

template<> struct Compiler_Options_fragment<1>
  {
    // Make single quotes behave similiar to double quotes. [useful when parsing JSON5 text]
    bool escapable_single_quotes : 1;
    // Parse keywords as identifiers. [useful when parsing JSON text]
    bool keywords_as_identifiers : 1;
    // Parse integer literals as real literals. [useful when parsing JSON text]
    bool integers_as_reals : 1;
    // Do not implement proper tail calls. [more commonly known as tail call optimization]
    bool no_proper_tail_calls : 1;
    // Suppress all optimization techniques.
    bool no_optimization : 1;
    // Do not generate code for single-step traps other than on function calls.
    bool no_plain_single_step_traps : 1;

    // Note: Please keep this struct as compact as possible.
  };

template<> struct Compiler_Options_fragment<2>
  {
    // Note: Please keep this struct as compact as possible.
  };

// These are aliases for historical versions.
using Compiler_Options_v1 = Compiler_Options_template<1>;
using Compiler_Options_v2 = Compiler_Options_template<2>;

// This is always an alias for the latest version.
using Compiler_Options = Compiler_Options_v2;

// This value is initialized statically and never destroyed.
extern const unsigned char null_value_storage[];
static const Value& null_value = reinterpret_cast<const Value&>(null_value_storage);

// Aliases for bindings (shorter but less readable)
using Global = Global_Context;
using Null = V_null;

using Bval = V_boolean;
using Ival = V_integer;
using Rval = V_real;
using Sval = V_string;
using Pval = V_opaque;
using Fval = V_function;
using Aval = V_array;
using Oval = V_object;

using Bopt = opt<V_boolean>;
using Iopt = opt<V_integer>;
using Ropt = opt<V_real>;
using Sopt = opt<V_string>;
using Popt = V_opaque;
using Fopt = V_function;
using Aopt = opt<V_array>;
using Oopt = opt<V_object>;

}  // namespace Asteria

#endif
