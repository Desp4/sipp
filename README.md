# SI++
Single header compile-time dimensional analysis library powered by C++20 concepts.
## Getting started
All the functionality is provided by the `sipp.hpp` header
### Creating basic units
To create first building blocks of a unit system, basic types have to be defined first.
```cpp
struct mass_b : sipp::basic_unit{};
using mass = sipp::unit_base<double, sipp::quantity<mass, 1>>;
```
`mass_b` in this example is an arbitrarily chosen name for an underlying unit type and `mass` is the actual type that you will be using.
If `SIPP_HELPERS` is defined SI++ enables a macro `SIPP_BASIC_TYPE(BASE, NAME)` which accomplishes the same goal.
```cpp
SIPP_BASIC_TYPE(double, mass);
```
### `unit_base` and `quantity`
`quantity` is a quantitative measure of every unit, it represents a unit raised to some non-zero power P, so the template takes two parameters: unit instance and an integer.
`unit_base` represents the units, the first argument is the underlying type of a unit(only arithmetic types are allowed), while the rest are the quantities the unit consists of.
### Derived unit declaration
Units can be defined in terms of other units, there are two ways to do it.
Using an explicit instance
```cpp
using velocity = sipp::unit_base<double, sipp::quantity<distance, 1>, sipp::quantity<time, -1>>; // velocity = distance^1 * time^-1
```
Or using a decltype
```cpp
using velocity = decltype(distance{} / time{});
```
Units can be nested inside other units and are not limited only to the basic types.
### Operators
`unit_base` instances have all the arithmetic operator overloads, comparison operators and constructors for units of equal type and explicit casts to arithmetic types
Operators such as addition and substraction are defined only for units of equal type, while multiplication and division operate on a pair of unit instances and produce a new unit instance.
### The `*`, The `/` and The `auto`
The results of multiplication and division of two units U1 and U2 will yield a new unit type U3. SI++ guarantees that the resulting unit will have the same quantities describing it, but it does not guarantee the order in which the quantities are meantioned in the template. That means that operator results that should have the same quantitative measure may actually not be the same type. SI++ accounts for that and guarantees that in such cases those types are still considered equal. Because of that, declaring the type with `auto` does not force any type and, while still convertible to types that have the same quantitative descriptions, isn't guaranteed to be the exact type you have aliased as valid.