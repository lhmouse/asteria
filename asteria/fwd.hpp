// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_
#define ASTERIA_FWD_

#include "version.h"
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
#include "../rocket/xascii.hpp"
#include "../rocket/bit_mask.hpp"
#include "../rocket/atomic.hpp"
#include <utility>
#include <stdexcept>
#include <typeinfo>
#include <cstddef>
#include <cstdint>
#include <limits.h>
#include <wchar.h>
#ifdef __SSE2__
#include <x86intrin.h>
#include <immintrin.h>
#endif  // __SSE2__
namespace asteria {
namespace noadl = asteria;

#define ASTERIA_COPYABLE_DESTRUCTOR(C)  \
    C(const C&) = default;  \
    C(C&&) noexcept = default;  \
    C& operator=(const C&) = default;  \
    C& operator=(C&&) & noexcept = default;  \
    ~C()  // no semicolon

#define ASTERIA_MOVABLE_DESTRUCTOR(C)  \
    C(const C&) = delete;  \
    C(C&&) noexcept = default;  \
    C& operator=(const C&) = delete;  \
    C& operator=(C&&) & noexcept = default;  \
    ~C()  // no semicolon

#define ASTERIA_NONCOPYABLE_DESTRUCTOR(C)  \
    C(const C&) = delete;  \
    C(C&&) noexcept = delete;  \
    C& operator=(const C&) = delete;  \
    C& operator=(C&&) & noexcept = delete;  \
    ~C()  // no semicolon

#define ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(C)  \
    C(const C&) = delete;  \
    C(C&&) noexcept = delete;  \
    C& operator=(const C&) = delete;  \
    C& operator=(C&&) & noexcept = delete;  \
    virtual ~C()  // no semicolon

#define ASTERIA_VARIANT(m, ...)  \
    ::rocket::variant<__VA_ARGS__> m  // no semicolon

#define ASTERIA_INCOMPLET(T)  \
    template<typename T##_IKYvW2aJ = T, typename T = T##_IKYvW2aJ>

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
using ::std::pair;

using ::rocket::nullopt_t;
using ::rocket::cow_string;
using ::rocket::cow_u16string;
using ::rocket::cow_u32string;
using ::rocket::tinybuf;
using ::rocket::tinyfmt;

using ::rocket::begin;
using ::rocket::end;
using ::rocket::xswap;
using ::rocket::swap;
using ::rocket::size;
using ::rocket::ssize;
using ::rocket::static_pointer_cast;
using ::rocket::dynamic_pointer_cast;
using ::rocket::const_pointer_cast;
using ::rocket::sref;
using ::rocket::nullopt;
using ::rocket::is;
using ::rocket::isnt;
using ::rocket::are;
using ::rocket::arent;

using ::rocket::unique_ptr;
using ::rocket::refcnt_ptr;
using ::rocket::cow_vector;
using ::rocket::cow_hashmap;
using ::rocket::static_vector;
using ::rocket::array;

using ::rocket::atomic;
using ::rocket::memory_order;
using ::rocket::atomic_relaxed;
using ::rocket::atomic_consume;
using ::rocket::atomic_acquire;
using ::rocket::atomic_release;
using ::rocket::atomic_acq_rel;
using ::rocket::atomic_seq_cst;

using stringR = const cow_string&;
using phsh_string = ::rocket::prehashed_string;
using phsh_stringR = const phsh_string&;

template<typename T, typename U>
using cow_bivector = cow_vector<pair<T, U>>;

template<typename E>
using cow_dictionary = cow_hashmap<phsh_string, E, phsh_string::hash>;

template<typename T>
using opt = ::rocket::optional<T>;

// Core
class Value;
class Source_Location;
class Recursion_Sentry;
class Simple_Script;

// Low-level data structures
class Variable_HashMap;
class Reference_Dictionary;
class Reference_Stack;
class AVM_Rod;

// Runtime
enum Xop : uint8_t;
enum AIR_Status : uint8_t;
enum AIR_Constant : uint8_t;
enum PTC_Aware : uint8_t;
struct Abstract_Hooks;
class Runtime_Error;
class Reference;
class Reference_Modifier;
class Variable;
class PTC_Arguments;
class Collector;
class Abstract_Context;
class Analytic_Context;
class Executive_Context;
class Global_Context;
class Garbage_Collector;
class Random_Engine;
class Module_Loader;
class Variadic_Arguer;
class Instantiated_Function;
class AIR_Node;
class Argument_Reader;
class Binding_Generator;

// Compiler
enum Punctuator : uint8_t;
enum Keyword : uint8_t;
enum Jump_Target : uint8_t;
enum Precedence : uint8_t;
class Compiler_Error;
class Token;
class Token_Stream;
class Expression_Unit;
class Statement;
class Infix_Element;
class Statement_Sequence;
class AIR_Optimizer;

// This typedef is for native bindings.
using simple_function =
    Reference& (Reference& self,           // `this` (in) / return (out)
                Global_Context& global,    // global scope
                Reference_Stack&& stack);  // positional arguments

// Type erasure
struct Rcbase : ::rocket::refcnt_base<Rcbase>
  {
    explicit
    Rcbase() noexcept = default;

