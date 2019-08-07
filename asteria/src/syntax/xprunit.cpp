// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "xprunit.hpp"
#include "statement.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/evaluation_stack.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/executive_context.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../runtime/exception.hpp"
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
        return G_integer(static_cast<uint64_t>(lhs) << rhs);
      }

    ROCKET_PURE_FUNCTION G_integer do_operator_srl(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs < 0) {
          ASTERIA_THROW_RUNTIME_ERROR("Bit shift count `", rhs, "` for `", lhs, "` is negative.");
        }
        if(rhs >= 64) {
          return 0;
        }
        return G_integer(static_cast<uint64_t>(lhs) >> rhs);
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
        auto mask_out = static_cast<uint64_t>(lhs) >> bc << bc;
        auto mask_sbt = static_cast<uint64_t>(lhs >> 63) << bc;
        if(mask_out != mask_sbt) {
          ASTERIA_THROW_RUNTIME_ERROR("Arithmetic left shift of `", lhs, "` by `", rhs, "` would result in overflow.");
        }
        return G_integer(static_cast<uint64_t>(lhs) << rhs);
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
        if(nchars > res.max_size() / static_cast<uint64_t>(rhs)) {
          ASTERIA_THROW_RUNTIME_ERROR("Duplication of `", lhs, "` up to `", rhs, "` times would result in an overlong string that cannot be allocated.");
        }
        auto times = static_cast<size_t>(rhs);
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
        if(static_cast<uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<size_t>(rhs);
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
        if(static_cast<uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<size_t>(rhs);
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
        if(static_cast<uint64_t>(rhs) >= res.max_size() - lhs.size()) {
          ASTERIA_THROW_RUNTIME_ERROR("Shifting `", lhs, "` to the left by `", rhs, "` bytes would result in an overlong string that cannot be allocated.");
        }
        auto count = static_cast<size_t>(rhs);
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
        if(static_cast<uint64_t>(rhs) >= lhs.size()) {
          return res;
        }
        auto count = static_cast<size_t>(rhs);
        // Return the substring in the left.
        res.append(lhs.data(), lhs.size() - count);
        return res;
      }

    cow_vector<uptr<AIR_Node>> do_generate_code_branch(const Compiler_Options& options, Xprunit::TCO_Awareness tco_awareness, const Analytic_Context& ctx,
                                                       const cow_vector<Xprunit>& units)
      {
        cow_vector<uptr<AIR_Node>> code;
        rocket::ranged_xfor(units.begin(), units.end(), [&](auto it) { it->generate_code(code, options, Xprunit::tco_none, ctx);  },
                                                        [&](auto it) { it->generate_code(code, options, tco_awareness, ctx);  });
        return code;
      }

    class AIR_push_literal : public virtual AIR_Node
      {
      private:
        Value m_value;

      public:
        explicit AIR_push_literal(const Value& value)
          : m_value(value)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Push the constant.
            Reference_Root::S_constant xref = { this->m_value };
            ctx.stack().push_reference(rocket::move(xref));
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            this->m_value.enumerate_variables(callback);
            return callback;
          }
      };

    template<typename ContextT> class Context_iterator
      {
      public:
        using iterator_category  = std::forward_iterator_tag;
        using value_type         = const ContextT;
        using pointer            = value_type*;
        using reference          = value_type&;
        using difference_type    = ptrdiff_t;

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

    class AIR_find_named_reference_global : public virtual AIR_Node
      {
      private:
        phsh_string m_name;

      public:
        AIR_find_named_reference_global(const phsh_string& name)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_find_named_reference_local : public virtual AIR_Node
      {
      private:
        phsh_string m_name;
        ptrdiff_t m_depth;

      public:
        AIR_find_named_reference_local(const phsh_string& name, ptrdiff_t depth)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_push_bound_reference : public virtual AIR_Node
      {
      private:
        Reference m_ref;

      public:
        explicit AIR_push_bound_reference(const Reference& ref)
          : m_ref(ref)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Push the reference as is.
            ctx.stack().push_reference(this->m_ref);
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            this->m_ref.enumerate_variables(callback);
            return callback;
          }
      };

    class AIR_execute_closure_function : public virtual AIR_Node
      {
      private:
        Compiler_Options m_options;
        Source_Location m_sloc;
        cow_vector<phsh_string> m_params;
        cow_vector<Statement> m_body;

      public:
        AIR_execute_closure_function(const Compiler_Options& options,
                                     const Source_Location& sloc, const cow_vector<phsh_string>& params, const cow_vector<Statement>& body)
          : m_options(options),
            m_sloc(sloc), m_params(params), m_body(body)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Generate code of the function body.
            cow_vector<uptr<AIR_Node>> code_body;
            Analytic_Context ctx_func(1, ctx, this->m_params);
            rocket::ranged_xfor(this->m_body.begin(), this->m_body.end(), [&](auto it) { it->generate_code(code_body, nullptr, ctx_func, this->m_options, false);  },
                                                                          [&](auto it) { it->generate_code(code_body, nullptr, ctx_func, this->m_options, true);  });
            // Format the prototype string.
            cow_osstream fmtss;
            fmtss.imbue(std::locale::classic());
            fmtss << "<closure> (";
            rocket::ranged_xfor(this->m_params.begin(), this->m_params.end(), [&](auto it) { fmtss << *it << ", ";  },
                                                                              [&](auto it) { fmtss << *it;  });
            fmtss <<")";
            // Instantiate the function.
            auto target = rocket::make_refcnt<Instantiated_Function>(this->m_sloc, fmtss.extract_string(), this->m_params, rocket::move(code_body));
            ASTERIA_DEBUG_LOG("New closure function: ", *target);
            Reference_Root::S_temporary xref = { G_function(rocket::move(target)) };
            ctx.stack().push_reference(rocket::move(xref));
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    void do_execute_subexpression(Executive_Context& ctx, const cow_vector<uptr<AIR_Node>>& code, bool assign)
      {
        rocket::for_each(code, [&](const uptr<AIR_Node>& q) { q->execute(ctx);  });
        ctx.stack().pop_next_reference(assign);
      }

    class AIR_execute_branch : public virtual AIR_Node
      {
      private:
        cow_vector<uptr<AIR_Node>> m_code_true;
        cow_vector<uptr<AIR_Node>> m_code_false;
        bool m_assign;

      public:
        AIR_execute_branch(cow_vector<uptr<AIR_Node>>&& code_true, cow_vector<uptr<AIR_Node>>&& code_false, bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    template<typename XcallT, typename... XaddT> AIR_Node::Status do_execute_function_common(Executive_Context& ctx,
                                                                                             const Source_Location& sloc, const cow_vector<bool>& args_by_refs,
                                                                                             XcallT&& xcall, XaddT&&... xadd)
      {
        Value value;
        // Allocate the argument vector.
        cow_vector<Reference> args;
        args.resize(args_by_refs.size());
        for(auto it = args.mut_rbegin(); it != args.rend(); ++it) {
          // Convert the argument to an rvalue if it shouldn't be passed by reference.
          bool by_ref = *(it - args.rbegin() + args_by_refs.rbegin());
          if(!by_ref) {
            ctx.stack().open_top_reference().convert_to_rvalue();
          }
          // Fill an argument.
          *it = rocket::move(ctx.stack().open_top_reference());
          ctx.stack().pop_reference();
        }
        // Get the target reference.
        value = ctx.stack().get_top_reference().read();
        if(!value.is_function()) {
          ASTERIA_THROW_RUNTIME_ERROR("An attempt was made to invoke `", value, "` which is not a function.");
        }
        // Call the function now.
        return rocket::forward<XcallT>(xcall)(sloc, ctx.zvarg()->get_function_signature(),  // sloc, func,
                                              value.as_function(), ctx.stack().open_top_reference().zoom_out(), rocket::move(args),  // target, self, args,
                                              rocket::forward<XaddT>(xadd)...);
      }

    AIR_Node::Status do_xcall_tail(const Source_Location& sloc, const cow_string& func,
                                   const rcobj<Abstract_Function>& target, Reference& self, cow_vector<Reference>&& args,
                                   Xprunit::TCO_Awareness tco_awareness)
      {
        // Pack arguments.
        auto& args_self = args;
        args_self.emplace_back(rocket::move(self));
        // Translate TCO flags.
        // These flags will be bitwise OR'd during tail call expansion. But here only one bit is set.
        uint32_t flags = 0;
        switch(rocket::weaken_enum(tco_awareness)) {
        case Xprunit::tco_by_value:
          flags |= Reference_Root::tcof_by_value;
          break;
        case Xprunit::tco_nullify:
          flags |= Reference_Root::tcof_nullify;
          break;
        }
        // Create a TCO wrapper. The caller shall unwrap the proxy reference when appropriate.
        Reference_Root::S_tail_call xref = { sloc, func, flags, target, rocket::move(args_self) };
        self = rocket::move(xref);
        return AIR_Node::status_return;
      }

    class AIR_execute_function_call_tail : public virtual AIR_Node
      {
      private:
        Source_Location m_sloc;
        cow_vector<bool> m_args_by_refs;
        Xprunit::TCO_Awareness m_tco_awareness;

      public:
        AIR_execute_function_call_tail(const Source_Location& sloc, const cow_vector<bool>& args_by_refs, Xprunit::TCO_Awareness tco_awareness)
          : m_sloc(sloc), m_args_by_refs(args_by_refs), m_tco_awareness(tco_awareness)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            return do_execute_function_common(ctx, this->m_sloc, this->m_args_by_refs, do_xcall_tail, this->m_tco_awareness);
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    AIR_Node::Status do_xcall_plain(const Source_Location& sloc, const cow_string& func,
                                    const rcobj<Abstract_Function>& target, Reference& self, cow_vector<Reference>&& args,
                                    const Global_Context& global)
      try {
        ASTERIA_DEBUG_LOG("Initiating function call at \'", sloc, "\' inside `", func, "`: target = ", *target);
        // Call the function now.
        target->invoke(self, global, rocket::move(args));
        self.finish_call(global);
        // The result will have been stored into `self`.
        ASTERIA_DEBUG_LOG("Returned from function call at \'", sloc, "\' inside `", func, "`: target = ", *target);
        return AIR_Node::status_next;
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

    class AIR_execute_function_call_plain : public virtual AIR_Node
      {
      private:
        Source_Location m_sloc;
        cow_vector<bool> m_args_by_refs;

      public:
        AIR_execute_function_call_plain(const Source_Location& sloc, const cow_vector<bool>& args_by_refs)
          : m_sloc(sloc), m_args_by_refs(args_by_refs)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            return do_execute_function_common(ctx, this->m_sloc, this->m_args_by_refs, do_xcall_plain, ctx.global());
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_member_access : public virtual AIR_Node
      {
      private:
        phsh_string m_name;

      public:
        explicit AIR_execute_member_access(const phsh_string& name)
          : m_name(name)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Append a modifier.
            Reference_Modifier::S_object_key xmod = { this->m_name };
            ctx.stack().open_top_reference().zoom_in(rocket::move(xmod));
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_postfix_inc : public virtual AIR_Node
      {
      private:
        //

      public:
        AIR_execute_operator_rpn_postfix_inc()
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_postfix_dec : public virtual AIR_Node
      {
      private:
        //

      public:
        AIR_execute_operator_rpn_postfix_dec()
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_postfix_subscr : public virtual AIR_Node
      {
      private:
        //

      public:
        AIR_execute_operator_rpn_postfix_subscr()
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_pos : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_pos(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_neg : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_neg(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_notb : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_notb(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_notl : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_notl(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_inc : public virtual AIR_Node
      {
      private:
        //

      public:
        AIR_execute_operator_rpn_prefix_inc()
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_dec : public virtual AIR_Node
      {
      private:
        //

      public:
        AIR_execute_operator_rpn_prefix_dec()
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_unset : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_unset(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_lengthof : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_lengthof(bool assign)
          : m_assign(assign)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This operator is unary.
            const auto& rhs = ctx.stack().get_top_reference().read();
            // Return the number of elements in the operand.
            size_t nelems;
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_typeof : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_typeof(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_sqrt : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_sqrt(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_isnan : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_isnan(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_isinf : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_isinf(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_abs : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_abs(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_signb : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_signb(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_round : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_round(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_floor : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_floor(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_ceil : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_ceil(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_trunc : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_trunc(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_iround : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_iround(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_ifloor : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_ifloor(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_iceil : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_iceil(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_prefix_itrunc : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_prefix_itrunc(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_cmp_xeq : public virtual AIR_Node
      {
      private:
        bool m_assign;
        bool m_negative;

      public:
        AIR_execute_operator_rpn_infix_cmp_xeq(bool assign, bool negative)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_cmp_xrel : public virtual AIR_Node
      {
      private:
        bool m_assign;
        Value::Compare m_expect;
        bool m_negative;

      public:
        AIR_execute_operator_rpn_infix_cmp_xrel(bool assign, Value::Compare expect, bool negative)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_cmp_3way : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_cmp_3way(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_add : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_add(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_sub : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_sub(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_mul : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_mul(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_div : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_div(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_mod : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_mod(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_sll : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_sll(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_srl : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_srl(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_sla : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_sla(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_sra : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_sra(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_andb : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_andb(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_orb : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_orb(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_xorb : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_rpn_infix_xorb(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_rpn_infix_assign : public virtual AIR_Node
      {
      private:
        //

      public:
        AIR_execute_operator_rpn_infix_assign()
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_unnamed_array : public virtual AIR_Node
      {
      private:
        size_t m_nelems;

      public:
        explicit AIR_execute_unnamed_array(size_t nelems)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_unnamed_object : public virtual AIR_Node
      {
      private:
        cow_vector<phsh_string> m_keys;

      public:
        explicit AIR_execute_unnamed_object(const cow_vector<phsh_string>& keys)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_coalescence : public virtual AIR_Node
      {
      private:
        cow_vector<uptr<AIR_Node>> m_code_null;
        bool m_assign;

      public:
        AIR_execute_coalescence(cow_vector<uptr<AIR_Node>>&& code_null, bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    class AIR_execute_operator_fma : public virtual AIR_Node
      {
      private:
        bool m_assign;

      public:
        explicit AIR_execute_operator_fma(bool assign)
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
            return AIR_Node::status_next;
          }
        Variable_Callback& enumerate_variables(Variable_Callback& callback) const override
          {
            return callback;
          }
      };

    }  // namespace

void Xprunit::generate_code(cow_vector<uptr<AIR_Node>>& code,
                            const Compiler_Options& options, Xprunit::TCO_Awareness tco_awareness, const Analytic_Context& ctx) const
  {
    switch(this->index()) {
    case index_literal:
      {
        const auto& altr = this->m_stor.as<index_literal>();
        // Encode arguments.
        code.emplace_back(rocket::make_unique<AIR_push_literal>(altr.value));
        return;
      }
    case index_named_reference:
      {
        const auto& altr = this->m_stor.as<index_named_reference>();
        // Perform early lookup when the expression is defined.
        // If a named reference is found, it will not be replaced or hidden by a later-declared one.
        const Reference* qref = nullptr;
        ptrdiff_t depth = -1;
        // Look for the name recursively.
        auto qctx = std::find_if(Context_iterator<Abstract_Context>(ctx), Context_iterator<Abstract_Context>(),
                                 [&](const Abstract_Context& r) { return ++depth, (qref = r.get_named_reference_opt(altr.name)) != nullptr;  });
        if(!qref) {
          // No name has been found so far. Assume that the name will be found in the global context later.
          code.emplace_back(rocket::make_unique<AIR_find_named_reference_global>(altr.name));
        }
        else if(qctx->is_analytic()) {
          // A reference declared later has been found. Record the context depth for later lookups.
          code.emplace_back(rocket::make_unique<AIR_find_named_reference_local>(altr.name, depth));
        }
        else {
          // A reference declared previously has been found. Bind it immediately.
          code.emplace_back(rocket::make_unique<AIR_push_bound_reference>(*qref));
        }
        return;
      }
    case index_closure_function:
      {
        const auto& altr = this->m_stor.as<index_closure_function>();
        // Encode arguments.
        code.emplace_back(rocket::make_unique<AIR_execute_closure_function>(options, altr.sloc, altr.params, altr.body));
        return;
      }
    case index_branch:
      {
        const auto& altr = this->m_stor.as<index_branch>();
        // Generate code.
        auto code_true = do_generate_code_branch(options, tco_awareness, ctx, altr.branch_true);
        auto code_false = do_generate_code_branch(options, tco_awareness, ctx, altr.branch_false);
        // Encode arguments.
        code.emplace_back(rocket::make_unique<AIR_execute_branch>(rocket::move(code_true), rocket::move(code_false), altr.assign));
        return;
      }
    case index_function_call:
      {
        const auto& altr = this->m_stor.as<index_function_call>();
        // Encode arguments.
        if(options.proper_tail_calls && (tco_awareness != tco_none)) {
          code.emplace_back(rocket::make_unique<AIR_execute_function_call_tail>(altr.sloc, altr.args_by_refs, tco_awareness));
        }
        else {
          code.emplace_back(rocket::make_unique<AIR_execute_function_call_plain>(altr.sloc, altr.args_by_refs));
        }
        return;
      }
    case index_member_access:
      {
        const auto& altr = this->m_stor.as<index_member_access>();
        // Encode arguments.
        code.emplace_back(rocket::make_unique<AIR_execute_member_access>(altr.name));
        return;
      }
    case index_operator_rpn:
      {
        const auto& altr = this->m_stor.as<index_operator_rpn>();
        switch(altr.xop) {
        case xop_postfix_inc:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_postfix_inc>());
            return;
          }
        case xop_postfix_dec:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_postfix_dec>());
            return;
          }
        case xop_postfix_subscr:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_postfix_subscr>());
            return;
          }
        case xop_prefix_pos:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_pos>(altr.assign));
            return;
          }
        case xop_prefix_neg:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_neg>(altr.assign));
            return;
          }
        case xop_prefix_notb:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_notb>(altr.assign));
            return;
          }
        case xop_prefix_notl:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_notl>(altr.assign));
            return;
          }
        case xop_prefix_inc:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_inc>());
            return;
          }
        case xop_prefix_dec:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_dec>());
            return;
          }
        case xop_prefix_unset:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_unset>(altr.assign));
            return;
          }
        case xop_prefix_lengthof:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_lengthof>(altr.assign));
            return;
          }
        case xop_prefix_typeof:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_typeof>(altr.assign));
            return;
          }
        case xop_prefix_sqrt:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_sqrt>(altr.assign));
            return;
          }
        case xop_prefix_isnan:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_isnan>(altr.assign));
            return;
          }
        case xop_prefix_isinf:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_isinf>(altr.assign));
            return;
          }
        case xop_prefix_abs:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_abs>(altr.assign));
            return;
          }
        case xop_prefix_signb:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_signb>(altr.assign));
            return;
          }
        case xop_prefix_round:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_round>(altr.assign));
            return;
          }
        case xop_prefix_floor:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_floor>(altr.assign));
            return;
          }
        case xop_prefix_ceil:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_ceil>(altr.assign));
            return;
          }
        case xop_prefix_trunc:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_trunc>(altr.assign));
            return;
          }
        case xop_prefix_iround:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_iround>(altr.assign));
            return;
          }
        case xop_prefix_ifloor:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_ifloor>(altr.assign));
            return;
          }
        case xop_prefix_iceil:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_iceil>(altr.assign));
            return;
          }
        case xop_prefix_itrunc:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_prefix_itrunc>(altr.assign));
            return;
          }
        case xop_infix_cmp_eq:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_cmp_xeq>(altr.assign, false));
            return;
          }
        case xop_infix_cmp_ne:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_cmp_xeq>(altr.assign, true));
            return;
          }
        case xop_infix_cmp_lt:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_cmp_xrel>(altr.assign, Value::compare_less, false));
            return;
          }
        case xop_infix_cmp_gt:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_cmp_xrel>(altr.assign, Value::compare_greater, false));
            return;
          }
        case xop_infix_cmp_lte:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_cmp_xrel>(altr.assign, Value::compare_greater, true));
            return;
          }
        case xop_infix_cmp_gte:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_cmp_xrel>(altr.assign, Value::compare_less, true));
            return;
          }
        case xop_infix_cmp_3way:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_cmp_3way>(altr.assign));
            return;
          }
        case xop_infix_add:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_add>(altr.assign));
            return;
          }
        case xop_infix_sub:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_sub>(altr.assign));
            return;
          }
        case xop_infix_mul:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_mul>(altr.assign));
            return;
          }
        case xop_infix_div:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_div>(altr.assign));
            return;
          }
        case xop_infix_mod:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_mod>(altr.assign));
            return;
          }
        case xop_infix_sll:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_sll>(altr.assign));
            return;
          }
        case xop_infix_srl:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_srl>(altr.assign));
            return;
          }
        case xop_infix_sla:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_sla>(altr.assign));
            return;
          }
        case xop_infix_sra:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_sra>(altr.assign));
            return;
          }
        case xop_infix_andb:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_andb>(altr.assign));
            return;
          }
        case xop_infix_orb:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_orb>(altr.assign));
            return;
          }
        case xop_infix_xorb:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_xorb>(altr.assign));
            return;
          }
        case xop_infix_assign:
          {
            code.emplace_back(rocket::make_unique<AIR_execute_operator_rpn_infix_assign>());
            return;
          }
        default:
          ASTERIA_TERMINATE("An unknown operator type enumeration `", altr.xop, "` has been encountered. This is likely a bug. Please report.");
        }
      }
    case index_unnamed_array:
      {
        const auto& altr = this->m_stor.as<index_unnamed_array>();
        // Encode arguments.
        code.emplace_back(rocket::make_unique<AIR_execute_unnamed_array>(altr.nelems));
        return;
      }
    case index_unnamed_object:
      {
        const auto& altr = this->m_stor.as<index_unnamed_object>();
        // Encode arguments.
        code.emplace_back(rocket::make_unique<AIR_execute_unnamed_object>(altr.keys));
        return;
      }
    case index_coalescence:
      {
        const auto& altr = this->m_stor.as<index_coalescence>();
        // Generate code.
        auto code_null = do_generate_code_branch(options, tco_awareness, ctx, altr.branch_null);
        // Encode arguments.
        code.emplace_back(rocket::make_unique<AIR_execute_coalescence>(rocket::move(code_null), altr.assign));
        return;
      }
    case index_operator_fma:
      {
        const auto& altr = this->m_stor.as<index_operator_fma>();
        // Encode arguments.
        code.emplace_back(rocket::make_unique<AIR_execute_operator_fma>(altr.assign));
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown expression unit type enumeration `", this->index(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
