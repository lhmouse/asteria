// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "stored_value.hpp"
#include "utilities.hpp"
#include "recycler.hpp"
#include <iostream>

using namespace Asteria;

int main(){
	const auto recycler = std::make_shared<Recycler>();
	const auto backup = std::make_shared<Recycler>();

	Xptr<Variable> root, copy;
	Xptr<Variable> first, second, third, route;
	Xptr<Variable> temp;

	D_array arr;
	recycler->set_variable(temp, nullptr);
	arr.emplace_back(std::move(temp));
	recycler->set_variable(temp, true);
	arr.emplace_back(std::move(temp));
	recycler->set_variable(first, std::move(arr));

	arr.clear();
	recycler->set_variable(temp, INT64_C(42));
	arr.emplace_back(std::move(temp));
	recycler->set_variable(temp, 123.456);
	arr.emplace_back(std::move(temp));
	recycler->set_variable(second, std::move(arr));

	arr.clear();
	recycler->set_variable(temp, std::string("hello"));
	arr.emplace_back(std::move(temp));
	recycler->set_variable(third, std::move(arr));

	D_object obj;
	obj.emplace("first", std::move(first));
	obj.emplace("second", std::move(second));
	recycler->set_variable(route, std::move(obj));

	obj.clear();
	obj.emplace("third", std::move(third));
	obj.emplace("route", std::move(route));
	recycler->set_variable(temp, std::string("世界"));
	obj.emplace("world", std::move(temp));

	recycler->set_variable(root, std::move(obj));
	backup->copy_variable_recursive(copy, root);

	Opaque_struct opaque = { { 0x12,0x34,0x56,0x78,0x9A,0xBC,0xDE,0xF0 }, 987654321, std::make_shared<int>() };
	backup->set_variable(temp, std::move(opaque));
	copy->get<D_object>().emplace("opaque", std::move(temp));

	std::cerr <<root <<std::endl;
	ASTERIA_DEBUG_LOG("---> ", "hello: ", 42);
	recycler->clear_variables();
	std::cerr <<copy <<std::endl;
	ASTERIA_DEBUG_LOG("<--- ", "good bye: ", 43);
	std::cerr <<root <<std::endl;
}
