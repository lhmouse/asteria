// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/variable.hpp"
#include "../src/reference.hpp"

using namespace Asteria;

int main(){
	auto var = make_value<Variable>(std::int64_t(42));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_integer);
	auto ref = Reference(Reference::Direct_reference(var.share()));
	auto ptr = ref.load();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 42);
	ASTERIA_TEST_CHECK_CATCH(ref.store(make_value<Variable>(std::int64_t(130))));

	ref = Reference(Reference::Null_reference());
	ptr = ref.load();
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK_CATCH(ref.store(make_value<Variable>(std::int64_t(130))));

	ref = Reference(Reference::Array_element(var.share(), 9));
	ASTERIA_TEST_CHECK_CATCH(ref.load());
	Array array;
	for(std::int64_t i = 0; i < 5; ++i){
		array.emplace_back(make_value<Variable>(i + 200));
	}
	var->set(std::move(array));
	ptr = ref.load();
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 5);
	ref.store(make_value<Variable>(Integer(67)));
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 10);
	ASTERIA_TEST_CHECK(var->get<Array>().at(8)->get<Null>() == nullptr);
	ASTERIA_TEST_CHECK(var->get<Array>().at(9)->get<Integer>() == 67);

	ref = Reference(Reference::Array_element(var.share(), -7));
	ptr = ref.load();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 203);
	ref.store(make_value<Variable>(std::int64_t(65)));
	ASTERIA_TEST_CHECK(var->get<Array>().at(3)->get<Integer>() == 65);

	ref = Reference(Reference::Array_element(var.share(), 1));
	ptr = ref.load();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 201);
	ref.store(make_value<Variable>(std::int64_t(26)));
	ASTERIA_TEST_CHECK(var->get<Array>().at(1)->get<Integer>() == 26);

	ref = Reference(Reference::Object_member(var.share(), "three"));
	ASTERIA_TEST_CHECK_CATCH(ref.load());
	Object object;
	object.emplace("one", make_value<Variable>(Integer(1)));
	object.emplace("two", make_value<Variable>(Integer(2)));
	var->set(std::move(object));
	ptr = ref.load();
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<Object>().size() == 2);
	ref.store(make_value<Variable>(Integer(3)));
	ASTERIA_TEST_CHECK(var->get<Object>().size() == 3);

	ref = Reference(Reference::Object_member(var.share(), "one"));
	ptr = ref.load();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 1);
	ref.store(make_value<Variable>(std::int64_t(72)));
	ASTERIA_TEST_CHECK(var->get<Object>().at("one")->get<Integer>() == 72);
}
