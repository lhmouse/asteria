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

	Xptr<Variable> var, nvar;
	set_variable(var, recycler, D_string("meow"));
	Xptr<Reference> ref;
	Reference::S_rvalue_static rsref = { var };
	ref.reset(std::make_shared<Reference>(std::move(rsref)));
	bool immutable;
	auto ptr = read_reference_opt(&immutable, ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == "meow");
	ASTERIA_TEST_CHECK(immutable == true);
	set_variable(nvar, recycler, D_integer(42));
	ASTERIA_TEST_CHECK_CATCH(write_reference_opt(ref, std::move(nvar)));
	ASTERIA_TEST_CHECK(ptr.get() == var.get());

	nvar.reset(var.share());
	Reference::S_rvalue_dynamic rdref = { std::move(nvar) };
	ref.reset(std::make_shared<Reference>(std::move(rdref)));
	ptr = read_reference_opt(&immutable, ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_string>() == "meow");
	ASTERIA_TEST_CHECK(immutable == true);
	set_variable(nvar, recycler, D_integer(63));
	ASTERIA_TEST_CHECK_CATCH(write_reference_opt(ref, std::move(nvar)));
	ASTERIA_TEST_CHECK(ptr.get() == var.get());

	auto scoped_var = std::make_shared<Scoped_variable>();
	set_variable(scoped_var->variable_opt, recycler, D_double(4.2));
	Reference::S_lvalue_scoped_variable lref = { scoped_var };
	ref.reset(std::make_shared<Reference>(std::move(lref)));
	ptr = read_reference_opt(&immutable, ref);
	ASTERIA_TEST_CHECK(ptr.get() == scoped_var->variable_opt.get());
	ASTERIA_TEST_CHECK(immutable == false);

	set_variable(var, recycler, true);
	ASTERIA_TEST_CHECK(var->get_type() == Variable::type_boolean);
	Reference::S_lvalue_array_element aref = { var, true, 9 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(&immutable, ref));
	D_array array;
	for(int i = 0; i < 5; ++i){
		set_variable(var, recycler, D_integer(i + 200));
		array.emplace_back(std::move(var));
	}
	set_variable(var, recycler, std::move(array));
	aref = { var, true, 9 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	ptr = read_reference_opt(&immutable, ref);
	ASTERIA_TEST_CHECK(ptr == nullptr);
	ASTERIA_TEST_CHECK(var->get<D_array>().size() == 5);
	set_variable(nvar, recycler, D_integer(67));
	ASTERIA_TEST_CHECK_CATCH(write_reference_opt(ref, std::move(nvar)));
	ASTERIA_TEST_CHECK(var->get<D_array>().size() == 5);
	aref = { var, false, 9 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	set_variable(nvar, recycler, D_integer(67));
	write_reference_opt(ref, std::move(nvar));
	ASTERIA_TEST_CHECK(var->get<D_array>().size() == 10);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(8) == nullptr);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(9)->get<D_integer>() == 67);
	aref = { var, true, 9 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	set_variable(nvar, recycler, D_integer(43));
	ASTERIA_TEST_CHECK_CATCH(write_reference_opt(ref, std::move(nvar)));

	aref = { var, false, -7 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	ptr = read_reference_opt(&immutable, ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 203);
	set_variable(nvar, recycler, D_integer(65));
	write_reference_opt(ref, std::move(nvar));
	ASTERIA_TEST_CHECK(var->get<D_array>().at(3)->get<D_integer>() == 65);

	aref = { var, false, 1 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	ptr = read_reference_opt(&immutable, ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 201);
	set_variable(nvar, recycler, D_integer(26));
	write_reference_opt(ref, std::move(nvar));
	ASTERIA_TEST_CHECK(var->get<D_array>().at(1)->get<D_integer>() == 26);

	aref = { var, false, -12 };
	ref.reset(std::make_shared<Reference>(std::move(aref)));
	ptr = read_reference_opt(&immutable, ref);
	ASTERIA_TEST_CHECK(ptr == nullptr);
	set_variable(nvar, recycler, D_integer(37));
	write_reference_opt(ref, std::move(nvar));
	ASTERIA_TEST_CHECK(var->get<D_array>().size() == 12);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(0)->get<D_integer>() == 37);
	ASTERIA_TEST_CHECK(var->get<D_array>().at(3)->get<D_integer>() == 26);

	Reference::S_lvalue_object_member oref = { var, true, "three" };
	ref.reset(std::make_shared<Reference>(std::move(oref)));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(&immutable, ref));
	D_object object;
	set_variable(var, recycler, D_integer(1));
	object.emplace("one", std::move(var));
	set_variable(var, recycler, D_integer(2));
	object.emplace("two", std::move(var));
	set_variable(var, recycler, std::move(object));
	oref = { var, true, "three" };
	ref.reset(std::make_shared<Reference>(std::move(oref)));
	ptr = read_reference_opt(&immutable, ref);
	ASTERIA_TEST_CHECK(ptr == nullptr);
	ASTERIA_TEST_CHECK(var->get<D_object>().size() == 2);
	set_variable(nvar, recycler, D_integer(92));
	ASTERIA_TEST_CHECK_CATCH(write_reference_opt(ref, std::move(nvar)));
	ASTERIA_TEST_CHECK(var->get<D_object>().size() == 2);
	oref = { var, false, "three" };
	ref.reset(std::make_shared<Reference>(std::move(oref)));
	set_variable(nvar, recycler, D_integer(92));
	write_reference_opt(ref, std::move(nvar));
	ASTERIA_TEST_CHECK(var->get<D_object>().size() == 3);
	oref = { var, true, "three" };
	ref.reset(std::make_shared<Reference>(std::move(oref)));
	set_variable(nvar, recycler, D_integer(43));
	ASTERIA_TEST_CHECK_CATCH(write_reference_opt(ref, std::move(nvar)));

	oref = { var, false, "one" };
	ref.reset(std::make_shared<Reference>(std::move(oref)));
	ptr = read_reference_opt(&immutable, ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->get<D_integer>() == 1);
	set_variable(nvar, recycler, D_integer(97));
	write_reference_opt(ref, std::move(nvar));
	ASTERIA_TEST_CHECK(var->get<D_object>().at("one")->get<D_integer>() == 97);
}
