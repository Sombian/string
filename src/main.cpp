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

	utf8 txt {u8"티라미수 먹고싶다 ㅠㅠ"};

	for (auto str : txt.split(u8" "))
	{
		std::cout << str << std::endl;
	}
}
