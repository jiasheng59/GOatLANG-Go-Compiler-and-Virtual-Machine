#ifndef THREAD_HPP
#define THREAD_HPP

#include "Common.hpp"

#include "CallStack.hpp"
#include "Code.hpp"
#include "InstructionStream.hpp"
#include "OperandStack.hpp"

class Runtime;

class Thread
{
public:
    Thread() = delete;
    Thread(const Thread&) = delete;
    Thread(Thread&&) = default;
    Thread& operator=(const Thread&) = delete;
    Thread& operator=(Thread&&) = default;

    Thread(Runtime& runtime);

    void initialize();
    void finalize();
    void run();

    void start();

    CallStack& get_call_stack() { return call_stack; }
    OperandStack& get_operand_stack() { return operand_stack; }
    InstructionStream& get_instruction_stream() { return instruction_stream; }

private:
    Runtime* runtime;
    InstructionStream instruction_stream;
    CallStack call_stack;
    OperandStack operand_stack;
};

#endif /* THREAD_HPP */
