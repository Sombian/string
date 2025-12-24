#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <variant>
#include <cstdlib>

#include "string.hpp"

#ifndef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <iostream>

int main() noexcept
{
	#ifdef _MSC_VER//############//;
	std::system("chcp 65001 > NUL");
	#endif//MSC_VER//############//;

	static const char8_t path[]
	{u8"./src/sample/utf8.txt"};

	std::visit([](auto&& txt)
	{
		std::cout << txt << '\n';
	},
	std::move(*fileof(path)));
}
#endif

#ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"

TEST_CASE("storage")
{
	c_str small {u8"ABCDEFGHIJKLMNOPQRSTUVW"};
	c_str large {u8"ÁBCDEFGHIJKLMNOPQRSTUVW"};

	CHECK(small.size() == small.capacity());
	CHECK(large.size() == large.capacity());
}

TEST_CASE("[API] string")
{
	SUBCASE("concatenation")
	{
		c_str foo {u8"티라"};
		c_str bar {u8"미수"};

		utf8 foo_foo {foo + foo};
		utf8 foo_bar {foo + bar};

		CHECK(foo_foo == u8"티라티라");
		CHECK(foo_foo.length() == 4);

		CHECK(foo_bar == u8"티라미수");
		CHECK(foo_bar.length() == 4);
	}

	SUBCASE("split & match")
	{
		c_str str {u8"티라미수"
		           u8"☆"
		           u8"치즈케잌"
		           u8"☆"
		           u8"말차라떼"};

		auto split {str.split(u8"☆")};

		CHECK(split[0] == u8"티라미수");
		CHECK(split[0] == u"티라미수");

		CHECK(split[1] == u8"치즈케잌");
		CHECK(split[1] == u"치즈케잌");

		CHECK(split[2] == u8"말차라떼");
		CHECK(split[2] == u"말차라떼");
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
#endif
