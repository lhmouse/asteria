// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TOKEN_HPP_
#define ASTERIA_TOKEN_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Token {
public:
	enum Type : unsigned {
		type_keyword          = 0,
		type_operator         = 1,
		type_identifier       = 2,
		type_literal_null     = 3,
		type_literal_boolean  = 4,
		type_literal_integer  = 5,
		type_literal_double   = 6,
		type_literal_string   = 7,
	};
	struct S_keyword {
		char cstr[16];
	};
	struct S_operator {
		char cstr[8];
	};
	struct S_identifier {
		Cow_string str;
	};
	struct S_literal_null {
		//
	};
	struct S_literal_boolean {
		Boolean value;
	};
	struct S_literal_integer {
		Signed_integer value;
	};
	struct S_literal_double {
		Double_precision value;
	};
	struct S_literal_string {
		Cow_string value;
	};
	using Variant = rocket::variant<ASTERIA_CDR(void
		, S_keyword          // 0
		, S_operator         // 1
		, S_identifier       // 2
		, S_literal_null     // 3
		, S_literal_boolean  // 4
		, S_literal_integer  // 5
		, S_literal_double   // 6
		, S_literal_string   // 7
	)>;

private:
	std::size_t m_source_line;
	std::size_t m_source_column;
	Variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Token, ValueT)>
	Token(std::size_t source_line, std::size_t source_column, ValueT &&value)
		: m_source_line(source_line), m_source_column(source_column), m_variant(std::forward<ValueT>(value))
	{ }
	Token(Token &&) noexcept;
	Token & operator=(Token &&) noexcept;
	~Token();

public:
	std::size_t get_source_line() const noexcept {
		return m_source_line;
	}
	std::size_t get_source_column() const noexcept {
		return m_source_column;
	}
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.index());
	}
	template<typename ExpectT>
	const ExpectT * get_opt() const noexcept {
		return m_variant.try_get<ExpectT>();
	}
	template<typename ExpectT>
	const ExpectT & get() const {
		return m_variant.get<ExpectT>();
	}
};

}

#endif
