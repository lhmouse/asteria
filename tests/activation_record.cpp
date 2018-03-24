// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/activation_record.hpp"
#include "../src/variable.hpp"

using namespace Asteria;

int main(){
	const auto parent = std::make_shared<ActivationRecord>("parent", nullptr);
	ASTERIA_TEST_CHECK(parent->get_name() == "parent");
	ASTERIA_TEST_CHECK(parent->get_parent() == nullptr);

	const auto child = std::make_shared<ActivationRecord>("child", parent);
	ASTERIA_TEST_CHECK(child->get_name() == "child");
	ASTERIA_TEST_CHECK(child->get_parent() == parent);

	const auto key1 = std::string("key1");
	ASTERIA_TEST_CHECK(child->has_variable(key1) == false);
	ASTERIA_TEST_CHECK(child->get_variable(key1) == nullptr);

	const auto var1 = std::make_shared<Variable>();
	child->set_variable(key1, var1);
	ASTERIA_TEST_CHECK(child->has_variable(key1) != false);
	ASTERIA_TEST_CHECK(child->get_variable(key1) == var1);

	const auto key2 = std::string("key2");
	const auto var2 = std::make_shared<Variable>();
	child->set_variable(key2, var2);
	child->set_variable(key1, nullptr);
	ASTERIA_TEST_CHECK(child->has_variable(key2) != false);
	ASTERIA_TEST_CHECK(child->get_variable(key2) == var2);
	ASTERIA_TEST_CHECK(child->has_variable(key1) == false);
	ASTERIA_TEST_CHECK(child->get_variable(key1) == nullptr);
}
