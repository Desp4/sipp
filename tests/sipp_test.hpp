#pragma once

#ifndef SIPP_TEST_H
#define SIPP_TEST_H

#include "../sipp.hpp"

using sipp::quantity;
using sipp::basic_unit;

template<typename... Qs>
using unit = sipp::unit_base<double, Qs...>;

#ifndef SIPP_HELPERS

struct _time_b : basic_unit {};
struct _distance_b : basic_unit {};
struct _mass_b : basic_unit {};

using time = unit<quantity<_time_b, 1>>;
using distance = unit<quantity<_distance_b, 1>>;
using mass = unit<quantity<_mass_b, 1>>;

#else

SIPP_BASIC_TYPE(double, time);
SIPP_BASIC_TYPE(double, distance);
SIPP_BASIC_TYPE(double, mass);

#endif

using dimless = unit<>;

#ifndef SIPP_DECLTYPE

using velocity = unit<quantity<distance, 1>, quantity<time, -1>>;
using acceleration = unit<quantity<velocity, 1>, quantity<time, -1>>;
using momentum = unit<quantity<velocity, 1>, quantity<mass, 1>>;
using force = unit<quantity<acceleration, 1>, quantity<mass, 1>>;

#else

using velocity = decltype(distance{} / time{});
using acceleration = decltype(velocity{} / time{});
using momentum = decltype(velocity{} * mass{});
using force = decltype(acceleration{} * mass{});

#endif

#endif
