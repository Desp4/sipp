#pragma once

#ifndef SIPP_H
#define SIPP_H

#ifdef SIPP_HELPERS

#define SIPP_BASIC_TYPE(BASE, NAME) \
    struct _ ## NAME ## _b : sipp::basic_unit {}; \
    using NAME = sipp::unit_base<BASE, sipp::quantity<_ ## NAME ## _b, 1>>;
#endif

#include <utility>
#include <tuple>

namespace sipp {

// internal forward declarations
namespace detail {

template<typename>
struct quantity_type : std::false_type {};
template<typename>
struct unit_type : std::false_type {};
template<typename, typename>
struct is_unit_assignable : std::false_type {};

}

// Base unit type concept
template<typename T>
concept plain_arithmetic = std::is_arithmetic_v<T> && std::is_same_v<T, std::remove_cv_t<T>>;
// Quantity type concept
template<typename T>
concept is_quantity = detail::quantity_type<T>::value;
// Base unit class
template<plain_arithmetic, is_quantity Q, is_quantity... Qs> struct unit_base;
// Basic unit parent class
struct basic_unit {};
// Unit type concept
template<typename T>
concept is_unit = detail::unit_type<T>::value;
// Quantitative measure of a unit
template<typename Unit, int Pow> requires(Pow != 0 && (std::is_base_of_v<basic_unit, Unit> || is_unit<Unit>))
struct quantity {
    using unit = Unit;
    static constexpr int power = Pow;
};
// Unit compatibility concept
template<typename T_1, typename T_2>
concept unit_assignable = detail::is_unit_assignable<T_1, T_2>::value;
// Explicit unit cast
template<is_unit Unit_0, is_unit Unit_1>
constexpr Unit_0 unit_cast(const Unit_1&) noexcept;

// ------------ internal ------------
namespace detail {

// type specializations
template<typename T, int P>
struct quantity_type<quantity<T, P>> : std::true_type {};
template<typename B, typename... Qs>
struct unit_type<unit_base<B, Qs...>> : std::true_type {};

// std::tuple_cat, in struct form
template<typename T, typename...>
struct tuple_cat_s {
    using type = T;
};

template<typename... Ts_0, typename... Ts_1, typename... Ts_2>
struct tuple_cat_s<std::tuple<Ts_0...>, std::tuple<Ts_1...>, Ts_2...> : tuple_cat_s<std::tuple<Ts_0..., Ts_1...>, Ts_2...> {};
template<typename... Ts>
using tuple_cat_s_t = typename tuple_cat_s<Ts...>::type;

// unit infold
template<int, typename>
struct unit_unfold;

template<int P, typename Unit> requires std::is_base_of_v<basic_unit, Unit>
    struct unit_unfold<P, Unit> {
        using type = std::tuple<quantity<Unit, P>>;
    };

    template<typename T, int P>
    using unit_unfold_t = typename unit_unfold<P, T>::type;

    template<int P, typename B, typename... Qs>
    struct unit_unfold<P, unit_base<B, Qs...>> {
        using type = tuple_cat_s_t<unit_unfold_t<typename Qs::unit, Qs::power* P>...>;
    };

    // power permissive contains
    template<typename, typename>
    struct contains_q;

    template<typename Q, typename... Qs, typename Q_T>
    struct contains_q<std::tuple<Q, Qs...>, Q_T> {
        static constexpr bool value = contains_q<std::tuple<Qs...>, Q_T>::value;
        using tuple_type = tuple_cat_s_t<typename contains_q<std::tuple<Qs...>, Q_T>::tuple_type, std::tuple<Q>>;
    };

    template<typename... Qs, typename U, int P_0, int P_1>
    struct contains_q<std::tuple<quantity<U, P_0>, Qs...>, quantity<U, P_1>> : std::true_type {
        using tuple_type = std::conditional_t<((P_0 + P_1) == 0),
            std::tuple<Qs...>, // don't include if the powers cancel out
            std::tuple<quantity<U, P_0 + P_1>, Qs...>>;
    };
    // closing instance if tuple depleated, have to drag tuple_type anyway sadly
    template<typename Q_T>
    struct contains_q<std::tuple<>, Q_T> : std::false_type {
        using tuple_type = std::tuple<>;
    };

    // unit tuple filter
    template<typename, typename>
    struct filter_q;

