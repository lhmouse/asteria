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
	const auto recycler = create_shared<Recycler>();

	Value_ptr<Variable> root, copy;
	Value_ptr<Variable> first, second, third, route;
	Value_ptr<Variable> temp;

	Array arr;
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
	recycler->set_variable(temp, Opaque{"opaque", create_shared<int>()});
	arr.emplace_back(std::move(temp));
	recycler->set_variable(third, std::move(arr));

	Object obj;
	obj.emplace("first", std::move(first));
	obj.emplace("second", std::move(second));
	recycler->set_variable(route, std::move(obj));

	obj.clear();
	obj.emplace("third", std::move(third));
	obj.emplace("route", std::move(route));
	recycler->set_variable(temp, std::string("世界"));
	obj.emplace("world", std::move(temp));

	recycler->set_variable(root, std::move(obj));
	//recycler->copy_variable(copy, root);
	copy = Value_ptr<Variable>(create_shared<Variable>(*root));

	std::cerr <<root <<std::endl;
	ASTERIA_DEBUG_LOG("---> ", "hello: ", 42);
	recycler->clear_variables();
	std::cerr <<copy <<std::endl;
	ASTERIA_DEBUG_LOG("<--- ", "good bye: ", 43);
	std::cerr <<root <<std::endl;
}
