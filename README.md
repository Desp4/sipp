# SI++
Single header compile-time dimensional analysis library powered by C++20 concepts.
## Getting started
All the functionality is provided by the `sipp.hpp` header.
### 1. Creating basic units
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
### 2. `unit_base` and `quantity`
`quantity` is a quantitative measure of every unit, it represents a unit raised to some non-zero power P, so the template takes two parameters: unit instance and an integer.
`unit_base` represents the units, the first argument is the underlying type of a unit(only arithmetic types are allowed), while the rest are the quantities the unit consists of. The order of quantities or their partitioning does not matter.
### 3. Derived unit declaration
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
### 4. Operators
`unit_base` has all the expected operators for an arithmetic type(including implicit casts to arithmetic types). Multiplication and division can be performed on units of any type and on any arithmetic types. The rest of binary operators, including comparison, can only be performed on units of equivalent tupes or arithmetic types.
### 5. Utility
`unit_cast`   - casts any unit U<sub>1</sub> to unit U<sub>2</sub>. Same usage pattern as `static_cast`.  
`unit_to_pow` - raises unit U to an integer power P. Note that this only produces a type and does not raise the value of the unit to that power.