#include <ranges>
#include <variant>
#include <iostream>

#include "string.hpp"

// #define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#ifndef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

int main() noexcept
{
	{
		utf::str str {u8"ABCDEFGHIJKLMNOPQRSTUVW"};

		for (auto code : std::ranges::subrange(str.begin(), str.end()))
		{
			code = U'♥'; std::cout << str << '\n';
			code = U'?'; std::cout << str << '\n';
		}
	}

	{
		utf::str str {u8"ABCDEFGHIJKLMNOPQRSTUVW"};

		for (auto code : std::ranges::subrange(str.rbegin(), str.rend()))
		{
			code = U'♥'; std::cout << str << '\n';
			code = U'?'; std::cout << str << '\n';
		}
	}
}

#endif//DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"

TEST_CASE("string")
{
	SUBCASE("SSO23")
	{
		utf::str small {u8"ABCDEFGHIJKLMNOPQRSTUVW"};
		utf::str large {u8"ÁBCDEFGHIJKLMNOPQRSTUVW"};

		CHECK(small.size() == small.capacity());
		CHECK(large.size() == large.capacity());
	}

	SUBCASE("concat")
	{
		utf::utf8 티라 {u8"티라"};
		utf::utf8 미수 {u8"미수"};

		CHECK(티라.starts_with(티라));
		CHECK(티라.ends_with(u"티라"));

		CHECK(미수.starts_with(미수));
		CHECK(미수.ends_with(u"미수"));

		utf::utf8 티라미수 {u"티라" + 미수};
		utf::utf8 티라티라 {u"티라" + 티라};

		CHECK(티라미수 == u8"티라미수");
		CHECK(티라미수.length() == 4);

		CHECK(티라티라 == u8"티라티라");
		CHECK(티라티라.length() == 4);
	}

	SUBCASE("split")
	{
		utf::utf8 str {u8"티라미수"
		               u8"☆"
		               u8"치즈케잌"
		               u8"☆"
		               u8"말차라떼"};

		auto split {str.split(u"☆")};

		CHECK(split[0] == u"티라미수");
		CHECK(split[1] == u"치즈케잌");
		CHECK(split[2] == u"말차라떼");

		CHECK(split[0] == u8"티라미수");
		CHECK(split[1] == u8"치즈케잌");
		CHECK(split[2] == u8"말차라떼");

		CHECK(str.split(str).size() == 0);
		CHECK(str.match(str).size() == 1);
	}

	SUBCASE("index")
	{
		utf::utf8 str {u8"티라미수"};

		using utf::range::N;

		CHECK(str[0] == U'티');
		CHECK(str[1] == U'라');
		CHECK(str[2] == U'미');
		CHECK(str[3] == U'수');

		CHECK(str.substr(0, N) == str);
		CHECK(str.substr(0, 4) == str);
	}
}

TEST_CASE("fileof")
{
	SUBCASE("UTF-8")
	{
		const auto file {utf::fileof("./tests/utf8.txt")};

		REQUIRE(file.has_value());

		CHECK(std::holds_alternative<utf::utf8>(file.value()));
	}

	SUBCASE("UTF-16-LE")
	{
		const auto file {utf::fileof("./tests/utf16le.txt")};

		REQUIRE(file.has_value());

		CHECK(std::holds_alternative<utf::utf16>(file.value()));
	}

	SUBCASE("UTF-16-BE")
	{
		const auto file {utf::fileof("./tests/utf16be.txt")};

		REQUIRE(file.has_value());

		CHECK(std::holds_alternative<utf::utf16>(file.value()));
	}
}

#endif//DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
