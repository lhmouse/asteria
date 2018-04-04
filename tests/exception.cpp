// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/exception.hpp"
#include "../src/variable.hpp"
#include "../src/recycler.hpp"
#include "../src/stored_value.hpp"
#include "../src/reference.hpp"

using namespace Asteria;

int main(){
	const auto recycler = std::make_shared<Recycler>();
	auto named_var = std::make_shared<Scoped_variable>();
	recycler->set_variable(named_var->variable, D_integer(42));
	ASTERIA_TEST_CHECK(named_var->variable->get_type() == Variable::type_integer);
	ASTERIA_TEST_CHECK(named_var->variable->get<D_integer>() == 42);
	try {
		Reference::S_lvalue_scoped_variable ref = { recycler, named_var };
		throw Exception(Xptr<Reference>(std::make_shared<Reference>(std::move(ref))));
	} catch(Exception &e){
		const auto ref = read_reference_opt(e.get_reference_opt());
		ASTERIA_TEST_CHECK(ref);
		ref->set(D_string("hello"));
	}
	ASTERIA_TEST_CHECK(named_var->variable->get_type() == Variable::type_string);
	ASTERIA_TEST_CHECK(named_var->variable->get<D_string>() == "hello");
}
