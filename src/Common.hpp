#include <cstdint>
#include <cstddef>
#include <cstring>
#include <type_traits>

static_assert(sizeof(double) == 8, "double should be 64-bit");

using i8 = std::int8_t;
using u8 = std::uint8_t;
using i16 = std::int16_t;
using u16 = std::uint16_t;
using i64 = std::int64_t;
using u64 = std::uint64_t;
using f64 = double;

struct Byte {
    std::byte data[1];
};

struct Word {
    std::byte data[8];
};

template<typename T>
T read(std::byte* pointer, u64 offset)
{
    T value;
    std::memcpy(&value, pointer + offset, sizeof(T));
    return value;
}

template<typename T>
void write(std::byte* pointer, u64 offset, T value)
{
    std::memcpy(pointer + offset, &value, sizeof(T));
}

template<typename T, typename R>
R bitcast(T value)
{
    if constexpr (std::is_same_v<T, R>) {
        return value;
    } else {
        R casted_value;
        std::memcpy(&casted_value, &value, sizeof(R));
        return casted_value;
    }
}
