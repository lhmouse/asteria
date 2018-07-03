// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/recycler.hpp"
#include "../src/value.hpp"

using namespace Asteria;

int main(){
	auto recycler = std::make_shared<Recycler>();

	Vp<Value> obj, value;
	set_value(obj, recycler, D_object());
	set_value(value, recycler, D_integer(42));
	auto pair = obj->get<D_object>().emplace(D_string::shallow("int"), std::move(value));
	auto weak_int = pair.first->second.weaken();
	set_value(value, recycler, D_string(D_string::shallow("hello")));
	pair = obj->get<D_object>().emplace(D_string::shallow("str"), std::move(value));
	auto weak_str = pair.first->second.weaken();

	ASTERIA_TEST_CHECK(weak_int.expired() == false);
	ASTERIA_TEST_CHECK(weak_str.expired() == false);
	recycler.reset();
	ASTERIA_TEST_CHECK(weak_int.expired() == true);
	ASTERIA_TEST_CHECK(weak_str.expired() == true);
}