    template<typename... Out_Qs, typename In_Q, typename... In_Qs>
    struct filter_q<std::tuple<Out_Qs...>, std::tuple<In_Q, In_Qs...>> {
    private:
        using contains_type = contains_q<std::tuple<Out_Qs...>, In_Q>;
    public:
        using type = std::conditional_t<contains_type::value,
            typename filter_q<typename contains_type::tuple_type, std::tuple<In_Qs...>>::type,
            typename filter_q<std::tuple<Out_Qs..., In_Q>, std::tuple<In_Qs...>>::type>;
    };

    template<typename Out>
    struct filter_q<Out, std::tuple<>> {
        using type = Out;
    };

    template<typename T>
    using simplified_q = typename filter_q<std::tuple<>, T>::type;
    template<typename T>
    using complete_unfold = simplified_q<unit_unfold_t<T, 1>>;

    // strict contains
    template<typename, typename>
    struct contains_q_strict;

    template<typename Q, typename... Qs, typename Q_T>
    struct contains_q_strict<std::tuple<Q, Qs...>, Q_T> : contains_q_strict<std::tuple<Qs...>, Q_T> { };

    template<typename... Qs, typename Q>
    struct contains_q_strict<std::tuple<Q, Qs...>, Q> : std::true_type {};

    template<typename Q_T>
    struct contains_q_strict<std::tuple<>, Q_T> : std::false_type { };

    // tuple forwarder for unit comparison
    template<typename, typename>
    struct tuple_compare;

    template<typename... Qs_0, typename... Qs_1>
    struct tuple_compare<std::tuple<Qs_0...>, std::tuple<Qs_1...>> {
    private:
        using main_tuple = std::tuple<Qs_0...>;
    public:
        static constexpr bool value = (sizeof...(Qs_0) == sizeof...(Qs_1)) &&
            (... && contains_q_strict<main_tuple, Qs_1>::value);

    };

    // tuple back to unit
    template<typename, typename>
    struct unit_from_tuple;
    template<typename B, typename... Qs>
    struct unit_from_tuple<B, std::tuple<Qs...>> {
        using type = unit_base<B, Qs...>;
    };

    template<typename B, typename T>
    using unit_from_tuple_t = typename unit_from_tuple<B, T>::type;

    // unit underlying type
    template<typename>
    struct unit_underlying;

    template<typename B, typename... Qs>
    struct unit_underlying<unit_base<B, Qs...>> {
        using type = B;
    };

    template<typename Unit>
    using unit_underlying_t = typename unit_underlying<Unit>::type;

    // unit assignable specialization
    template<typename B_0, typename B_1, typename... Qs_0, typename... Qs_1>
    struct is_unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>> {
    private:
        using tuple_0 = complete_unfold<unit_base<B_0, Qs_0...>>;
        using tuple_1 = complete_unfold<unit_base<B_1, Qs_1...>>;
    public:
        static constexpr bool value = tuple_compare<tuple_0, tuple_1>::value;
    };

} // end detail

// unit definition
template<plain_arithmetic Base, is_quantity Q, is_quantity... Qs>
struct unit_base {
    constexpr unit_base() noexcept = default;
    template<plain_arithmetic Num>
    constexpr unit_base(Num num) noexcept : value{ static_cast<Base>(num) } {}

    constexpr unit_base(const unit_base&) noexcept = default;
    template<is_unit Unit> requires(unit_assignable<unit_base, Unit>)
        constexpr unit_base(const Unit& oth) noexcept : value{ static_cast<Base>(oth) } {}

    constexpr unit_base(unit_base&&) noexcept = default;
    template<is_unit Unit> requires(unit_assignable<unit_base, Unit>)
        constexpr unit_base(Unit&& oth) noexcept : value{ std::move(static_cast<Base>(oth)) } {}

    constexpr unit_base& operator=(const unit_base&) noexcept = default;
    template<is_unit Unit> requires(unit_assignable<unit_base, Unit>)
        constexpr unit_base& operator=(const Unit& oth) noexcept {
        value = static_cast<Base>(oth);
        return *this;
    }

    constexpr unit_base& operator=(unit_base&&) noexcept = default;
    template<is_unit Unit> requires(unit_assignable<unit_base, Unit>)
        constexpr unit_base& operator=(Unit&& oth) noexcept {
        value = std::move(static_cast<Base>(oth));
        return *this;
    }

    template<plain_arithmetic Num>
    constexpr unit_base& operator=(Num num) noexcept {
        value = static_cast<Base>(num);
        return *this;
    }

    template<plain_arithmetic Num>
    explicit constexpr operator Num() const noexcept { return static_cast<Num>(value); }

    template<typename Oth_B, typename... Oth_Qs>
    friend constexpr unit_base<Oth_B, Oth_Qs...> unit_cast(const unit_base& unt) noexcept;

