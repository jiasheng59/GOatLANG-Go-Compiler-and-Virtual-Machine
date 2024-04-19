#ifndef INSTRUCTION_STREAM_HPP
#define INSTRUCTION_STREAM_HPP

#include <vector>

#include "Common.hpp"
#include "Code.hpp"

class InstructionStream
{
public:
    const Instruction* next();
    void jump_to(const Function& function, u64 new_program_counter = 0);

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
