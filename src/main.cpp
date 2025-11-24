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
	SUBCASE("concat")
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

	SUBCASE("substr")
	{
		const utf8 str {u8"티라미수"
		                u8"☆"
		                u8"치즈케잌"
		                u8"☆"
		                u8"말차라떼"
		                u8"☆"
		                u8"딸기우유"};

		auto src {str.split(u"☆")};

		CHECK(src[0] == u"티라미수");
		CHECK(src[1] == u"치즈케잌");
		CHECK(src[2] == u"말차라떼");
		CHECK(src[3] == u"딸기우유");
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
