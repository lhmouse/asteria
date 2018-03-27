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
	arr.emplace_back(make_value<Variable>(true));
	auto first = make_value<Variable>(std::move(arr));
	arr.clear();
	arr.emplace_back(make_value<Variable>((std::int64_t)42));
	arr.emplace_back(make_value<Variable>(123.45));
	auto second = make_value<Variable>(std::move(arr));
	arr.clear();
	arr.emplace_back(make_value<Variable>(std::string("hello")));
	arr.emplace_back(make_value<Variable>(std::make_pair("opaque", std::make_shared<int>())));
	auto third = make_value<Variable>(std::move(arr));
	Object obj;
	obj.emplace("first", std::move(first));
	obj.emplace("second", std::move(second));
	auto route = make_value<Variable>(std::move(obj));
	obj.clear();
	obj.emplace("route", std::move(route));
	obj.emplace("third", std::move(third));
	obj.emplace("world", make_value<Variable>(std::string("世界")));
	auto root = make_value<Variable>(std::move(obj));
	std::cerr <<root <<std::endl;
	ASTERIA_DEBUG_LOG("<--- ", "good bye: ", 43);
}
