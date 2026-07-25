#ifndef BOREALOS_TRAITS_H
#define BOREALOS_TRAITS_H

// Implementation of some C++ type templating stuff

namespace Utility::Traits {
    template<typename T>
    struct TypeIdentity {
        using type = T;
    };

    template<typename T>
    using TypeIdentityT = typename TypeIdentity<T>::type;
} // TypeIdentityT

namespace Utility::Traits {
    template<typename T>
    struct RemoveReference { using type = T; };

    template<typename T>
    struct RemoveReference<T&> { using type = T; };

    template<typename T>
    struct RemoveReference<T&&> { using type = T; };

    template<typename T>
    using RemoveReferenceT = typename RemoveReference<T>::type;

    template<typename T>
    struct RemoveConst { using type = T; };

    template<typename T>
    struct RemoveConst<const T> { using type = T; };

    template<typename T>
    struct Decay {
    private:
        using U = RemoveReferenceT<T>;
    public:
        using type = typename RemoveConst<U>::type;
    };

    template<typename T>
    using DecayT = typename Decay<T>::type;

    template<typename T, typename U>
    struct IsSame { static constexpr bool value = false; };

    template<typename T>
    struct IsSame<T, T> { static constexpr bool value = true; };

    template<typename T, typename U>
    inline constexpr bool IsSameV = IsSame<T, U>::value;

    template<typename T>
    constexpr typename RemoveReference<T>::type&& Move(T&& t) noexcept {
        return static_cast<typename RemoveReference<T>::type&&>(t);
    }

    template<typename T, typename... Args>
    constexpr T* MoveConstruct(Args&&... args) {
        return new T(Utility::Traits::Move(args)...);
    }
} // DecayT, Move

#endif //BOREALOS_TRAITS_H
