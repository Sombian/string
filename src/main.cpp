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
	SUBCASE("concatenation")
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

	SUBCASE("split & match")
	{
		const utf8 str {u8"티라미수"
		                u8"☆"
		                u8"치즈케잌"
		                u8"☆"
		                u8"말차라떼"
		                u8"☆"
		                u8"딸기우유"};

		auto split {str.split(u8"☆")};

		CHECK(split[0] == u"티라미수");
		CHECK(split[1] == u"치즈케잌");
		CHECK(split[2] == u"말차라떼");
		CHECK(split[3] == u"딸기우유");

		auto match {str.match(u8"☆")};

		CHECK(match[0] == u"☆");
		CHECK(match[1] == u"☆");
		CHECK(match[2] == u"☆");
	}
}

TEST_CASE("[API] fileof")
{
	SUBCASE("UTF-8")
	{
		const auto file {fileof("./src/sample/utf8.txt")};

		REQUIRE(file.has_value());

		CHECK(std::holds_alternative<utf8>(file.value()));
	}

	SUBCASE("UTF-8-BOM")
	{
		const auto file {fileof("./src/sample/utf8bom.txt")};

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