    explicit
    Rcbase(const Rcbase&) noexcept = default;

    Rcbase&
    operator=(const Rcbase&) & noexcept = default;

    virtual
    ~Rcbase() = default;

    virtual
    void
    vtable_key_function_sLBHstEX() noexcept;
  };

template<typename RealT>
struct rcfwd : virtual Rcbase
  {
    explicit
    rcfwd() noexcept = default;

    explicit
    rcfwd(const rcfwd&) noexcept = default;

    rcfwd&
    operator=(const rcfwd&) & noexcept = default;

    virtual
    void
    vtable_key_function_GklPAslB() noexcept;

    template<typename XRealT = RealT>
    refcnt_ptr<const XRealT>
    share_this() const
      { return this->Rcbase::template share_this<XRealT, rcfwd>();  }

    template<typename XRealT = RealT>
    refcnt_ptr<XRealT>
    share_this()
      { return this->Rcbase::template share_this<XRealT, rcfwd>();  }
  };

template<typename RealT>
void
rcfwd<RealT>::
vtable_key_function_GklPAslB() noexcept
  {
  }

template<typename RealT>
using rcfwd_ptr = refcnt_ptr<typename ::rocket::copy_cv<
        rcfwd<typename ::std::remove_cv<RealT>::type>, RealT>::type>;

template<typename TargetT, typename RealT>
constexpr
TargetT
unerase_cast(const rcfwd<RealT>* ptr) noexcept  // like `static_cast`
  {
    return static_cast<TargetT>(ptr);
  }

template<typename TargetT, typename RealT>
constexpr
TargetT
unerase_cast(rcfwd<RealT>* ptr) noexcept  // like `static_cast`
  {
    return static_cast<TargetT>(ptr);
  }

template<typename TargetT, typename RealT>
constexpr
refcnt_ptr<TargetT>
unerase_pointer_cast(const refcnt_ptr<rcfwd<RealT>>& ptr) noexcept  // like `static_pointer_cast`
  {
    return static_pointer_cast<TargetT>(ptr);
  }

template<typename TargetT, typename RealT>
constexpr
refcnt_ptr<TargetT>
unerase_pointer_cast(const refcnt_ptr<const rcfwd<RealT>>& ptr) noexcept  // like `static_pointer_cast`
  {
    return static_pointer_cast<TargetT>(ptr);
  }

// Opaque (user-defined) type support
struct Abstract_Opaque
  :
    public rcfwd<Abstract_Opaque>
  {
    explicit
    Abstract_Opaque() noexcept = default;

    ASTERIA_COPYABLE_DESTRUCTOR(Abstract_Opaque);

    // This function is called to convert this object to a human-readble string.
    virtual
    tinyfmt&
    describe(tinyfmt& fmt) const = 0;

    // This function is called during garbage collection to mark variables that are
    // not directly reachable. Although strong exception safety is guaranteed, it is
    // discouraged to throw exceptions in this function, as it prevents garbage
    // collection from running.
    virtual
    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const = 0;

    // This function is called when a mutable reference is requested and the current
    // instance is shared. If this function returns a null pointer, the shared
    // instance is used. If this function returns a non-null pointer, it replaces
    // the current value. Derived classes that are not copyable should throw an
    // exception in this function.
    virtual
    Abstract_Opaque*
    clone_opt(refcnt_ptr<Abstract_Opaque>& out) const = 0;
  };

inline
tinyfmt&
operator<<(tinyfmt& fmt, const Abstract_Opaque& opaq)
  {
    return opaq.describe(fmt);
  }

struct Abstract_Function
  :
    public rcfwd<Abstract_Function>
  {
    explicit
    Abstract_Function() noexcept = default;

    ASTERIA_COPYABLE_DESTRUCTOR(Abstract_Function);

    // This function is called to convert this object to a human-readble string.
    virtual
    tinyfmt&
    describe(tinyfmt& fmt) const = 0;

    // This function is called during garbage collection to mark variables that are
    // not directly reachable. Although strong exception safety is guaranteed, it is
    // discouraged to throw exceptions in this function, as it prevents garbage
    // collection from running.
    virtual
    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const = 0;

    // This function may return a proper tail call wrapper.
    virtual
    Reference&
    invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const = 0;
  };

inline
tinyfmt&
operator<<(tinyfmt& fmt, const Abstract_Function& func)
  {
    return func.describe(fmt);
  }

class cow_opaque
  {
  private:
    refcnt_ptr<Abstract_Opaque> m_sptr;

