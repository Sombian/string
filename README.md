# Sombian/string

[![hits](https://hits.sh/github.com/Sombian/string.svg)]()
[![test](https://badgen.net/github/stars/Sombian/string)]()
[![test](https://badgen.net/github/forks/Sombian/string)]()

a header only string impl. requires **C++23** ot later.  
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

### DISCLAIMER

code point random accessing is O(N) for variable width encoding.  
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

### `c_str`

- size
	- (getter)
	- returns the number of `code unit`s, excluding NULL-TERMINATOR.

- length
	- (getter)
	- returns the number of `code point`s, excluding NULL-TERMINATOR.

- capacity
	- (getter)
	- returns the number of `code unit`s it can hold, excluding NULL-TERMINATOR.

- capacity
	- (setter)
	- changes the number of `code unit`s it can hold, excluding NULL-TERMINATOR.

- split
	- (getter)
	- returns a list of string `slice`, of which are product of split aka division.

- match
	- (getter)
	- returns a list of string `slice`, of which are product of search occurrence.

- starts_width
	- (getter)
	- *self explanatory* returns whether or not it starts with *parameter*.

- ends_with
	- (getter)
	- *self explanatory* returns whether or not it ends with *parameter*.

- contains
	- (getter)
	- *self explanatory* returns whether or not it contains *parameter*.

- [A, B]
	- (getter)
	- returns a `slice` in range, similar to that of Python's slicing.

### `slice`

- size
	- (getter)
	- returns the number of `code unit`s, excluding NULL-TERMINATOR.

- length
	- (getter)
	- returns the number of `code point`s, excluding NULL-TERMINATOR.

- split
	- (getter)
	- returns a list of string `slice`, of which are product of split aka division.

- match
	- (getter)
	- returns a list of string `slice`, of which are product of search occurrence.

- starts_width
	- (getter)
	- *self explanatory* returns whether or not it starts with *parameter*.

- ends_with
	- (getter)
	- *self explanatory* returns whether or not it ends with *parameter*.

- contains
	- (getter)
	- *self explanatory* returns whether or not it contains *parameter*.

- [A, B]
	- (getter)
	- returns a `slice` in range, similar to that of Python's slicing.
