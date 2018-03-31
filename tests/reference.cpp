// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/variable.hpp"
#include "../src/reference.hpp"
#include "../src/stored_value.hpp"
#include "../src/recycler.hpp"

using namespace Asteria;

int main(){
	const auto recycler = create_shared<Recycler>();

	auto var = Xptr<Variable>(create_shared<Variable>(Integer(42)));
	Reference::Rvalue_generic rref = { var };
	auto ref = Reference(recycler, std::move(rref));
	auto ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 42);
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, Integer(130)));

	auto nvar = create_shared<Scoped_variable>();
	nvar->variable = Xptr<Variable>(create_shared<Variable>(4.2));
	Reference::Lvalue_generic lref = { nvar };
	ref = Reference(recycler, std::move(lref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr.get() == nvar->variable.get());

	var->set(true);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	Reference::Lvalue_array_element aref = { var, true, 9 };
	ref = Reference(recycler, std::move(aref));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	Array array;
	for(std::int64_t i = 0; i < 5; ++i){
		array.emplace_back(Xptr<Variable>(create_shared<Variable>(i + 200)));
	}
	var->set(std::move(array));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 5);
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, Integer(67)));
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 5);
	aref = { var, false, 9 };
	ref = Reference(recycler, std::move(aref));
	write_reference(ref, Integer(67));
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 10);
	ASTERIA_TEST_CHECK(var->get<Array>().at(8) == nullptr);
	ASTERIA_TEST_CHECK(var->get<Array>().at(9)->get<Integer>() == 67);

	aref = { var, false, -7 };
	ref = Reference(recycler, std::move(aref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 203);
	write_reference(ref, Integer(65));
	ASTERIA_TEST_CHECK(var->get<Array>().at(3)->get<Integer>() == 65);

	aref = { var, false, 1 };
	ref = Reference(recycler, std::move(aref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 201);
	write_reference(ref, Integer(26));
	ASTERIA_TEST_CHECK(var->get<Array>().at(1)->get<Integer>() == 26);

	aref = { var, false, -12 };
	ref = Reference(recycler, std::move(aref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(!ptr);
	write_reference(ref, Integer(37));
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 12);
	ASTERIA_TEST_CHECK(var->get<Array>().at(0)->get<Integer>() == 37);
	ASTERIA_TEST_CHECK(var->get<Array>().at(3)->get<Integer>() == 26);

	Reference::Lvalue_object_member oref = { var, true, "three" };
	ref = Reference(recycler, std::move(oref));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	Object object;
	object.emplace("one", Xptr<Variable>(create_shared<Variable>(Integer(1))));
	object.emplace("two", Xptr<Variable>(create_shared<Variable>(Integer(2))));
	var->set(std::move(object));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<Object>().size() == 2);
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, Integer(92)));
	ASTERIA_TEST_CHECK(var->get<Object>().size() == 2);
	oref = { var, false, "three" };
	ref = Reference(recycler, std::move(oref));
	write_reference(ref, Integer(92));
	ASTERIA_TEST_CHECK(var->get<Object>().size() == 3);

	oref = { var, false, "one" };
	ref = Reference(recycler, std::move(oref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 1);
	write_reference(ref, Integer(97));
	ASTERIA_TEST_CHECK(var->get<Object>().at("one")->get<Integer>() == 97);
}
