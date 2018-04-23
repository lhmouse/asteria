// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/scope.hpp"
#include "../src/recycler.hpp"
#include "../src/reference.hpp"
#include "../src/stored_reference.hpp"
#include "../src/variable.hpp"
#include "../src/stored_value.hpp"
#include "../src/local_variable.hpp"

using namespace Asteria;

int main(){
	const auto recycler = std::make_shared<Recycler>();

	auto parent = std::make_shared<Scope>(Scope::type_plain, nullptr);
	auto one = std::make_shared<Local_variable>();
	set_variable(one->drill_for_variable(), recycler, D_integer(42));
	one->set_immutable(true);
	Reference::S_local_variable lref = { one };
	auto wref = parent->drill_for_local_reference("one");
	set_reference(wref, std::move(lref));

	auto ref = parent->get_local_reference_opt("one");
	ASTERIA_TEST_CHECK(ref);
	auto ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr.get() == one->get_variable_opt().get());

	ref = parent->get_local_reference_opt("nonexistent");
	ASTERIA_TEST_CHECK(ref == nullptr);

	auto child = std::make_shared<Scope>(Scope::type_plain, parent);
	auto hidden_p = std::make_shared<Local_variable>();
	set_variable(hidden_p->drill_for_variable(), recycler, D_string("in parent"));
	lref = { hidden_p };
	wref = parent->drill_for_local_reference("hidden");
	set_reference(wref, std::move(lref));
	auto hidden_c = std::make_shared<Local_variable>();
	set_variable(hidden_c->drill_for_variable(), recycler, D_string("in child"));
	lref = { hidden_c };
	wref = child->drill_for_local_reference("hidden");
	set_reference(wref, std::move(lref));
	ref = get_local_reference_cascade_opt(child, "hidden");
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == "in child");

	child->clear_local_references();
	ref = get_local_reference_cascade_opt(child, "hidden");
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == "in parent");

	parent->clear_local_references();
	ASTERIA_TEST_CHECK(get_local_reference_cascade_opt(child, "hidden") == nullptr);
}
