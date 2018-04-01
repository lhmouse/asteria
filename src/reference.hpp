// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_HPP_
#define ASTERIA_REFERENCE_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Reference {
public:
	enum Type : unsigned {
		type_rvalue_generic        = 0,
		type_lvalue_generic        = 1,
		type_lvalue_array_element  = 2,
		type_lvalue_object_member  = 3,
	};
	struct Rvalue_generic {
		Sptr<Variable> xvar_opt;
	};
	struct Lvalue_generic {
		Sptr<Scoped_variable> scoped_var;
	};
	struct Lvalue_array_element {
		Sptr<Variable> rvar;
		bool immutable;
		std::int64_t index_bidirectional;
	};
	struct Lvalue_object_member {
		Sptr<Variable> rvar;
		bool immutable;
		std::string key;
	};
	using Types = Type_tuple< Rvalue_generic        // 0
	                        , Lvalue_generic        // 1
	                        , Lvalue_array_element  // 2
	                        , Lvalue_object_member  // 3
		>;

private:
	Sptr<Recycler> m_recycler;
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Reference, ValueT)>
	Reference(Sptr<Recycler> recycler, ValueT &&value)
		: m_recycler(std::move(recycler)), m_variant(std::forward<ValueT>(value))
	{ }

	Reference(Reference &&);
	Reference &operator=(Reference &&);
	~Reference();

public:
	Spref<Recycler> get_recycler() const noexcept {
		return m_recycler;
	}
	void set_recycler(Sptr<Recycler> recycler) noexcept {
		m_recycler = std::move(recycler);
	}

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

extern Sptr<Variable> read_reference_opt(const Reference &reference);
extern void write_reference(Reference &reference, Stored_value &&value_opt);
extern Xptr<Variable> extract_variable_from_reference(Reference &&reference);

}

#endif
