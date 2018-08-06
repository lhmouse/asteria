// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/utilities.hpp"

using namespace Asteria;

int main()
{
	try {
		ASTERIA_THROW_RUNTIME_ERROR("test", ' ', "exception: ", 42, '$');
		std::terminate();
	} catch(std::runtime_error &e) {
		ASTERIA_TEST_CHECK(std::strstr(e.what(), "test exception: 42$") != nullptr);
	}
}
