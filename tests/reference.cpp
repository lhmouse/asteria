// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/variable.hpp"
#include "../src/reference.hpp"
#include "../src/stored_value.hpp"
#include "../src/recycler.hpp"

using namespace Asteria;

int main(){
	const auto recycler = std::make_shared<Recycler>();

	auto var = Value_ptr<Variable>(std::make_shared<Variable>(Integer(42)));
	Reference::Rvalue_generic rref = { var };
	auto ref = Reference(std::move(rref));
	auto ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 42);
	ASTERIA_TEST_CHECK_CATCH(ref.store(recycler, Integer(130)));

	auto nvar = std::make_shared<Named_variable>();
	nvar->variable = Value_ptr<Variable>(std::make_shared<Variable>(4.2));
	Reference::Lvalue_generic lref = { nvar };
	ref = Reference(std::move(lref));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(ptr.get() == nvar->variable.get());

	var->set(true);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	Reference::Lvalue_array_element aref = { var, 9 };
	ref = Reference(std::move(aref));
	ASTERIA_TEST_CHECK_CATCH(ref.load_opt());
	Array array;
	for(std::int64_t i = 0; i < 5; ++i){
		array.emplace_back(Value_ptr<Variable>(std::make_shared<Variable>(i + 200)));
	}
	var->set(std::move(array));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 5);
	ref.store(recycler, Integer(67));
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 10);
	ASTERIA_TEST_CHECK(var->get<Array>().at(8) == nullptr);
	ASTERIA_TEST_CHECK(var->get<Array>().at(9)->get<Integer>() == 67);

	aref = { var, -7 };
	ref = Reference(std::move(aref));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 203);
	ref.store(recycler, Integer(65));
	ASTERIA_TEST_CHECK(var->get<Array>().at(3)->get<Integer>() == 65);

	aref = { var, 1 };
	ref = Reference(std::move(aref));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 201);
	ref.store(recycler, Integer(26));
	ASTERIA_TEST_CHECK(var->get<Array>().at(1)->get<Integer>() == 26);

	aref = { var, -12 };
	ref = Reference(std::move(aref));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(!ptr);
	ref.store(recycler, Integer(37));
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 12);
	ASTERIA_TEST_CHECK(var->get<Array>().at(0)->get<Integer>() == 37);
	ASTERIA_TEST_CHECK(var->get<Array>().at(3)->get<Integer>() == 26);

	Reference::Lvalue_object_member oref = { var, "three" };
	ref = Reference(std::move(oref));
	ASTERIA_TEST_CHECK_CATCH(ref.load_opt());
	Object object;
	object.emplace("one", Value_ptr<Variable>(std::make_shared<Variable>(Integer(1))));
	object.emplace("two", Value_ptr<Variable>(std::make_shared<Variable>(Integer(2))));
	var->set(std::move(object));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<Object>().size() == 2);
	ref.store(recycler, Integer(92));
	ASTERIA_TEST_CHECK(var->get<Object>().size() == 3);

	oref = { var, "one" };
	ref = Reference(std::move(oref));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 1);
	ref.store(recycler, Integer(97));
	ASTERIA_TEST_CHECK(var->get<Object>().at("one")->get<Integer>() == 97);
}
