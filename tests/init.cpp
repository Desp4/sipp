#include "sipp_test.hpp"

int main() {
    velocity v0 = 0;
    velocity v1{ 3.5f };
    velocity v2{ v1 + 2.0 };
    time t0 = sipp::unit_cast<time>(v2 * v1);
    acceleration a0{ v2 / t0 };

    static_assert(!sipp::unit_assignable<acceleration, decltype(v2 * t0)>);
    static_assert(!sipp::unit_assignable<time, decltype(a0 / v0)>);
}