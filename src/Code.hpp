#ifndef CODE_HPP
#define CODE_HPP

#include <vector>

#include "Common.hpp"
#include "BitSet.hpp"

enum class Opcode
{
    // MISC
    nop,
    // LOAD LOCAL VARIABLE
    load,
    // STORE LOCAL VARIABLE
    store,
    // OPERAND STACK MANIPULATION
    push,
    pop,
    dup,
    swap,
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
    Opcode opcode;
    union
    {
        u64 index = 0;
        Word value;
    };
};

class Runtime;
class Thread;
using NativeFunction = void(*)(Runtime&, Thread&);

struct Function
{
    u16 capc;
    u16 argc;
    u16 varc;
    u64 index;
    BitSet pointer_map;
    std::vector<Instruction> code;
};

struct ClosureHeader
{
    u64 function_index;
};

struct Type
{
    u64 memc; // number of elements
    u64 size; // total size
    u64 index;
    BitSet pointer_map;
};

#endif /* CODE_HPP */
