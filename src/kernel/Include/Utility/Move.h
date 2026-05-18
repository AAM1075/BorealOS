#ifndef BOREALOS_MOVE_H
#define BOREALOS_MOVE_H

namespace Utility {
    template<typename T> struct RemoveReference       { using Type = T; };
    template<typename T> struct RemoveReference<T&>   { using Type = T; };
    template<typename T> struct RemoveReference<T&&>  { using Type = T; };

    template<typename T>
    constexpr typename RemoveReference<T>::Type&& Move(T&& t) noexcept {
        return static_cast<typename RemoveReference<T>::Type&&>(t);
    }

    template<typename T, typename... Args>
    constexpr T* MoveConstruct(Args&&... args) {
        return new T(Utility::Move(args)...);
    }
}

#endif //BOREALOS_MOVE_H
