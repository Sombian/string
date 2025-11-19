#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"

#include <utility>
#include <variant>

#include "string.hpp"

TEST_CASE("invariant")
{
	SUBCASE("empty string")
	{
		c_str str {/* none */};

		CHECK(str.size() == 0);
	}

	SUBCASE("0 < str.size()")
	{
		c_str str {"foo bar"};

		CHECK(str.size() == 8);
	}
}

TEST_CASE("equaility")
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
		CHECK(foo + foo == "foofoo");
		CHECK("foofoo" == foo + foo);

		CHECK(foo + bar == "foobar");
		CHECK("foobar" == foo + bar);
	}
}

TEST_CASE("file_of")
{
	SUBCASE("UTF-8")
	{
		c_str path {"sample/utf8.txt"};

		if (auto file {file_of(path)})
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

		if (auto file {file_of(path)})
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

		if (auto file {file_of(path)})
		{
			std::visit([&](auto&& _)
			{
				CHECK(std::is_same_v<decltype(_), utf16>);
			},
			std::move(file.value()));
		}
	}
}