  public:
    constexpr
    cow_opaque(nullptr_t = nullptr) noexcept
      { }

    template<typename OpaqT>
    cow_opaque(const refcnt_ptr<OpaqT>& sptr) noexcept
      :
        m_sptr(sptr)
      { }

    template<typename OpaqT>
    cow_opaque(refcnt_ptr<OpaqT>&& sptr) noexcept
      :
        m_sptr(::std::move(sptr))
      { }

    cow_opaque&
    operator=(nullptr_t) &
      {
        this->reset();
        return *this;
      }

    cow_opaque&
    swap(cow_opaque& other) noexcept
      {
        this->m_sptr.swap(other.m_sptr);
        return *this;
      }

  public:
    bool
    unique() const noexcept
      { return this->m_sptr.unique();  }

    long
    use_count() const noexcept
      { return this->m_sptr.use_count();  }

    cow_opaque&
    reset() noexcept
      {
        this->m_sptr = nullptr;
        return *this;
      }

    template<typename OpaqT>
    cow_opaque&
    reset(const refcnt_ptr<OpaqT>& sptr) noexcept
      {
        this->m_sptr = sptr;
        return *this;
      }

    template<typename OpaqT>
    cow_opaque&
    reset(refcnt_ptr<OpaqT>&& sptr) noexcept
      {
        this->m_sptr = ::std::move(sptr);
        return *this;
      }

    explicit operator
    bool() const noexcept
      { return static_cast<bool>(this->m_sptr);  }

    const type_info&
    type() const
      { return typeid(*(this->m_sptr.get()));  }  // may throw `std::bad_typeid`

    tinyfmt&
    describe(tinyfmt& fmt) const;

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
      {
        if(this->m_sptr)
          this->m_sptr->collect_variables(staged, temp);
      }

    template<typename OpaqueT = Abstract_Opaque>
    const OpaqueT&
    get() const;

    template<typename OpaqueT = Abstract_Opaque>
    OpaqueT&
    open();
  };

template<typename OpaqueT>
inline
const OpaqueT&
cow_opaque::
get() const
  {
    auto tptr = this->m_sptr.get();
    if(!tptr)
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
            "cow_opaque: invalid dynamic cast to `%s` from a null pointer",
            typeid(OpaqueT).name());

    auto toptr = dynamic_cast<const OpaqueT*>(tptr);
    if(!toptr)
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
            "cow_opaque: invalid dynamic cast to `%s` from `%s`",
            typeid(OpaqueT).name(), typeid(*tptr).name());

    return *toptr;
  }

