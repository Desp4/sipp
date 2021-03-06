#pragma once

#include <utility>
#include <tuple>

namespace sipp {

// ----------- general templates -----------

// >>> arithmetic with no cv
template<typename T>
concept plain_arithmetic = std::is_arithmetic_v<T> && std::is_same_v<T, std::remove_cv_t<T>>;

// >>> std::tuple_cat, in struct form
template<typename T, typename...>
struct tuple_cat_s {
    using type = T;
};

template<typename... Ts_0, typename... Ts_1, typename... Ts_2>
struct tuple_cat_s<std::tuple<Ts_0...>, std::tuple<Ts_1...>, Ts_2...> : tuple_cat_s<std::tuple<Ts_0..., Ts_1...>, Ts_2...> {};

template<typename... Ts>
using tuple_cat_s_t = typename tuple_cat_s<Ts...>::type;

// ----------- base sipp types -----------
// forward decl
template<typename>
struct quantity_type : std::false_type {};

template<typename T>
concept is_quantity = quantity_type<T>::value;
// forward decl
template<plain_arithmetic, is_quantity... Qs> struct unit_base;

// >>> unit derived concept
template<typename B, typename... Qs>
constexpr bool unit_parent(const unit_base<B, Qs...>&) { return true; }
constexpr bool unit_parent(...) { return false; }

template<typename T>
concept unit_based = unit_parent(T{});

// >>> quantitative measure of a unit
template<unit_based Unit, int Pow>
struct quantity {
    using unit = Unit;
    static constexpr int power = Pow;
};

// specialization for is_quantity
template<unit_based T, int P>
struct quantity_type<quantity<T, P>> : std::true_type {};

// >>> fwd for unit comparison concept, implementation later
template<typename, typename>
struct is_unit_assignable : std::false_type {};

template<typename T_1, typename T_2>
concept unit_assignable = is_unit_assignable<T_1, T_2>::value;

// >>> base unit, right here
template<plain_arithmetic Base, is_quantity... Qs>
struct unit_base {
    using base_unit = unit_base;

    Base value;

    constexpr unit_base() noexcept = default;
    template<plain_arithmetic Num>
    explicit constexpr unit_base(Num num) noexcept : value{ static_cast<Base>(num) } {}

    template<typename Oth_Base, typename... Oth_Qs> requires(unit_assignable<unit_base, unit_base<Oth_Base, Oth_Qs...>>)
    constexpr unit_base(const unit_base<Oth_Base, Oth_Qs...>& oth) noexcept : value{ static_cast<Base>(oth.value) } {}

    template<typename Oth_Base, typename... Oth_Qs> requires(unit_assignable<unit_base, unit_base<Oth_Base, Oth_Qs...>>)
    constexpr unit_base(unit_base<Oth_Base, Oth_Qs...>&& oth) noexcept : value{ std::move(static_cast<Base>(oth.value)) } {}

    template<typename Oth_Base, typename... Oth_Qs> requires(unit_assignable<unit_base, unit_base<Oth_Base, Oth_Qs...>>)
    constexpr unit_base& operator=(const unit_base<Oth_Base, Oth_Qs...>& oth) noexcept {
        value = static_cast<Base>(oth.value);
        return *this;
    }

    template<typename Oth_Base, typename... Oth_Qs> requires(unit_assignable<unit_base, unit_base<Oth_Base, Oth_Qs...>>)
    constexpr unit_base& operator=(unit_base<Oth_Base, Oth_Qs...>&& oth) noexcept {
        value = std::move(static_cast<Base>(oth.value));
        return *this;
    }

