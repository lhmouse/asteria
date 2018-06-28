// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "utilities.hpp"
#include "recycler.hpp"
#include "value.hpp"
#include "opaque_base.hpp"
#include <iostream>

using namespace Asteria;

int main(){
	auto recycler = std::make_shared<Recycler>();
	auto backup = std::make_shared<Recycler>();

	Vp<Value> root, copy;
	Vp<Value> first, second, third, route;
	Vp<Value> temp;

	D_array arr;
	set_value(temp, recycler, D_null());
	arr.emplace_back(std::move(temp));
	set_value(temp, recycler, D_boolean(true));
	arr.emplace_back(std::move(temp));
	set_value(first, recycler, std::move(arr));

	arr.clear();
	set_value(temp, recycler, D_integer(42));
	arr.emplace_back(std::move(temp));
	set_value(temp, recycler, D_double(123.456));
	arr.emplace_back(std::move(temp));
	set_value(second, recycler, std::move(arr));

	arr.clear();
	set_value(temp, recycler, D_string("hello"));
	arr.emplace_back(std::move(temp));
	set_value(third, recycler, std::move(arr));

	D_object obj;
	obj.emplace(D_string::shallow("first"), std::move(first));
	obj.emplace(D_string::shallow("second"), std::move(second));
	set_value(route, recycler, std::move(obj));

	obj.clear();
	obj.emplace(D_string::shallow("third"), std::move(third));
	obj.emplace(D_string::shallow("route"), std::move(route));
	set_value(temp, recycler, D_string("世界"));
	obj.emplace(D_string::shallow("world"), std::move(temp));

	set_value(root, recycler, std::move(obj));
	copy_value(copy, backup, root);

	set_value(temp, recycler, D_string("my string"));
	copy->get<D_object>().emplace(D_string::shallow("new"), std::move(temp));

	std::cerr <<sp_fmt(root) <<std::endl;
	ASTERIA_DEBUG_LOG("---> ", "hello: ", 42);
	recycler.reset();
	std::cerr <<sp_fmt(copy) <<std::endl;
	ASTERIA_DEBUG_LOG("<--- ", "good bye: ", 43);
	std::cerr <<sp_fmt(root) <<std::endl;
}
