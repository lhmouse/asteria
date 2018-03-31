// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INITIALIZER_HPP_
#define ASTERIA_INITIALIZER_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Initializer {
public:
	enum Type : unsigned {
		type_none                 = -1u,
		type_assignment_init      =  0,
		type_bracketed_init_list  =  1,
		type_braced_init_list     =  2,
	};
	struct Assignment_init {
		Value_ptr<Expression> expression;
	};
	struct Bracketed_init_list {
		Value_ptr_vector<Initializer> initializers;
	};
	struct Braced_init_list {
		Value_ptr_map<std::string, Initializer> key_values;
	};
	using Types = Type_tuple< Assignment_init      // 0
	                        , Bracketed_init_list  // 1
	                        , Braced_init_list     // 2
		>;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Initializer, ValueT)>
	Initializer(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }

	Initializer(const Initializer &);
	Initializer &operator=(const Initializer &);
	Initializer(Initializer &&);
	Initializer &operator=(Initializer &&);
	~Initializer();

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.which());
	}
	template<typename ExpectT>
	const ExpectT *get_opt() const noexcept {
		return boost::get<ExpectT>(&m_variant);
	}
	template<typename ExpectT>
	ExpectT *get_opt() noexcept {
		return boost::get<ExpectT>(&m_variant);
	}
	template<typename ExpectT>
	const ExpectT &get() const {
		return boost::get<ExpectT>(m_variant);
	}
	template<typename ExpectT>
	ExpectT &get(){
		return boost::get<ExpectT>(m_variant);
	}
	template<typename ValueT>
	void set(ValueT &&value){
		m_variant = std::forward<ValueT>(value);
	}
};

extern Initializer::Type get_initializer_type(Spref<const Initializer> initializer_opt) noexcept;
extern void set_variable_using_initializer_recursive(Value_ptr<Variable> &variable_out_opt, Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Initializer> initializer_opt);

}

#endif
