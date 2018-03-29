// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "stored_value.hpp"
#include "misc.hpp"
#include "recycler.hpp"
#include <iostream>

using namespace Asteria;

int main(){
	const auto recycler = std::make_shared<Recycler>();
	Array arr;
	arr.emplace_back(nullptr);
	arr.emplace_back(recycler->create_variable_opt(true));
	auto first = recycler->create_variable_opt(std::move(arr));
	arr.clear();
	arr.emplace_back(recycler->create_variable_opt(std::int64_t(42)));
	arr.emplace_back(recycler->create_variable_opt(123.45));
	auto second = recycler->create_variable_opt(std::move(arr));
	arr.clear();
	arr.emplace_back(recycler->create_variable_opt(std::string("hello")));
	arr.emplace_back(recycler->create_variable_opt(Opaque{"opaque", std::make_shared<int>()}));
	auto third = recycler->create_variable_opt(std::move(arr));
	Object obj;
	obj.emplace("first", std::move(first));
	obj.emplace("second", std::move(second));
	auto route = recycler->create_variable_opt(std::move(obj));
	obj.clear();
	obj.emplace("route", std::move(route));
	obj.emplace("third", std::move(third));
	obj.emplace("world", recycler->create_variable_opt(std::string("世界")));
	auto root = recycler->create_variable_opt(std::move(obj));
	auto root2 = recycler->create_variable_opt(*root);
	std::cerr <<root <<std::endl;
	ASTERIA_DEBUG_LOG("---> ", "hello: ", 42);
	recycler->clear_variables();
	std::cerr <<root2 <<std::endl;
	ASTERIA_DEBUG_LOG("---- ", "working");
	std::cerr <<root <<std::endl;
	ASTERIA_DEBUG_LOG("<--- ", "good bye: ", 43);
	std::cerr <<root2 <<std::endl;
}
