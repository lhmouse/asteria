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
	arr.emplace_back(std::make_shared<Variable>(true));
	auto first = std::make_shared<Variable>(std::move(arr));
	arr.clear();
	arr.emplace_back(std::make_shared<Variable>(std::int64_t(42)));
	arr.emplace_back(std::make_shared<Variable>(123.45));
	auto second = std::make_shared<Variable>(std::move(arr));
	arr.clear();
	arr.emplace_back(std::make_shared<Variable>(std::string("hello")));
	arr.emplace_back(std::make_shared<Variable>(Opaque{"opaque", std::make_shared<int>()}));
	auto third = std::make_shared<Variable>(std::move(arr));
	Object obj;
	obj.emplace("first", std::move(first));
	obj.emplace("second", std::move(second));
	auto route = std::make_shared<Variable>(std::move(obj));
	obj.clear();
	obj.emplace("route", std::move(route));
	obj.emplace("third", std::move(third));
	obj.emplace("world", std::make_shared<Variable>(std::string("世界")));
	auto root = std::make_shared<Variable>(std::move(obj));
	std::cerr <<root <<std::endl;
	ASTERIA_DEBUG_LOG("<--- ", "good bye: ", 43);
}
