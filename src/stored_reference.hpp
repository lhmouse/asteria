// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STORED_REFERENCE_HPP_
#define ASTERIA_STORED_REFERENCE_HPP_

#include "fwd.hpp"
#include "reference.hpp"

namespace Asteria {

class Stored_reference {
	friend Reference;

public:
	using Variant = rocket::variant<D_null, Reference::Variant>;

private:
	Variant m_value_opt;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Stored_reference, ValueT)>
	Stored_reference(ValueT &&value_opt)
		: m_value_opt(std::forward<ValueT>(value_opt))
	{ }
	Stored_reference(Stored_reference &&) noexcept;
	Stored_reference &operator=(Stored_reference &&);
	~Stored_reference();

public:
	bool has_value() const noexcept {
		return m_value_opt.index() == 1;
	}
	const Reference::Variant *get_opt() const noexcept {
		return m_value_opt.try_get<Reference::Variant>();
	}
	Reference::Variant *get_opt() noexcept {
		return m_value_opt.try_get<Reference::Variant>();
	}
	const Reference::Variant &get() const {
		return m_value_opt.get<Reference::Variant>();
	}
	Reference::Variant &get(){
		return m_value_opt.get<Reference::Variant>();
	}
	template<typename ValueT>
	void set(ValueT &&value){
		m_value_opt = std::forward<ValueT>(value);
	}
};

extern void set_reference(Xptr<Reference> &reference_out, Stored_reference &&value_opt);

}

#endif
