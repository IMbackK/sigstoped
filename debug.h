#pragma once
#include <string>
#include <iostream>

constexpr bool DEBUG = false;

inline void debug(const std::string& in)
{
    if constexpr (DEBUG) std::cout<<in<<'\n';
}
