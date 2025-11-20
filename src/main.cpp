#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"

#include <variant>

#include "string.hpp"

TEST_CASE("storage")
{
	c_str small {"ABCDEFGHIJKLMNOPQRSTUVW"};
	c_str large {"ÁBCDEFGHIJKLMNOPQRSTUVW"};

	CHECK(small.size() == small.capacity());
	CHECK(large.size() == large.capacity());
}

TEST_CASE("equality")
{
	c_str foo {"foo"};
	c_str bar {"bar"};

	SUBCASE("A == B")
	{
		CHECK(foo == "foo");
		CHECK("foo" == foo);

		CHECK(foo != "bar");
		CHECK("bar" != foo);
	}

	SUBCASE("A + A")
	{
		c_str foo_foo {foo + foo};

		CHECK(foo_foo == "foofoo");
		CHECK("foofoo" == foo_foo);

		c_str foo_bar {foo + bar};

		CHECK(foo_bar == "foobar");
		CHECK("foobar" == foo_bar);
	}

	SUBCASE("slice")
	{
		utf8 str {u8"티라미수☆치즈케잌"};

		auto parts {str.split(u8"☆")};

		const auto 티라미수 {parts[0]};
		const auto 치즈케잌 {parts[1]};

		CHECK(티라미수 == u8"티라미수");
		CHECK(티라미수.length() == 4);

		CHECK(치즈케잌 == u8"치즈케잌");
		CHECK(치즈케잌.length() == 4);
	}
}

TEST_CASE("subscript")
{
	utf8 str {u8"샤인모수끼"};

	SUBCASE("read & write")
	{
		CHECK((str[0] = U'치') == U'치');
		CHECK((str[1] = U'즈') == U'즈');
		CHECK((str[2] = U'케') == U'케');
		CHECK((str[3] = U'잌') == U'잌');
	}

	SUBCASE("(clamp, clamp)")
	{
		using range::N;

		CHECK(str[N - 3, N - 2] == u8"모");
	}

	SUBCASE("(clamp, range)")
	{
		using range::N;

		CHECK(str[N - 3, N] == u8"모수끼");
	}

	SUBCASE("(size_t, clamp)")
	{
		using range::N;

		CHECK(str[2, N - 1] == u8"모수");
	}

	SUBCASE("(size_t, range)")
	{
		using range::N;

		CHECK(str[2, N] == u8"모수끼");
	}

	SUBCASE("(size_t, size_t)")
	{
		using range::N;

		CHECK(str[2, 3] == u8"모");
	}
}

TEST_CASE("fileof")
{
	SUBCASE("UTF-8")
	{
		const auto file {fileof("./src/sample/utf8.txt")};

		REQUIRE(file.has_value());

		CHECK(std::holds_alternative<utf8>(file.value()));
	}

	SUBCASE("UTF-16-LE")
	{
		const auto file {fileof("./src/sample/utf16le.txt")};

		REQUIRE(file.has_value());

		CHECK(std::holds_alternative<utf16>(file.value()));
	}

	SUBCASE("UTF-16-BE")
	{
		const auto file {fileof("./src/sample/utf16be.txt")};

		REQUIRE(file.has_value());

		CHECK(std::holds_alternative<utf16>(file.value()));
	}
}
