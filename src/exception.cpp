// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "exception.hpp"

namespace Asteria {

Exception::~Exception(){
	//
}

const char *Exception::what() const noexcept {
	return "Asteria::Exception";
}

Reference Exception::get_reference() const {
	if(!m_reference_opt){
		Reference::Direct_reference null_ref = { nullptr };
		return std::move(null_ref);
	}
	return *m_reference_opt;
}
void Exception::set_reference(Reference reference){
	m_reference_opt = std::make_shared<Reference>(std::move(reference));
}
void Exception::clear_reference() noexcept {
	m_reference_opt = nullptr;
}

}
