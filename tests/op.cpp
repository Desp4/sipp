#include "sipp_test.hpp"

int main() {
    distance d0{ 1 };
    time t0{ 2 };

    velocity v0 = d0 / t0;
    v0 = v0 / 3;
    v0 = 1 / (1 / v0);
    v0 /= 0.333;

    t0 = 1 / (v0 * (1 / d0));
    t0 = t0 * 2;
    t0 = 0.5 * t0;
    t0 *= 1.5;

    t0 = d0 / v0 + t0;
    acceleration a0 = v0 * v0 / d0 * 4 - 5;
    a0 += -2;
    a0 -= 6.0f;
    v0 = ++a0 * t0;
    v0 = t0 * a0--;
    d0 = -(v0 * t0);
    bool res0 = d0 > (a0 * t0 * t0);
    bool res1 = t0 == (v0 / a0);
    a0 = d0 * sipp::unit_to_pow<time, -2>{};
}