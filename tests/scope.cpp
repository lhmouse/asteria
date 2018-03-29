// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/scope.hpp"

using namespace Asteria;

int main(){
	auto parent = std::make_shared<Scope>(nullptr);
	auto hidden_p = parent->declare_variable_local("hidden");
	auto one = parent->declare_variable_local("one");

	auto ptr = get_variable_recursive_opt(parent, "nonexistent");
	ASTERIA_TEST_CHECK(ptr == nullptr);

	auto child = std::make_shared<Scope>(parent);
	auto hidden_c = child->declare_variable_local("hidden");
	auto two = child->declare_variable_local("two");

	ptr = get_variable_recursive_opt(child, "hidden");
	ASTERIA_TEST_CHECK(ptr == hidden_c);

	ptr = get_variable_recursive_opt(parent, "hidden");
	ASTERIA_TEST_CHECK(ptr == hidden_p);

	ptr = get_variable_recursive_opt(child, "one");
	ASTERIA_TEST_CHECK(ptr == one);

	ptr = get_variable_recursive_opt(child, "two");
	ASTERIA_TEST_CHECK(ptr == two);

	child->clear_variables_local();
	ptr = get_variable_recursive_opt(child, "hidden");
	ASTERIA_TEST_CHECK(ptr == hidden_p);
}
