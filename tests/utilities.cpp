// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/utilities.hpp"

using namespace Asteria;

int main(){
	try {
		ASTERIA_THROW_RUNTIME_ERROR("test", ' ', "exception: ", 42);
		std::terminate();
	} catch(std::runtime_error &e){
		ASTERIA_TEST_CHECK(std::strstr(e.what(), "test exception: 42") != nullptr);
	}

	volatile bool flag = false;
	try {
		ASTERIA_VERIFY(flag, "flag is ", flag);
		std::terminate();
	} catch(std::runtime_error &e){
		ASTERIA_TEST_CHECK(std::strstr(e.what(), "flag is false") != nullptr);
	}
	ASTERIA_VERIFY(!flag, "should not throw");
}
