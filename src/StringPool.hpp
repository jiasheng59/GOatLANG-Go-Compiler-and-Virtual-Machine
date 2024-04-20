#ifndef STRING_POOL_HPP
#define STRING_POOL_HPP

#include <string>
#include <vector>

#include "Common.hpp"

class StringPool
{
    std::vector<std::string> strings;

public:
    u64 new_string(std::string&& s)
    {
        u64 string_index = strings.size();
        strings.emplace_back(std::move(s));
        return string_index;
    }

    const std::string& get(u64 string_index)
    {
        return strings[string_index];
    }
};

#endif /* STRING_POOL_HPP */
