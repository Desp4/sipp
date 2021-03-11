#pragma once

#ifndef SIPP_H
#define SIPP_H

#ifdef SIPP_HELPERS

#define SIPP_BASIC_TYPE(BASE, NAME) \
    struct _ ## NAME ## _b : sipp::basic_unit {}; \
    using NAME = sipp::unit_base<BASE, sipp::quantity<_ ## NAME ## _b, 1>>;
#endif

#include <type_traits>
#include <compare>

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
concept plain_arithmetic = std::is_arithmetic_v<T> && std::is_same_v<T, std::decay_t<T>>;
// Quantity type concept
template<typename T>
concept is_quantity = detail::quantity_type<T>::value;
// Base unit class
template<plain_arithmetic, is_quantity... Qs> struct unit_base;
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
constexpr Unit_0 unit_cast(const Unit_1) noexcept;

// ------------ internal ------------
namespace detail {

// type specializations
template<typename T, int P>
struct quantity_type<quantity<T, P>> : std::true_type {};
template<typename B, typename... Qs>
struct unit_type<unit_base<B, Qs...>> : std::true_type {};

// unit concatenation
template<typename B, typename...>
struct unit_cat {
    using type = unit_base<B>;
};

template<typename B, typename U, typename... Us>
struct unit_cat<B, U, Us...>{
    using type = U;
};

template<typename B, typename B_0, typename B_1, typename... Ts_0, typename... Ts_1, typename... Ts_2>
struct unit_cat<B, unit_base<B_0, Ts_0...>, unit_base<B_1, Ts_1...>, Ts_2...> : unit_cat<B, unit_base<B, Ts_0..., Ts_1...>, Ts_2...> {};
template<typename Base, typename... Ts>
using unit_cat_t = typename unit_cat<Base, Ts...>::type;

// unit underlying type
template<typename>
struct unit_underlying;

template<typename B, typename... Qs>
struct unit_underlying<unit_base<B, Qs...>> {
    using type = B;
};

template<typename Unit>
using unit_underlying_t = typename unit_underlying<Unit>::type;

// unit infold
template<typename, int, typename>
struct unit_unfold;

template<typename B, int P, typename Unit> requires std::is_base_of_v<basic_unit, Unit>
    struct unit_unfold<B, P, Unit> {
        using type = unit_base<B, quantity<Unit, P>>;
    };

    template<typename U, int P>
    using unit_unfold_t = typename unit_unfold<unit_underlying_t<U>, P, U>::type;
    template<typename Base, typename U, int P>
    using unit_unfold_wbase_t = typename unit_unfold<Base, P, U>::type;

    template<typename B, int P, typename... Qs>
    struct unit_unfold<B, P, unit_base<B, Qs...>> {
        using type = unit_cat_t<B, unit_unfold_wbase_t<B, typename Qs::unit, Qs::power * P>...>;
    };

    // helper to get around constraint in quantity for pow != 0
    template<int P, typename B, typename U, typename... Qs>
    struct quantity_resolve {
        using type = unit_base<B, quantity<U, P>, Qs...>;
    };

    template<typename B, typename U, typename... Qs>
    struct quantity_resolve<0, B, U, Qs...> {
        using type = unit_base<B, Qs...>;
    };

    // power permissive contains
    template<typename, typename>
    struct contains_q;

    template<typename B, typename Q, typename... Qs, typename Q_T>
    struct contains_q<unit_base<B, Q, Qs...>, Q_T> {
        static constexpr bool value = contains_q<unit_base<B, Qs...>, Q_T>::value;
        using unit_type = unit_cat_t<B, typename contains_q<unit_base<B, Qs...>, Q_T>::unit_type, unit_base<B, Q>>;
    };

    template<typename B, typename... Qs, typename U, int P_0, int P_1>
    struct contains_q<unit_base<B, quantity<U, P_0>, Qs...>, quantity<U, P_1>> : std::true_type {
        using unit_type = typename quantity_resolve<P_0 + P_1, B, U, Qs...>::type;
    };
    // closing instance if tuple depleated, have to drag unit_type anyway sadly
    template<typename B, typename Q_T>
    struct contains_q<unit_base<B>, Q_T> : std::false_type {
        using unit_type = unit_base<B>;
    };

    // unit filter
    template<typename, typename>
    struct filter_q;