    // arithmetic operators
    template<is_unit U> requires(unit_assignable<unit_base, U>)
        friend constexpr auto operator<=>(const unit_base& l, const U& r) noexcept {
        return l.value <=> static_cast<Base>(r);
    }
    template<is_unit U>
    friend constexpr auto operator*(const unit_base& l, const U& r) noexcept {
        using namespace detail;
        using new_base = decltype(l.value * unit_underlying_t<U>{});
        using new_unit = unit_from_tuple_t<new_base, simplified_q<tuple_cat_s_t<unit_unfold_t<unit_base, 1>, unit_unfold_t<U, 1>>>>;
        return new_unit{ static_cast<new_base>(l.value) * static_cast<new_base>(r) };
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator*(Num num, const unit_base& unt) noexcept {
        return unit_base{ static_cast<Base>(num) * unt.value };
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator*(const unit_base& unt, Num num) noexcept {
        return unit_base{ static_cast<Base>(num) * unt.value };
    }
    template<plain_arithmetic Num>
    friend constexpr auto& operator*=(unit_base& unt, Num num) noexcept {
        unt.value *= static_cast<Base>(num);
        return unt;
    }
    template<is_unit U>
    friend constexpr auto operator/(const unit_base& l, const U& r) noexcept {
        using namespace detail;
        using new_base = decltype(l.value / unit_underlying_t<U>{});
        using new_unit = unit_from_tuple_t<new_base, simplified_q<tuple_cat_s_t<unit_unfold_t<unit_base, 1>, unit_unfold_t<U, -1>>>>;
        return new_unit{ static_cast<new_base>(l.value) / static_cast<new_base>(r) };
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator/(Num num, const unit_base& unt) noexcept {
        using namespace detail;
        using one_over_unit = unit_from_tuple_t<Base, simplified_q<unit_unfold_t<unit_base, -1>>>;
        return one_over_unit{ static_cast<Base>(num) / unt.value };
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator/(const unit_base& unt, Num num) noexcept {
        return unit_base{ unt.value / static_cast<Base>(num) };
    }
    template<plain_arithmetic Num>
    friend constexpr auto& operator/=(unit_base& unt, Num num) noexcept {
        unt.value /= static_cast<Base>(num);
        return unt;
    }
    template<is_unit U> requires(unit_assignable<unit_base, U>)
    friend constexpr auto operator+(const unit_base& l, const U& r) noexcept {
        using new_base = decltype(l.value + detail::unit_underlying_t<U>{});
        using new_unit = std::conditional_t<std::is_same_v<Base, new_base>, unit_base, U>;
        return new_unit{ static_cast<new_base>(l.value) + static_cast<new_base>(r) };
    }
    template<is_unit U> requires(unit_assignable<unit_base, U>)
    friend constexpr auto& operator+=(unit_base& l, const U& r) noexcept {
        l.value += static_cast<Base>(r);
        return l;
    }
    template<is_unit U> requires(unit_assignable<unit_base, U>)
    friend constexpr auto operator-(const unit_base& l, const U& r) noexcept {
        using new_base = decltype(l.value - detail::unit_underlying_t<U>{});
        using new_unit = std::conditional_t<std::is_same_v<Base, new_base>, unit_base, U>;
        return new_unit{ static_cast<new_base>(l.value) - static_cast<new_base>(r) };
    }
    template<is_unit U> requires(unit_assignable<unit_base, U>)
        friend constexpr auto& operator-=(unit_base& l, const U& r) noexcept {
        l.value -= static_cast<Base>(r);
        return l;
    }
    friend constexpr auto operator+(const unit_base& unt) noexcept {
        return unit_base{ +unt.value };
    }
    friend constexpr auto operator-(const unit_base& unt) noexcept {
        return unit_base{ -unt.value };
    }
    friend constexpr auto& operator++(unit_base& unt) noexcept {
        ++unt.value;
        return unt;
    }
    friend constexpr auto operator++(unit_base& unt, int) noexcept {
        unit_base ret{ unt };
        ++unt.value;
        return ret;
    }
    friend constexpr auto& operator--(unit_base& unt) noexcept {
        --unt.value;
        return unt;
    }
    friend constexpr auto operator--(unit_base& unt, int) noexcept {
        unit_base ret{ unt };
        --unt.value;
        return ret;
    }

private:
    Base value;
};

template<is_unit Unit_0, is_unit Unit_1>
constexpr Unit_0 unit_cast(const Unit_1& unt) noexcept {
    return Unit_0{ static_cast<detail::unit_underlying_t<Unit_0>>(unt) };
}

}

#endif
