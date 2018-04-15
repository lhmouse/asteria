// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/variable.hpp"
#include "../src/stored_value.hpp"
#include "../src/reference.hpp"
#include "../src/stored_reference.hpp"
#include "../src/recycler.hpp"

using namespace Asteria;

int main(){
	const auto recycler = std::make_shared<Recycler>();

	Xptr<Variable> var;
	set_variable(var, recycler, D_string("meow"));
	Xptr<Reference> ref;
	Reference::S_constant rsref = { var };
	set_reference(ref, std::move(rsref));
	auto ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == "meow");
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_integer(42)));
	ASTERIA_TEST_CHECK(ptr.get() == var.get());

	Reference::S_temporary_value rdref = { Xptr<Variable>(var.share()) };
	set_reference(ref, std::move(rdref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == "meow");
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_integer(63)));
	ASTERIA_TEST_CHECK(ptr.get() == var.get());

	auto local_var = std::make_shared<Local_variable>();
	set_variable(local_var->variable_opt, recycler, D_double(4.2));
	Reference::S_local_variable lref = { local_var };
	set_reference(ref, std::move(lref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr.get() == local_var->variable_opt.get());

	Xptr<Reference> ref_local;
	copy_reference(ref_local, ref);

	set_variable(local_var->variable_opt, recycler, true);
	ASTERIA_TEST_CHECK(local_var->variable_opt->get_type() == Variable::type_boolean);
	Reference::S_array_element aref = { Xptr<Reference>(ref_local.share()), 9 };
	set_reference(ref, std::move(aref));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	D_array array;
	for(int i = 0; i < 5; ++i){
		set_variable(var, recycler, D_integer(i + 200));
		array.emplace_back(std::move(var));
	}
	set_variable(local_var->variable_opt, recycler, std::move(array));
	aref = { Xptr<Reference>(ref_local.share()), 9 };
	set_reference(ref, std::move(aref));
	local_var->immutable = true;
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr == nullptr);
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_array>().size() == 5);
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_integer(67)));
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_array>().size() == 5);
	local_var->immutable = false;
	set_variable(drill_reference(ref), recycler, D_integer(67));
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_array>().size() == 10);
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_array>().at(8) == nullptr);
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_array>().at(9)->get<D_integer>() == 67);
	aref = { Xptr<Reference>(ref_local.share()), 9 };
	set_reference(ref, std::move(aref));
	local_var->immutable = true;
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_integer(43)));

	aref = { Xptr<Reference>(ref_local.share()), -7 };
	set_reference(ref, std::move(aref));
	local_var->immutable = false;
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 203);
	set_variable(drill_reference(ref), recycler, D_integer(65));
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_array>().at(3)->get<D_integer>() == 65);

	aref = { Xptr<Reference>(ref_local.share()), 1 };
	set_reference(ref, std::move(aref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 201);
	set_variable(drill_reference(ref), recycler, D_integer(26));
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_array>().at(1)->get<D_integer>() == 26);

	aref = { Xptr<Reference>(ref_local.share()), -12 };
	set_reference(ref, std::move(aref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr == nullptr);
	set_variable(drill_reference(ref), recycler, D_integer(37));
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_array>().size() == 12);
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_array>().at(0)->get<D_integer>() == 37);
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_array>().at(3)->get<D_integer>() == 26);

	Reference::S_object_member oref = { Xptr<Reference>(ref_local.share()), "three" };
	set_reference(ref, std::move(oref));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	D_object object;
	set_variable(var, recycler, D_integer(1));
	object.emplace("one", std::move(var));
	set_variable(var, recycler, D_integer(2));
	object.emplace("two", std::move(var));
	set_variable(local_var->variable_opt, recycler, std::move(object));
	oref = { Xptr<Reference>(ref_local.share()), "three" };
	set_reference(ref, std::move(oref));
	local_var->immutable = true;
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr == nullptr);
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_object>().size() == 2);
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_integer(92)));
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_object>().size() == 2);
	oref = { Xptr<Reference>(ref_local.share()), "three" };
	set_reference(ref, std::move(oref));
	local_var->immutable = false;
	set_variable(drill_reference(ref), recycler, D_integer(92));
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_object>().size() == 3);
	oref = { Xptr<Reference>(ref_local.share()), "three" };
	set_reference(ref, std::move(oref));
	local_var->immutable = true;
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, D_integer(43)));

	oref = { Xptr<Reference>(ref_local.share()), "one" };
	set_reference(ref, std::move(oref));
	local_var->immutable = false;
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 1);
	set_variable(drill_reference(ref), recycler, D_integer(97));
	ASTERIA_TEST_CHECK(local_var->variable_opt->get<D_object>().at("one")->get<D_integer>() == 97);

	array.clear();
	set_variable(var, recycler, D_string("first"));
	array.emplace_back(std::move(var));
	set_variable(var, recycler, D_string("second"));
	array.emplace_back(std::move(var));
	set_variable(var, recycler, D_string("third"));
	array.emplace_back(std::move(var));
	set_variable(var, recycler, std::move(array));
	rdref = { std::move(var) };
	set_reference(ref, std::move(rdref));
	aref = { std::move(ref), 2 };
	set_reference(ref, std::move(aref));
	ASTERIA_TEST_CHECK_CATCH(set_variable(drill_reference(ref), recycler, "meow"));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == "third");
	materialize_reference(ref, recycler);
	set_variable(drill_reference(ref), recycler, D_string("meow"));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == "meow");
}
