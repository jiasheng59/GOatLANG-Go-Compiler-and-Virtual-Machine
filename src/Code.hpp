#ifndef CODE_HPP
#define CODE_HPP

#include <vector>

#include "BitSet.hpp"
#include "Common.hpp"

enum class OpCode
{
    // MISC
    nop,
    // LOAD LOCAL VARIABLE
    load,
    // STORE LOCAL VARIABLE
    store,
    // PUSH TO OPERAND STACK
    push,
    // POP FROM OPERAND STACK
    pop,
    // LOAD ADDRESS
    wload,
    bload,
    // STORE ADDRESS
    wstore,
    bstore,
    // CAST
    i2f,
    f2i,
    // INTEGER ARITHMETIC
    iadd,
    isub,
    imul,
    idiv,
    irem,
    ineg,
    iinc,
    idec,
    // INTEGER BITWISE
    ishl,
    ishr,
    ixor,
    ior,
    iand,
    inot,
    // FLOATING POINT ARITHMETIC
    fadd,
    fsub,
    fmul,
    fdiv,
    fneg,
    // INTEGER COMPARISON
    ieq,
    ilt,
    igt,
    ine,
    ile,
    ige,
    // FLOATING POINT COMPARISON
    feq,
    flt,
    fgt,
    fne,
    fle,
    fge,
    // LOGICAL NOT
    lnot,
    // CONTROL FLOW
    goto_,
    if_t,
    if_f,
    // FUNCTION INVOCATION
    invoke_static,
    invoke_dynamic,
    invoke_native,
    ret,
    // MEMORY ALLOCATION
    new_,
    new_array,
    array_length,
};

struct Instruction
{
    OpCode opcode;
    union
    {
        u64 index;
        Word value;
    };
};

class Runtime;
class Thread;
using NativeFunction = void(*)(Runtime& runtime_ref, Thread& thread_ref);

struct Function
{
    u16 capc;
    u16 argc;
    u16 varc;
    BitSet pointer_map;
    std::vector<Instruction> code;
};

struct ClosureHeader
{
    u64 function_index;
};

struct Type
{
    BitSet pointer_map;
    u64 memc; // number of elements
    u64 size; // total size
};

#endif /* CODE_HPP */