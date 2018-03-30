// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/exception.hpp"
#include "../src/variable.hpp"

using namespace Asteria;

int main(){
	auto var = Value_ptr<Variable>(create_shared<Variable>(Integer(42)));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_integer);
	ASTERIA_TEST_CHECK(var->get<Integer>() == 42);
	try {
		throw Exception(var);
	} catch(Exception &e){
		const auto ptr = e.get_variable_opt();
		ASTERIA_TEST_CHECK(ptr);
		ptr->set(String("hello"));
	}
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_string);
	ASTERIA_TEST_CHECK(var->get<String>() == "hello");
}
