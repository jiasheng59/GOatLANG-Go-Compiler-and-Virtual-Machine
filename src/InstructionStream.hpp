#ifndef INSTRUCTION_STREAM_HPP
#define INSTRUCTION_STREAM_HPP

#include <vector>

#include "Code.hpp"

class InstructionStream
{
public:
    const Instruction* next()
    {
        if (program_counter >= code->size()) {
            return nullptr;
        }
        return &(*code)[program_counter++];
    }

    void jump_to(const Function& function, u64 new_program_counter = 0)
    {
        code = &function.code;
        program_counter = new_program_counter;
    }

    u64 get_program_counter() const
    {
        return program_counter;
    }

    void set_program_counter(u64 new_program_counter)
    {
        program_counter = new_program_counter;
    }

private:
    const std::vector<Instruction>* code;
    u64 program_counter;
};

#endif /* INSTRUCTION_STREAM_HPP */
