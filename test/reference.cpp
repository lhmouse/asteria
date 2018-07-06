// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/value.hpp"
#include "../src/stored_reference.hpp"

using namespace Asteria;

int main(){
	Vp<Value> value;
	set_value(value, D_string(D_string::shallow("meow")));
	Vp<Reference> ref;
	Reference::S_constant rsref = { value };
	set_reference(ref, std::move(rsref));
	auto ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->as<D_string>() == D_string::shallow("meow"));
	ASTERIA_TEST_CHECK_CATCH(set_value(drill_reference(ref), D_integer(42)));
	ASTERIA_TEST_CHECK(ptr.get() == value.get());

	Reference::S_temporary_value tmpref = { Vp<Value>(value.share()) };
	set_reference(ref, std::move(tmpref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->as<D_string>() == D_string::shallow("meow"));
	ASTERIA_TEST_CHECK_CATCH(set_value(drill_reference(ref), D_integer(63)));
	ASTERIA_TEST_CHECK(ptr.get() == value.get());

	auto var = std::make_shared<Variable>();
	set_value(var->mutate_value(), D_double(4.2));
	Reference::S_variable lref = { var };
	set_reference(ref, std::move(lref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr.get() == var->get_value_opt().get());

	Vp<Reference> ref_local;
	copy_reference(ref_local, ref);

	set_value(var->mutate_value(), true);
	ASTERIA_TEST_CHECK(var->get_value_opt()->which() == Value::type_boolean);
	Reference::S_array_element aref = { Vp<Reference>(ref_local.share()), 9 };
	set_reference(ref, std::move(aref));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	D_array array;
	for(int i = 0; i < 5; ++i){
		set_value(value, D_integer(i + 200));
		array.emplace_back(std::move(value));
	}
	set_value(var->mutate_value(), std::move(array));
	aref = { Vp<Reference>(ref_local.share()), 9 };
	set_reference(ref, std::move(aref));
	var->set_immutable(true);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr == nullptr);
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_array>().size() == 5);
	ASTERIA_TEST_CHECK_CATCH(set_value(drill_reference(ref), D_integer(67)));
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_array>().size() == 5);
	var->set_immutable(false);
	set_value(drill_reference(ref), D_integer(67));
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_array>().size() == 10);
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_array>().at(8) == nullptr);
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_array>().at(9)->as<D_integer>() == 67);
	aref = { Vp<Reference>(ref_local.share()), 9 };
	set_reference(ref, std::move(aref));
	var->set_immutable(true);
	ASTERIA_TEST_CHECK_CATCH(set_value(drill_reference(ref), D_integer(43)));

	aref = { Vp<Reference>(ref_local.share()), -7 };
	set_reference(ref, std::move(aref));
	var->set_immutable(false);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->as<D_integer>() == 203);
	set_value(drill_reference(ref), D_integer(65));
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_array>().at(3)->as<D_integer>() == 65);

	aref = { Vp<Reference>(ref_local.share()), 1 };
	set_reference(ref, std::move(aref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->as<D_integer>() == 201);
	set_value(drill_reference(ref), D_integer(26));
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_array>().at(1)->as<D_integer>() == 26);

	aref = { Vp<Reference>(ref_local.share()), -12 };
	set_reference(ref, std::move(aref));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr == nullptr);
	set_value(drill_reference(ref), D_integer(37));
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_array>().size() == 12);
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_array>().at(0)->as<D_integer>() == 37);
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_array>().at(3)->as<D_integer>() == 26);

	Reference::S_object_member oref = { Vp<Reference>(ref_local.share()), D_string::shallow("three") };
	set_reference(ref, std::move(oref));
	ASTERIA_TEST_CHECK_CATCH(read_reference_opt(ref));
	D_object object;
	set_value(value, D_integer(1));
	object.emplace(D_string::shallow("one"), std::move(value));
	set_value(value, D_integer(2));
	object.emplace(D_string::shallow("two"), std::move(value));
	set_value(var->mutate_value(), std::move(object));
	oref = { Vp<Reference>(ref_local.share()), D_string::shallow("three") };
	set_reference(ref, std::move(oref));
	var->set_immutable(true);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr == nullptr);
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_object>().size() == 2);
	ASTERIA_TEST_CHECK_CATCH(set_value(drill_reference(ref), D_integer(92)));
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_object>().size() == 2);
	oref = { Vp<Reference>(ref_local.share()), D_string::shallow("three") };
	set_reference(ref, std::move(oref));
	var->set_immutable(false);
	set_value(drill_reference(ref), D_integer(92));
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_object>().size() == 3);
	oref = { Vp<Reference>(ref_local.share()), D_string::shallow("three") };
	set_reference(ref, std::move(oref));
	var->set_immutable(true);
	ASTERIA_TEST_CHECK_CATCH(set_value(drill_reference(ref), D_integer(43)));

	oref = { Vp<Reference>(ref_local.share()), D_string::shallow("one") };
	set_reference(ref, std::move(oref));
	var->set_immutable(false);
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->as<D_integer>() == 1);
	set_value(drill_reference(ref), D_integer(97));
	ASTERIA_TEST_CHECK(var->get_value_opt()->as<D_object>().at(D_string::shallow("one"))->as<D_integer>() == 97);

	array.clear();
	set_value(value, D_string(D_string::shallow("first")));
	array.emplace_back(std::move(value));
	set_value(value, D_string(D_string::shallow("second")));
	array.emplace_back(std::move(value));
	set_value(value, D_string(D_string::shallow("third")));
	array.emplace_back(std::move(value));
	set_value(value, std::move(array));
	tmpref = { std::move(value) };
	set_reference(ref, std::move(tmpref));
	aref = { std::move(ref), 2 };
	set_reference(ref, std::move(aref));
	ASTERIA_TEST_CHECK_CATCH(set_value(drill_reference(ref), D_string(D_string::shallow("meow"))));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->as<D_string>() == D_string::shallow("third"));
	materialize_reference(ref, false);
	set_value(drill_reference(ref), D_string(D_string::shallow("meow")));
	ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr->as<D_string>() == D_string::shallow("meow"));
}
