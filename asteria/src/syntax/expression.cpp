// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "expression.hpp"
#include "xpnode.hpp"
#include "statement.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/reference_stack.hpp"
#include "../utilities.hpp"

namespace Asteria {

Expression::~Expression()
  {
  }

void Expression::do_compile()
  {
    this->m_jinsts.clear();
    this->m_jinsts.reserve(this->m_nodes.size());
    for(const auto &node : this->m_nodes) {
      this->m_jinsts.emplace_back(node.compile());
    }
  }

Expression Expression::bind(const Global_context &global, const Analytic_context &ctx) const
  {
    rocket::cow_vector<Xpnode> nodes_bnd;
    nodes_bnd.reserve(this->m_nodes.size());
    for(const auto &node : this->m_nodes) {
      auto node_bnd = node.bind(global, ctx);
      nodes_bnd.emplace_back(std::move(node_bnd));
    }
    return std::move(nodes_bnd);
  }

bool Expression::evaluate_partial(Reference_stack &stack_io, Global_context &global, const Executive_context &ctx) const
  {
    auto rptr = this->m_jinsts.data();
    const auto eptr = rptr + this->m_jinsts.size();
    if(rptr == eptr) {
      return false;
    }
    // Unroll the loop using Duff's Device.
    const auto rem = static_cast<std::uintptr_t>(eptr - rptr - 1) % 4;
    rptr += rem + 1;
    switch(rem) {
#define STEP(x_)  \
        do {  \
          rptr[x_](stack_io, global, ctx);  \
        } while(false)
      do {
        rptr += 4;
//==========================================================
        // Fallthrough.
    case 3:
        STEP(-4);
        // Fallthrough.
    case 2:
        STEP(-3);
        // Fallthrough.
    case 1:
        STEP(-2);
        // Fallthrough.
    default:
        STEP(-1);
//==========================================================
      } while(rptr != eptr);
#undef STEP
    }
    return true;
  }

void Expression::evaluate(Reference &ref_out, Global_context &global, const Executive_context &ctx) const
  {
    Reference_stack stack;
    if(!this->evaluate_partial(stack, global, ctx)) {
      ref_out = Reference_root::S_null();
      return;
    }
    ref_out = std::move(stack.mut_top());
  }

void Expression::enumerate_variables(const Abstract_variable_callback &callback) const
  {
    for(const auto &node : this->m_nodes) {
      node.enumerate_variables(callback);
    }
  }

}