    template<typename B, typename... Out_Qs, typename In_Q, typename... In_Qs>
    struct filter_q<unit_base<B, Out_Qs...>, unit_base<B, In_Q, In_Qs...>> {
    private:
        using contains_type = contains_q<unit_base<B, Out_Qs...>, In_Q>;
    public:
        using type = std::conditional_t<contains_type::value,
            typename filter_q<typename contains_type::unit_type, unit_base<B, In_Qs...>>::type,
            typename filter_q<unit_base<B, Out_Qs..., In_Q>, unit_base<B, In_Qs...>>::type>;
    };

    template<typename B, typename Out>
    struct filter_q<Out, unit_base<B>> {
        using type = Out;
    };

    template<typename T>
    using simplified_q = typename filter_q<unit_base<unit_underlying_t<T>>, T>::type;
    template<typename T>
    using complete_unfold = simplified_q<unit_unfold_t<T, 1>>;

    // strict contains
    template<typename, typename>
    struct contains_q_strict;

    template<typename B, typename Q, typename... Qs, typename Q_T>
    struct contains_q_strict<unit_base<B, Q, Qs...>, Q_T> : contains_q_strict<unit_base<B, Qs...>, Q_T> { };

    template<typename B, typename... Qs, typename Q>
    struct contains_q_strict<unit_base<B, Q, Qs...>, Q> : std::true_type {};

    template<typename B, typename Q_T>
    struct contains_q_strict<unit_base<B>, Q_T> : std::false_type { };

    template<typename, typename>
    struct is_unit_assignable_helper;

    template<typename B_0, typename B_1, typename... Qs_0, typename... Qs_1>
    struct is_unit_assignable_helper<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>> {
        static constexpr bool value = (sizeof...(Qs_0) == sizeof...(Qs_1)) &&
            (... && contains_q_strict<unit_base<B_0, Qs_0...>, Qs_1>::value);
    };

    // unit assignable specialization
    template<typename B_0, typename B_1, typename... Qs_0, typename... Qs_1>
    struct is_unit_assignable<unit_base<B_0, Qs_0...>, unit_base<B_1, Qs_1...>> {
        static constexpr bool value = is_unit_assignable_helper<
            complete_unfold<unit_base<B_0, Qs_0...>>,
            complete_unfold<unit_base<B_1, Qs_1...>>>::value;
            
    };

} // end detail

// unit definition
template<plain_arithmetic Base, is_quantity... Qs>
struct unit_base {
    constexpr unit_base() noexcept = default;
    constexpr unit_base(const unit_base&) noexcept = default;
    constexpr unit_base(unit_base&&) noexcept = default;
    constexpr unit_base& operator=(const unit_base&) noexcept = default;
    constexpr unit_base& operator=(unit_base&&) noexcept = default;

    template<plain_arithmetic Num>
    constexpr unit_base(Num num) noexcept : value{ static_cast<Base>(num) } {}

    template<is_unit Unit> requires(unit_assignable<unit_base, Unit>)
    constexpr unit_base(const Unit oth) noexcept : value{ static_cast<Base>(oth) } {}

    template<is_unit Unit> requires(unit_assignable<unit_base, Unit>)
    constexpr unit_base& operator=(const Unit oth) noexcept {
        value = static_cast<Base>(oth);
        return *this;
    }

    template<plain_arithmetic Num>
    constexpr unit_base& operator=(Num num) noexcept {
        value = static_cast<Base>(num);
        return *this;
    }

    template<plain_arithmetic Num>
    constexpr operator Num() const noexcept { return static_cast<Num>(value); }

    template<typename Oth_B, typename... Oth_Qs>
    friend constexpr unit_base<Oth_B, Oth_Qs...> unit_cast(const unit_base) noexcept;

