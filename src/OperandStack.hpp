#ifndef OPERAND_STACK_HPP
#define OPERAND_STACK_HPP

#include "Common.hpp"

class OperandStack
{
public:
    template<typename T>
    T pop()
    {
        static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
        top -= sizeof(T);
        return read<T>(memory, top);
    }

    template<typename T>
    void push(T value)
    {
        static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
        write(memory, top, value);
        top += sizeof(T);
    };

    std::byte* memory;
    u64 size;
    u64 top;
};

#endif