    constexpr operator Base() const noexcept { return value; }
    constexpr operator bool() const noexcept { return value == static_cast<Base>(0); }
};

// ----------- internal types -----------
// not many concepts and other safety mechanisms in internal, be cautious using
// ----------- unfolding -----------
// >>> closing overload for unit unfold
template<typename T, plain_arithmetic B, int P>
constexpr auto unit_unfold(const T&, const unit_base<B>&, std::integral_constant<int, P>) {
    return std::tuple<quantity<T, P>>{};
}

template<typename T, int P>
using unit_unfold_t = decltype(unit_unfold(std::declval<T>(), std::declval<T>(), std::integral_constant<int, P>{}));
// >>> unfold quantities, propagate power coefficients
template<int B, typename... Qs>
struct quantity_unfold {
    using type = tuple_cat_s_t<unit_unfold_t<typename Qs::unit, Qs::power + B - 1>...>;
};
// >>> recursive unit unfold
template<typename T, plain_arithmetic B, is_quantity... Qs, int P>
constexpr auto unit_unfold(const T&, const unit_base<B, Qs...>&, std::integral_constant<int, P>) {
    return tuple_cat_s_t<typename quantity_unfold<P, Qs>::type...>{};
}

// ----------- tuple filter -----------
template<typename, typename>
struct contains_q;
// >>> recursive contains for some T in tuple of Ts...
template<typename Q, typename... Qs, typename Q_T>
struct contains_q<std::tuple<Q, Qs...>, Q_T> {
    static constexpr bool value = contains_q<std::tuple<Qs...>, Q_T>::value;
    using tuple_type = tuple_cat_s_t<typename contains_q<std::tuple<Qs...>, Q_T>::tuple_type, std::tuple<Q>>;
};
// >>> closing instance if found
template<typename... Qs, typename U, int P_0, int P_1>
struct contains_q<std::tuple<quantity<U, P_0>, Qs...>, quantity<U, P_1>> : std::true_type {
    using tuple_type = std::conditional_t<((P_0 + P_1) == 0),
        std::tuple<Qs...>,
        std::tuple<quantity<U, P_0 + P_1>, Qs...>>;
};
// >>> closing instance if tuple depleated, have to drag tuple_type anyway sadly
template<typename Q_T>
struct contains_q<std::tuple<>, Q_T> : std::false_type {
    using tuple_type = std::tuple<>;
};

template<typename, typename>
struct filter_q;
// >>> recursive filter, utilizes contains for merging types
template<typename... Out_Qs, typename In_Q, typename... In_Qs>
struct filter_q<std::tuple<Out_Qs...>, std::tuple<In_Q, In_Qs...>> {
    using contains_type = contains_q<std::tuple<Out_Qs...>, In_Q>;
    using type = std::conditional_t<contains_type::value,
        typename filter_q<typename contains_type::tuple_type, std::tuple<In_Qs...>>::type,
        typename filter_q<std::tuple<Out_Qs..., In_Q>, std::tuple<In_Qs...>>::type>;
};
// >>> closes filter recursion
template<typename Out>
struct filter_q<Out, std::tuple<>> {
    using type = Out;
};

// ----------- internal shorthands, utils -----------
template<typename T>
using simplified_q = typename filter_q<std::tuple<>, T>::type;
template<typename T>
using complete_unfold = simplified_q<unit_unfold_t<T, 1>>;

// >>> implementation of unit comparison
template<typename, typename>
struct contains_q_strict;

template<typename Q, typename... Qs, typename Q_T>
struct contains_q_strict<std::tuple<Q, Qs...>, Q_T> : contains_q_strict<std::tuple<Qs...>, Q_T> { };

template<typename... Qs, typename Q>
struct contains_q_strict<std::tuple<Q, Qs...>, Q> : std::true_type {};

template<typename Q_T>
struct contains_q_strict<std::tuple<>, Q_T> : std::false_type { };

// >>> tuple forwarder for unit comparison
template<typename, typename>
struct tuple_compare;

template<typename... Qs_0, typename... Qs_1>
struct tuple_compare<std::tuple<Qs_0...>, std::tuple<Qs_1...>> {
    using main_tuple = std::tuple<Qs_0...>;
    static constexpr bool value = (sizeof...(Qs_0) == sizeof...(Qs_1)) &&
        (... && contains_q_strict<main_tuple, Qs_1>::value);

};

// >>> assignable impl
template<typename B_0, typename B_1, typename... Qs_0, typename... Qs_1>
struct is_unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>> {
    using tuple_0 = complete_unfold<unit_base<B_0, Qs_0...>>;
    using tuple_1 = complete_unfold<unit_base<B_1, Qs_1...>>;

