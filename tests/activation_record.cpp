// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/activation_record.hpp"
#include "../src/object.hpp"

using namespace Asteria;

int main(){
	const auto parent = std::make_shared<ActivationRecord>("parent", nullptr);
	TEST_CHECK(parent->get_name() == "parent");
	TEST_CHECK(parent->get_parent() == nullptr);

	const auto child = std::make_shared<ActivationRecord>("child", parent);
	TEST_CHECK(child->get_name() == "child");
	TEST_CHECK(child->get_parent() == parent);

	const auto key1 = std::string("key1");
	TEST_CHECK(child->has_object(key1) == false);
	TEST_CHECK(child->get_object(key1) == nullptr);

	const auto object1 = std::make_shared<Object>();
	child->set_object(key1, object1);
	TEST_CHECK(child->has_object(key1) != false);
	TEST_CHECK(child->get_object(key1) == object1);

	const auto key2 = std::string("key2");
	const auto object2 = std::make_shared<Object>();
	child->set_object(key2, object2);
	child->set_object(key1, nullptr);
	TEST_CHECK(child->has_object(key2) != false);
	TEST_CHECK(child->get_object(key2) == object2);
	TEST_CHECK(child->has_object(key1) == false);
	TEST_CHECK(child->get_object(key1) == nullptr);
}
