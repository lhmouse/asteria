// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "xprunit.hpp"
#include "statement.hpp"
#include "../runtime/evaluation_stack.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/executive_context.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../runtime/exception.hpp"
#include "../llds/air_queue.hpp"
#include "../utilities.hpp"

namespace Asteria {

const char* Xprunit::describe_operator(Xprunit::Xop xop) noexcept
  {
    switch(xop) {
    case xop_postfix_inc:
      {
        return "postfix increment";
      }
    case xop_postfix_dec:
      {
        return "postfix decrement";
      }
    case xop_postfix_subscr:
      {
        return "postfix subscript";
      }
    case xop_prefix_pos:
      {
        return "unary promotion";
      }
    case xop_prefix_neg:
      {
        return "unary negation";
      }
    case xop_prefix_notb:
      {
        return "bitwise not";
      }
    case xop_prefix_notl:
      {
        return "logical not";
      }
    case xop_prefix_inc:
      {
        return "prefix increment";
      }
    case xop_prefix_dec:
      {
        return "prefix decrement";
      }
    case xop_prefix_unset:
      {
        return "prefix `unset`";
      }
    case xop_prefix_lengthof:
      {
        return "prefix `lengthof`";
      }
    case xop_prefix_typeof:
      {
        return "prefix `typeof`";
      }
    case xop_prefix_sqrt:
      {
        return "prefix `__sqrt`";
      }
    case xop_prefix_isnan:
      {
        return "prefix `__isnan`";
      }
    case xop_prefix_isinf:
      {
        return "prefix `__isinf`";
      }
    case xop_prefix_abs:
      {
        return "prefix `__abs`";
      }
    case xop_prefix_signb:
      {
        return "prefix `__signb`";
      }
    case xop_prefix_round:
      {
        return "prefix `__round`";
      }
    case xop_prefix_floor:
      {
        return "prefix `__floor`";
      }
    case xop_prefix_ceil:
      {
        return "prefix `__ceil`";
      }
    case xop_prefix_trunc:
      {
        return "prefix `__trunc`";
      }
    case xop_prefix_iround:
      {
        return "prefix `__iround`";
      }
    case xop_prefix_ifloor:
      {
        return "prefix `__ifloor`";
      }
    case xop_prefix_iceil:
      {
        return "prefix `__iceil`";
      }
    case xop_prefix_itrunc:
      {
        return "prefix `__itrunc`";
      }
    case xop_infix_cmp_eq:
      {
        return "equality comparison";
      }
    case xop_infix_cmp_ne:
      {
        return "inequality comparison";
      }
    case xop_infix_cmp_lt:
      {
        return "less-than comparison";
      }
    case xop_infix_cmp_gt:
      {
        return "greater-than comparison";
      }
    case xop_infix_cmp_lte:
      {
        return "less-than-or-equal comparison";
      }
    case xop_infix_cmp_gte:
      {
        return "greater-than-or-equal comparison";
      }
    case xop_infix_cmp_3way:
      {
        return "three-way comparison";
      }
    case xop_infix_add:
      {
        return "addition";
      }
    case xop_infix_sub:
      {
        return "subtraction";
      }
    case xop_infix_mul:
      {
        return "multiplication";
      }
    case xop_infix_div:
      {
        return "division";
      }
    case xop_infix_mod:
      {
        return "modulo";
      }
    case xop_infix_sll:
      {
        return "logical left shift";
      }
    case xop_infix_srl:
      {
        return "arithmetic left shift";
      }
    case xop_infix_sla:
      {
        return "logical right shift";
      }
    case xop_infix_sra:
      {
        return "arithmetic right shift";
      }
    case xop_infix_andb:
      {
        return "bitwise and";
      }
    case xop_infix_orb:
      {
        return "bitwise or";
      }
    case xop_infix_xorb:
      {
        return "bitwise xor";
      }
    case xop_infix_assign:
      {
        return "assginment";
      }
    default:
      return "<unknown operator>";
    }
  }

    namespace {

