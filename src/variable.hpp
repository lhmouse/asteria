// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HPP_
#define ASTERIA_VARIABLE_HPP_

#include <boost/variant.hpp>
#include <boost/container/deque.hpp>
#include <boost/container/flat_map.hpp>
#include <functional>
#include <utility>
#include <string>
#include <memory>
#include <iosfwd>
#include <cstdint>
#include "fwd.hpp"

extern template class boost::container::deque<std::shared_ptr<Asteria::Variable>>;
extern template class boost::container::flat_map<std::string, std::shared_ptr<Asteria::Variable>>;
extern template class std::function<std::shared_ptr<Asteria::Variable> (boost::container::deque<std::shared_ptr<Asteria::Variable>> &&)>;

namespace Asteria {

using Array    = boost::container::deque<std::shared_ptr<Variable>>;
using Object   = boost::container::flat_map<std::string, std::shared_ptr<Variable>>;
using Function = std::function<std::shared_ptr<Variable> (boost::container::deque<std::shared_ptr<Variable>> &&)>;

class Variable {
public:
	enum Type : unsigned char {
		type_null      = 0,
		type_boolean   = 1,
		type_integer   = 2,
		type_double    = 3,
		type_string    = 4,
		type_opaque    = 5,
		type_array     = 6,
		type_object    = 7,
		type_function  = 8,
	};

	using Type_null      = std::nullptr_t;
	using Type_boolean   = bool;
	using Type_integer   = std::int64_t;
	using Type_double    = double;
	using Type_string    = std::string;
	using Type_opaque    = std::shared_ptr<void>;
	using Type_array     = Array;
	using Type_object    = Object;
	using Type_function  = Function;

public:
	static const char *get_name_of_type(Type type) noexcept;

private:
	// There are so many types...
	boost::variant< Type_null        // 0
	              , Type_boolean     // 1
	              , Type_integer     // 2
	              , Type_double      // 3
	              , Type_string      // 4
	              , Type_opaque      // 5
	              , Type_array       // 6
	              , Type_object      // 7
	              , Type_function    // 8
		> m_variant;
	// Am I constant ?
	bool m_immutable;

public:
	Variable()
		: m_variant(nullptr), m_immutable(false)
	{ }
	template<typename ValueT>
	Variable(ValueT &&value, bool immutable)
		: m_variant(std::forward<ValueT>(value)), m_immutable(immutable)
	{ }
	~Variable();

private:
	__attribute__((__noreturn__)) void do_throw_immutable() const;
	void do_dump_recursive(std::ostream &os, unsigned indent_next, unsigned indent_increment) const;

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.which());
	}
	const char *get_type_name() const noexcept {
		return get_name_of_type(get_type());
	}
	template<typename ValueT>
	const ValueT &get() const {
		return boost::get<ValueT>(m_variant);
	}
	template<typename ValueT>
	ValueT &get(){
		return boost::get<ValueT>(m_variant);
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

	void dump(std::ostream &os, unsigned indent_increment = 2) const {
		do_dump_recursive(os, 0, indent_increment);
	}
};

// These functions are for debugring purposes only.
extern std::ostream &operator<<(std::ostream &os, const Variable &rhs);
extern std::ostream &operator<<(std::ostream &os, const Variable *rhs);
extern std::ostream &operator<<(std::ostream &os, Variable *rhs);
extern std::ostream &operator<<(std::ostream &os, const std::shared_ptr<const Variable> &rhs);
extern std::ostream &operator<<(std::ostream &os, const std::shared_ptr<Variable> &rhs);

}

#endif
