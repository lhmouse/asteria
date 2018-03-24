// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "misc.hpp"
#include "activation_record.hpp"

using namespace Asteria;

int main(){
	const auto ar = std::make_shared<ActivationRecord>("meow", nullptr);
	DEBUG_PRINTF("new AR: %p\n", (void *)ar.get());
}
