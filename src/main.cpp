#include <cstdlib>
#include <cassert>

#include <variant>
#include <utility>
#include <iostream>

#include "string.hpp"

static char LF {'\n'};

int main() noexcept
{
	#ifdef _MSC_VER//############;
	std::system("chcp 65001>NUL");
	#endif//MSC_VER//############;

	c_str path {"src/sample.txt"};

	if (auto file {file_of(path)})
	{
		std::visit([&](auto&& _)
		{
			std::cout << _ << LF;
		},
		std::move(file.value()));
	}
}