    ROCKET_PURE_FUNCTION G_boolean do_operator_not(const G_boolean& rhs)
      {
        return !rhs;
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_and(const G_boolean& lhs, const G_boolean& rhs)
      {
        return lhs & rhs;
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_or(const G_boolean& lhs, const G_boolean& rhs)
      {
        return lhs | rhs;
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_xor(const G_boolean& lhs, const G_boolean& rhs)
      {
        return lhs ^ rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_neg(const G_integer& rhs)
      {
        if(rhs == INT64_MIN) {
          ASTERIA_THROW_RUNTIME_ERROR("The opposite of `", rhs, "` cannot be represented as an `integer`.");
        }
        return -rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_sqrt(const G_integer& rhs)
      {
        return std::sqrt(G_real(rhs));
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isinf(const G_integer& /*rhs*/)
      {
        return false;
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isnan(const G_integer& /*rhs*/)
      {
        return false;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_abs(const G_integer& rhs)
      {
        if(rhs == INT64_MIN) {
          ASTERIA_THROW_RUNTIME_ERROR("The absolute value of `", rhs, "` cannot be represented as an `integer`.");
        }
        return std::abs(rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_signb(const G_integer& rhs)
      {
        return rhs >> 63;
      }

    [[noreturn]] void do_throw_integral_overflow(const char* op, const G_integer& lhs, const G_integer& rhs)
      {
        ASTERIA_THROW_RUNTIME_ERROR("Integral ", op, " of `", lhs, "` and `", rhs, "` would result in overflow.");
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_add(const G_integer& lhs, const G_integer& rhs)
      {
        if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs)) {
          do_throw_integral_overflow("addition", lhs, rhs);
        }
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sub(const G_integer& lhs, const G_integer& rhs)
      {
        if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs)) {
          do_throw_integral_overflow("subtraction", lhs, rhs);
        }
        return lhs - rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_mul(const G_integer& lhs, const G_integer& rhs)
      {
        if((lhs == 0) || (rhs == 0)) {
          return 0;
        }
        if((lhs == 1) || (rhs == 1)) {
          return (lhs ^ rhs) ^ 1;
        }
        if((lhs == INT64_MIN) || (rhs == INT64_MIN)) {
          do_throw_integral_overflow("multiplication", lhs, rhs);
        }
        if((lhs == -1) || (rhs == -1)) {
          return (lhs ^ rhs) + 1;
        }
        // absolute lhs and signed rhs
        auto m = lhs >> 63;
        auto alhs = (lhs ^ m) - m;
        auto srhs = (rhs ^ m) - m;
        // `alhs` may only be positive here.
        if((srhs >= 0) ? (alhs > INT64_MAX / srhs) : (alhs > INT64_MIN / srhs)) {
          do_throw_integral_overflow("multiplication", lhs, rhs);
        }
        return alhs * srhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_div(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs == 0) {
          ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
        }
        if((lhs == INT64_MIN) && (rhs == -1)) {
          do_throw_integral_overflow("division", lhs, rhs);
        }
        return lhs / rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_mod(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs == 0) {
          ASTERIA_THROW_RUNTIME_ERROR("The divisor for `", lhs, "` was zero.");
        }
        if((lhs == INT64_MIN) && (rhs == -1)) {
          do_throw_integral_overflow("division", lhs, rhs);
        }
        return lhs % rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sll(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return 0;
        }
        return G_integer(static_cast<std::uint64_t>(lhs) << rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_srl(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return 0;
        }
        return G_integer(static_cast<std::uint64_t>(lhs) >> rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sla(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(lhs == 0) {
          return 0;
        }
        if(rhs >= 64) {
          ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
        }
        auto bc = static_cast<int>(63 - rhs);
        auto mask_out = static_cast<std::uint64_t>(lhs) >> bc << bc;
        auto mask_sbt = static_cast<std::uint64_t>(lhs >> 63) << bc;
        if(mask_out != mask_sbt) {
          ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
        }
        return G_integer(static_cast<std::uint64_t>(lhs) << rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_sra(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return lhs >> 63;
        }
        return lhs >> rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_not(const G_integer& rhs)
      {
        return ~rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_and(const G_integer& lhs, const G_integer& rhs)
      {
        return lhs & rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_or(const G_integer& lhs, const G_integer& rhs)
      {
        return lhs | rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_xor(const G_integer& lhs, const G_integer& rhs)
      {
        return lhs ^ rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_neg(const G_real& rhs)
      {
        return -rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_sqrt(const G_real& rhs)
      {
        return std::sqrt(rhs);
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isinf(const G_real& rhs)
      {
        return std::isinf(rhs);
      }

    ROCKET_PURE_FUNCTION G_boolean do_operator_isnan(const G_real& rhs)
      {
        return std::isnan(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_abs(const G_real& rhs)
      {
        return std::fabs(rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_signb(const G_real& rhs)
      {
        return std::signbit(rhs) ? -1 : 0;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_round(const G_real& rhs)
      {
        return std::round(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_floor(const G_real& rhs)
      {
        return std::floor(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_ceil(const G_real& rhs)
      {
        return std::ceil(rhs);
      }

    ROCKET_PURE_FUNCTION G_real do_operator_trunc(const G_real& rhs)
      {
        return std::trunc(rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_icast(const G_real& value)
      {
        if(!std::islessequal(INT64_MIN, value) || !std::islessequal(value, INT64_MAX)) {
          ASTERIA_THROW_RUNTIME_ERROR("The `real` value `", value, "` cannot be represented as an `integer`.");
        }
        return G_integer(value);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_iround(const G_real& rhs)
      {
        return do_icast(std::round(rhs));
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_ifloor(const G_real& rhs)
      {
        return do_icast(std::floor(rhs));
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_iceil(const G_real& rhs)
      {
        return do_icast(std::ceil(rhs));
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_itrunc(const G_real& rhs)
      {
        return do_icast(std::trunc(rhs));
      }

    ROCKET_PURE_FUNCTION G_real do_operator_add(const G_real& lhs, const G_real& rhs)
      {
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_sub(const G_real& lhs, const G_real& rhs)
      {
        return lhs - rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_mul(const G_real& lhs, const G_real& rhs)
      {
        return lhs * rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_div(const G_real& lhs, const G_real& rhs)
      {
        return lhs / rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_operator_mod(const G_real& lhs, const G_real& rhs)
      {
        return std::fmod(lhs, rhs);
      }

    ROCKET_PURE_FUNCTION G_string do_operator_add(const G_string& lhs, const G_string& rhs)
      {
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_mul(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String duplication count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        auto nchars = lhs.size();
        if((nchars == 0) || (rhs == 0)) {
          return res;
        }
        if(nchars > res.max_size() / static_cast<std::uint64_t>(rhs)) {
          ASTERIA_THROW_RUNTIME_ERROR("Duplication of `", lhs, "` up to `", rhs, "` times would result in an overlong string that cannot be allocated.");
        }
        auto times = static_cast<std::size_t>(rhs);
        if(nchars == 1) {
          // Fast fill.
          res.assign(times, lhs.front());
          return res;
        }
        // Reserve space for the result string.
        auto ptr = res.assign(nchars * times, '*').mut_data();
        // Copy the source string once.
        std::memcpy(ptr, lhs.data(), nchars);
        // Append the result string to itself, doubling its length, until more than half of the result string has been populated.
        while(nchars <= res.size() / 2) {
          std::memcpy(ptr + nchars, ptr, nchars);
          nchars *= 2;
        }
        // Copy remaining characters, if any.
        if(nchars < res.size()) {
          std::memcpy(ptr + nchars, ptr, res.size() - nchars);
        }
        return res;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_mul(const G_integer& lhs, const G_string& rhs)
      {
        return do_operator_mul(rhs, lhs);
      }

    ROCKET_PURE_FUNCTION G_string do_operator_sll(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        // Reserve space for the result string.
        auto ptr = rocket::unfancy(res.insert(res.begin(), lhs.size(), ' '));
        if(static_cast<std::uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<std::size_t>(rhs);
        // Copy the substring in the right.
        std::memcpy(ptr, lhs.data() + count, lhs.size() - count);
        return res;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_srl(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        // Reserve space for the result string.
        auto ptr = rocket::unfancy(res.insert(res.begin(), lhs.size(), ' '));
        if(static_cast<std::uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<std::size_t>(rhs);
        // Copy the substring in the left.
        std::memcpy(ptr + count, lhs.data(), lhs.size() - count);
        return res;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_sla(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        if(static_cast<std::uint64_t>(rhs) >= res.max_size() - lhs.size()) {
          ASTERIA_THROW_RUNTIME_ERROR("Shifting `", lhs, "` to the left by `", rhs, "` bytes would result in an overlong string that cannot be allocated.");
        }
        auto count = static_cast<std::size_t>(rhs);
        // Append spaces in the right and return the result.
        res.assign(G_string::shallow_type(lhs));
        res.append(count, ' ');
        return res;
      }

    ROCKET_PURE_FUNCTION G_string do_operator_sra(const G_string& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("String shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        G_string res;
        if(static_cast<std::uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<std::size_t>(rhs);
        // Return the substring in the left.
        res.append(lhs.data(), lhs.size() - count);
        return res;
      }

    Air_Queue do_generate_code_branch(const Compiler_Options& options, Xprunit::TCO_Awareness tco, const Analytic_Context& ctx,
                                                       const Cow_Vector<Xprunit>& units)
      {
        Air_Queue code;
        // Only the last operatro may be TCO'd.
        rocket::for_each(units,  [&](const Xprunit& unit) { unit.generate_code(code, options,
                                                                               rocket::same(unit, units.back()) ? tco
                                                                                                                : Xprunit::tco_none, ctx);  });
        return code;
      }

    class Air_push_literal : public Air_Node
      {
      private:
        Value m_value;

      public:
        explicit Air_push_literal(const Value& value)
          : m_value(value)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Push the constant.
            Reference_Root::S_constant xref = { this->m_value };
            ctx.stack().push_reference(rocket::move(xref));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& callback) const override
          {
            this->m_value.enumerate_variables(callback);
          }
      };

    template<typename ContextT> class Context_iterator
      {
      public:
        using iterator_category  = std::forward_iterator_tag;
        using value_type         = const ContextT;
        using pointer            = value_type*;
        using reference          = value_type&;
        using difference_type    = std::ptrdiff_t;

      private:
        value_type* m_qctx;

      public:
        constexpr Context_iterator() noexcept
          : m_qctx(nullptr)
          {
          }
        explicit constexpr Context_iterator(value_type& ctx) noexcept
          : m_qctx(std::addressof(ctx))
          {
          }

      public:
        constexpr reference operator*() const noexcept
          {
            return *(this->m_qctx);
          }
        constexpr pointer operator->() const noexcept
          {
            return this->m_qctx;
          }

        constexpr bool operator==(const Context_iterator& other) const noexcept
          {
            return this->m_qctx == other.m_qctx;
          }
        constexpr bool operator!=(const Context_iterator& other) const noexcept
          {
            return this->m_qctx != other.m_qctx;
          }

        Context_iterator& operator++() noexcept
          {
            return std::exchange(this->m_qctx, this->m_qctx->get_parent_opt()), *this;
          }
        Context_iterator operator++(int) noexcept
          {
            return Context_iterator(std::exchange(this->m_qctx, this->m_qctx->get_parent_opt()));
          }
      };

    class Air_find_named_reference_global : public Air_Node
      {
      private:
        PreHashed_String m_name;

      public:
        Air_find_named_reference_global(const PreHashed_String& name)
          : m_name(name)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Search for the name in the global context.
            auto qref = ctx.global().get_named_reference_opt(this->m_name);
            if(!qref) {
              ASTERIA_THROW_RUNTIME_ERROR("The identifier `", this->m_name, "` has not been declared yet.");
            }
            ctx.stack().push_reference(*qref);
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_find_named_reference_local : public Air_Node
      {
      private:
        PreHashed_String m_name;
        std::ptrdiff_t m_depth;

      public:
        Air_find_named_reference_local(const PreHashed_String& name, std::ptrdiff_t depth)
          : m_name(name), m_depth(depth)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Locate the context.
            auto qctx = std::next(Context_iterator<Executive_Context>(ctx), this->m_depth);
            ROCKET_ASSERT(qctx != Context_iterator<Executive_Context>());
            // Search for the name in the target context. It has to exist, or we would be having a bug here.
            auto qref = qctx->get_named_reference_opt(this->m_name);
            if(!qref) {
              ASTERIA_THROW_RUNTIME_ERROR("The identifier `", this->m_name, "` has not been declared yet.");
            }
            ctx.stack().push_reference(*qref);
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_push_bound_reference : public Air_Node
      {
      private:
        Reference m_ref;

      public:
        explicit Air_push_bound_reference(const Reference& ref)
          : m_ref(ref)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Push the reference as is.
            ctx.stack().push_reference(this->m_ref);
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& callback) const override
          {
            this->m_ref.enumerate_variables(callback);
          }
      };

    class Air_execute_closure_function : public Air_Node
      {
      private:
        Compiler_Options m_options;
        Source_Location m_sloc;
        Cow_Vector<PreHashed_String> m_params;
        Cow_Vector<Statement> m_body;

      public:
        Air_execute_closure_function(const Compiler_Options& options,
                                     const Source_Location& sloc, const Cow_Vector<PreHashed_String>& params, const Cow_Vector<Statement>& body)
          : m_options(options),
            m_sloc(sloc), m_params(params), m_body(body)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Generate code of the function body.
            Air_Queue code_body;
            Analytic_Context ctx_func(1, ctx, this->m_params);
            rocket::for_each(this->m_body, [&](const Statement& stmt) { stmt.generate_code(code_body, nullptr, ctx_func, this->m_options);  });
            // Format the prototype string.
            Cow_osstream fmtss;
            fmtss.imbue(std::locale::classic());
            fmtss << "<closure> (";
            if(!this->m_params.empty()) {
              std::for_each(this->m_params.begin(), this->m_params.end() - 1, [&](const PreHashed_String& param) { fmtss << param << ", ";  });
              fmtss << this->m_params.back();
            }
            fmtss <<")";
            // Instantiate the function.
            Rcobj<Instantiated_Function> closure(this->m_sloc, fmtss.extract_string(), this->m_params, rocket::move(code_body));
            ASTERIA_DEBUG_LOG("New closure function: ", closure);
            // Push the function object.
            Reference_Root::S_temporary xref = { G_function(rocket::move(closure)) };
            ctx.stack().push_reference(rocket::move(xref));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    void do_execute_subexpression(Executive_Context& ctx, const Air_Queue& code, bool assign)
      {
        auto status = Air_Node::status_next;
        code.execute(status, ctx);
        ROCKET_ASSERT(status == Air_Node::status_next);
        ctx.stack().pop_next_reference(assign);
      }

    class Air_execute_branch : public Air_Node
      {
      private:
        Air_Queue m_code_true;
        Air_Queue m_code_false;
        bool m_assign;

      public:
        Air_execute_branch(Air_Queue&& code_true, Air_Queue&& code_false, bool assign)
          : m_code_true(rocket::move(code_true)), m_code_false(rocket::move(code_false)), m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Pick a branch basing on the condition.
            if(ctx.stack().get_top_reference().read().test()) {
              // Evaluate the true branch. If the branch is empty, leave the condition on the ctx.stack().
              do_execute_subexpression(ctx, this->m_code_true, this->m_assign);
            }
            else {
              // Evaluate the false branch. If the branch is empty, leave the condition on the ctx.stack().
              do_execute_subexpression(ctx, this->m_code_false, this->m_assign);
            }
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    template<typename XcallT, typename... XaddT> Air_Node::Status do_execute_function_common(Executive_Context& ctx,
                                                                                             const Source_Location& sloc, const Cow_Vector<bool>& by_refs,
                                                                                             XcallT&& xcall, XaddT&&... xadd)
      {
        Value value;
        // Allocate the argument vector.
        Cow_Vector<Reference> args;
        args.resize(by_refs.size());
        for(auto it = args.mut_rbegin(); it != args.rend(); ++it) {
          if(!ctx.stack().get_top_reference().is_rvalue() && !by_refs.rbegin()[it - args.rbegin()]) {
            // If this argument is not an rvalue and it is not passed by reference, convert it to an rvalue.
            ctx.stack().set_temporary_reference(false, rocket::move(value = ctx.stack().get_top_reference().read()));
          }
          *it = rocket::move(ctx.stack().open_top_reference());
          ctx.stack().pop_reference();
        }
        // Get the target reference.
        value = ctx.stack().get_top_reference().read();
        if(!value.is_function()) {
          ASTERIA_THROW_RUNTIME_ERROR("An attempt was made to invoke `", value, "` which is not a function.");
        }
        // Call the function now.
        rocket::forward<XcallT>(xcall)(sloc, ctx.zvarg()->get_function_signature(),  // sloc, func,
                                       value.as_function(), ctx.stack().open_top_reference().zoom_out(), rocket::move(args),  // target, self, args,
                                       rocket::forward<XaddT>(xadd)...);
        return Air_Node::status_next;
      }

    Air_Node::Status do_xcall_tail(const Source_Location& sloc, const Cow_String& func,
                                   const Rcobj<Abstract_Function>& target, Reference& self, Cow_Vector<Reference>&& args,
                                   bool tco_by_ref)
      {
        // Pack arguments.
        auto& args_self = args;
        args_self.emplace_back(rocket::move(self));
        // Create a TCO wrapper. The caller shall unwrap the proxy reference when appropriate.
        Reference_Root::S_tail_call xref = { sloc, func, tco_by_ref, target, rocket::move(args_self) };
        self = rocket::move(xref);
        return Air_Node::status_next;
      }

    class Air_execute_function_tail_call : public Air_Node
      {
      private:
        Source_Location m_sloc;
        Cow_Vector<bool> m_by_refs;
        bool m_tco_by_ref;

      public:
        Air_execute_function_tail_call(const Source_Location& sloc, const Cow_Vector<bool>& by_refs, bool tco_by_ref)
          : m_sloc(sloc), m_by_refs(by_refs), m_tco_by_ref(tco_by_ref)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            return do_execute_function_common(ctx, this->m_sloc, this->m_by_refs, do_xcall_tail, this->m_tco_by_ref);
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    Air_Node::Status do_xcall_plain(const Source_Location& sloc, const Cow_String& func,
                                    const Rcobj<Abstract_Function>& target, Reference& self, Cow_Vector<Reference>&& args,
                                    const Global_Context& global)
      try {
        ASTERIA_DEBUG_LOG("Initiating function call at \'", sloc, "\' inside `", func, "`: target = ", *target);
        // Call the function now.
        target->invoke(self, global, rocket::move(args));
        self.finish_call(global);
        // The result will have been stored into `self`.
        ASTERIA_DEBUG_LOG("Returned from function call at \'", sloc, "\' inside `", func, "`: target = ", *target);
        return Air_Node::status_next;
      }
      catch(Exception& except) {
        ASTERIA_DEBUG_LOG("Caught `Asteria::Exception` thrown inside function call at \'", sloc, "\' inside `", func, "`: ", except.get_value());
        // Append the current frame and rethrow the exception.
        except.push_frame_func(sloc, func);
        throw;
      }
      catch(const std::exception& stdex) {
        ASTERIA_DEBUG_LOG("Caught `std::exception` thrown inside function call at \'", sloc, "\' inside `", func, "`: ", stdex.what());
        // Translate the exception, append the current frame, and throw the new exception.
        Exception except(stdex);
        except.push_frame_func(sloc, func);
        throw except;
      }

    class Air_execute_function_call : public Air_Node
      {
      private:
        Source_Location m_sloc;
        Cow_Vector<bool> m_by_refs;

      public:
        Air_execute_function_call(const Source_Location& sloc, const Cow_Vector<bool>& by_refs)
          : m_sloc(sloc), m_by_refs(by_refs)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            return do_execute_function_common(ctx, this->m_sloc, this->m_by_refs, do_xcall_plain, ctx.global());
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_member_access : public Air_Node
      {
      private:
        PreHashed_String m_name;

      public:
        explicit Air_execute_member_access(const PreHashed_String& name)
          : m_name(name)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Append a modifier.
            Reference_Modifier::S_object_key xmod = { this->m_name };
            ctx.stack().open_top_reference().zoom_in(rocket::move(xmod));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_postfix_inc : public Air_Node
      {
      private:
        //

      public:
        Air_execute_operator_rpn_postfix_inc()
          // :
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto& lhs = ctx.stack().get_top_reference().open();
            // Increment the operand and return the old value. `altr.assign` is ignored.
            if(lhs.is_integer()) {
              auto& reg = lhs.mut_integer();
              ctx.stack().set_temporary_reference(false, rocket::move(lhs));
              reg = do_operator_add(reg, G_integer(1));
            }
            else if(lhs.is_real()) {
              auto& reg = lhs.mut_real();
              ctx.stack().set_temporary_reference(false, rocket::move(lhs));
              reg = do_operator_add(reg, G_real(1));
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Postfix increment is not defined for `", lhs, "`.");
            }
            // The operand has been modified in place. No further action needs to be taken.
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_postfix_dec : public Air_Node
      {
      private:
        //

      public:
        Air_execute_operator_rpn_postfix_dec()
          // :
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto& lhs = ctx.stack().get_top_reference().open();
            // Decrement the operand and return the old value. `altr.assign` is ignored.
            if(lhs.is_integer()) {
              auto& reg = lhs.mut_integer();
              ctx.stack().set_temporary_reference(false, rocket::move(lhs));
              reg = do_operator_sub(reg, G_integer(1));
            }
            else if(lhs.is_real()) {
              auto& reg = lhs.mut_real();
              ctx.stack().set_temporary_reference(false, rocket::move(lhs));
              reg = do_operator_sub(reg, G_real(1));
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Postfix decrement is not defined for `", lhs, "`.");
            }
            // The operand has been modified in place. No further action needs to be taken.
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_postfix_subscr : public Air_Node
      {
      private:
        //

      public:
        Air_execute_operator_rpn_postfix_subscr()
          // :
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            auto& lref = ctx.stack().open_top_reference();
            // Append a reference modifier. `altr.assign` is ignored.
            if(rhs.is_integer()) {
              auto& reg = rhs.mut_integer();
              Reference_Modifier::S_array_index xmod = { rocket::move(reg) };
              lref.zoom_in(rocket::move(xmod));
            }
            else if(rhs.is_string()) {
              auto& reg = rhs.mut_string();
              Reference_Modifier::S_object_key xmod = { rocket::move(reg) };
              lref.zoom_in(rocket::move(xmod));
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The value `", rhs, "` cannot be used as a subscript.");
            }
            // The operand has been modified in place. No further action needs to be taken.
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_pos : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_pos(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Copy the operand to create a temporary value, then return it.
            // N.B. This is one of the few operators that work on all types.
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_neg : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_neg(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Get the opposite of the operand as a temporary value, then return it.
            if(rhs.is_integer()) {
              auto& reg = rhs.mut_integer();
              reg = do_operator_neg(reg);
            }
            else if(rhs.is_real()) {
              auto& reg = rhs.mut_real();
              reg = do_operator_neg(reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix negation is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_notb : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_notb(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Perform bitwise NOT operation on the operand to create a temporary value, then return it.
            if(rhs.is_boolean()) {
              auto& reg = rhs.mut_boolean();
              reg = do_operator_not(reg);
            }
            else if(rhs.is_integer()) {
              auto& reg = rhs.mut_integer();
              reg = do_operator_not(reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix bitwise NOT is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_notl : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_notl(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            const auto& rhs = ctx.stack().get_top_reference().read();
            // Perform logical NOT operation on the operand to create a temporary value, then return it.
            // N.B. This is one of the few operators that work on all types.
            ctx.stack().set_temporary_reference(this->m_assign, do_operator_not(rhs.test()));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_inc : public Air_Node
      {
      private:
        //

      public:
        Air_execute_operator_rpn_prefix_inc()
          // :
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto& rhs = ctx.stack().get_top_reference().open();
            // Increment the operand and return it. `altr.assign` is ignored.
            if(rhs.is_integer()) {
              auto& reg = rhs.mut_integer();
              reg = do_operator_add(reg, G_integer(1));
            }
            else if(rhs.is_real()) {
              auto& reg = rhs.mut_real();
              reg = do_operator_add(reg, G_real(1));
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix increment is not defined for `", rhs, "`.");
            }
            // The operand has been modified in place. No further action needs to be taken.
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_dec : public Air_Node
      {
      private:
        //

      public:
        Air_execute_operator_rpn_prefix_dec()
          // :
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto& rhs = ctx.stack().get_top_reference().open();
            // Decrement the operand and return it. `altr.assign` is ignored.
            if(rhs.is_integer()) {
              auto& reg = rhs.mut_integer();
              reg = do_operator_sub(reg, G_integer(1));
            }
            else if(rhs.is_real()) {
              auto& reg = rhs.mut_real();
              reg = do_operator_sub(reg, G_real(1));
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix increment is not defined for `", rhs, "`.");
            }
            // The operand has been modified in place. No further action needs to be taken.
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_unset : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_unset(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().unset();
            // Unset the reference and return the old value.
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_lengthof : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_lengthof(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            const auto& rhs = ctx.stack().get_top_reference().read();
            // Return the number of elements in the operand.
            std::size_t nelems;
            if(rhs.is_null()) {
              nelems = 0;
            }
            else if(rhs.is_string()) {
              nelems = rhs.as_string().size();
            }
            else if(rhs.is_array()) {
              nelems = rhs.as_array().size();
            }
            else if(rhs.is_object()) {
              nelems = rhs.as_object().size();
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `lengthof` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, G_integer(nelems));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_typeof : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_typeof(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            const auto& rhs = ctx.stack().get_top_reference().read();
            // Return the type name of the operand.
            // N.B. This is one of the few operators that work on all types.
            ctx.stack().set_temporary_reference(this->m_assign, G_string(rocket::sref(rhs.gtype_name())));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_sqrt : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_sqrt(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Get the square root of the operand as a temporary value, then return it.
            if(rhs.is_integer()) {
              // Note that `rhs` does not have type `G_real`, thus this branch can't be optimized.
              rhs = do_operator_sqrt(rhs.as_integer());
            }
            else if(rhs.is_real()) {
              auto& reg = rhs.mut_real();
              reg = do_operator_sqrt(reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__sqrt` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_isnan : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_isnan(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Check whether the operand is a NaN, store the result in a temporary value, then return it.
            if(rhs.is_integer()) {
              // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
              rhs = do_operator_isnan(rhs.as_integer());
            }
            else if(rhs.is_real()) {
              // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
              rhs = do_operator_isnan(rhs.as_real());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__isnan` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_isinf : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_isinf(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Check whether the operand is an infinity, store the result in a temporary value, then return it.
            if(rhs.is_integer()) {
              // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
              rhs = do_operator_isinf(rhs.as_integer());
            }
            else if(rhs.is_real()) {
              // Note that `rhs` does not have type `G_boolean`, thus this branch can't be optimized.
              rhs = do_operator_isinf(rhs.as_real());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__isinf` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_abs : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_abs(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Get the absolute value of the operand as a temporary value, then return it.
            if(rhs.is_integer()) {
              auto& reg = rhs.mut_integer();
              reg = do_operator_abs(reg);
            }
            else if(rhs.is_real()) {
              auto& reg = rhs.mut_real();
              reg = do_operator_abs(reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__abs` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_signb : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_signb(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Get the sign bit of the operand as a temporary value, then return it.
            if(rhs.is_integer()) {
              auto& reg = rhs.mut_integer();
              reg = do_operator_signb(reg);
            }
            else if(rhs.is_real()) {
              // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
              rhs = do_operator_signb(rhs.as_real());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__signb` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_round : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_round(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Round the operand to the nearest integer as a temporary value, then return it.
            if(rhs.is_integer()) {
              // No conversion is required.
              // Return `rhs` as is.
            }
            else if(rhs.is_real()) {
              auto& reg = rhs.mut_real();
              reg = do_operator_round(reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__round` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_floor : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_floor(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Round the operand towards negative infinity as a temporary value, then return it.
            if(rhs.is_integer()) {
              // No conversion is required.
              // Return `rhs` as is.
            }
            else if(rhs.is_real()) {
              auto& reg = rhs.mut_real();
              reg = do_operator_floor(reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__floor` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_ceil : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_ceil(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Round the operand towards negative infinity as a temporary value, then return it.
            if(rhs.is_integer()) {
              // No conversion is required.
              // Return `rhs` as is.
            }
            else if(rhs.is_real()) {
              auto& reg = rhs.mut_real();
              reg = do_operator_ceil(reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__ceil` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_trunc : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_trunc(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Round the operand towards negative infinity as a temporary value, then return it.
            if(rhs.is_integer()) {
              // No conversion is required.
              // Return `rhs` as is.
            }
            else if(rhs.is_real()) {
              auto& reg = rhs.mut_real();
              reg = do_operator_trunc(reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__trunc` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_iround : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_iround(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Round the operand to the nearest integer as a temporary value, then return it as an `integer`.
            if(rhs.is_integer()) {
              // No conversion is required.
              // Return `rhs` as is.
            }
            else if(rhs.is_real()) {
              // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
              rhs = do_operator_iround(rhs.as_real());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__iround` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_ifloor : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_ifloor(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Round the operand towards negative infinity as a temporary value, then return it as an `integer`.
            if(rhs.is_integer()) {
              // No conversion is required.
              // Return `rhs` as is.
            }
            else if(rhs.is_real()) {
              // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
              rhs = do_operator_ifloor(rhs.as_real());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__ifloor` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_iceil : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_iceil(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Round the operand towards negative infinity as a temporary value, then return it as an `integer`.
            if(rhs.is_integer()) {
              // No conversion is required.
              // Return `rhs` as is.
            }
            else if(rhs.is_real()) {
              // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
              rhs = do_operator_iceil(rhs.as_real());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__iceil` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_prefix_itrunc : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_prefix_itrunc(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            auto rhs = ctx.stack().get_top_reference().read();
            // Round the operand towards negative infinity as a temporary value, then return it as an `integer`.
            if(rhs.is_integer()) {
              // No conversion is required.
              // Return `rhs` as is.
            }
            else if(rhs.is_real()) {
              // Note that `rhs` does not have type `G_integer`, thus this branch can't be optimized.
              rhs = do_operator_itrunc(rhs.as_real());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Prefix `__itrunc` is not defined for `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_cmp_xeq : public Air_Node
      {
      private:
        bool m_assign;
        bool m_negative;

      public:
        Air_execute_operator_rpn_infix_cmp_xeq(bool assign, bool negative)
          : m_assign(assign), m_negative(negative)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            // Report unordered operands as being unequal.
            // N.B. This is one of the few operators that work on all types.
            auto comp = lhs.compare(rhs);
            rhs = G_boolean((comp == Value::compare_equal) != this->m_negative);
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_cmp_xrel : public Air_Node
      {
      private:
        bool m_assign;
        Value::Compare m_expect;
        bool m_negative;

      public:
        Air_execute_operator_rpn_infix_cmp_xrel(bool assign, Value::Compare expect, bool negative)
          : m_assign(assign), m_expect(expect), m_negative(negative)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            // Report unordered operands as being unequal.
            // N.B. This is one of the few operators that work on all types.
            auto comp = lhs.compare(rhs);
            if(comp == Value::compare_unordered) {
              ASTERIA_THROW_RUNTIME_ERROR("The operands `", lhs, "` and `", rhs, "` are unordered.");
            }
            rhs = G_boolean((comp == this->m_expect) != this->m_negative);
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_cmp_3way : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_cmp_3way(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            // Report unordered operands as being unequal.
            // N.B. This is one of the few operators that work on all types.
            auto comp = lhs.compare(rhs);
            if(comp == Value::compare_unordered) {
              rhs = G_string(rocket::sref("<unordered>"));
            }
            else if(comp == Value::compare_less) {
              rhs = G_integer(-1);
            }
            else if(comp == Value::compare_greater) {
              rhs = G_integer(+1);
            }
            else {
              rhs = G_integer(0);
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_add : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_add(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_boolean() && rhs.is_boolean()) {
              // For the `boolean` type, return the logical OR'd result of both operands.
              auto& reg = rhs.mut_boolean();
              reg = do_operator_or(lhs.as_boolean(), reg);
            }
            else if(lhs.is_integer() && rhs.is_integer()) {
              // For the `integer` and `real` types, return the sum of both operands.
              auto& reg = rhs.mut_integer();
              reg = do_operator_add(lhs.as_integer(), reg);
            }
            else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
              // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
              rhs = do_operator_add(lhs.convert_to_real(), rhs.convert_to_real());
            }
            else if(lhs.is_string() && rhs.is_string()) {
              // For the `string` type, concatenate the operands in lexical order to create a new string, then return it.
              auto& reg = rhs.mut_string();
              reg = do_operator_add(lhs.as_string(), reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Infix addition is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_sub : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_sub(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_boolean() && rhs.is_boolean()) {
              // For the `boolean` type, return the logical XOR'd result of both operands.
              auto& reg = rhs.mut_boolean();
              reg = do_operator_xor(lhs.as_boolean(), reg);
            }
            else if(lhs.is_integer() && rhs.is_integer()) {
              // For the `integer` and `real` types, return the difference of both operands.
              auto& reg = rhs.mut_integer();
              reg = do_operator_sub(lhs.as_integer(), reg);
            }
            else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
              // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
              rhs = do_operator_sub(lhs.convert_to_real(), rhs.convert_to_real());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Infix subtraction is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_mul : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_mul(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_boolean() && rhs.is_boolean()) {
              // For the `boolean` type, return the logical AND'd result of both operands.
              auto& reg = rhs.mut_boolean();
              reg = do_operator_and(lhs.as_boolean(), reg);
            }
            else if(lhs.is_integer() && rhs.is_integer()) {
              // For the `integer` and `real` types, return the product of both operands.
              auto& reg = rhs.mut_integer();
              reg = do_operator_mul(lhs.as_integer(), reg);
            }
            else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
              // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
              rhs = do_operator_mul(lhs.convert_to_real(), rhs.convert_to_real());
            }
            else if(lhs.is_string() && rhs.is_integer()) {
              // If either operand has type `string` and the other has type `integer`, duplicate the string up to the specified number of times and return the result.
              // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
              rhs = do_operator_mul(lhs.as_string(), rhs.as_integer());
            }
            else if(lhs.is_integer() && rhs.is_string()) {
              auto& reg = rhs.mut_string();
              reg = do_operator_mul(lhs.as_integer(), reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Infix multiplication is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_div : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_div(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_integer() && rhs.is_integer()) {
              // For the `integer` and `real` types, return the quotient of both operands.
              auto& reg = rhs.mut_integer();
              reg = do_operator_div(lhs.as_integer(), reg);
            }
            else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
              // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
              rhs = do_operator_div(lhs.convert_to_real(), rhs.convert_to_real());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Infix division is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_mod : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_mod(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_integer() && rhs.is_integer()) {
              // For the `integer` and `real` types, return the remainder of both operands.
              auto& reg = rhs.mut_integer();
              reg = do_operator_mod(lhs.as_integer(), reg);
            }
            else if(lhs.is_convertible_to_real() && rhs.is_convertible_to_real()) {
              // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
              rhs = do_operator_mod(lhs.convert_to_real(), rhs.convert_to_real());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Infix modulo operation is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_sll : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_sll(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_integer() && rhs.is_integer()) {
              // If the LHS operand has type `integer`, shift the LHS operand to the left by the number of bits specified by the RHS operand.
              // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
              auto& reg = rhs.mut_integer();
              reg = do_operator_sll(lhs.as_integer(), reg);
            }
            else if(lhs.is_string() && rhs.is_integer()) {
              // If the LHS operand has type `string`, fill space characters in the right and discard characters from the left.
              // The number of bytes in the LHS operand will be preserved.
              // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
              rhs = do_operator_sll(lhs.as_string(), rhs.as_integer());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Infix logical shift to the left is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_srl : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_srl(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_integer() && rhs.is_integer()) {
              // If the LHS operand has type `integer`, shift the LHS operand to the right by the number of bits specified by the RHS operand.
              // Bits shifted out are discarded. Bits shifted in are filled with zeroes.
              auto& reg = rhs.mut_integer();
              reg = do_operator_srl(lhs.as_integer(), reg);
            }
            else if(lhs.is_string() && rhs.is_integer()) {
              // If the LHS operand has type `string`, fill space characters in the left and discard characters from the right.
              // The number of bytes in the LHS operand will be preserved.
              // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
              rhs = do_operator_srl(lhs.as_string(), rhs.as_integer());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Infix logical shift to the right is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_sla : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_sla(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_integer() && rhs.is_integer()) {
              // If the LHS operand is of type `integer`, shift the LHS operand to the left by the number of bits specified by the RHS operand.
              // Bits shifted out that are equal to the sign bit are discarded. Bits shifted in are filled with zeroes.
              // If any bits that are different from the sign bit would be shifted out, an exception is thrown.
              auto& reg = rhs.mut_integer();
              reg = do_operator_sla(lhs.as_integer(), reg);
            }
            else if(lhs.is_string() && rhs.is_integer()) {
              // If the LHS operand has type `string`, fill space characters in the right.
              // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
              rhs = do_operator_sla(lhs.as_string(), rhs.as_integer());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Infix arithmetic shift to the left is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_sra : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_sra(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_integer() && rhs.is_integer()) {
              // If the LHS operand is of type `integer`, shift the LHS operand to the right by the number of bits specified by the RHS operand.
              // Bits shifted out are discarded. Bits shifted in are filled with the sign bit.
              auto& reg = rhs.mut_integer();
              reg = do_operator_sra(lhs.as_integer(), reg);
            }
            else if(lhs.is_string() && rhs.is_integer()) {
              // If the LHS operand has type `string`, discard characters from the right.
              // Note that `rhs` does not have type `G_string`, thus this branch can't be optimized.
              rhs = do_operator_sra(lhs.as_string(), rhs.as_integer());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Infix arithmetic shift to the right is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_andb : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_andb(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_boolean() && rhs.is_boolean()) {
              // For the `boolean` type, return the logical AND'd result of both operands.
              auto& reg = rhs.mut_boolean();
              reg = do_operator_and(lhs.as_boolean(), reg);
            }
            else if(lhs.is_integer() && rhs.is_integer()) {
              // For the `integer` type, return bitwise AND'd result of both operands.
              auto& reg = rhs.mut_integer();
              reg = do_operator_and(lhs.as_integer(), reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Infix bitwise AND is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_orb : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_orb(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_boolean() && rhs.is_boolean()) {
              // For the `boolean` type, return the logical OR'd result of both operands.
              auto& reg = rhs.mut_boolean();
              reg = do_operator_or(lhs.as_boolean(), reg);
            }
            else if(lhs.is_integer() && rhs.is_integer()) {
              // For the `integer` type, return bitwise OR'd result of both operands.
              auto& reg = rhs.mut_integer();
              reg = do_operator_or(lhs.as_integer(), reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Infix bitwise OR is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_xorb : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_rpn_infix_xorb(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is binary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_boolean() && rhs.is_boolean()) {
              // For the `boolean` type, return the logical XOR'd result of both operands.
              auto& reg = rhs.mut_boolean();
              reg = do_operator_xor(lhs.as_boolean(), reg);
            }
            else if(lhs.is_integer() && rhs.is_integer()) {
              // For the `integer` type, return bitwise XOR'd result of both operands.
              auto& reg = rhs.mut_integer();
              reg = do_operator_xor(lhs.as_integer(), reg);
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Infix bitwise XOR is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_rpn_infix_assign : public Air_Node
      {
      private:
        //

      public:
        Air_execute_operator_rpn_infix_assign()
          // :
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Pop the RHS operand followed.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            // Copy the value to the LHS operand which is write-only. `altr.assign` is ignored.
            ctx.stack().set_temporary_reference(true, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_unnamed_array : public Air_Node
      {
      private:
        std::size_t m_nelems;

      public:
        explicit Air_execute_unnamed_array(std::size_t nelems)
          : m_nelems(nelems)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Pop references to create an array.
            G_array array;
            array.resize(this->m_nelems);
            for(auto it = array.mut_rbegin(); it != array.rend(); ++it) {
              *it = ctx.stack().get_top_reference().read();
              ctx.stack().pop_reference();
            }
            // Push the array as a temporary.
            Reference_Root::S_temporary xref = { rocket::move(array) };
            ctx.stack().push_reference(rocket::move(xref));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_unnamed_object : public Air_Node
      {
      private:
        Cow_Vector<PreHashed_String> m_keys;

      public:
        explicit Air_execute_unnamed_object(const Cow_Vector<PreHashed_String>& keys)
          : m_keys(keys)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Pop references to create an object.
            G_object object;
            object.reserve(this->m_keys.size());
            for(auto it = this->m_keys.rbegin(); it != this->m_keys.rend(); ++it) {
              object.try_emplace(*it, ctx.stack().get_top_reference().read());
              ctx.stack().pop_reference();
            }
            // Push the object as a temporary.
            Reference_Root::S_temporary xref = { rocket::move(object) };
            ctx.stack().push_reference(rocket::move(xref));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_coalescence : public Air_Node
      {
      private:
        Air_Queue m_code_null;
        bool m_assign;

      public:
        Air_execute_coalescence(Air_Queue&& code_null, bool assign)
          : m_code_null(rocket::move(code_null)), m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Pick a branch basing on the condition.
            if(ctx.stack().get_top_reference().read().is_null()) {
              // Evaluate the null branch. If the branch is empty, leave the condition on the ctx.stack().
              do_execute_subexpression(ctx, this->m_code_null, this->m_assign);
            }
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_operator_fma : public Air_Node
      {
      private:
        bool m_assign;

      public:
        explicit Air_execute_operator_fma(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is ternary.
            auto rhs = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            auto mid = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            const auto& lhs = ctx.stack().get_top_reference().read();
            if(lhs.is_convertible_to_real() && mid.is_convertible_to_real() && rhs.is_convertible_to_real()) {
              // Calculate the fused multiply-add result of the operands.
              // Note that `rhs` might not have type `G_real`, thus this branch can't be optimized.
              rhs = std::fma(lhs.convert_to_real(), mid.convert_to_real(), rhs.convert_to_real());
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("Fused multiply-add is not defined for `", lhs, "` and `", rhs, "`.");
            }
            ctx.stack().set_temporary_reference(this->m_assign, rocket::move(rhs));
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    }  // namespace

void Xprunit::generate_code(Air_Queue& code, const Compiler_Options& options, Xprunit::TCO_Awareness tco, const Analytic_Context& ctx) const
  {
    switch(this->index()) {
    case index_literal:
      {
        const auto& altr = this->m_stor.as<index_literal>();
        // Encode arguments.
        code.push<Air_push_literal>(altr.value);
        return;
      }
    case index_named_reference:
      {
        const auto& altr = this->m_stor.as<index_named_reference>();
        // Perform early lookup when the expression is defined.
        // If a named reference is found, it will not be replaced or hidden by a later-declared one.
        const Reference* qref = nullptr;
        std::ptrdiff_t depth = -1;
        // Look for the name recursively.
        auto qctx = std::find_if(Context_iterator<Abstract_Context>(ctx), Context_iterator<Abstract_Context>(),
                                 [&](const Abstract_Context& r) { return ++depth, (qref = r.get_named_reference_opt(altr.name)) != nullptr;  });
        if(!qref) {
          // No name has been found so far. Assume that the name will be found in the global context later.
          code.push<Air_find_named_reference_global>(altr.name);
        }
        else if(qctx->is_analytic()) {
          // A reference declared later has been found. Record the context depth for later lookups.
          code.push<Air_find_named_reference_local>(altr.name, depth);
        }
        else {
          // A reference declared previously has been found. Bind it immediately.
          code.push<Air_push_bound_reference>(*qref);
        }
        return;
      }
    case index_closure_function:
      {
        const auto& altr = this->m_stor.as<index_closure_function>();
        // Encode arguments.
        code.push<Air_execute_closure_function>(options, altr.sloc, altr.params, altr.body);
        return;
      }
    case index_branch:
      {
        const auto& altr = this->m_stor.as<index_branch>();
        // Generate code.
        auto code_true = do_generate_code_branch(options, tco, ctx, altr.branch_true);
        auto code_false = do_generate_code_branch(options, tco, ctx, altr.branch_false);
        // Encode arguments.
        code.push<Air_execute_branch>(rocket::move(code_true), rocket::move(code_false), altr.assign);
        return;
      }
    case index_function_call:
      {
        const auto& altr = this->m_stor.as<index_function_call>();
        // Encode arguments.
        if(!options.disable_tco && (tco != tco_none)) {
          code.push<Air_execute_function_tail_call>(altr.sloc, altr.by_refs, tco == tco_by_ref);
          return;
        }
        code.push<Air_execute_function_call>(altr.sloc, altr.by_refs);
        return;
      }
    case index_member_access:
      {
        const auto& altr = this->m_stor.as<index_member_access>();
        // Encode arguments.
        code.push<Air_execute_member_access>(altr.name);
        return;
      }
    case index_operator_rpn:
      {
        const auto& altr = this->m_stor.as<index_operator_rpn>();
        switch(altr.xop) {
        case xop_postfix_inc:
          {
            code.push<Air_execute_operator_rpn_postfix_inc>();
            return;
          }
        case xop_postfix_dec:
          {
            code.push<Air_execute_operator_rpn_postfix_dec>();
            return;
          }
        case xop_postfix_subscr:
          {
            code.push<Air_execute_operator_rpn_postfix_subscr>();
            return;
          }
        case xop_prefix_pos:
          {
            code.push<Air_execute_operator_rpn_prefix_pos>(altr.assign);
            return;
          }
        case xop_prefix_neg:
          {
            code.push<Air_execute_operator_rpn_prefix_neg>(altr.assign);
            return;
          }
        case xop_prefix_notb:
          {
            code.push<Air_execute_operator_rpn_prefix_notb>(altr.assign);
            return;
          }
        case xop_prefix_notl:
          {
            code.push<Air_execute_operator_rpn_prefix_notl>(altr.assign);
            return;
          }
        case xop_prefix_inc:
          {
            code.push<Air_execute_operator_rpn_prefix_inc>();
            return;
          }
        case xop_prefix_dec:
          {
            code.push<Air_execute_operator_rpn_prefix_dec>();
            return;
          }
        case xop_prefix_unset:
          {
            code.push<Air_execute_operator_rpn_prefix_unset>(altr.assign);
            return;
          }
        case xop_prefix_lengthof:
          {
            code.push<Air_execute_operator_rpn_prefix_lengthof>(altr.assign);
            return;
          }
        case xop_prefix_typeof:
          {
            code.push<Air_execute_operator_rpn_prefix_typeof>(altr.assign);
            return;
          }
        case xop_prefix_sqrt:
          {
            code.push<Air_execute_operator_rpn_prefix_sqrt>(altr.assign);
            return;
          }
        case xop_prefix_isnan:
          {
            code.push<Air_execute_operator_rpn_prefix_isnan>(altr.assign);
            return;
          }
        case xop_prefix_isinf:
          {
            code.push<Air_execute_operator_rpn_prefix_isinf>(altr.assign);
            return;
          }
        case xop_prefix_abs:
          {
            code.push<Air_execute_operator_rpn_prefix_abs>(altr.assign);
            return;
          }
        case xop_prefix_signb:
          {
            code.push<Air_execute_operator_rpn_prefix_signb>(altr.assign);
            return;
          }
        case xop_prefix_round:
          {
            code.push<Air_execute_operator_rpn_prefix_round>(altr.assign);
            return;
          }
        case xop_prefix_floor:
          {
            code.push<Air_execute_operator_rpn_prefix_floor>(altr.assign);
            return;
          }
        case xop_prefix_ceil:
          {
            code.push<Air_execute_operator_rpn_prefix_ceil>(altr.assign);
            return;
          }
        case xop_prefix_trunc:
          {
            code.push<Air_execute_operator_rpn_prefix_trunc>(altr.assign);
            return;
          }
        case xop_prefix_iround:
          {
            code.push<Air_execute_operator_rpn_prefix_iround>(altr.assign);
            return;
          }
        case xop_prefix_ifloor:
          {
            code.push<Air_execute_operator_rpn_prefix_ifloor>(altr.assign);
            return;
          }
        case xop_prefix_iceil:
          {
            code.push<Air_execute_operator_rpn_prefix_iceil>(altr.assign);
            return;
          }
        case xop_prefix_itrunc:
          {
            code.push<Air_execute_operator_rpn_prefix_itrunc>(altr.assign);
            return;
          }
        case xop_infix_cmp_eq:
          {
            code.push<Air_execute_operator_rpn_infix_cmp_xeq>(altr.assign, false);
            return;
          }
        case xop_infix_cmp_ne:
          {
            code.push<Air_execute_operator_rpn_infix_cmp_xeq>(altr.assign, true);
            return;
          }
        case xop_infix_cmp_lt:
          {
            code.push<Air_execute_operator_rpn_infix_cmp_xrel>(altr.assign, Value::compare_less, false);
            return;
          }
        case xop_infix_cmp_gt:
          {
            code.push<Air_execute_operator_rpn_infix_cmp_xrel>(altr.assign, Value::compare_greater, false);
            return;
          }
        case xop_infix_cmp_lte:
          {
            code.push<Air_execute_operator_rpn_infix_cmp_xrel>(altr.assign, Value::compare_greater, true);
            return;
          }
        case xop_infix_cmp_gte:
          {
            code.push<Air_execute_operator_rpn_infix_cmp_xrel>(altr.assign, Value::compare_less, true);
            return;
          }
        case xop_infix_cmp_3way:
          {
            code.push<Air_execute_operator_rpn_infix_cmp_3way>(altr.assign);
            return;
          }
        case xop_infix_add:
          {
            code.push<Air_execute_operator_rpn_infix_add>(altr.assign);
            return;
          }
        case xop_infix_sub:
          {
            code.push<Air_execute_operator_rpn_infix_sub>(altr.assign);
            return;
          }
        case xop_infix_mul:
          {
            code.push<Air_execute_operator_rpn_infix_mul>(altr.assign);
            return;
          }
        case xop_infix_div:
          {
            code.push<Air_execute_operator_rpn_infix_div>(altr.assign);
            return;
          }
        case xop_infix_mod:
          {
            code.push<Air_execute_operator_rpn_infix_mod>(altr.assign);
            return;
          }
        case xop_infix_sll:
          {
            code.push<Air_execute_operator_rpn_infix_sll>(altr.assign);
            return;
          }
        case xop_infix_srl:
          {
            code.push<Air_execute_operator_rpn_infix_srl>(altr.assign);
            return;
          }
        case xop_infix_sla:
          {
            code.push<Air_execute_operator_rpn_infix_sla>(altr.assign);
            return;
          }
        case xop_infix_sra:
          {
            code.push<Air_execute_operator_rpn_infix_sra>(altr.assign);
            return;
          }
        case xop_infix_andb:
          {
            code.push<Air_execute_operator_rpn_infix_andb>(altr.assign);
            return;
          }
        case xop_infix_orb:
          {
            code.push<Air_execute_operator_rpn_infix_orb>(altr.assign);
            return;
          }
        case xop_infix_xorb:
          {
            code.push<Air_execute_operator_rpn_infix_xorb>(altr.assign);
            return;
          }
        case xop_infix_assign:
          {
            code.push<Air_execute_operator_rpn_infix_assign>();
            return;
          }
        default:
          ASTERIA_TERMINATE("An unknown operator type enumeration `", altr.xop, "` has been encountered.");
        }
      }
    case index_unnamed_array:
      {
        const auto& altr = this->m_stor.as<index_unnamed_array>();
        // Encode arguments.
        code.push<Air_execute_unnamed_array>(altr.nelems);
        return;
      }
    case index_unnamed_object:
      {
        const auto& altr = this->m_stor.as<index_unnamed_object>();
        // Encode arguments.
        code.push<Air_execute_unnamed_object>(altr.keys);
        return;
      }
    case index_coalescence:
      {
        const auto& altr = this->m_stor.as<index_coalescence>();
        // Generate code.
        auto code_null = do_generate_code_branch(options, tco, ctx, altr.branch_null);
        // Encode arguments.
        code.push<Air_execute_coalescence>(rocket::move(code_null), altr.assign);
        return;
      }
    case index_operator_fma:
      {
        const auto& altr = this->m_stor.as<index_operator_fma>();
        // Encode arguments.
        code.push<Air_execute_operator_fma>(altr.assign);
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown expression unit type enumeration `", this->index(), "` has been encountered.");
    }
  }

}  // namespace Asteria
