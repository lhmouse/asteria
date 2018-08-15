// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_HPP_
#define ASTERIA_VALUE_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria
{

class Value
{
public:
	enum Type
		: std::uint8_t
	{
		type_null      = 0,
		type_boolean   = 1,
		type_integer   = 2,
		type_double    = 3,
		type_string    = 4,
		type_opaque    = 5,
		type_function  = 6,
		type_array     = 7,
		type_object    = 8,
	};
	using Variant = rocket::variant<
	ROCKET_CDR(
		, D_null      // 0
		, D_boolean   // 1
		, D_integer   // 2
		, D_double    // 3
		, D_string    // 4
		, D_opaque    // 5
		, D_function  // 6
		, D_array     // 7
		, D_object    // 8
	)>;

	enum Comparison_result
		: std::uint8_t
	{
		comparison_unordered  = 0,
		comparison_less       = 1,
		comparison_equal      = 2,
		comparison_greater    = 3,
	};

private:
	Variant m_variant;

public:
	template<typename CandidateT, typename std::enable_if<std::is_constructible<Variant, CandidateT &&>::value>::type * = nullptr>
	Value(CandidateT &&cand)
		: m_variant(std::forward<CandidateT>(cand))
	{
	}
	Value(const Value &); // TODO These will be `noexcept`.
	Value & operator=(const Value &);
	Value(Value &&) noexcept;
	Value & operator=(Value &&) noexcept;
	~Value();

public:
	Type which() const noexcept
	{
		return static_cast<Type>(m_variant.index());
	}
	template<typename ExpectT>
	const ExpectT * get_opt() const noexcept
	{
		return m_variant.get<ExpectT>();
	}
	template<typename ExpectT>
	ExpectT * get_opt() noexcept
	{
		return m_variant.get<ExpectT>();
	}
	template<typename ExpectT>
	const ExpectT & as() const
	{
		return m_variant.as<ExpectT>();
	}
	template<typename ExpectT>
	ExpectT & as()
	{
		return m_variant.as<ExpectT>();
	}
	template<typename CandidateT>
	void set(CandidateT &&cand)
	{
		m_variant.set(std::forward<CandidateT>(cand));
	}
	void swap(Value &other) noexcept
	{
		m_variant.swap(other.m_variant);
	}
};

inline void swap(Value &lhs, Value &rhs) noexcept
{
	lhs.swap(rhs);
}

extern const char * get_type_name(Value::Type type) noexcept;
extern const char * get_value_type_name(const Value &value) noexcept;

extern bool test_value(const Value &value);
extern Value::Comparison_result compare_values(const Value &lhs, const Value &rhs) noexcept;
extern void dump_value(std::ostream &os, const Value &value, unsigned indent_next = 0, unsigned indent_increment = 2);
extern std::ostream & operator<<(std::ostream &os, const Value &value);

}

#endif
