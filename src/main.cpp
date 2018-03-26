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
	arr.emplace_back(make_value<Variable>(nullptr, true));
	arr.emplace_back(make_value<Variable>(true, false));
	auto first = make_value<Variable>(std::move(arr), true);
	arr.clear();
	arr.emplace_back(make_value<Variable>((std::int64_t)42, false));
	arr.emplace_back(make_value<Variable>(123.45, true));
	auto second = make_value<Variable>(std::move(arr), false);
	arr.clear();
	arr.emplace_back(make_value<Variable>(std::string("hello"), true));
	arr.emplace_back(make_value<Variable>(std::make_shared<int>(), false));
	auto third = make_value<Variable>(std::move(arr), true);
	Object obj;
	obj.emplace("first", std::move(first));
	obj.emplace("second", std::move(second));
	auto route = make_value<Variable>(std::move(obj), false);
	obj.clear();
	obj.emplace("route", std::move(route));
	obj.emplace("third", std::move(third));
	obj.emplace("world", make_value<Variable>(std::string("世界"), true));
	auto root = make_value<Variable>(std::move(obj), false);
	std::cerr <<*root <<std::endl;
	ASTERIA_DEBUG_LOG("<--- ", "good bye: ", 43);
}
