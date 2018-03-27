// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/variable.hpp"
#include "../src/reference.hpp"
#include "../src/scope.hpp"

using namespace Asteria;

int main(){
	auto scope = std::make_shared<Scope>(nullptr);
	Reference::Scoped_variable sref = { scope, "my" };
	auto ref = Reference(std::move(sref));
	ASTERIA_TEST_CHECK_CATCH(ref.load_opt());
	auto &local_var = scope->declare_local_variable("my", make_value<Variable>(String("value")));
	auto ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr.get() == local_var.get());

	auto var = make_value<Variable>(true);
	Reference::Array_element aref = { var.share(), 9 };
	ref = Reference(std::move(aref));
	ASTERIA_TEST_CHECK_CATCH(ref.load_opt());
	Array array;
	for(std::int64_t i = 0; i < 5; ++i){
		array.emplace_back(make_value<Variable>(i + 200));
	}
	var->set(std::move(array));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 5);
	ref.store(make_value<Variable>(Integer(67)));
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 10);
	ASTERIA_TEST_CHECK(var->get<Array>().at(8) == nullptr);
	ASTERIA_TEST_CHECK(var->get<Array>().at(9)->get<Integer>() == 67);

	aref = { var.share(), -7 };
	ref = Reference(std::move(aref));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 203);
	ref.store(make_value<Variable>(std::int64_t(65)));
	ASTERIA_TEST_CHECK(var->get<Array>().at(3)->get<Integer>() == 65);

	aref = { var.share(), 1 };
	ref = Reference(std::move(aref));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 201);
	ref.store(make_value<Variable>(std::int64_t(26)));
	ASTERIA_TEST_CHECK(var->get<Array>().at(1)->get<Integer>() == 26);

	Reference::Object_member oref = { var.share(), "three" };
	ref = Reference(std::move(oref));
	ASTERIA_TEST_CHECK_CATCH(ref.load_opt());
	Object object;
	object.emplace("one", make_value<Variable>(Integer(1)));
	object.emplace("two", make_value<Variable>(Integer(2)));
	var->set(std::move(object));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<Object>().size() == 2);
	ref.store(make_value<Variable>(Integer(3)));
	ASTERIA_TEST_CHECK(var->get<Object>().size() == 3);

	oref = { var.share(), "one" };
	ref = Reference(std::move(oref));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 1);
	ref.store(make_value<Variable>(std::int64_t(72)));
	ASTERIA_TEST_CHECK(var->get<Object>().at("one")->get<Integer>() == 72);

	var->set(std::int64_t(42));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_integer);
	Reference::Pure_rvalue rref = { var.share() };
	ref = Reference(std::move(rref));
	ptr = ref.load_opt();
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 42);
	ASTERIA_TEST_CHECK_CATCH(ref.store(make_value<Variable>(std::int64_t(130))));
}
