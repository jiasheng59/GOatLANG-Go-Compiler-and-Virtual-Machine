#ifndef OPERAND_STACK_HPP
#define OPERAND_STACK_HPP

#include <iostream>

#include "Common.hpp"

class OperandStack
{
public:
    OperandStack() = default;
    OperandStack(const OperandStack&) = delete;
    OperandStack(OperandStack&&) = delete;

    OperandStack& operator=(const OperandStack&) = delete;
    OperandStack& operator=(OperandStack&&) = delete;

    OperandStack(u64 stack_size) : memory{new std::byte[stack_size]},
                                   top{0},
                                   size{stack_size}
    {
    }

    ~OperandStack()
    {
        std::cerr << "Destroying the operand stack" << std::endl;
        delete[] memory;
    }

    u64 get_size() const { return size; }

    template <typename T>
    T pop()
    {
        static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
        top -= sizeof(T);
        return read<T>(memory, top);
    }

    template <typename T>
    T peek()
    {
        static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
        return read<T>(memory, top);
    }

    template <typename T>
    void push(T value)
    {
        static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
        write(memory, top, value);
        top += sizeof(T);
    }

private:
    std::byte* memory;
    u64 size;
    u64 top;
};

#endif /* OPERAND_STACK_HPP */
