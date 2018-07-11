// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_HPP_
#define ASTERIA_VALUE_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Value {
public:
	enum Type : std::uint8_t {
		type_null      = -1,
		type_boolean   =  0,
		type_integer   =  1,
		type_double    =  2,
		type_string    =  3,
		type_opaque    =  4,
		type_function  =  5,
		type_array     =  6,
		type_object    =  7,
	};
	using Variant = rocket::variant<ROCKET_CDR(
		, D_boolean   //  0
		, D_integer   //  1
		, D_double    //  2
		, D_string    //  3
		, D_opaque    //  4
		, D_function  //  5
		, D_array     //  6
		, D_object    //  7
	)>;

	enum Comparison_result : std::uint8_t {
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
	{ }
	~Value();

	Value(const Value &) = delete;
	Value & operator=(const Value &) = delete;

public:
	Type which() const noexcept {
		return static_cast<Type>(m_variant.index());
	}
	template<typename ExpectT>
	const ExpectT * get_opt() const noexcept {
		return m_variant.get<ExpectT>();
	}
	template<typename ExpectT>
	ExpectT * get_opt() noexcept {
		return m_variant.get<ExpectT>();
	}
	template<typename ExpectT>
	const ExpectT & as() const {
		return m_variant.as<ExpectT>();
	}
	template<typename ExpectT>
	ExpectT & as(){
		return m_variant.as<ExpectT>();
	}
	template<typename CandidateT>
	void set(CandidateT &&cand){
		m_variant.set(std::forward<CandidateT>(cand));
	}
};

extern const char * get_type_name(Value::Type type) noexcept;
extern Value::Type get_value_type(Sp_cref<const Value> value_opt) noexcept;
extern const char * get_value_type_name(Sp_cref<const Value> value_opt) noexcept;

extern void dump_value(std::ostream &os, Sp_cref<const Value> value_opt, unsigned indent_next = 0, unsigned indent_increment = 2);
extern std::ostream & operator<<(std::ostream &os, Sp_cref<const Value> value_opt);
extern std::ostream & operator<<(std::ostream &os, Vp_cref<const Value> value_opt);

extern void allocate_value(Vp<Value> &value_out);
extern void copy_value(Vp<Value> &value_out, Sp_cref<const Value> src_opt);
extern bool test_value(Sp_cref<const Value> value_opt) noexcept;
extern Value::Comparison_result compare_values(Sp_cref<const Value> lhs_opt, Sp_cref<const Value> rhs_opt) noexcept;

template<typename CandidateT>
inline void set_value(Vp<Value> &value_out, CandidateT &&cand){
	if(!value_out){
		((allocate_value))(value_out);
	}
	value_out->set(std::forward<CandidateT>(cand));
}
inline void clear_value(Vp<Value> &value_out){
	value_out.reset();
}

}

#endif
