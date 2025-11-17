#include <cassert>
#include <iostream>

#include "string.hpp"

auto main() noexcept -> int
{
	#ifdef _MSC_VER
	{
		#ifdef _WIN32
		//|-----<change code page>-----|
		std::system("chcp 65001 > NUL");
		//|----------------------------|
		#endif//WIN32
	}
	#endif//MSC_VER

	utf8 foo {u8"안녕"};
	utf8 bar {u8"월드"};

	assert(foo.length() == 2);
	assert(bar.length() == 2);

	foo += bar;

	assert(foo.length() == 4);
	assert(bar.length() == 2);

	utf8 txt {u8"티라미수 치즈케잌 크로아상"};

	const auto parts {txt.split(u8" ")};

	for (const auto str : parts)
	{
		std::cout << str << '\n';
	}

	assert(parts[0] == u8"티라미수");
	assert(parts[1] == u8"치즈케잌");
	assert(parts[2] == u8"크로아상");
}
