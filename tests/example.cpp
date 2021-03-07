#include "sipp.hpp"

using sipp::quantity;
using sipp::basic_unit;

template<typename... Args>
using unit = sipp::unit_base<double, Args...>;


struct time_b : basic_unit {};
struct distance_b : basic_unit {};
struct mass_b : basic_unit {};

using time = unit<quantity<time_b, 1>>;
using distance = unit<quantity<distance_b, 1>>;
using mass = unit<quantity<mass_b, 1>>;

using velocity = unit<quantity<distance, 1>, quantity<time, -1>>;
using acceleration = unit<quantity<velocity, 1>, quantity<time, -1>>;
using momentum = unit<quantity<velocity, 1>, quantity<mass, 1>>;
using force = unit<quantity<acceleration, 1>, quantity<mass, 1>>;

int main() {
    time t0 = 13.0, t1 = 42.0;
    distance d = 1337.0;
    mass m = 5.0;

    momentum mom = m * d / (t1 - t0);
}