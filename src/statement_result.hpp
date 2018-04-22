// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_STATEMENT_RESULT_HPP_
#define ASTERIA_STATEMENT_RESULT_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Statement_result {
public:
	enum Type : unsigned {
		type_normal     = 0,
		type_break      = 1,
		type_continue   = 2,
		type_return     = 3,
	};

	struct S_normal {
		// Nothing.
	};
	struct S_break {
		// Nothing.
	};
	struct S_continue {
		// Nothing.
	};
	struct S_return {
		Xptr<Reference> result_opt;
	};
	using Variant = rocket::variant<ASTERIA_CDR(void
		, S_normal     // 0
		, S_break      // 1
		, S_continue   // 2
		, S_return     // 3
	)>;

private:
	Variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Statement_result, ValueT)>
	Statement_result(ValueT &&variant)
		: m_variant(std::forward<ValueT>(variant))
	{ }
	Statement_result(Statement_result &&) noexcept;
	Statement_result &operator=(Statement_result &&);
	~Statement_result();

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

}

#endif
