#ifndef THREAD_HPP
#define THREAD_HPP

#include "Common.hpp"

#include "Code.hpp"
#include "CallStack.hpp"
#include "OperandStack.hpp"
#include "InstructionStream.hpp"

class Runtime;

class Thread
{
public:
    Thread() = delete;
    Thread(const Thread&) = delete;
    Thread(Thread&&) = default;

    Thread& operator=(const Thread&) = delete;
    Thread& operator=(Thread&&) = default;

    Thread(Runtime& runtime) :
        runtime{runtime},
        instruction_stream{},
        call_stack{runtime.configuration.call_stack_size},
        operand_stack{runtime.configuration.operand_stack_size}
    {
    }

    void initialize();
    void finalize();
    void start(u64 init_function_index);
    void run(u64 init_function_index);

    CallStack& get_call_stack() { return call_stack; }
    OperandStack& get_operand_stack() { return operand_stack; }
    InstructionStream& get_instruction_stream() { return instruction_stream; }
private:
    Runtime& runtime;
    InstructionStream instruction_stream;
    CallStack call_stack;
    OperandStack operand_stack;
};

#endif /* THREAD_HPP */
