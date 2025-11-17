#include <cassert>
#include <iostream>

#include "string.hpp"

auto main() noexcept -> int
{
	#ifdef _MSC_VER
	{
		#ifdef _WIN32//################;
		std::system("chcp 65001 > NUL");
		#endif//WIN32//################;
	}
	#endif//MSC_VER

	utf8 str;

	str += u8"티라미수"; std::cout << str << '\n';
	str += u8"치즈케잌"; std::cout << str << '\n';
	str += u8"크로아상"; std::cout << str << '\n';
}
