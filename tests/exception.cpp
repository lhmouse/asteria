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
	auto local_var = std::make_shared<Local_variable>();
	set_variable(local_var->variable_opt, recycler, D_integer(42));
	ASTERIA_TEST_CHECK(local_var->variable_opt->get_type() == Variable::type_integer);
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_integer>() == 42);
	try {
		Reference::S_local_variable ref = { local_var };
		throw Exception(Xptr<Reference>(std::make_shared<Reference>(std::move(ref))));
	} catch(Exception &e){
		const auto ref = e.get_reference_opt();
		ASTERIA_TEST_CHECK(ref);
		set_variable(drill_reference(ref), recycler, D_string("hello"));
	}
	ASTERIA_TEST_CHECK(local_var->variable_opt->get_type() == Variable::type_string);
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_string>() == "hello");
}
