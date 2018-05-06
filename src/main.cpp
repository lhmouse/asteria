// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "stored_value.hpp"
#include "utilities.hpp"
#include "recycler.hpp"
#include "opaque_base.hpp"
#include <iostream>

using namespace Asteria;

namespace {
	class My_opaque : public Opaque_base {
	public:
		const char * describe() const noexcept override {
			return "my fancy opaque class";
		}
	};
}

int main(){
	auto recycler = std::make_shared<Recycler>();
	auto backup = std::make_shared<Recycler>();

	Xptr<Variable> root, copy;
	Xptr<Variable> first, second, third, route;
	Xptr<Variable> temp;

	D_array arr;
	set_variable(temp, recycler, nullptr);
	arr.emplace_back(std::move(temp));
	set_variable(temp, recycler, true);
	arr.emplace_back(std::move(temp));
	set_variable(first, recycler, std::move(arr));

	arr.clear();
	set_variable(temp, recycler, INT64_C(42));
	arr.emplace_back(std::move(temp));
	set_variable(temp, recycler, 123.456);
	arr.emplace_back(std::move(temp));
	set_variable(second, recycler, std::move(arr));

	arr.clear();
	set_variable(temp, recycler, std::string("hello"));
	arr.emplace_back(std::move(temp));
	set_variable(third, recycler, std::move(arr));

	D_object obj;
	obj.emplace("first", std::move(first));
	obj.emplace("second", std::move(second));
	set_variable(route, recycler, std::move(obj));

	obj.clear();
	obj.emplace("third", std::move(third));
	obj.emplace("route", std::move(route));
	set_variable(temp, recycler, std::string("世界"));
	obj.emplace("world", std::move(temp));

	set_variable(root, recycler, std::move(obj));
	copy_variable(copy, backup, root);

	D_opaque opaque;
	opaque.reset(std::make_shared<My_opaque>());
	set_variable(temp, backup, std::move(opaque));
	copy->get<D_object>().emplace("opaque", std::move(temp));

	std::cerr <<root <<std::endl;
	ASTERIA_DEBUG_LOG("---> ", "hello: ", 42);
	recycler.reset();
	std::cerr <<copy <<std::endl;
	ASTERIA_DEBUG_LOG("<--- ", "good bye: ", 43);
	std::cerr <<root <<std::endl;
}
