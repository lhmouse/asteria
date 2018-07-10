// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "utilities.hpp"
#include "value.hpp"
#include "opaque_base.hpp"
#include <iostream>

using namespace Asteria;

int main(){
	Value root, copy;
	Value first, second, third, route;
	Value temp;

	D_array arr;
	clear_value(temp);
	arr.emplace_back(std::move(temp));
	set_value(temp, D_boolean(true));
	arr.emplace_back(std::move(temp));
	set_value(first, std::move(arr));

	arr.clear();
	set_value(temp, D_integer(42));
	arr.emplace_back(std::move(temp));
	set_value(temp, D_double(123.456));
	arr.emplace_back(std::move(temp));
	set_value(second, std::move(arr));

	arr.clear();
	set_value(temp, D_string("hello"));
	arr.emplace_back(std::move(temp));
	set_value(third, std::move(arr));

	D_object obj;
	obj.emplace(D_string::shallow("first"), std::move(first));
	obj.emplace(D_string::shallow("second"), std::move(second));
	set_value(route, std::move(obj));

	obj.clear();
	obj.emplace(D_string::shallow("third"), std::move(third));
	obj.emplace(D_string::shallow("route"), std::move(route));
	set_value(temp, D_string("世界"));
	obj.emplace(D_string::shallow("world"), std::move(temp));

	set_value(root, std::move(obj));
	copy_value(copy, root);

	set_value(temp, D_string("my string"));
	copy->as<D_object>().emplace(D_string::shallow("new"), std::move(temp));

	std::cerr <<root <<std::endl;
	ASTERIA_DEBUG_LOG("---> ", "hello: ", 42);
	std::cerr <<copy <<std::endl;
	ASTERIA_DEBUG_LOG("<--- ", "good bye: ", 43);
	std::cerr <<root <<std::endl;
}
