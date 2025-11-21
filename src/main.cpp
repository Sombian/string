#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"

#include <variant>
#include <utility>

#include "string.hpp"

TEST_CASE("storage")
{
	c_str small {"ABCDEFGHIJKLMNOPQRSTUVW"};
	c_str large {"ÁBCDEFGHIJKLMNOPQRSTUVW"};

	CHECK(small.size() == small.capacity());
	CHECK(large.size() == large.capacity());
}

TEST_CASE("[API] string")
{
	SUBCASE("impl + ???")
	{
		const utf8 foo {u8"티라"};
		const utf8 bar {u8"미수"};

		auto foo_foo {foo + foo};
		auto foo_bar {foo + bar};

		CHECK(foo_foo == u8"티라티라");
		CHECK(foo_foo.length() == 4);

		CHECK(foo_bar == u8"티라미수");
		CHECK(foo_bar.length() == 4);
	}

	SUBCASE("impl.split")
	{
		const utf8 str {u8"티라미수"
		                  "☆"
		                  "치즈케잌"
		                  "☆"
		                  "말차라떼"
		                  "☆"
		                  "딸기우유"};

		// split by utf-16 literal
		auto src {str.split(u"☆")};

		CHECK(src[0] == u"티라미수");
		CHECK(src[0] == U"티라미수");
		CHECK(src[1] == u"치즈케잌");
		CHECK(src[1] == U"치즈케잌");
		CHECK(src[2] == u"말차라떼");
		CHECK(src[2] == U"말차라떼");
		CHECK(src[3] == u"딸기우유");
		CHECK(src[3] == U"딸기우유");
	}

	SUBCASE("range syntax")
	{
		// init from utf-16 literal
		const utf8 str {u"샤인모수끼"};

		using range::N;

		CHECK(str[N - 3, N - 2] == u8"모");
		CHECK(str[N - 3, N]  == u8"모수끼");
		CHECK(str[2, N - 0]  == u8"모수끼");
		CHECK(str[2, N]      == u8"모수끼");
		CHECK(str[2, 3]         == u8"모");
	}
}

TEST_CASE("[API] fileof")
{
	SUBCASE("UTF-8")
	{
		const auto file {fileof(u8"./src/sample/utf8.txt")};

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
