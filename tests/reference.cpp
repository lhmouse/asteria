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

	auto var = std::make_shared<Variable>(D_string("meow"));
	Xptr<Reference> ref;
	Reference::S_rvalue_static rsref = { var };
	ref.reset(std::make_shared<Reference>(std::move(rsref)));
	auto ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == "meow");
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, recycler, D_integer(42)));
	ASTERIA_TEST_CHECK(ptr.get() == var.get());
	Xptr<Variable> tvar;
	set_variable_using_reference(tvar, recycler, std::move(ref));
	ASTERIA_TEST_CHECK(tvar);
	ASTERIA_TEST_CHECK(tvar->get<D_string>() == "meow");
	ASTERIA_TEST_CHECK(var->get<D_string>() == "meow");

	Reference::S_rvalue_dynamic rrref = { var };
	ref.reset(std::make_shared<Reference>(std::move(rrref)));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == "meow");
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, recycler, D_integer(42)));
	ASTERIA_TEST_CHECK(ptr.get() == var.get());
	set_variable_using_reference(tvar, recycler, std::move(ref));
	ASTERIA_TEST_CHECK(tvar);
	ASTERIA_TEST_CHECK(tvar->get<D_string>() == "meow");
	ASTERIA_TEST_CHECK(var->get<D_string>() == "");

	auto nvar = std::make_shared<Scoped_variable>();
	nvar->variable_opt = Xptr<Variable>(std::make_shared<Variable>(4.2));
	Reference::S_lvalue_scoped_variable lref = { nvar };
	ref.reset(std::make_shared<Reference>(std::move(lref)));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr.get() == nvar->variable_opt.get());

	var->set(true);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	Reference::S_lvalue_array_element aref = { var, true, 9 };
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
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, recycler, D_integer(67)));
	ASTERIA_TEST_CHECK(var->get<D_array>().size() == 5);
	aref = { var, false, 9 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	write_reference(ref, recycler, D_integer(67));
	ASTERIA_TEST_CHECK(var->get<D_array>().size() == 10);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(8) == nullptr);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(9)->get<D_integer>() == 67);

	aref = { var, false, -7 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 203);
	write_reference(ref, recycler, D_integer(65));
	ASTERIA_TEST_CHECK(var->get<D_array>().at(3)->get<D_integer>() == 65);

	aref = { var, false, 1 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 201);
	write_reference(ref, recycler, D_integer(26));
	ASTERIA_TEST_CHECK(var->get<D_array>().at(1)->get<D_integer>() == 26);

	aref = { var, false, -12 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(!ptr);
	write_reference(ref, recycler, D_integer(37));
	ASTERIA_TEST_CHECK(var->get<D_array>().size() == 12);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(0)->get<D_integer>() == 37);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(3)->get<D_integer>() == 26);

	Reference::S_lvalue_object_member oref = { var, true, "three" };
	ref.reset(std::make_shared<Reference>(std::move(oref)));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	D_object object;
	object.emplace("one", Xptr<Variable>(std::make_shared<Variable>(D_integer(1))));
	object.emplace("two", Xptr<Variable>(std::make_shared<Variable>(D_integer(2))));
	var->set(std::move(object));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(!ptr);
	ASTERIA_TEST_CHECK(var->get<D_object>().size() == 2);
	ASTERIA_TEST_CHECK_CATCH(write_reference(ref, recycler, D_integer(92)));
	ASTERIA_TEST_CHECK(var->get<D_object>().size() == 2);
	oref = { var, false, "three" };
	ref.reset(std::make_shared<Reference>(std::move(oref)));
	write_reference(ref, recycler, D_integer(92));
	ASTERIA_TEST_CHECK(var->get<D_object>().size() == 3);

	oref = { var, false, "one" };
	ref.reset(std::make_shared<Reference>(std::move(oref)));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 1);
	write_reference(ref, recycler, D_integer(97));
	ASTERIA_TEST_CHECK(var->get<D_object>().at("one")->get<D_integer>() == 97);
}
