// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/exception.hpp"
#include "../src/value.hpp"
#include "../src/stored_reference.hpp"

using namespace Asteria;

int main(){
	auto var = std::make_shared<Variable>();
	set_value(var->mutate_value(), D_integer(42));
	ASTERIA_TEST_CHECK(var->get_value_opt()->get_type() == Value::type_integer);
	ASTERIA_TEST_CHECK(var->get_value_opt()->get<D_integer>() == 42);
	try {
		Reference::S_variable ref_l = { var };
		Vp<Reference> ref;
		set_reference(ref, std::move(ref_l));
		throw Exception(std::move(ref));
	} catch(Exception &e){
		const auto ref = e.get_reference_opt();
		ASTERIA_TEST_CHECK(ref);
		set_value(drill_reference(ref), D_string(D_string::shallow("hello")));
	}
	ASTERIA_TEST_CHECK(var->get_value_opt()->get_type() == Value::type_string);
	ASTERIA_TEST_CHECK(var->get_value_opt()->get<D_string>() == D_string::shallow("hello"));
}
