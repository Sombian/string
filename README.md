requires **C++23** ot later.

```c++
using range::N;

c_str str {"hello world"};

std::cout << str[0, N]     << '\n';
std::cout << str[0, 5]     << '\n';
std::cout << str[0, N - 5] << '\n';
std::cout << str[N - 5, N] << '\n';
```

### c_str

- size
	- (getter)
	- returns the number of code units, excluding NULL-TERMINATOR.

- length
	- (getter)
	- returns the number of code points, excluding NULL-TERMINATOR.

- capacity
	- (getter)
	- returns the number of code units it can hold, excluding NULL-TERMINATOR.

- capacity
	- (setter)
	- changes the number of code units it can hold, excluding NULL-TERMINATOR.

- split
	- (getter)
	- returns a list of string slice, of which are product of split aka division.

- match
	- (getter)
	- returns a list of string slice, of which are product of search occurrence.

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
	- returns a slice in range, simmilar to that of Python's slicing.

### slice

- size
	- (getter)
	- returns the number of code units, excluding NULL-TERMINATOR.

- length
	- (getter)
	- returns the number of code points, excluding NULL-TERMINATOR.

- split
	- (getter)
	- returns a list of string slice, of which are product of split aka division.

- match
	- (getter)
	- returns a list of string slice, of which are product of search occurrence.

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
	- returns a slice in range, simmilar to that of Python's slicing.
