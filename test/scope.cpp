// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/scope.hpp"
#include "../src/recycler.hpp"
#include "../src/stored_value.hpp"
#include "../src/stored_reference.hpp"
#include "../src/local_variable.hpp"
#include "../src/function_base.hpp"

using namespace Asteria;

namespace {
	int g_fancy_value = 42;

	class Fancy_deferred_callback : public Function_base {
	private:
		int m_multiplier;
		int m_addend;

	public:
		Fancy_deferred_callback(int multiplier, int addend)
			: Function_base(String::shallow("fancy deferred callback"))
			, m_multiplier(multiplier), m_addend(addend)
		{ }

	public:
		void invoke(Xptr<Reference> &/*result_out*/, Sparg<Recycler> /*recycler*/, Xptr<Reference> &&/*this_opt*/, Xptr_vector<Reference> &&/*arguments_opt*/) const override {
			g_fancy_value = g_fancy_value * m_multiplier + m_addend;
		}
	};
}

int main(){
	const auto recycler = std::make_shared<Recycler>();

	auto scope = std::make_shared<Scope>(Scope::purpose_plain, nullptr);
	auto one = std::make_shared<Local_variable>();
	set_variable(one->drill_for_variable(), recycler, D_integer(42));
	one->set_constant(true);
	Reference::S_local_variable lref = { one };
	auto wref = scope->drill_for_local_reference(String::shallow("one"));
	set_reference(wref, std::move(lref));

	auto ref = scope->get_local_reference_opt(String::shallow("one"));
	ASTERIA_TEST_CHECK(ref);
	auto ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr.get() == one->get_variable_opt().get());

	ref = scope->get_local_reference_opt(String::shallow("nonexistent"));
	ASTERIA_TEST_CHECK(ref == nullptr);

	scope->defer_callback(std::make_shared<Fancy_deferred_callback>(3, -220)); // 78 * 3 - 220 = 14
	scope->defer_callback(std::make_shared<Fancy_deferred_callback>(2, -100)); // 89 * 2 - 100 = 78
	scope->defer_callback(std::make_shared<Fancy_deferred_callback>(2,    5)); // 42 * 2 +   5 = 89
	scope.reset();
	ASTERIA_TEST_CHECK(g_fancy_value == 14);
}