template<typename OpaqueT>
inline
OpaqueT&
cow_opaque::
open()
  {
    auto tptr = this->m_sptr.get();
    if(!tptr)
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
            "cow_opaque: invalid dynamic cast to `%s` from a null pointer",
            typeid(OpaqueT).name());

    auto toptr = dynamic_cast<OpaqueT*>(tptr);
    if(!toptr)
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
            "cow_opaque: invalid dynamic cast to `%s` from `%s`",
            typeid(OpaqueT).name(), typeid(*tptr).name());

    // If the value is unique, return it.
    if(tptr->use_count() == 1)
      return *toptr;

    // Clone an existent instance if it is shared. A final overrider should return
    // a null pointer to request that the shared instance be used.
    refcnt_ptr<Abstract_Opaque> csptr;
    OpaqueT* coptr = toptr->clone_opt(csptr);
    if(!coptr)
      return *toptr;

    // Take ownership of the clone. The return type is covariant with the base.
    ROCKET_ASSERT(coptr == csptr.get());
    this->m_sptr = ::std::move(csptr);
    return *coptr;
  }

inline
void
swap(cow_opaque& lhs, cow_opaque& rhs) noexcept
  {
    lhs.swap(rhs);
  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const cow_opaque& opaq)
  {
    return opaq.describe(fmt);
  }

// Function type support
class cow_function
  {
  private:
    const char* m_desc = nullptr;
    simple_function* m_fptr = nullptr;
    refcnt_ptr<const Abstract_Function> m_sptr;

  public:
    constexpr
    cow_function(nullptr_t = nullptr) noexcept
      { }

    constexpr
    cow_function(const char* desc, simple_function* fptr) noexcept
      :
        m_desc(desc), m_fptr(fptr)
      { }

    template<typename FuncT>
    cow_function(const refcnt_ptr<FuncT>& sptr) noexcept
      :
        m_sptr(sptr)
      { }

    template<typename FuncT>
    cow_function(refcnt_ptr<FuncT>&& sptr) noexcept
      :
        m_sptr(::std::move(sptr))
      { }

    cow_function&
    operator=(nullptr_t) &
      {
        this->reset();
        return *this;
      }

    cow_function&
    swap(cow_function& other) noexcept
      {
        this->m_sptr.swap(other.m_sptr);
        return *this;
      }

  public:
    bool
    unique() const noexcept
      { return this->m_sptr.unique();  }

    long
    use_count() const noexcept
      { return this->m_sptr.use_count();  }

    cow_function&
    reset() noexcept
      {
        this->m_desc = nullptr;
        this->m_fptr = nullptr;
        this->m_sptr = nullptr;
        return *this;
      }

    cow_function&
    reset(const char* desc, simple_function* fptr) noexcept
      {
        this->m_desc = desc;
        this->m_fptr = fptr;
        this->m_sptr = nullptr;
        return *this;
      }

    template<typename FuncT>
    cow_function&
    reset(const refcnt_ptr<FuncT>& sptr) noexcept
      {
        this->m_desc = nullptr;
        this->m_fptr = nullptr;
        this->m_sptr = sptr;
        return *this;
      }

    template<typename FuncT>
    cow_function&
    reset(refcnt_ptr<FuncT>&& sptr) noexcept
      {
        this->m_desc = nullptr;
        this->m_fptr = nullptr;
        this->m_sptr = ::std::move(sptr);
        return *this;
      }

    explicit operator
    bool() const noexcept
      { return this->m_fptr || this->m_sptr;  }

    const type_info&
    type() const
      {
        return this->m_fptr ? typeid(simple_function)
                 : typeid(*(this->m_sptr.get()));  // may throw `std::bad_typeid`
      }

    tinyfmt&
    describe(tinyfmt& fmt) const;

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
      {
        if(this->m_sptr)
          this->m_sptr->collect_variables(staged, temp);
      }

    Reference&
    invoke_ptc_aware(Reference& self, Global_Context& global, Reference_Stack&& stack) const;

    Reference&
    invoke(Reference& self, Global_Context& global, Reference_Stack&& stack) const;
  };

inline
void
swap(cow_function& lhs, cow_function& rhs) noexcept
  {
    lhs.swap(rhs);
  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const cow_function& func)
  {
    return func.describe(fmt);
  }

// Value types
using V_null      = nullopt_t;
using V_boolean   = bool;
using V_integer   = int64_t;
using V_real      = double;
using V_string    = cow_string;
using V_opaque    = cow_opaque;
using V_function  = cow_function;
using V_array     = cow_vector<Value>;
using V_object    = cow_dictionary<Value>;

