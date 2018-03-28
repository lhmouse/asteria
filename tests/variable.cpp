// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/variable.hpp"
#include "../src/reference.hpp"

using namespace Asteria;

int main(){
	auto var = make_value<Variable>(true, true);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	ASTERIA_TEST_CHECK(var->get<Boolean>() == true);
	ASTERIA_TEST_CHECK_CATCH(var->get<String>());
	ASTERIA_TEST_CHECK(var->try_get<Double>() == nullptr);
	ASTERIA_TEST_CHECK(var->is_immutable());
	ASTERIA_TEST_CHECK_CATCH(var->set(Integer(42)));
	var->set_immutable(false);

	var->set(Integer(42));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_integer);
	ASTERIA_TEST_CHECK(var->get<Integer>() == 42);

	var->set(1.5);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_double);
	ASTERIA_TEST_CHECK(var->get<Double>() == 1.5);

	var->set(std::string("hello"));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_string);
	ASTERIA_TEST_CHECK(var->get<String>() == "hello");

	const auto opaque = std::pair<std::string, std::shared_ptr<void>>("hello", std::make_shared<char>());
	var->set(opaque);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_opaque);
	ASTERIA_TEST_CHECK(var->get<Opaque>() == opaque);

	Array array;
	array.emplace_back(make_value<Variable>(true));
	array.emplace_back(make_value<Variable>(std::string("world")));
	var->set(std::move(array));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_array);
	ASTERIA_TEST_CHECK(var->get<Array>().at(0)->get<Boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<Array>().at(1)->get<String>() == "world");

	Object object;
	object.emplace("one", make_value<Variable>(true));
	object.emplace("two", make_value<Variable>(std::string("world")));
	var->set(std::move(object));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_object);
	ASTERIA_TEST_CHECK(var->get<Object>().at("one")->get<Boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<Object>().at("two")->get<String>() == "world");

	Function function = [](boost::container::vector<Reference> &&params){
		const auto param_one = params.at(0).load_opt();
		ASTERIA_TEST_CHECK(param_one);
		const auto param_two = params.at(1).load_opt();
		ASTERIA_TEST_CHECK(param_two);
		Reference::Rvalue_generic ref = { std::make_shared<Variable>(param_one->get<Integer>() * param_two->get<Integer>()) };
		return Reference(std::move(ref));
	};
	var->set(std::move(function));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_function);
	boost::container::vector<Reference> params;
	Reference::Rvalue_generic ref = { std::make_shared<Variable>(Integer(12)) };
	params.emplace_back(std::move(ref));
	ref = { std::make_shared<Variable>(Integer(15)) };
	params.emplace_back(std::move(ref));
	auto result = var->get<Function>()(std::move(params));
	auto rv = result.load_opt();
	ASTERIA_TEST_CHECK(rv->get<Integer>() == 180);
}
