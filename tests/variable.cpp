// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/variable.hpp"

using namespace Asteria;

int main(){
	auto var = create_variable_opt(true);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	ASTERIA_TEST_CHECK(var->get<Boolean>() == true);
	ASTERIA_TEST_CHECK_CATCH(var->get<String>());
	ASTERIA_TEST_CHECK(var->get_opt<Double>() == nullptr);

	var->set_immutable();
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
	array.emplace_back(create_variable_opt(true));
	array.emplace_back(create_variable_opt(std::string("world")));
	var->set(std::move(array));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_array);
	ASTERIA_TEST_CHECK(var->get<Array>().at(0)->get<Boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<Array>().at(1)->get<String>() == "world");

	Object object;
	object.emplace("one", create_variable_opt(true));
	object.emplace("two", create_variable_opt(std::string("world")));
	var->set(std::move(object));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_object);
	ASTERIA_TEST_CHECK(var->get<Object>().at("one")->get<Boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<Object>().at("two")->get<String>() == "world");

	Function function = [](boost::container::vector<std::shared_ptr<Variable>> &&params){
		const auto param_one = params.at(0);
		ASTERIA_TEST_CHECK(param_one);
		const auto param_two = params.at(1);
		ASTERIA_TEST_CHECK(param_two);
		return std::make_shared<Variable>(param_one->get<Integer>() * param_two->get<Integer>());
	};
	var->set(std::move(function));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_function);
	boost::container::vector<std::shared_ptr<Variable>> params;
	params.emplace_back(std::make_shared<Variable>(Integer(12)));
	params.emplace_back(std::make_shared<Variable>(Integer(15)));
	auto rv = var->get<Function>()(std::move(params));
	ASTERIA_TEST_CHECK(rv->get<Integer>() == 180);
}
