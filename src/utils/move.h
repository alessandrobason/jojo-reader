#pragma once

template<typename T>
struct remove_reference      { typedef T type; };

template<typename T>
struct remove_reference<T&>  { typedef T type; };

template<typename T>
struct remove_reference<T&&> { typedef T type; };

template<typename T>
constexpr typename remove_reference<T>::type&&
move(T&& t) noexcept
{ return static_cast<typename remove_reference<T>::type&&>(t); }
