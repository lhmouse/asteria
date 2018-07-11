// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/scope.hpp"
#include "../src/value.hpp"
#include "../src/stored_reference.hpp"
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
			: m_multiplier(multiplier), m_addend(addend)
		{ }

	public:
		Cow_string describe() const override {
			return Cow_string::shallow("fancy deferred callback");
		}
		void invoke(Vp<Reference> &result_out, Vp<Reference> &&this_opt, Vector<Vp<Reference>> &&args) const override {
			result_out.reset();
			(void)this_opt;
			(void)args;
			g_fancy_value = g_fancy_value * m_multiplier + m_addend;
		}
	};
}

int main(){
	auto scope = std::make_shared<Scope>(Scope::purpose_plain, nullptr);
	auto one = std::make_shared<Variable>();
	set_value(one->mutate_value(), D_integer(42));
	one->set_immutable(true);
	Reference::S_variable lref = { one };
	auto wref = scope->mutate_named_reference(D_string::shallow("one"));
	set_reference(wref, std::move(lref));

	auto ref = scope->get_named_reference_opt(D_string::shallow("one"));
	ASTERIA_TEST_CHECK(ref);
	auto ptr = read_reference_opt(ref);
	ASTERIA_TEST_CHECK(ptr);
	ASTERIA_TEST_CHECK(ptr.get() == one->get_value_opt().get());

	ref = scope->get_named_reference_opt(D_string::shallow("nonexistent"));
	ASTERIA_TEST_CHECK(ref == nullptr);

	scope->defer_callback(std::make_shared<Fancy_deferred_callback>(3, -220)); // 78 * 3 - 220 = 14
	scope->defer_callback(std::make_shared<Fancy_deferred_callback>(2, -100)); // 89 * 2 - 100 = 78
	scope->defer_callback(std::make_shared<Fancy_deferred_callback>(2,    5)); // 42 * 2 +   5 = 89
	scope.reset();
	ASTERIA_TEST_CHECK(g_fancy_value == 14);
}
