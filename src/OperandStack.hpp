#ifndef OPERAND_STACK_HPP
#define OPERAND_STACK_HPP

#include <memory>
#include <stdexcept>

#include "Common.hpp"

class OperandStack
{
public:
    OperandStack() = default;
    OperandStack(const OperandStack&) = delete;
    OperandStack(OperandStack&&) = default;
    OperandStack& operator=(const OperandStack&) = delete;
    OperandStack& operator=(OperandStack&&) = default;

    OperandStack(u64 stack_size) : managed_memory{std::make_unique<std::byte[]>(stack_size)},
                                   memory{managed_memory.get()},
                                   top{0},
                                   size{stack_size}
    {
    }

    u64 get_size() const { return size; }

    template <typename T>
    T pop()
    {
        static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
        if (top == 0) {
            throw std::runtime_error("operand stack underflow!");
        }
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
        if (top >= size) {
            throw std::runtime_error("operand stack overflow!");
        }
        write(memory, top, value);
        top += sizeof(T);
    }

private:
    std::unique_ptr<std::byte[]> managed_memory;
    std::byte* memory;
    u64 size;
    u64 top;
};

#endif /* OPERAND_STACK_HPP */
