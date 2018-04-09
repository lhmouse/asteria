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

	auto scope = std::make_shared<Scope>(Scope::type_plain, nullptr);
	auto one = std::make_shared<Local_variable>();
	set_variable(one->variable_opt, recycler, D_integer(42));
	one->immutable = true;
	Reference::S_local_variable lref = { one };
	scope->set_local_reference("one", Xptr<Reference>(std::make_shared<Reference>(std::move(lref))));

	auto ref = scope->get_local_reference_opt("one");
	ASTERIA_TEST_CHECK(ref);
	auto ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr.get() == one->variable_opt.get());

	ref = scope->get_local_reference_opt("nonexistent");
	ASTERIA_TEST_CHECK(ref == nullptr);
}
