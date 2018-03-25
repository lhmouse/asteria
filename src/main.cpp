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
	arr.emplace_back(std::make_shared<Variable>(nullptr, true));
	arr.emplace_back(std::make_shared<Variable>(true, false));
	auto first = std::make_shared<Variable>(std::move(arr), true);
	arr.clear();
	arr.emplace_back(std::make_shared<Variable>((std::int64_t)42, false));
	arr.emplace_back(std::make_shared<Variable>(123.45, true));
	auto second = std::make_shared<Variable>(std::move(arr), false);
	arr.clear();
	arr.emplace_back(std::make_shared<Variable>(std::string("hello"), true));
	arr.emplace_back(std::make_shared<Variable>(std::make_shared<int>(), false));
	auto third = std::make_shared<Variable>(std::move(arr), true);
	Object obj;
	obj.emplace("first", std::move(first));
	obj.emplace("second", std::move(second));
	auto route = std::make_shared<Variable>(std::move(obj), false);
	obj.clear();
	obj.emplace("route", std::move(route));
	obj.emplace("third", std::move(third));
	obj.emplace("world", std::make_shared<Variable>(std::string("world 这里有中文"), true));
	auto root = std::make_shared<Variable>(std::move(obj), false);
	std::cerr <<root <<std::endl;
	ASTERIA_DEBUG_LOG("<--- ", "good byte: ", 43);
}
