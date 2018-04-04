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

	auto var = Xptr<Variable>(std::make_shared<Variable>(Integer(42)));
	Reference::S_rvalue_generic rref = { var };
	Reference ref = std::move(rref);
	auto ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 42);
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, Integer(130)));

	auto nvar = std::make_shared<Scoped_variable>();
	nvar->variable = Xptr<Variable>(std::make_shared<Variable>(4.2));
	Reference::S_lvalue_scoped_variable lref = { recycler, nvar };
	ref = std::move(lref);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr.get() == nvar->variable.get());

	var->set(true);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	Reference::S_lvalue_array_element aref = { recycler, var, true, 9 };
	ref = std::move(aref);
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	Array array;
	for(std::int64_t i = 0; i < 5; ++i){
		array.emplace_back(Xptr<Variable>(std::make_shared<Variable>(i + 200)));
	}
	var->set(std::move(array));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 5);
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, Integer(67)));
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 5);
	aref = { recycler, var, false, 9 };
	ref = std::move(aref);
	write_reference(ref, Integer(67));
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 10);
	ASTERIA_TEST_CHECK(var->get<Array>().at(8) == nullptr);
	ASTERIA_TEST_CHECK(var->get<Array>().at(9)->get<Integer>() == 67);

	aref = { recycler, var, false, -7 };
	ref = std::move(aref);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 203);
	write_reference(ref, Integer(65));
	ASTERIA_TEST_CHECK(var->get<Array>().at(3)->get<Integer>() == 65);

	aref = { recycler, var, false, 1 };
	ref = std::move(aref);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 201);
	write_reference(ref, Integer(26));
	ASTERIA_TEST_CHECK(var->get<Array>().at(1)->get<Integer>() == 26);

	aref = { recycler, var, false, -12 };
	ref = std::move(aref);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(!ptr);
	write_reference(ref, Integer(37));
	ASTERIA_TEST_CHECK(var->get<Array>().size() == 12);
	ASTERIA_TEST_CHECK(var->get<Array>().at(0)->get<Integer>() == 37);
	ASTERIA_TEST_CHECK(var->get<Array>().at(3)->get<Integer>() == 26);

	Reference::S_lvalue_object_member oref = { recycler, var, true, "three" };
	ref = std::move(oref);
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	Object object;
	object.emplace("one", Xptr<Variable>(std::make_shared<Variable>(Integer(1))));
	object.emplace("two", Xptr<Variable>(std::make_shared<Variable>(Integer(2))));
	var->set(std::move(object));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<Object>().size() == 2);
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, Integer(92)));
	ASTERIA_TEST_CHECK(var->get<Object>().size() == 2);
	oref = { recycler, var, false, "three" };
	ref = std::move(oref);
	write_reference(ref, Integer(92));
	ASTERIA_TEST_CHECK(var->get<Object>().size() == 3);

	oref = { recycler, var, false, "one" };
	ref = std::move(oref);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<Integer>() == 1);
	write_reference(ref, Integer(97));
	ASTERIA_TEST_CHECK(var->get<Object>().at("one")->get<Integer>() == 97);
}