using optV_boolean   = opt<V_boolean>;
using optV_integer   = opt<V_integer>;
using optV_real      = opt<V_real>;
using optV_string    = opt<V_string>;
using optV_opaque    = V_opaque;
using optV_function  = V_function;
using optV_array     = opt<V_array>;
using optV_object    = opt<V_object>;

enum Type : uint8_t
  {
    type_null      = 0,
    type_boolean   = 1,
    type_integer   = 2,
    type_real      = 3,
    type_string    = 4,
    type_opaque    = 5,
    type_function  = 6,
    type_array     = 7,
    type_object    = 8,
  };

ROCKET_CONST
const char*
describe_type(Type type) noexcept;

// Reference types
enum Xref : uint8_t
  {
    xref_invalid    = 0,
    xref_void       = 1,
    xref_temporary  = 2,
    xref_variable   = 3,
    xref_ptc        = 4,
  };

ROCKET_CONST
const char*
describe_xref(Xref xref) noexcept;

// Stack frame types
enum Frame_Type : uint8_t
  {
    frame_type_native  = 0,  // The frame could not be determined [initiates a new chain].
    frame_type_throw   = 1,  // An exception was thrown here [initiates a new chain].
    frame_type_catch   = 2,  // An exception was caught here.
    frame_type_plain   = 3,  // An exception propagated across an IR node.
    frame_type_func    = 4,  // An exception propagated across a function boundary.
    frame_type_defer   = 5,  // A new exception was thrown here.
    frame_type_assert  = 6,  // An assertion failed here [initiates a new chain].
    frame_type_try     = 7,  // An exception propagated across a try block.
  };

ROCKET_CONST
const char*
describe_frame_type(Frame_Type type) noexcept;

// Compiler status codes
enum Compiler_Status : uint32_t
  {
    compiler_status_success                                    =    0,

    // lexical errors
    compiler_status_utf8_sequence_invalid                      = 1001,
    compiler_status_utf8_sequence_incomplete                   = 1002,
    compiler_status_utf_code_point_invalid                     = 1003,
    compiler_status_null_character_disallowed                  = 1004,
    compiler_status_conflict_marker_detected                   = 1005,
    compiler_status_token_character_unrecognized               = 1006,
    compiler_status_string_literal_unclosed                    = 1007,
    compiler_status_escape_sequence_unknown                    = 1008,
    compiler_status_escape_sequence_incomplete                 = 1009,
    compiler_status_escape_sequence_invalid_hex                = 1010,
    compiler_status_escape_utf_code_point_invalid              = 1011,
    compiler_status_reserved_1012                              = 1012,
    compiler_status_integer_literal_overflow                   = 1013,
    compiler_status_integer_literal_inexact                    = 1014,
    compiler_status_real_literal_overflow                      = 1015,
    compiler_status_real_literal_underflow                     = 1016,
    compiler_status_numeric_literal_suffix_invalid             = 1017,
    compiler_status_block_comment_unclosed                     = 1018,
    compiler_status_digit_separator_following_nondigit         = 1019,

    // syntax errors
    compiler_status_duplicate_key_in_object                    = 2001,
    compiler_status_identifier_expected                        = 2002,
    compiler_status_semicolon_expected                         = 2003,
    compiler_status_string_literal_expected                    = 2004,
    compiler_status_statement_expected                         = 2005,
    compiler_status_equals_sign_expected                       = 2006,
    compiler_status_expression_expected                        = 2007,
    compiler_status_open_brace_expected                        = 2008,
    compiler_status_closing_brace_or_statement_expected        = 2009,
    compiler_status_open_parenthesis_expected                  = 2010,
    compiler_status_closing_parenthesis_or_comma_expected      = 2011,
    compiler_status_closing_parenthesis_expected               = 2012,
    compiler_status_colon_expected                             = 2013,
    compiler_status_closing_brace_or_switch_clause_expected    = 2014,
    compiler_status_keyword_while_expected                     = 2015,
    compiler_status_keyword_catch_expected                     = 2016,
    compiler_status_comma_expected                             = 2017,
    compiler_status_for_statement_initializer_expected         = 2018,
    compiler_status_semicolon_or_expression_expected           = 2019,
    compiler_status_closing_brace_expected                     = 2020,
    compiler_status_too_many_elements                          = 2021,
    compiler_status_closing_bracket_expected                   = 2022,
    compiler_status_open_brace_or_initializer_expected         = 2023,
    compiler_status_equals_sign_or_colon_expected              = 2024,
    compiler_status_closing_bracket_or_comma_expected          = 2025,
    compiler_status_closing_brace_or_comma_expected            = 2026,
    compiler_status_closing_bracket_or_expression_expected     = 2027,
    compiler_status_closing_brace_or_json5_key_expected        = 2028,
    compiler_status_argument_expected                          = 2029,
    compiler_status_closing_parenthesis_or_argument_expected   = 2030,
    compiler_status_arrow_expected                             = 2031,
    compiler_status_reserved_identifier_not_declarable         = 2032,
    compiler_status_break_no_matching_scope                    = 2033,
    compiler_status_continue_no_matching_scope                 = 2034,
    compiler_status_closing_bracket_or_identifier_expected     = 2035,
    compiler_status_closing_brace_or_identifier_expected       = 2036,
    compiler_status_invalid_expression                         = 2037,
    compiler_status_multiple_default                           = 2038,
    compiler_status_duplicate_name_in_structured_binding       = 2039,
    compiler_status_duplicate_name_in_parameter_list           = 2040,
    compiler_status_nondeclaration_statement_expected          = 2041,

    // semantic errors
    compiler_status_undeclared_identifier                      = 3001,
  };

