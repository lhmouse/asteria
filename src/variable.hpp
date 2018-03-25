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

using Null      = std::nullptr_t;
using Boolean   = bool;
using Integer   = std::int64_t;
using Double    = double;
using String    = std::string;
using Opaque    = std::shared_ptr<void>;
using Array     = boost::container::deque<std::shared_ptr<Variable>>;
using Object    = boost::container::flat_map<std::string, std::shared_ptr<Variable>>;
using Function  = std::function<std::shared_ptr<Variable> (boost::container::deque<std::shared_ptr<Variable>> &&)>;

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

private:
	// There are so many types...
	boost::variant< Null        // 0
	              , Boolean     // 1
	              , Integer     // 2
	              , Double      // 3
	              , String      // 4
	              , Opaque      // 5
	              , Array       // 6
	              , Object      // 7
	              , Function    // 8
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

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.which());
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
};

// These functions are for debugging purposes only.

extern void dump_with_indent(std::ostream &os, const Array &array, unsigned indent_next, unsigned indent_increment);
extern void dump_with_indent(std::ostream &os, const Object &object, unsigned indent_next, unsigned indent_increment);
extern void dump_with_indent(std::ostream &os, const Variable &variable, unsigned indent_next, unsigned indent_increment);

extern std::ostream &operator<<(std::ostream &os, const Array &rhs);
extern std::ostream &operator<<(std::ostream &os, const Object &rhs);
extern std::ostream &operator<<(std::ostream &os, const Variable &rhs);

}

#endif
