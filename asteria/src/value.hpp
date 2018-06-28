// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_HPP_
#define ASTERIA_VALUE_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Value {
public:
	enum Type : signed char {
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
	using Variant = rocket::variant<ASTERIA_CDR(void
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
		comparison_result_unordered  = 0,
		comparison_result_less       = 1,
		comparison_result_equal      = 2,
		comparison_result_greater    = 3,
	};

private:
	const Wp<Recycler> m_weak_recycler;
	Variant m_variant;

public:
	template<typename CandidateT, ASTERIA_ENABLE_IF_ACCEPTABLE_BY_VARIANT(CandidateT, Variant)>
	Value(CandidateT &&cand)
		: m_weak_recycler(), m_variant(std::forward<CandidateT>(cand))
	{ }
	template<typename CandidateT>
	Value(Wp<Recycler> weak_recycler, CandidateT &&cand)
		: m_weak_recycler(std::move(weak_recycler)), m_variant(std::forward<CandidateT>(cand))
	{ }
	~Value();

	Value(const Value &) = delete;
	Value & operator=(const Value &) = delete;

private:
	ROCKET_NORETURN void do_throw_type_mismatch(Type expect) const;

public:
	Sp<Recycler> get_recycler_opt() const noexcept {
		return m_weak_recycler.lock();
	}

	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.index());
	}
	template<typename ExpectT>
	const ExpectT * get_opt() const noexcept {
		return m_variant.try_get<ExpectT>();
	}
	template<typename ExpectT>
	ExpectT * get_opt() noexcept {
		return m_variant.try_get<ExpectT>();
	}
	template<typename ExpectT>
	const ExpectT & get() const {
		const auto ptr = m_variant.try_get<ExpectT>();
		if(!ptr){
			do_throw_type_mismatch(static_cast<Type>(Variant::index_of<ExpectT>::value));
		}
		return *ptr;
	}
	template<typename ExpectT>
	ExpectT & get(){
		const auto ptr = m_variant.try_get<ExpectT>();
		if(!ptr){
			do_throw_type_mismatch(static_cast<Type>(Variant::index_of<ExpectT>::value));
		}
		return *ptr;
	}
	template<typename CandidateT>
	void set(CandidateT &&cand){
		m_variant = std::forward<CandidateT>(cand);
	}
};

extern const char * get_type_name(Value::Type type) noexcept;

extern Value::Type get_value_type(Spr<const Value> value_opt) noexcept;
extern const char * get_value_type_name(Spr<const Value> value_opt) noexcept;

extern bool test_value(Spr<const Value> value_opt) noexcept;
extern void dump_value(std::ostream &os, Spr<const Value> value_opt, unsigned indent_next = 0, unsigned indent_increment = 2);
extern std::ostream & operator<<(std::ostream &os, const Sp_formatter<Value> &value_fmt);

extern void allocate_value(Vp<Value> &value_out, Spr<Recycler> recycler_out);

template<typename CandidateT>
inline void set_value(Vp<Value> &value_out, Spr<Recycler> recycler_out, CandidateT &&cand){
	if(value_out == nullptr){
		allocate_value(value_out, recycler_out);
	}
	value_out->set(std::forward<CandidateT>(cand));
}
inline void set_value(Vp<Value> &value_out, Spr<Recycler> /*recycler_out*/, Nullptr){
	value_out.reset();
}

extern void copy_value(Vp<Value> &value_out, Spr<Recycler> recycler_inout, Spr<const Value> src_opt);
extern void move_value(Vp<Value> &value_out, Spr<Recycler> recycler_inout, Vp<Value> &&src_opt);

// This function is useful for breaking dependency circles.
extern void purge_value(Spr<Value> value_opt) noexcept;

extern Value::Comparison_result compare_values(Spr<const Value> lhs_opt, Spr<const Value> rhs_opt) noexcept;

}

#endif
