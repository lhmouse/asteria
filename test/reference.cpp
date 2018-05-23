// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/stored_value.hpp"
#include "../src/stored_reference.hpp"
#include "../src/recycler.hpp"
#include "../src/local_variable.hpp"

using namespace Asteria;

int main(){
	const auto recycler = std::make_shared<Recycler>();

	Xptr<Variable> var;
	set_variable(var, recycler, D_string(String::shallow("meow")));
	Xptr<Reference> ref;
	Reference::S_constant rsref = { var };
	set_reference(ref, std::move(rsref));
	auto ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == String::shallow("meow"));
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_integer(42)));
	ASTERIA_TEST_CHECK(ptr.get() == var.get());

	Reference::S_temporary_value tmpref = { Xptr<Variable>(var.share()) };
	set_reference(ref, std::move(tmpref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == String::shallow("meow"));
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_integer(63)));
	ASTERIA_TEST_CHECK(ptr.get() == var.get());

	auto local_var = std::make_shared<Local_variable>();
	set_variable(local_var->drill_for_variable(), recycler, D_double(4.2));
	Reference::S_local_variable lref = { local_var };
	set_reference(ref, std::move(lref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr.get() == local_var->get_variable_opt().get());

	Xptr<Reference> ref_local;
	copy_reference(ref_local, ref);

	set_variable(local_var->drill_for_variable(), recycler, true);
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get_type() == Variable::type_boolean);
	Reference::S_array_element aref = { Xptr<Reference>(ref_local.share()), 9 };
	set_reference(ref, std::move(aref));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	D_array array;
	for(int i = 0; i < 5; ++i){
		set_variable(var, recycler, D_integer(i + 200));
		array.emplace_back(std::move(var));
	}
	set_variable(local_var->drill_for_variable(), recycler, std::move(array));
	aref = { Xptr<Reference>(ref_local.share()), 9 };
	set_reference(ref, std::move(aref));
	local_var->set_constant(true);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr == nullptr);
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_array>().size() == 5);
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_integer(67)));
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_array>().size() == 5);
	local_var->set_constant(false);
	set_variable(drill_reference(ref), recycler, D_integer(67));
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_array>().size() == 10);
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_array>().at(8) == nullptr);
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_array>().at(9)->get<D_integer>() == 67);
	aref = { Xptr<Reference>(ref_local.share()), 9 };
	set_reference(ref, std::move(aref));
	local_var->set_constant(true);
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_integer(43)));

	aref = { Xptr<Reference>(ref_local.share()), -7 };
	set_reference(ref, std::move(aref));
	local_var->set_constant(false);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 203);
	set_variable(drill_reference(ref), recycler, D_integer(65));
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_array>().at(3)->get<D_integer>() == 65);

	aref = { Xptr<Reference>(ref_local.share()), 1 };
	set_reference(ref, std::move(aref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 201);
	set_variable(drill_reference(ref), recycler, D_integer(26));
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_array>().at(1)->get<D_integer>() == 26);

	aref = { Xptr<Reference>(ref_local.share()), -12 };
	set_reference(ref, std::move(aref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr == nullptr);
	set_variable(drill_reference(ref), recycler, D_integer(37));
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_array>().size() == 12);
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_array>().at(0)->get<D_integer>() == 37);
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_array>().at(3)->get<D_integer>() == 26);

	Reference::S_object_member oref = { Xptr<Reference>(ref_local.share()), String::shallow("three") };
	set_reference(ref, std::move(oref));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	D_object object;
	set_variable(var, recycler, D_integer(1));
	object.emplace(String::shallow("one"), std::move(var));
	set_variable(var, recycler, D_integer(2));
	object.emplace(String::shallow("two"), std::move(var));
	set_variable(local_var->drill_for_variable(), recycler, std::move(object));
	oref = { Xptr<Reference>(ref_local.share()), String::shallow("three") };
	set_reference(ref, std::move(oref));
	local_var->set_constant(true);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr == nullptr);
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_object>().size() == 2);
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_integer(92)));
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_object>().size() == 2);
	oref = { Xptr<Reference>(ref_local.share()), String::shallow("three") };
	set_reference(ref, std::move(oref));
	local_var->set_constant(false);
	set_variable(drill_reference(ref), recycler, D_integer(92));
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_object>().size() == 3);
	oref = { Xptr<Reference>(ref_local.share()), String::shallow("three") };
	set_reference(ref, std::move(oref));
	local_var->set_constant(true);
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_integer(43)));

	oref = { Xptr<Reference>(ref_local.share()), String::shallow("one") };
	set_reference(ref, std::move(oref));
	local_var->set_constant(false);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 1);
	set_variable(drill_reference(ref), recycler, D_integer(97));
	ASTERIA_TEST_CHECK(local_var->get_variable_opt()->get<D_object>().at(String::shallow("one"))->get<D_integer>() == 97);

	array.clear();
	set_variable(var, recycler, D_string(String::shallow("first")));
	array.emplace_back(std::move(var));
	set_variable(var, recycler, D_string(String::shallow("second")));
	array.emplace_back(std::move(var));
	set_variable(var, recycler, D_string(String::shallow("third")));
	array.emplace_back(std::move(var));
	set_variable(var, recycler, std::move(array));
	tmpref = { std::move(var) };
	set_reference(ref, std::move(tmpref));
	aref = { std::move(ref), 2 };
	set_reference(ref, std::move(aref));
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_string(String::shallow("meow"))));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == String::shallow("third"));
	materialize_reference(ref, recycler, false);
	set_variable(drill_reference(ref), recycler, D_string(String::shallow("meow")));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == String::shallow("meow"));
}
