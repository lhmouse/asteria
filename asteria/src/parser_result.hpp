// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PARSER_RESULT_HPP_
#define ASTERIA_PARSER_RESULT_HPP_

#include "fwd.hpp"

namespace Asteria {

class Parser_result {
public:
	enum Error_code : unsigned {
		error_success             =  0,
		error_comment_not_closed  =  1,
		error_invalid_character   =  2,
	};

private:
	Error_code m_error_code;
	std::size_t m_line;
	std::size_t m_column;
	std::size_t m_length;

public:
	Parser_result(Error_code error_code, std::size_t line, std::size_t column, std::size_t length)
		: m_error_code(error_code), m_line(line), m_column(column), m_length(length)
	{ }

public:
	Error_code get_error_code() const noexcept {
		return m_error_code;
	}
	std::size_t get_line() const noexcept {
		return m_line;
	}
	std::size_t get_column() const noexcept {
		return m_column;
	}
	std::size_t get_length() const noexcept {
		return m_length;
	}
};

}

#endif
