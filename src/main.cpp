#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"

#include <utility>
#include <variant>

#include "string.hpp"

TEST_CASE("capacity")
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

	#ifndef _MSC_VER
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
	#endif//_MSC_VER
}

TEST_CASE("fileof")
{
	SUBCASE("UTF-8")
	{
		c_str path {"sample/utf8.txt"};

		if (auto file {fileof(path)})
		{
			std::visit([&](auto&& _)
			{
				CHECK(std::is_same_v<decltype(_), utf8>);
			},
			std::move(file.value()));
		}
	}

	SUBCASE("UTF-16-LE")
	{
		c_str path {"sample/utf16le.txt"};

		if (auto file {fileof(path)})
		{
			std::visit([&](auto&& _)
			{
				CHECK(std::is_same_v<decltype(_), utf16>);
			},
			std::move(file.value()));
		}
	}

	SUBCASE("UTF-16-BE")
	{
		c_str path {"test/utf16be.txt"};

		if (auto file {fileof(path)})
		{
			std::visit([&](auto&& _)
			{
				CHECK(std::is_same_v<decltype(_), utf16>);
			},
			std::move(file.value()));
		}
	}
}
