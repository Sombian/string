# Sombian/string

[![wiki](https://deepwiki.com/badge.svg)]()
[![hits](https://hits.sh/github.com/Sombian/string.svg)]()
[![stats](https://badgen.net/github/stars/Sombian/string)]()
[![stats](https://badgen.net/github/forks/Sombian/string)]()

a header only string impl. requires **C++23** or later.  
(incompatible with MSVC for its lack of C++23 supports.)  

```c++
#include "string.hpp"

int main() noexcept
{
	using range::N;

	c_str str {"hello world"};

	assert(str == str[0, N]);

	std::cout << str << '\n';
}
```

code point random accessing is **O(N)** for variable width encoding.  
for this reason using an iterator is recommended for linear traversal.  

**Bad**

```c++
utf8 str {u8"hello world"};

// time complexity: O(N)
const auto len {str.length()};

// time complexity: O(N^2)
for (int i {0}; i < len; ++i)
{
	// time complexity: O(N)
	const auto code {str[i]};
	// do something with it
}
```

**Good**

```c++
utf8 str {u8"hello world"};

// time complexity: O(N)
for (const auto code : str)
{
	// do something with it
}
```
