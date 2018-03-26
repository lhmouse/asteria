// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/variable.hpp"

using namespace Asteria;

int main(){
	auto var = make_value<Variable>();
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_null);
	ASTERIA_TEST_CHECK(var->get<Null>() == nullptr);
	ASTERIA_TEST_CHECK(var->is_immutable() == false);

	var = make_value<Variable>(true, true);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	ASTERIA_TEST_CHECK(var->get<Boolean>() == true);
	ASTERIA_TEST_CHECK(var->is_immutable() == true);
	try {
		var->set(std::int64_t(42));
		ASTERIA_TEST_CHECK(false);
	} catch(std::runtime_error &e){
		ASTERIA_DEBUG_LOG("Caught exception: ", e.what());
	}
	try {
		var->get<String>();
		ASTERIA_TEST_CHECK(false);
	} catch(std::exception &e){
		ASTERIA_DEBUG_LOG("Caught exception: ", e.what());
	}
	var->set_immutable(false);
	ASTERIA_TEST_CHECK(var->is_immutable() == false);

	var->set(std::int64_t(42));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_integer);
	ASTERIA_TEST_CHECK(var->get<Integer>() == 42);

	auto nptr = variable_pointer_cast<std::int64_t>(var);
	ASTERIA_TEST_CHECK(nptr);
	ASTERIA_TEST_CHECK(*nptr == 42);
	nptr = nullptr;
	auto sptr = variable_pointer_cast<std::string>(var);
	ASTERIA_TEST_CHECK(!sptr);

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

	Array array;
	array.emplace_back(make_value<Variable>(true, true));
	array.emplace_back(make_value<Variable>(std::string("world"), true));
	var->set(std::move(array));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_array);
	ASTERIA_TEST_CHECK(var->get<Array>().at(0)->get<Boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<Array>().at(1)->get<String>() == "world");

	Object object;
	object.emplace("one", make_value<Variable>(true, true));
	object.emplace("two", make_value<Variable>(std::string("world"), true));
	var->set(std::move(object));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_object);
	ASTERIA_TEST_CHECK(var->get<Object>().at("one")->get<Boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<Object>().at("two")->get<String>() == "world");

	Function function = [](boost::container::deque<Asteria::Value_ptr<Asteria::Variable>> &&params){
		auto result = make_value<Variable>();
		result->set(params.at(0)->get<Integer>() * params.at(1)->get<Integer>());
		return result;
	};
	var->set(std::move(function));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_function);
	array.clear();
	array.emplace_back(make_value<Variable>(std::int64_t(12), true));
	array.emplace_back(make_value<Variable>(std::int64_t(15), true));
	ASTERIA_TEST_CHECK(var->get<Function>()(std::move(array))->get<Integer>() == 180);\
}
