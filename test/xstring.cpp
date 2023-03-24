// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../rocket/xstring.hpp"
using namespace ::rocket;

template<typename charT>
void
test_xstring()
  {
    constexpr charT hello[] = { 'h','e','l','l','o',0 };
    constexpr charT meowTTThello[] = { 'm','e','o','w','T','T','T','h','e','l','l','o',0 };

    static_assert(xstrlen(hello + 5) == 0);
    ASTERIA_TEST_CHECK(xstrlen((volatile const charT*) hello + 5) == 0);
    static_assert(xstrlen(hello) == 5);
    ASTERIA_TEST_CHECK(xstrlen((volatile const charT*) hello) == 5);

    static_assert(xstrchr(hello, 'l') == hello + 2);
    ASTERIA_TEST_CHECK(xstrchr((volatile const charT*) hello, 'l') == hello + 2);
    static_assert(xstrchr(hello, 0) == hello + 5);
    ASTERIA_TEST_CHECK(xstrchr((volatile const charT*) hello, 0) == hello + 5);
    static_assert(xstrchr(hello, 'a') == nullptr);
    ASTERIA_TEST_CHECK(xstrchr((volatile const charT*) hello, 'a') == nullptr);

    static_assert(xmemchr(hello, 'l', 2) == nullptr);
    ASTERIA_TEST_CHECK(xmemchr((volatile const charT*) hello, 'l', 2) == nullptr);
    static_assert(xmemchr(hello, 'l', 5) == hello + 2);
    ASTERIA_TEST_CHECK(xmemchr((volatile const charT*) hello, 'l', 5) == hello + 2);
    static_assert(xmemchr(hello, 0, 5) == nullptr);
    ASTERIA_TEST_CHECK(xmemchr((volatile const charT*) hello, 0, 5) == nullptr);
    static_assert(xmemchr(hello, 'a', 5) == nullptr);
    ASTERIA_TEST_CHECK(xmemchr((volatile const charT*) hello, 'a', 5) == nullptr);

    charT temp[100] = { };
    charT* pos = temp;
    charT* r = xmemrpcpy(pos, meowTTThello, 4);
    ASTERIA_TEST_CHECK(r == pos);
    ASTERIA_TEST_CHECK(r == temp + 4);
    ASTERIA_TEST_CHECK(*r == 0);
    ASTERIA_TEST_CHECK(::memcmp(temp, meowTTThello, sizeof(charT) * 4) == 0);
    ASTERIA_TEST_CHECK(xmemcmp(temp, meowTTThello, 4) == 0);
    ASTERIA_TEST_CHECK(xmemcmp(temp, meowTTThello, 5) < 0);
    ASTERIA_TEST_CHECK(xmemcmp(meowTTThello, temp, 5) > 0);

    r = xmemrpset(pos, 'T', 3);
    ASTERIA_TEST_CHECK(r == pos);
    ASTERIA_TEST_CHECK(r == temp + 7);
    ASTERIA_TEST_CHECK(*r == 0);
    ASTERIA_TEST_CHECK(::memcmp(temp, meowTTThello, sizeof(charT) * 7) == 0);
    ASTERIA_TEST_CHECK(xmemcmp(temp, meowTTThello, 7) == 0);
    ASTERIA_TEST_CHECK(xmemcmp(temp, meowTTThello, 8) < 0);
    ASTERIA_TEST_CHECK(xmemcmp(meowTTThello, temp, 8) > 0);

    r = xstrrpcpy(pos, hello);
    ASTERIA_TEST_CHECK(r == pos);
    ASTERIA_TEST_CHECK(r == temp + 12);
    ASTERIA_TEST_CHECK(*r == 0);
    ASTERIA_TEST_CHECK(::memcmp(temp, meowTTThello, sizeof(charT) * 13) == 0);
    ASTERIA_TEST_CHECK(xmemcmp(temp, meowTTThello, 13) == 0);
  }

int main()
  {
    test_xstring<char>();
    test_xstring<signed char>();
    test_xstring<unsigned char>();
    test_xstring<wchar_t>();
    test_xstring<char16_t>();
    test_xstring<char32_t>();
    test_xstring<long long>();
  }
