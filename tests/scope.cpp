// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/scope.hpp"

using namespace Asteria;

int main(){
	auto parent = std::make_shared<Scope>(nullptr);
	auto hidden_p = parent->declare_local_variable("hidden").share();
	auto one = parent->declare_local_variable("one").share();

	auto child = std::make_shared<Scope>(parent);
	auto hidden_c = child->declare_local_variable("hidden").share();
	auto two = child->declare_local_variable("two").share();

	auto pptr = child->try_get_local_variable_recursive("hidden");
	ASTERIA_TEST_CHECK(pptr);
	ASTERIA_TEST_CHECK(pptr->get() == hidden_c.get());

	pptr = parent->try_get_local_variable_recursive("hidden");
	ASTERIA_TEST_CHECK(pptr);
	ASTERIA_TEST_CHECK(pptr->get() == hidden_p.get());

	pptr = child->try_get_local_variable_recursive("one");
	ASTERIA_TEST_CHECK(pptr);
	ASTERIA_TEST_CHECK(pptr->get() == one.get());

	pptr = child->try_get_local_variable_recursive("two");
	ASTERIA_TEST_CHECK(pptr);
	ASTERIA_TEST_CHECK(pptr->get() == two.get());

	pptr = child->try_get_local_variable_recursive("nonexistent");
	ASTERIA_TEST_CHECK(!pptr);

	child->clear_local_variables();
	pptr = child->try_get_local_variable_recursive("hidden");
	ASTERIA_TEST_CHECK(pptr);
	ASTERIA_TEST_CHECK(pptr->get() == hidden_p.get());
}
