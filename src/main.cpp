// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "misc.hpp"
#include <iostream>

using namespace Asteria;

int main(){
	ASTERIA_DEBUG_LOG("---> ", "hello: ", 42);
	Array arr;
	arr.emplace_back(nullptr);
	arr.emplace_back(create_variable_opt(true));
	auto first = create_variable_opt(std::move(arr));
	arr.clear();
	arr.emplace_back(create_variable_opt((std::int64_t)42));
	arr.emplace_back(create_variable_opt(123.45));
	auto second = create_variable_opt(std::move(arr));
	arr.clear();
	arr.emplace_back(create_variable_opt(std::string("hello")));
	arr.emplace_back(create_variable_opt(std::make_pair("opaque", std::make_shared<int>())));
	auto third = create_variable_opt(std::move(arr));
	Object obj;
	obj.emplace("first", std::move(first));
	obj.emplace("second", std::move(second));
	auto route = create_variable_opt(std::move(obj));
	obj.clear();
	obj.emplace("route", std::move(route));
	obj.emplace("third", std::move(third));
	obj.emplace("world", create_variable_opt(std::string("世界")));
	auto root = create_variable_opt(std::move(obj));
	std::cerr <<root <<std::endl;
	ASTERIA_DEBUG_LOG("<--- ", "good bye: ", 43);
}
