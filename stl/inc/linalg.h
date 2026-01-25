// linalg standard header

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef _LINALG_
#define _LINALG_
#include <yvals.h>
#if _STL_COMPILER_PREPROCESSOR
// #if !_HAS_CXX23 || !defined(__cpp_lib_concepts) // TRANSITION, GH-395
//_EMIT_STL_WARNING(STL4038, "The contents of <mdspan> are available only with C++23 or later.");
// #else // ^^^ not supported / supported language mode vvv
#include <concepts>
#include <mdspan>
#include <type_traits>
#include <utility>

#pragma pack(push, _CRT_PACKING)
#pragma warning(push, _STL_WARNING_LEVEL)
#pragma warning(disable : _STL_DISABLED_WARNINGS)
_STL_DISABLE_CLANG_WARNINGS
#pragma push_macro("new")
#undef new

_STD_BEGIN
namespace linalg {
    template <class T>
    struct _Is_mdspan : false_type {}; // exposition only

    template <class _ElementType, class _Extents, class _Layout, class _Accessor>
    struct _Is_mdspan<mdspan<_ElementType, _Extents, _Layout, _Accessor>> : true_type {}; // exposition only

    template <class _Ty>
    struct _Is_float_or_complex : is_floating_point<_Ty> {};

    template <class _Ty>
    struct _Is_float_or_complex<complex<_Ty>> : true_type {};

    template <class T>
    concept _In_vector = // exposition only
        _Is_mdspan<T>::value && T::rank() == 1;

    template <class T>
    concept _Scalar = semiregular<T> && !_Is_mdspan<T>::value && !is_execution_policy_v<T>;

    namespace _Abs_if_needed_ns {
#if defined(__clang__) || defined(__EDG__) // TRANSITION, VSO-1681199
        template <class _Ty>
        _Ty abs(_Ty) = delete; // Block unqualified name lookup
#else // ^^^ no workaround / workaround vvv
        template <class _Ty>
        _Ty abs(_Ty);
#endif // ^^^ workaround ^^^


        template <class _Ty>
        concept _Has_ADL = requires(_Ty __t) {
            abs(__t); // intentional ADL
        };


        class _Cpo {
        private:
            enum class _St { _Ident, _Abs, _Custom, _None };

            template <class _Ty>
            _NODISCARD static consteval _Choice_t<_St> _Choose() noexcept {
                if constexpr (_Is_standard_integer<_Ty> && is_unsigned_v<_Ty>) {
                    return {_St::_Ident, true};
                } else if constexpr (is_arithmetic_v<_Ty>) {
                    return {_St::_Abs, noexcept(_STD abs(_STD declval<_Ty>()))};
                } else if constexpr (_Has_ADL<_Ty>) {
                    return {_St::_Custom, noexcept(abs(_STD declval<_Ty>()))}; // intentional ADL
                } else {
                    return {_St::_None};
                }
            }

            template <class _Ty>
            static constexpr _Choice_t<_St> _Choice = _Choose<_Ty>();

        public:
            template <class _Ty>
                requires (_Choice<_Ty>._Strategy != _St::_None)
            constexpr auto operator()(_Ty _Val) const noexcept(_Choice<_Ty>._No_throw) {
                constexpr _St _Strat = _Choice<_Ty>._Strategy;

                if constexpr (_Strat == _St::_Ident) {
                    return _Val;
                } else if constexpr (_Strat == _St::_Abs) {
                    return _STD abs(_Val);
                } else if constexpr (_Strat == _St::_Custom) {
                    return abs(_Val); // intentional ADL
                } else {
                    static_assert(false); // unexpected strategy
                }
            }
        };
    } // namespace _Abs_if_needed_ns

    namespace _Real_if_needed_ns {
#if defined(__clang__) || defined(__EDG__) // TRANSITION, VSO-1681199
        template <class _Ty>
        _Ty real(_Ty&) = delete; // Block unqualified name lookup
#else // ^^^ no workaround / workaround vvv
        template <class _Ty>
        _Ty real(_Ty&);
#endif // ^^^ workaround ^^^


        template <class _Ty>
        concept _Has_ADL = requires(const _Ty& __t) {
            real(__t); // intentional ADL
        };


        class _Cpo {
        private:
            enum class _St { _Ident, _Real };

