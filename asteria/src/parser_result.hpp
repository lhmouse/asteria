// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PARSER_RESULT_HPP_
#define ASTERIA_PARSER_RESULT_HPP_

#include "fwd.hpp"

namespace Asteria {

class Parser_result {
public:
	enum Error_code : std::uint32_t {
		error_code_success                  =  0,
		error_code_comment_not_closed       =  1,
		error_code_invalid_token_character  =  2,
		error_code_invalid_numeric_literal  =  3,
		error_code_unclosed_string_literal  =  4,
		error_code_unknown_escape_sequence  =  5,
		error_code_escqpe_no_enough_digits  =  6,
		error_code_invalid_utf_code_point   =  7,
	};

private:
	std::size_t m_line;
	std::size_t m_column;
	std::size_t m_length;
	Error_code m_error_code;

public:
	constexpr Parser_result(std::size_t line, std::size_t column, std::size_t length, Error_code error_code) noexcept
		: m_line(line), m_column(column), m_length(length), m_error_code(error_code)
	{ }

public:
	std::size_t get_line() const noexcept {
		return m_line;
	}
	std::size_t get_column() const noexcept {
		return m_column;
	}
	std::size_t get_length() const noexcept {
		return m_length;
	}
	Error_code get_error_code() const noexcept {
		return m_error_code;
	}

public:
	explicit operator bool() const noexcept {
		return m_error_code == error_code_success;
	}
};

}

#endif
