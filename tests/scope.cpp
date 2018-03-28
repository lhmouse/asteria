// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/scope.hpp"

using namespace Asteria;

int main(){
	auto parent = std::make_shared<Scope>(nullptr);
	auto hidden_p = parent->declare_local_variable("hidden");
	auto one = parent->declare_local_variable("one");

	auto ptr = parent->get_variable_recursive_opt("nonexistent");
	ASTERIA_TEST_CHECK(ptr == nullptr);

	auto child = std::make_shared<Scope>(parent);
	auto hidden_c = child->declare_local_variable("hidden");
	auto two = child->declare_local_variable("two");

	ptr = child->get_variable_recursive_opt("hidden");
	ASTERIA_TEST_CHECK(ptr == hidden_c);

	ptr = parent->get_variable_recursive_opt("hidden");
	ASTERIA_TEST_CHECK(ptr == hidden_p);

	ptr = child->get_variable_recursive_opt("one");
	ASTERIA_TEST_CHECK(ptr == one);

	ptr = child->get_variable_recursive_opt("two");
	ASTERIA_TEST_CHECK(ptr == two);

	child->clear_local_variables();
	ptr = child->get_variable_recursive_opt("hidden");
	ASTERIA_TEST_CHECK(ptr == hidden_p);
}
