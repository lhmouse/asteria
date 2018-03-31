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
	recycler->set_variable(named_var->variable, Integer(42));
	ASTERIA_TEST_CHECK(named_var->variable->get_type() == Variable::type_integer);
	ASTERIA_TEST_CHECK(named_var->variable->get<Integer>() == 42);
	try {
		throw Exception(Reference(recycler, Reference::Lvalue_generic{named_var}));
	} catch(Exception &e){
		const auto ptr = read_reference_opt(e.get_reference());
		ASTERIA_TEST_CHECK(ptr);
		ptr->set(String("hello"));
	}
	ASTERIA_TEST_CHECK(named_var->variable->get_type() == Variable::type_string);
	ASTERIA_TEST_CHECK(named_var->variable->get<String>() == "hello");
}