            template <class _Ty>
            _NODISCARD static consteval _Choice_t<_St> _Choose() noexcept {
                if constexpr (!is_arithmetic_v<_Ty> && _Has_ADL<_Ty>) {
                    return {_St::_Real, noexcept(real(_STD declval<_Ty>()))}; // intentional ADL
                } else {
                    return {_St::_Ident};
                }
            }

            template <class _Ty>
            static constexpr _Choice_t<_St> _Choice = _Choose<_Ty>();

        public:
            template <class _Ty>
            constexpr auto operator()(const _Ty& _Val) const noexcept(_Choice<_Ty>._No_throw) {
                constexpr _St _Strat = _Choice<_Ty>._Strategy;

                if constexpr (_Strat == _St::_Ident) {
                    return _Val;
                } else if constexpr (_Strat == _St::_Real) {
                    return real(_Val); // intentional ADL
                } else {
                    static_assert(false); // unexpected strategy
                }
            }
        };
    } // namespace _Real_if_needed_ns

    inline namespace _Cpos {
        _EXPORT_STD inline constexpr _Abs_if_needed_ns::_Cpo _Abs_if_needed;
    }

    inline namespace _Cpos {
        _EXPORT_STD inline constexpr _Real_if_needed_ns::_Cpo _Real_if_needed;
    }

#define _MDSPAN_VECTOR_TMPL(i)  class _ElemT##i, class _IndexT##i, size_t _Size##i, class _Layout##i, class _Accessor##i
#define _MDSPAN_VECTOR_PARAM(i) mdspan<_ElemT##i, extents<_IndexT##i, _Size##i>, _Layout##i, _Accessor##i>

    template <_MDSPAN_VECTOR_TMPL(1), _MDSPAN_VECTOR_TMPL(2), class _Scalar>
    _Scalar dot(_MDSPAN_VECTOR_PARAM(1) _V1, _MDSPAN_VECTOR_PARAM(2) _V2, _Scalar _Init) {
        static_assert(_Size1 == dynamic_extent || _Size2 == dynamic_extent || _Size1 == _Size2);
        using _Index_type = common_type_t<_IndexT1, _IndexT2>;
        for (_Index_type _Idx = 0; _Idx < _V1.extent(0); ++_Idx) {
            _Init += _V1[array{_Idx}] * _V2[array{_Idx}];
        }

        return _Init;
    }

    template <class _ExPo, _In_vector _InVec1, _In_vector _InVec2, class _Scalar>
    _Scalar dot(_ExPo&& _Exec, _InVec1 _V1, _InVec2 _V2, _Scalar _Init) {
        return dot(_V1, _V2, _Init);
    }

    template <_In_vector _InVec1, _In_vector _InVec2>
    auto dot(_InVec1 _V1, _InVec2 _V2) {
        using _Return_type =
            decltype(_STD declval<typename _InVec1::value_type>() * _STD declval<typename _InVec2::value_type>());
        return dot(_V1, _V2, _Return_type{});
    }

    template <class _ExPo, _In_vector _InVec1, _In_vector _InVec2>
    auto dot(_ExPo&& _Exec, _InVec1 _V1, _InVec2 _V2) {
        using _Return_type = decltype(declval<_InVec1::value_type>() * declval<_InVec2::value_type>());
        return dot(_V1, _V2, _Return_type{});
    }

    // template <_In_vector _InVec, _Scalar _S>
    //_S vector_two_norm(_InVec v, _S init);

    // template <class _ExPo, _In_vector _InVec, _Scalar _S>
    //_S vector_two_norm(_ExPo&& _Exec, _InVec v, _S init);

    // template <_In_vector _InVec>
    // auto vector_two_norm(_InVec v)
    //{
    //     return vector_two_norm(v, _Ty{});
    // }

    // template <class _ExPo, _In_vector _InVec>
    // auto vector_two_norm(_ExPo&& _Exec, _InVec v)
    //{
    //     using _Abs = decltype(_Abs_if_needed(declval<typename _In_vector::value_type>()));
    //     using _Ty  = decltype(declval<_Abs>() * declval<_Abs>);
    //     return vector_two_norm(_Exec, v, _Ty{});
    // }

#undef _MDSPAN_VECTOR_TMPL
#undef _MDSPAN_VECTOR_PARAM
} // namespace linalg

_STD_END
#pragma pop_macro("new")
_STL_RESTORE_CLANG_WARNINGS
#pragma warning(pop)
#pragma pack(pop)
// #endif // ^^^ supported language mode ^^^
#endif // _STL_COMPILER_PREPROCESSOR
#endif // _MDSPAN_