    static constexpr bool value = tuple_compare<tuple_0, tuple_1>::value;
};

// >>> tuple back to unit
template<typename, typename>
struct unit_from_tuple;
template<typename B, typename... Qs>
struct unit_from_tuple<B, std::tuple<Qs...>> {
    using type = unit_base<B, Qs...>;
};

template<typename B, typename T>
using unit_from_tuple_t = typename unit_from_tuple<B, T>::type;

// >>> unit to -1 power
template<typename>
struct one_over_unit_tuple;

template<typename... Qs>
struct one_over_unit_tuple<std::tuple<Qs...>> {
    using type = std::tuple<quantity<typename Qs::unit, -Qs::power>...>;
};

template<typename Unit>
using one_over_unit_tuple_t = typename one_over_unit_tuple<Unit>::type;

// ----------- operators -----------
template<unit_based U_0, unit_based U_1>
constexpr auto operator*(const U_0&l, const U_1&r) noexcept {
    using new_base = decltype(l.value * r.value);
    using new_unit = unit_from_tuple_t<new_base, simplified_q<tuple_cat_s_t<unit_unfold_t<U_0, 1>, unit_unfold_t<U_1, 1>>>>;
    return new_unit{ static_cast<new_base>(l.value) * static_cast<new_base>(r.value) };
}
template<plain_arithmetic Num, unit_based Unit>
constexpr auto operator*(Num num, const Unit& unt) noexcept {
    return Unit{ static_cast<decltype(unt.value)>(num) * unt.value };
}
template<plain_arithmetic Num, unit_based Unit>
constexpr auto operator*(const Unit& unt, Num num) noexcept {
    return Unit{ static_cast<decltype(unt.value)>(num) * unt.value };
}

template<plain_arithmetic Num, unit_based Unit>
constexpr auto& operator*=(Unit& unt, Num num) noexcept {
    unt.value *= static_cast<decltype(unt.value)>(num);
    return unt;
}

template<unit_based U_0, unit_based U_1>
constexpr auto operator/(const U_0& l, const U_1& r) noexcept {
    using new_base = decltype(l.value / r.value);
    using new_unit = unit_from_tuple_t<new_base, simplified_q<tuple_cat_s_t<unit_unfold_t<U_0, 1>, one_over_unit_tuple_t<unit_unfold_t<U_1, 1>>>>>;
    return new_unit{ static_cast<new_base>(l.value) / static_cast<new_base>(r.value) };
}
template<plain_arithmetic Num, unit_based Unit>
constexpr auto operator/(Num num, const Unit& unt) noexcept {
    using new_base = decltype(unt.value);
    using one_over = unit_from_tuple_t<new_base, simplified_q<one_over_unit_tuple_t<unit_unfold_t<Unit, 1>>>>;
    return one_over{ static_cast<new_base>(num) / unt.value };
}
template<plain_arithmetic Num, unit_based Unit>
constexpr auto operator/(const Unit& unt, Num num) noexcept {
    return Unit{ static_cast<decltype(unt.value)>(num) / unt.value };
}

template<plain_arithmetic Num, unit_based Unit>
constexpr auto& operator/=(Unit& unt, Num num) noexcept {
    unt.value /= static_cast<decltype(unt.value)>(num);
    return unt;
}

template<typename B_0, typename... Qs_0, typename B_1, typename... Qs_1> requires (unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>>)
constexpr auto operator+(const unit_base<B_0, Qs_0...>& l, const unit_base<B_1, Qs_1...>& r) noexcept {
    return unit_base<B_0, Qs_0...>{ l.value + r.value };
}
template<typename B_0, typename... Qs_0, typename B_1, typename... Qs_1> requires (unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>>)
constexpr auto& operator+=(unit_base<B_0, Qs_0...>& l, const unit_base<B_1, Qs_1...>& r) noexcept {
    l.value += static_cast<B_0>(r.value);
    return l;
}

template<typename B_0, typename... Qs_0, typename B_1, typename... Qs_1> requires (unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>>)
constexpr auto operator-(const unit_base<B_0, Qs_0...>& l, const unit_base<B_1, Qs_1...>& r) noexcept {
    return unit_base<B_0, Qs_0...>{ l.value - r.value };
}
template<typename B_0, typename... Qs_0, typename B_1, typename... Qs_1> requires (unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>>)
constexpr auto& operator-=(unit_base<B_0, Qs_0...>& l, const unit_base<B_1, Qs_1...>& r) noexcept {
    l.value -= static_cast<B_0>(r.value);
    return l;
}

template<unit_based Unit>
constexpr auto operator+(const Unit& unt) noexcept {
    return Unit{ +unt.value };
}
template<unit_based Unit>
constexpr auto operator-(const Unit& unt) noexcept {
    return Unit{ -unt.value };
}

template<unit_based Unit>
constexpr auto& operator++(Unit& unt) noexcept {
    ++unt.value;
    return unt;
}
template<unit_based Unit>
constexpr auto& operator++(Unit& unt, int) noexcept {
    Unit ret{ unt };
    ++unt.value;
    return ret;
}

template<unit_based Unit>
constexpr auto& operator--(Unit& unt) noexcept {
    --unt.value;
    return unt;
}
template<unit_based Unit>
constexpr auto& operator--(Unit& unt, int) noexcept {
    Unit ret{ unt };
    --unt.value;
    return ret;
}
// I don't understand spaceship, replacing OP with <=> doesn't bind to anything
template<typename B_0, typename... Qs_0, typename B_1, typename... Qs_1> requires (unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>>)
constexpr bool operator==(const unit_base<B_0, Qs_0...>& l, const unit_base<B_1, Qs_1...>& r) noexcept {
    return l.value == r.value;
}
template<typename B_0, typename... Qs_0, typename B_1, typename... Qs_1> requires (unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>>)
constexpr bool operator!=(const unit_base<B_0, Qs_0...>& l, const unit_base<B_1, Qs_1...>& r) noexcept {
    return l.value != r.value;
}
template<typename B_0, typename... Qs_0, typename B_1, typename... Qs_1> requires (unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>>)
constexpr bool operator>(const unit_base<B_0, Qs_0...>& l, const unit_base<B_1, Qs_1...>& r) noexcept {
    return l.value > r.value;
}
template<typename B_0, typename... Qs_0, typename B_1, typename... Qs_1> requires (unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>>)
constexpr bool operator<(const unit_base<B_0, Qs_0...>& l, const unit_base<B_1, Qs_1...>& r) noexcept {
    return l.value < r.value;
}
template<typename B_0, typename... Qs_0, typename B_1, typename... Qs_1> requires (unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>>)
constexpr bool operator<=(const unit_base<B_0, Qs_0...>& l, const unit_base<B_1, Qs_1...>& r) noexcept {
    return l.value <= r.value;
}
template<typename B_0, typename... Qs_0, typename B_1, typename... Qs_1> requires (unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>>)
constexpr bool operator>=(const unit_base<B_0, Qs_0...>& l, const unit_base<B_1, Qs_1...>& r) noexcept {
    return l.value >= r.value;
}

}
