// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_init.hpp"
#include "../src/insertable_ostream.hpp"

using namespace Asteria;

int main(){
	Insertable_ostream os;
	os.exceptions(std::ios_base::failbit | std::ios_base::eofbit);

	os <<"hello" <<42 <<"world" <<std::boolalpha <<true;
	ASTERIA_TEST_CHECK(os.get_string() == "hello42worldtrue");
	ASTERIA_TEST_CHECK(os.get_caret() == 16);

	os.set_string("meow");
	ASTERIA_TEST_CHECK(os.get_string() == "meow");
	ASTERIA_TEST_CHECK(os.get_caret() == 4);

	os.set_caret(3);
	os <<"woof";
	ASTERIA_TEST_CHECK(os.get_string() == "meowoofw");
	ASTERIA_TEST_CHECK(os.get_caret() == 7);
}
