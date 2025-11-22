[![hits](https://hits.sh/github.com/Sombian/string.svg)]()
[![test](https://badgen.net/github/stars/Sombian/string)]()
[![test](https://badgen.net/github/forks/Sombian/string)]()

requires **C++23** ot later.

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
