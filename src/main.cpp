#include <cassert>
#include <variant>
#include <iostream>

#include "string.hpp"

int main() noexcept
{
	#ifdef _MSC_VER
	{
		#ifdef _WIN32//################;
		std::system("chcp 65001 > NUL");
		#endif//WIN32//################;
	}
	#endif//MSC_VER

	c_str path {"src/sample.txt"};

	if (auto file {file_of(path)})
	{
		std::visit([&](auto&& content) { std::cout << content << '\n'; }, file.value());
	}
}
