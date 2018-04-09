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

	Xptr<Variable> var;
	set_variable(var, recycler, true);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	ASTERIA_TEST_CHECK(var->get<D_boolean>() == true);
	ASTERIA_TEST_CHECK_CATCH(var->get<D_string>());
	ASTERIA_TEST_CHECK(var->get_opt<D_double>() == nullptr);

	set_variable(var, recycler, std::int64_t(42));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_integer);
	ASTERIA_TEST_CHECK(var->get<D_integer>() == 42);

	set_variable(var, recycler, 1.5);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_double);
	ASTERIA_TEST_CHECK(var->get<D_double>() == 1.5);

	set_variable(var, recycler, std::string("hello"));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_string);
	ASTERIA_TEST_CHECK(var->get<D_string>() == "hello");

	std::array<unsigned char, 16> uuid = { 1,2,3,4,5,6,7,8,2,2,3,4,5,6,7,8 };
	D_opaque opaque = { uuid, std::make_shared<char>() };
	set_variable(var, recycler, opaque);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_opaque);
	ASTERIA_TEST_CHECK(var->get<D_opaque>().uuid == opaque.uuid);
	ASTERIA_TEST_CHECK(var->get<D_opaque>().handle == opaque.handle);

	boost::container::vector<Xptr<Reference>> params;
	set_variable(var, recycler, D_integer(12));
	Reference::S_temporary_value ref = { std::move(var) };
	params.emplace_back(std::make_shared<Reference>(std::move(ref)));
	set_variable(var, recycler, D_integer(15));
	ref = { std::move(var) };
	params.emplace_back(std::make_shared<Reference>(std::move(ref)));
	D_function function = {
		{ },
		[](Spref<Recycler> recycler, boost::container::vector<Xptr<Reference>> &&params) -> Xptr<Reference> {
			auto param_one = read_reference_opt(params.at(0));
			ASTERIA_TEST_CHECK(param_one);
			auto param_two = read_reference_opt(params.at(1));
			ASTERIA_TEST_CHECK(param_two);
			Xptr<Variable> xptr;
			set_variable(xptr, recycler, param_one->get<D_integer>() * param_two->get<D_integer>());
			Reference::S_temporary_value ref = { std::move(xptr) };
			return Xptr<Reference>(std::make_shared<Reference>(std::move(ref)));
		}
	};
	set_variable(var, recycler, std::move(function));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_function);
	auto result = var->get<D_function>().function(recycler, std::move(params));
	auto rptr = read_reference_opt(result);
	ASTERIA_TEST_CHECK(rptr);
	ASTERIA_TEST_CHECK(rptr->get<D_integer>() == 180);

	D_array array;
	set_variable(var, recycler, D_boolean(true));
	array.emplace_back(std::move(var));
	set_variable(var, recycler, D_string("world"));
	array.emplace_back(std::move(var));
	set_variable(var, recycler, std::move(array));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_array);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(0)->get<D_boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(1)->get<D_string>() == "world");

	D_object object;
	set_variable(var, recycler, D_boolean(true));
	object.emplace("one", std::move(var));
	set_variable(var, recycler, D_string("world"));
	object.emplace("two", std::move(var));
	set_variable(var, recycler, std::move(object));
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_object);
	ASTERIA_TEST_CHECK(var->get<D_object>().at("one")->get<D_boolean>() == true);
	ASTERIA_TEST_CHECK(var->get<D_object>().at("two")->get<D_string>() == "world");
}
