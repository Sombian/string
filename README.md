# Sombian/string

[![wiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/Sombian/string)
[![hits](https://hits.sh/github.com/Sombian/string.svg)](https://github.com/Sombian/string)
[![stats](https://badgen.net/github/stars/Sombian/string)](https://github.com/Sombian/string)
[![stats](https://badgen.net/github/forks/Sombian/string)](https://github.com/Sombian/string)

a header only string impl. requires **C++23** or later.  
(compatible with following compilers: GCC, Clang, MSVC)  

```c++
#include "string.hpp"

int main() noexcept
{
	utf::str str {u8"メスガキ"};

	std::cout << str << '\n';
}
```

## str

`str` is a struct that owns the string content.  
it can convert from, and to any available encoding encoded string.  
likewise, all its API accepts any available encoding encoded string.  

## txt

`txt` is a struct that holds ptr to the string.  
it can convert from, and to any available encoding encoded string.  
likewise, all its API accepts any available encoding encoded string.  

## trivia

code point random accessing is **O(N)** for variable width encoding.  
for this reason using an iterator is recommended for linear traversal.  

### ✔️ O(N)

```c++
utf::str str {u8"hello world"};

// time complexity: O(N)
for (const auto code : str)
{
	// do something with it
}
```

### ❌ O(N^2)

```c++
utf::str str {u8"hello world"};

// time complexity: O(N)
const auto len {str.length()};

// time complexity: O(N)
for (int i {0}; i < len; ++i)
{
	// time complexity: O(N)
	const auto code {str[i]};
	// do something with it
}
```
