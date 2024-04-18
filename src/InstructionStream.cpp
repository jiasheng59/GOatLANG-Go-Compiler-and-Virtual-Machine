#include "InstructionStream.hpp"

const Instruction* InstructionStream::next()
{
    if (program_counter >= code->size()) {
        return nullptr;
    }
    return &(*code)[program_counter++];
}

void InstructionStream::jump_to(const Function& function, u64 new_program_counter)
{
    code = &function.code;
    program_counter = new_program_counter;
}
