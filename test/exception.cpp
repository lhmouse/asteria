// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/exception.hpp"
#include "../src/stored_value.hpp"
#include "../src/stored_reference.hpp"
#include "../src/recycler.hpp"
#include "../src/variable.hpp"

using namespace Asteria;

int main(){
	const auto recycler = std::make_shared<Recycler>();
	auto var = std::make_shared<Variable>();
	set_value(var->drill_for_value(), recycler, D_integer(42));
	ASTERIA_TEST_CHECK(var->get_value_opt()->get_type() == Value::type_integer);
	ASTERIA_TEST_CHECK(var->get_value_opt()->get<D_integer>() == 42);
	try {
		Reference::S_variable ref_l = { var };
		Xptr<Reference> ref;
		set_reference(ref, std::move(ref_l));
		throw Exception(std::move(ref));
	} catch(Exception &e){
		const auto ref = e.get_reference_opt();
		ASTERIA_TEST_CHECK(ref);
		set_value(drill_reference(ref), recycler, D_string(D_string::shallow("hello")));
	}
	ASTERIA_TEST_CHECK(var->get_value_opt()->get_type() == Value::type_string);
	ASTERIA_TEST_CHECK(var->get_value_opt()->get<D_string>() == D_string::shallow("hello"));
}
