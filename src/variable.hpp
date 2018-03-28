// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HPP_
#define ASTERIA_VARIABLE_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"
#include <boost/optional.hpp>

namespace Asteria {

class Variable {
public:
	enum Type : unsigned {
		type_null      = -1u,
		type_boolean   =  0,
		type_integer   =  1,
		type_double    =  2,
		type_string    =  3,
		type_opaque    =  4,
		type_array     =  5,
		type_object    =  6,
		type_function  =  7,
	};
	using Types = Type_tuple< Boolean    //  0
	                        , Integer    //  1
	                        , Double     //  2
	                        , String     //  3
	                        , Opaque     //  4
	                        , Array      //  5
	                        , Object     //  6
	                        , Function   //  7
		>;

private:
	Types::rebound_variant m_variant;
	bool m_immutable;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Variable, ValueT)>
	Variable(ValueT &&value, bool immutable = false)
		: m_variant(std::forward<ValueT>(value)), m_immutable(immutable)
	{ }
	~Variable();

	Variable &operator=(const Variable &rhs){
		if(m_immutable){
			do_throw_immutable();
		}
		m_variant = rhs.m_variant;
		return *this;
	}
	Variable &operator=(Variable &&rhs){
		if(m_immutable){
			do_throw_immutable();
		}
		m_variant = std::move(rhs.m_variant);
		return *this;
	}

	explicit Variable(const Variable &) = default;
	Variable(Variable &&) = default;

private:
	__attribute__((__noreturn__)) void do_throw_type_mismatch(Type expect) const;
	__attribute__((__noreturn__)) void do_throw_immutable() const;

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.which());
	}

	template<Type expectT>
	const typename Types::at<expectT>::type *get_opt() const noexcept {
		return boost::get<typename Types::at<expectT>::type>(&m_variant);
	}
	template<Type expectT>
	typename Types::at<expectT>::type *get_opt() noexcept {
		return boost::get<typename Types::at<expectT>::type>(&m_variant);
	}
	template<typename ExpectT>
	const ExpectT *get_opt() const noexcept {
		return boost::get<ExpectT>(&m_variant);
	}
	template<typename ExpectT>
	ExpectT *get_opt() noexcept {
		return boost::get<ExpectT>(&m_variant);
	}

	template<Type expectT>
	const typename Types::at<expectT>::type &get() const {
		const auto ptr = get_opt<expectT>();
		if(!ptr){
			do_throw_type_mismatch(expectT);
		}
		return *ptr;
	}
	template<Type expectT>
	typename Types::at<expectT>::type &get(){
		const auto ptr = get_opt<expectT>();
		if(!ptr){
			do_throw_type_mismatch(expectT);
		}
		return *ptr;
	}
	template<typename ExpectT>
	const ExpectT &get() const {
		const auto ptr = get_opt<ExpectT>();
		if(!ptr){
			do_throw_type_mismatch(static_cast<Type>(Types::index_of<ExpectT>::value));
		}
		return *ptr;
	}
	template<typename ExpectT>
	ExpectT &get(){
		const auto ptr = get_opt<ExpectT>();
		if(!ptr){
			do_throw_type_mismatch(static_cast<Type>(Types::index_of<ExpectT>::value));
		}
		return *ptr;
	}

	template<typename ValueT>
	void set(ValueT &&value){
		if(m_immutable){
			do_throw_immutable();
		}
		m_variant = std::forward<ValueT>(value);
	}

	bool is_immutable() const noexcept {
		return m_immutable;
	}
	void set_immutable(bool immutable = true) noexcept {
		m_immutable = immutable;
	}
};

// Non-member functions. All pointer parameters here are OPTIONAL.

extern Value_ptr<Variable> create_variable_opt(boost::optional<Variable> &&value_new_opt = boost::none);
extern Value_ptr<Variable> create_variable_opt(Variable &&value_new);
extern void set_variable(Value_ptr<Variable> &variable, boost::optional<Variable> &&value_new_opt);
extern void set_variable(Value_ptr<Variable> &variable, Variable &&value_new);

extern const char *get_type_name(Variable::Type type) noexcept;
extern Variable::Type get_variable_type(Observer_ptr<const Variable> variable_opt) noexcept;
extern const char *get_variable_type_name(Observer_ptr<const Variable> variable_opt) noexcept;

extern void dump_variable_recursive(std::ostream &os, Observer_ptr<const Variable> variable_opt, unsigned indent_next = 0, unsigned indent_increment = 2);

extern std::ostream &operator<<(std::ostream &os, Observer_ptr<const Variable> variable_opt);

}

#endif
