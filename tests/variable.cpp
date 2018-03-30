// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/variable.hpp"
#include "../src/reference.hpp"
#include "../src/recycler.hpp"

using namespace Asteria;

int main(){
	auto var = Value_ptr<Variable>(create_shared<Variable>(true));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	ASTERIA_TEST_CHECK(var->get<Boolean>() == true);
	ASTERIA_TEST_CHECK_CATCH(var->get<String>());
	ASTERIA_TEST_CHECK(var->get_opt<Double>() == nullptr);

	var->set(Integer(42));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_integer);
	ASTERIA_TEST_CHECK(var->get<Integer>() == 42);

	var->set(1.5);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_double);
	ASTERIA_TEST_CHECK(var->get<Double>() == 1.5);

	var->set(std::string("hello"));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_string);
	ASTERIA_TEST_CHECK(var->get<String>() == "hello");

	Magic_handle opaque = { "hello", create_shared<char>() };
	var->set(opaque);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_opaque);
	ASTERIA_TEST_CHECK(var->get<Opaque>().magic == opaque.magic);
	ASTERIA_TEST_CHECK(var->get<Opaque>().handle == opaque.handle);

	const auto recycler = create_shared<Recycler>();
	Function function = [recycler](boost::container::vector<Reference> &&params) -> Reference {
		auto param_one = params.at(0).load_opt();
		ASTERIA_TEST_CHECK(param_one);
		auto param_two = params.at(1).load_opt();
		ASTERIA_TEST_CHECK(param_two);
		Reference::Rvalue_generic ref = { create_shared<Variable>(param_one->get<Integer>() * param_two->get<Integer>()) };
		return Reference(recycler, std::move(ref));
	};
	var->set(std::move(function));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_function);
	boost::container::vector<Reference> params;
	Reference::Rvalue_generic ref = { create_shared<Variable>(Integer(12)) };
	params.emplace_back(recycler, std::move(ref));
	ref = { create_shared<Variable>(Integer(15)) };
	params.emplace_back(recycler, std::move(ref));
	auto result = var->get<Function>()(std::move(params));
	auto rptr = result.load_opt();
	ASTERIA_TEST_CHECK(rptr);
	ASTERIA_TEST_CHECK(rptr->get<Integer>() == 180);

	Array array;
	array.emplace_back(Value_ptr<Variable>(create_shared<Variable>(true)));
	array.emplace_back(Value_ptr<Variable>(create_shared<Variable>(std::string("world"))));
	var->set(std::move(array));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_array);
	ASTERIA_TEST_CHECK(var->get<Array>().at(0)->get<Boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<Array>().at(1)->get<String>() == "world");

	Object object;
	object.emplace("one", Value_ptr<Variable>(create_shared<Variable>(true)));
	object.emplace("two", Value_ptr<Variable>(create_shared<Variable>(std::string("world"))));
	var->set(std::move(object));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_object);
	ASTERIA_TEST_CHECK(var->get<Object>().at("one")->get<Boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<Object>().at("two")->get<String>() == "world");
}