    template<is_unit U>
    friend constexpr auto operator*(const unit_base l, const U r) noexcept {
        using namespace detail;
        using new_base = decltype(l.value * unit_underlying_t<U>{});
        using new_unit = simplified_q<unit_cat_t<new_base, unit_unfold_t<unit_base, 1>, unit_unfold_t<U, 1>>>;
        return new_unit{ l.value * static_cast<unit_underlying_t<U>>(r) };
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator*(Num num, const unit_base unt) noexcept {
        using new_base = decltype(num * unt.value);
        return unit_base<new_base, Qs...>{ num * unt.value };
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator*(const unit_base unt, Num num) noexcept {
        using new_base = decltype(unt.value * num);
        return unit_base<new_base, Qs...>{ num * unt.value };
    }
    template<plain_arithmetic Num>
    friend constexpr auto& operator*=(unit_base& unt, Num num) noexcept {
        unt.value *= num;
        return unt;
    }
    template<is_unit U>
    friend constexpr auto operator/(const unit_base l, const U r) noexcept {
        using namespace detail;
        using new_base = decltype(l.value / unit_underlying_t<U>{});
        using new_unit = simplified_q<unit_cat_t<new_base, unit_unfold_t<unit_base, 1>, unit_unfold_t<U, -1>>>;
        return new_unit{ l.value / static_cast<unit_underlying_t<U>>(r) };
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator/(Num num, const unit_base unt) noexcept {
        using namespace detail;
        using new_base = decltype(num / unt.value);
        using new_unit = simplified_q<unit_unfold_wbase_t<new_base, unit_base, -1>>;
        return new_unit{ num / unt.value };
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator/(const unit_base unt, Num num) noexcept {
        using new_base = decltype(unt.value / num);
        return unit_base{ unt.value / num };
    }
    template<plain_arithmetic Num>
    friend constexpr auto& operator/=(unit_base& unt, Num num) noexcept {
        unt.value /= num;
        return unt;
    }
    template<is_unit U> requires(unit_assignable<unit_base, U>)
    friend constexpr auto operator+(const unit_base l, const U r) noexcept {
        using new_base = decltype(l.value + detail::unit_underlying_t<U>{});
        return unit_base<new_base, Qs...>{ l.value + static_cast<detail::unit_underlying_t<U>>(r) };
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator+(const unit_base l, Num r) noexcept {
        using new_base = decltype(l.value + r);
        return unit_base<new_base, Qs...>{ l.value + r };
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator+(Num l, const unit_base r) noexcept {
        using new_base = decltype(l + r.value);
        return unit_base<new_base, Qs...>{ l + r.value };
    }
    template<is_unit U> requires(unit_assignable<unit_base, U>)
    friend constexpr auto& operator+=(unit_base& l, const U r) noexcept {
        l.value += static_cast<detail::unit_underlying_t<U>>(r);
        return l;
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator+=(unit_base& l, Num r) noexcept {
        l.value += r;
        return l;
    }
    template<is_unit U> requires(unit_assignable<unit_base, U>)
    friend constexpr auto operator-(const unit_base l, const U r) noexcept {
        using new_base = decltype(l.value - detail::unit_underlying_t<U>{});
        return unit_base<new_base, Qs...>{ l.value - static_cast<detail::unit_underlying_t<U>>(r) };
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator-(const unit_base l, Num r) noexcept {
        using new_base = decltype(l.value - r);
        return unit_base<new_base, Qs...>{ l.value - r };
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator-(Num l, const unit_base r) noexcept {
        using new_base = decltype(l - r.value);
        return unit_base<new_base, Qs...>{ l - r.value };
    }
    template<is_unit U> requires(unit_assignable<unit_base, U>)
        friend constexpr auto& operator-=(unit_base& l, const U r) noexcept {
        l.value -= static_cast<detail::unit_underlying_t<U>>(r);
        return l;
    }
    template<plain_arithmetic Num>
    friend constexpr auto operator-=(unit_base& l, Num r) noexcept {
        l.value -= r;
        return l;
    }
    friend constexpr auto operator+(const unit_base unt) noexcept {
        return unit_base{ +unt.value };
    }
    friend constexpr auto operator-(const unit_base unt) noexcept {
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
    // comparison
    template<is_unit U> requires(unit_assignable<unit_base, U>)
    friend constexpr bool operator<=>(const unit_base l, const U r) noexcept {
        return l.value <=> static_cast<detail::unit_underlying_t<U>>(r);
    }
    template<is_unit U> requires(unit_assignable<unit_base, U>)
    friend constexpr bool operator==(const unit_base l, const U r) noexcept {
        return l.value == static_cast<detail::unit_underlying_t<U>>(r);
    }
    template<plain_arithmetic Num>
    friend constexpr bool operator<=>(const unit_base l, Num r) noexcept {
        return l.value <=> r;
    }
    template<plain_arithmetic Num>
    friend constexpr bool operator==(const unit_base l, Num r) noexcept {
        return l.value == r;
    }
    template<plain_arithmetic Num>
    friend constexpr bool operator<=>(Num l, const unit_base r) noexcept {
        return l <=> r.value;
    }
    template<plain_arithmetic Num>
    friend constexpr bool operator==(Num l, const unit_base r) noexcept {
        return l == r.value;
    }

private:
    Base value;
};

template<is_unit Unit_0, is_unit Unit_1>
constexpr Unit_0 unit_cast(const Unit_1 unt) noexcept {
    return Unit_0{ static_cast<detail::unit_underlying_t<Unit_0>>(unt) };
}

}

#endif
