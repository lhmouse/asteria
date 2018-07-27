// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "utilities.hpp"
#include "value.hpp"
#include "opaque_base.hpp"
#include <iostream>

using namespace Asteria;

int main()
{
	D_array arr;
	arr.emplace_back(D_null());
	arr.emplace_back(D_boolean(true));
	Value first(std::move(arr));

	arr.clear();
	arr.emplace_back(D_integer(42));
	arr.emplace_back(D_double(123.456));
	Value second(std::move(arr));

	arr.clear();
	arr.emplace_back(D_string("hello"));
	Value third(std::move(arr));

	D_object obj;
	obj.emplace(D_string::shallow("first"), std::move(first));
	obj.emplace(D_string::shallow("second"), std::move(second));
	Value route(std::move(obj));

	obj.clear();
	obj.emplace(D_string::shallow("third"), std::move(third));
	obj.emplace(D_string::shallow("route"), std::move(route));
	obj.emplace(D_string::shallow("world"), D_string("世界"));
	Value root(std::move(obj));

	Value copy(root);
	copy.as<D_object>().emplace(D_string::shallow("new"), D_string("my string"));

	std::cerr <<root <<std::endl;
	ASTERIA_DEBUG_LOG("---> ", "hello: ", 42);
	std::cerr <<copy <<std::endl;
	ASTERIA_DEBUG_LOG("<--- ", "good bye: ", 43);
	std::cerr <<root <<std::endl;
}
