// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INITIALIZER_HPP_
#define ASTERIA_INITIALIZER_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Initializer {
public:
	enum Type : unsigned {
		type_null                 = -1u,
		type_assignment_init      =  0,
		type_bracketed_init_list  =  1,
		type_braced_init_list     =  2,
	};
	struct S_assignment_init {
		Xptr<Expression> expression;
	};
	struct S_bracketed_init_list {
		Xptr_vector<Initializer> initializers;
	};
	struct S_braced_init_list {
		Xptr_map<std::string, Initializer> key_values;
	};
	using Variant = rocket::variant< S_assignment_init      // 0
	                               , S_bracketed_init_list  // 1
	                               , S_braced_init_list     // 2
		>;

private:
	Variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Initializer, ValueT)>
	Initializer(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }
	Initializer(Initializer &&);
	Initializer &operator=(Initializer &&);
	~Initializer();

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.index());
	}
	template<typename ExpectT>
	const ExpectT *get_opt() const noexcept {
		return m_variant.try_get<ExpectT>();
	}
	template<typename ExpectT>
	const ExpectT &get() const {
		return m_variant.get<ExpectT>();
	}
};

extern Initializer::Type get_initializer_type(Spref<const Initializer> initializer_opt) noexcept;

extern void initialize_variable(Xptr<Variable> &variable_out, Spref<Recycler> recycler, Spref<const Initializer> initializer_opt, Spref<const Scope> scope);

}

#endif
