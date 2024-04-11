#ifndef THREAD_HPP
#define THREAD_HPP

#include "Common.hpp"

#include "Code.hpp"
#include "Heap.hpp"
#include "CallStack.hpp"
#include "OperandStack.hpp"

class Runtime;

class Thread
{
public:
    void run();

    Runtime& runtime;
    CallStack call_stack;
    OperandStack operand_stack;
};

#endif
