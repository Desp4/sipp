#include "sipp.hpp"

template<typename... Args>
using unit = sipp::unit_base<double, Args...>;

#define SIPP_OP_CTOR using base_unit::operator=; using base_unit::base_unit; 

struct time_impl : unit<> { };
struct distance_impl : unit<> { };
struct mass_impl : unit<> { };

// need to derive from implementations to have proper assignment for basic types, other would need extra template args
struct time : unit<sipp::quantity<time_impl, 1>> { SIPP_OP_CTOR };
struct distance : unit<sipp::quantity<distance_impl, 1>> { SIPP_OP_CTOR };
struct mass : unit<sipp::quantity<mass_impl, 1>> { SIPP_OP_CTOR };

struct velocity : unit<sipp::quantity<distance, 1>, sipp::quantity<time, -1>> { SIPP_OP_CTOR };
struct acceleration : unit<sipp::quantity<velocity, 1>, sipp::quantity<time, -1>> { SIPP_OP_CTOR };
struct force : unit<sipp::quantity<acceleration, 1>, sipp::quantity<mass, 1>> { SIPP_OP_CTOR };

int main() {
    time t{ 0.0 };
    acceleration a{ 1.0 };
    mass m{ 4.0 };
    velocity v{ 2.0 };
    distance d{ 5.0 };

    force f = a * m;
    mass m2 = f / a;
    mass m3 = m2 + f / a;
    m3 += m2;
    bool eq0 = !m2;
    bool eq1 = m3 >= m;
    bool eq2 = -(m2 * a) == +f;
    velocity v2{ static_cast<double>(f * t + v * m) };
}