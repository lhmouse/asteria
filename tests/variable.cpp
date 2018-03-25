// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/variable.hpp"

using namespace Asteria;

int main(){
	auto var = std::make_shared<Variable>();
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_null);
	ASTERIA_TEST_CHECK(var->get<Null>() == nullptr);
	ASTERIA_TEST_CHECK(var->is_immutable() == false);

	var = std::make_shared<Variable>(true, true);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	ASTERIA_TEST_CHECK(var->get<Boolean>() == true);
	ASTERIA_TEST_CHECK(var->is_immutable() == true);
	try {
		var->set(std::int64_t(42));
		ASTERIA_TEST_CHECK(false);
	} catch(std::runtime_error &e){
		//
	}
	try {
		var->get<String>();
	} catch(std::exception &e){
		//
	}
	var->set_immutable(false);

	var->set(std::int64_t(42));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_integer);
	ASTERIA_TEST_CHECK(var->get<Integer>() == 42);

	var->set(1.5);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_double);
	ASTERIA_TEST_CHECK(var->get<Double>() == 1.5);

	var->set(std::string("hello"));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_string);
	ASTERIA_TEST_CHECK(var->get<String>() == "hello");

	const auto opaque = std::static_pointer_cast<void>(std::make_shared<char>());
	var->set(opaque);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_opaque);
	ASTERIA_TEST_CHECK(var->get<Opaque>() == opaque);

	Array array({
		std::make_shared<Variable>(true, true),
		std::make_shared<Variable>(std::string("world"), true),
	});
	var->set(std::move(array));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_array);
	ASTERIA_TEST_CHECK(var->get<Array>().at(0)->get<Boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<Array>().at(1)->get<String>() == "world");

	Object object({
		std::make_pair("one", std::make_shared<Variable>(true, true)),
		std::make_pair("two", std::make_shared<Variable>(std::string("world"), true)),
	});
	var->set(std::move(object));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_object);
	ASTERIA_TEST_CHECK(var->get<Object>().at("one")->get<Boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<Object>().at("two")->get<String>() == "world");

	Function function = [](boost::container::deque<std::shared_ptr<Asteria::Variable>> &&params){
		auto result = std::make_shared<Variable>();
		result->set(params.at(0)->get<Integer>() * params.at(1)->get<Integer>());
		return result;
	};
	var->set(std::move(function));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_function);
	Array parameters({
		std::make_shared<Variable>(std::int64_t(12), true),
		std::make_shared<Variable>(std::int64_t(15), true),
	});
	ASTERIA_TEST_CHECK(var->get<Function>()(std::move(parameters))->get<Integer>() == 180);
}
