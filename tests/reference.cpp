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

	auto var = std::make_shared<Variable>(D_integer(42));
	Xptr<Reference> ref;
	Reference::S_rvalue_generic rref = { var };
	ref.reset(std::make_shared<Reference>(std::move(rref)));
	auto ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 42);
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, D_integer(130)));

	auto nvar = std::make_shared<Scoped_variable>();
	nvar->variable = Xptr<Variable>(std::make_shared<Variable>(4.2));
	Reference::S_lvalue_scoped_variable lref = { recycler, nvar };
	ref.reset(std::make_shared<Reference>(std::move(lref)));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr.get() == nvar->variable.get());

	var->set(true);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	Reference::S_lvalue_array_element aref = { recycler, var, true, 9 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	D_array array;
	for(std::int64_t i = 0; i < 5; ++i){
		array.emplace_back(Xptr<Variable>(std::make_shared<Variable>(i + 200)));
	}
	var->set(std::move(array));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<D_array>().size() == 5);
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, D_integer(67)));
	ASTERIA_TEST_CHECK(var->get<D_array>().size() == 5);
	aref = { recycler, var, false, 9 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	write_reference(ref, D_integer(67));
	ASTERIA_TEST_CHECK(var->get<D_array>().size() == 10);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(8) == nullptr);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(9)->get<D_integer>() == 67);

	aref = { recycler, var, false, -7 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 203);
	write_reference(ref, D_integer(65));
	ASTERIA_TEST_CHECK(var->get<D_array>().at(3)->get<D_integer>() == 65);

	aref = { recycler, var, false, 1 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 201);
	write_reference(ref, D_integer(26));
	ASTERIA_TEST_CHECK(var->get<D_array>().at(1)->get<D_integer>() == 26);

	aref = { recycler, var, false, -12 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(!ptr);
	write_reference(ref, D_integer(37));
	ASTERIA_TEST_CHECK(var->get<D_array>().size() == 12);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(0)->get<D_integer>() == 37);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(3)->get<D_integer>() == 26);

	Reference::S_lvalue_object_member oref = { recycler, var, true, "three" };
	ref.reset(std::make_shared<Reference>(std::move(oref)));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	D_object object;
	object.emplace("one", Xptr<Variable>(std::make_shared<Variable>(D_integer(1))));
	object.emplace("two", Xptr<Variable>(std::make_shared<Variable>(D_integer(2))));
	var->set(std::move(object));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<D_object>().size() == 2);
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, D_integer(92)));
	ASTERIA_TEST_CHECK(var->get<D_object>().size() == 2);
	oref = { recycler, var, false, "three" };
	ref.reset(std::make_shared<Reference>(std::move(oref)));
	write_reference(ref, D_integer(92));
	ASTERIA_TEST_CHECK(var->get<D_object>().size() == 3);

	oref = { recycler, var, false, "one" };
	ref.reset(std::make_shared<Reference>(std::move(oref)));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 1);
	write_reference(ref, D_integer(97));
	ASTERIA_TEST_CHECK(var->get<D_object>().at("one")->get<D_integer>() == 97);
}