ROCKET_CONST
const char*
describe_compiler_status(Compiler_Status status) noexcept;

// Value comparison results
enum Compare : uint8_t
  {
    compare_unordered  = 0,
    compare_less       = 1,
    compare_equal      = 2,
    compare_greater    = 3,
  };

// API versioning of the standard library
enum API_Version : uint32_t
  {
    api_version_none       = 0x00000000,  // no standard library
    api_version_0001_0000  = 0x00010000,  // version 1.0
    api_version_sentinel   /* auto */,    // (max version + 1)
    api_version_latest     = 0xFFFFFFFF,  // everything
  };

// Garbage collection generations
enum GC_Generation : uint8_t
  {
    gc_generation_newest  = 0,
    gc_generation_middle  = 1,
    gc_generation_oldest  = 2,
  };

// Options for source parsing and code generation
template<uint32_t... paramsT>
struct Compiler_Options_template;

template<uint32_t fragmentT>
struct Compiler_Options_fragment;

template<uint32_t versionT>
struct Compiler_Options_template<versionT>
  :
    Compiler_Options_template<versionT, versionT>
  {
  };

template<uint32_t versionT, uint32_t fragmentT>
struct Compiler_Options_template<versionT, fragmentT>
  :
    Compiler_Options_template<versionT, fragmentT - 1>, Compiler_Options_fragment<fragmentT>
  {
    // All members from `Compiler_Options_fragment<fragmentT>` have been brought in.
    // This struct does not define anything by itself.
  };

template<uint32_t versionT>
struct Compiler_Options_template<versionT, 0>
  {
    // This member exists in all derived structs. We would like these structs to be
    // trivial, hence virtual bases are not an option.
    uint8_t version = versionT;
  };

template<>
struct Compiler_Options_fragment<1>
  {
    // Make single quotes behave similiar to double quotes.
    // [useful when parsing JSON5 text]
    bool escapable_single_quotes = false;

    // Parse keywords as identifiers.
    // `NaN` and `Infinity`, as well as their lowercase forms, are not keywords.
    // [useful when parsing JSON text]
    bool keywords_as_identifiers = false;

    // Parse integer literals as real literals.
    // [useful when parsing JSON text]
    bool integers_as_reals = false;

    // Enable proper tail calls.
    // This is semantical behavior and is not subject to optimization.
    // [more commonly known as tail call optimization]
    bool proper_tail_calls = true;

    // Generate calls to single-step hooks for every expression, not just function
    // calls.
    bool verbose_single_step_traps = false;

    // Treat unresolved names as global references.
    bool implicit_global_names = false;

    // Enable optimization.
    uint8_t optimization_level = 2;
  };

template<>
struct Compiler_Options_fragment<2>
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

}  // namespace asteria
#endif
