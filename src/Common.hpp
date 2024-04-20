#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

static_assert(sizeof(float) == 4, "float should be 32-bit");
static_assert(sizeof(double) == 8, "double should be 64-bit");

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using f32 = float;
using f64 = double;

struct Byte
{
    std::byte data[1];
};

struct Word
{
    std::byte data[8];
};

template <typename T>
T& read(std::byte* buffer, u64 offset)
{
    return *reinterpret_cast<T*>(buffer + offset);
}

template <typename T>
void write(std::byte* buffer, u64 offset, const T& value)
{
    std::memcpy(buffer + offset, &value, sizeof(T));
}

template <typename T, typename R>
R bitcast(const T& value)
{
    R result;
    std::memcpy(&result, &value, sizeof(R));
    return result;
}

#endif /* COMMON_HPP */
