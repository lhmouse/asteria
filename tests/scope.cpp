// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/scope.hpp"
#include "../src/recycler.hpp"
#include "../src/reference.hpp"
#include "../src/variable.hpp"
#include "../src/stored_value.hpp"

using namespace Asteria;

int main(){
	const auto recycler = std::make_shared<Recycler>();

	auto parent = std::make_shared<Scope>(Scope::type_plain, nullptr);
	auto hidden_p = std::make_shared<Scoped_variable>();
	set_variable_opt(hidden_p->variable_opt, recycler, D_boolean(true));
	Reference::S_lvalue_scoped_variable lref = { hidden_p };
	parent->set_local_reference("hidden", Xptr<Reference>(std::make_shared<Reference>(std::move(lref))));
	auto one = std::make_shared<Scoped_variable>();
	set_variable_opt(one->variable_opt, recycler, D_integer(42));
	lref = { one };
	parent->set_local_reference("one", Xptr<Reference>(std::make_shared<Reference>(std::move(lref))));

	auto ptr = parent->get_reference_recursive_opt("nonexistent");
	ASTERIA_TEST_CHECK(read_reference_opt(nullptr, ptr).get() == nullptr);

	auto child = std::make_shared<Scope>(Scope::type_plain, parent);
	auto hidden_c = std::make_shared<Scoped_variable>();
	set_variable_opt(hidden_c->variable_opt, recycler, D_string("hello"));
	lref = { hidden_c };
	child->set_local_reference("hidden", Xptr<Reference>(std::make_shared<Reference>(std::move(lref))));
	auto two = std::make_shared<Scoped_variable>();
	set_variable_opt(two->variable_opt, recycler, D_array(5));
	lref = { two };
	child->set_local_reference("two", Xptr<Reference>(std::make_shared<Reference>(std::move(lref))));

	ptr = child->get_reference_recursive_opt("hidden");
	ASTERIA_TEST_CHECK(read_reference_opt(nullptr, ptr).get() == hidden_c->variable_opt.get());

	ptr = parent->get_reference_recursive_opt("hidden");
	ASTERIA_TEST_CHECK(read_reference_opt(nullptr, ptr).get() == hidden_p->variable_opt.get());

	ptr = child->get_reference_recursive_opt("one");
	ASTERIA_TEST_CHECK(read_reference_opt(nullptr, ptr).get() == one->variable_opt.get());

	ptr = child->get_reference_recursive_opt("two");
	ASTERIA_TEST_CHECK(read_reference_opt(nullptr, ptr).get() == two->variable_opt.get());

	child->clear_local_references();
	ptr = child->get_reference_recursive_opt("hidden");
	ASTERIA_TEST_CHECK(read_reference_opt(nullptr, ptr).get() == hidden_p->variable_opt.get());
}
