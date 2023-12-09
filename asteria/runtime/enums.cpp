// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "enums.hpp"
namespace asteria {

const char*
describe_xop(Xop xop) noexcept
  {
    switch(xop) {
      case xop_inc:
        return "++";

      case xop_dec:
        return "--";

      case xop_index:
        return "[]";

      case xop_pos:
        return "+";

      case xop_neg:
        return "-";

      case xop_notb:
        return "~";

      case xop_notl:
        return "!";

      case xop_unset:
        return "unset";

      case xop_countof:
        return "countof";

      case xop_typeof:
        return "typeof";

      case xop_sqrt:
        return "__sqrt";

      case xop_isnan:
        return "__isnan";

      case xop_isinf:
        return "__isinf";

      case xop_abs:
        return "__abs";

      case xop_sign:
        return "__sign";

      case xop_round:
        return "__round";

      case xop_floor:
        return "__floor";

      case xop_ceil:
        return "__ceil";

      case xop_trunc:
        return "__trunc";

      case xop_iround:
        return "__iround";

      case xop_ifloor:
        return "__ifloor";

      case xop_iceil:
        return "__iceil";

      case xop_itrunc:
        return "__itrunc";

      case xop_cmp_eq:
        return "==";

      case xop_cmp_ne:
        return "!=";

      case xop_cmp_lt:
        return "<";

      case xop_cmp_gt:
        return ">";

      case xop_cmp_lte:
        return "<=";

      case xop_cmp_gte:
        return ">=";

      case xop_cmp_3way:
        return "<=>";

      case xop_cmp_un:
        return "</>";

      case xop_add:
        return "+";

      case xop_sub:
        return "-";

      case xop_mul:
        return "*";

      case xop_div:
        return "/";

      case xop_mod:
        return "%";

      case xop_sll:
        return "<<<";

      case xop_srl:
        return ">>>";

      case xop_sla:
        return "<<";

      case xop_sra:
        return ">>";

      case xop_andb:
        return "&";

      case xop_orb:
        return "|";

      case xop_xorb:
        return "^";

      case xop_assign:
        return "=";

      case xop_fma:
        return "__fma";

      case xop_head:
        return "[^]";

      case xop_tail:
        return "[$]";

      case xop_lzcnt:
        return "__lzcnt";

      case xop_tzcnt:
        return "__tzcnt";

      case xop_popcnt:
        return "__popcnt";

      case xop_addm:
        return "__addm";

      case xop_subm:
        return "__subm";

      case xop_mulm:
        return "__mulm";

      case xop_adds:
        return "__adds";

      case xop_subs:
        return "__subs";

      case xop_muls:
        return "__muls";

      case xop_random:
        return "[?]";

      case xop_isvoid:
        return "__isvoid";

      default:
        return "[unknown operator]";
    }
  }

}  // namespace asteria
