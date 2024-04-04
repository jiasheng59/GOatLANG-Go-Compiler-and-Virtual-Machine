// VIRTUAL MACHINE:
// |- CALL STACK
// |- OPERAND STACK
// |- STACK POINTER (SP)
// |- PROGRAM COUNTER (PC)
//
// INSTRUCTION SET
// We will assume that, everything on the operand stack
// and the call stack are 64-bit value
//
// MISC
// nop (do nothing)
//
// LOAD LOCAL VARIABLE
// (read a value from a local slot, and push it to the operand stack)
// load <index> : [...] -> [..., 64-bit value]
//
// STORE LOCAL VARIABLE
// (pop a value from the operand stack, and write it to a local slot)
// store <index> : [..., 64-bit value] -> [...]
//
// PUSH
// (push a constant to the operand stack)
// push <64-bit value> : [...] -> [64-bit value]
//
// POP
// (pop a value from the operand stack)
// pop : [..., 64-bit value] -> []
//
// LOAD ADDRESS
// (read a value from an address, and push it to the operand stack)
//
// aload : [..., 64-bit address] -> [..., 64-bit value]
// bload : [..., 64-bit address] -> [..., 64-bit value]
// (load a SINGLE byte, and promote it to a 64-bit value)
//
// STORE ADDRESS
// (pop a value from the operand stack, and write it to an address)
//
// astore : [..., 64-bit address, 64-bit value] -> [...]
// bstore : [..., 64-bit address, 64-bit value] -> [...]
// (demote a 64-bit value to an 8-bit value, and store it)
//
// CAST
//
// i2f : [..., 64-bit value] -> [..., 64-bit value]
// f2i : [..., 64-bit value] -> [..., 64-bit value]
//
// INTEGER ARITHMETIC
//
// iadd
// isub
// imul
// idiv
// irem
// ineg
//
// INTEGER BITWISE
//
// ishl
// ishr
// ixor
// ior
// iand
// inot
//
// FLOATING POINT ARITHMETIC
//
// fadd
// fsub
// fmul
// fdiv
// fneg
//
// INTEGER COMPARISON
//
// ieq (==)
// ilt (<)
// igt (>)
// ine (!=)
// ile (<=)
// ige (>=)
//
// FLOATING POINT COMPARISON
//
// feq (==)
// flt (<)
// fgt (>)
// fne (!=)
// fle (<=)
// fge (>=)
//
// LOGICAL NOT
// lnot (!)
//
// CONTROL FLOW
//
// goto <index>
// (go to index)
//
// ifne <index> : [..., 64-bit value] -> [...]
// (branch to index, if the value is not zero)
//
// ifeq <index> : [..., 64-bit value] -> [...]
// (branch to index, if the value is zero)
//
// call <index>
// (call a function)
//
// ret
// (return from a function call)

#ifndef VM_HPP
#define VM_HPP

#include <memory>
#include <vector>
#include <cstdint>
#include <cstddef>

static_assert(sizeof(double) == 8, "double should be 64-bit");

using i64 = std::int64_t;
using u64 = std::uint64_t;
using f64 = double;
using i8 = std::int8_t;
using u8 = std::uint8_t;

enum class OpCode
{
    nop,

    load,
    store,
    push,
    pop,

    aload,
    bload,

    astore,
    bstore,

    i2f,
    f2i,

    iadd,
    isub,
    imul,
    idiv,
    irem,
    ineg,

    ishl,
    ishr,
    ixor,
    ior,
    iand,
    inot,

    fadd,
    fsub,
    fmul,
    fdiv,
    fneg,
    //
    ieq,
    ilt,
    igt,
    ine,
    ile,
    ige,
    //
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
    ifne,
    ifeq,
    call,
    ret,
};

struct Byte
{
    std::byte data[1];
};

struct Word
{
    std::byte data[8];
};

struct Instruction
{
    OpCode opcode;
    union
    {
        u64 index;
        Word value;
    };
    u8 nargs;
    u8 nvars;
};

class VM
{
public:
    VM() = default;
    VM(const VM&) = delete;
    VM(VM&&) = delete;
    VM& operator=(const VM&) = delete;
    VM& operator=(VM&&) = delete;

    void initialize(
        const std::vector<Instruction>& instructions,
        u64 memory_size,
        u64 call_stack_size,
        u64 operand_size);
    void run();
private:
    std::vector<Instruction> instructions;
    u64 program_size;
    u64 program_counter;

    // memory management
    std::unique_ptr<std::byte[]> managed_memory;
    std::unique_ptr<std::byte[]> managed_operand_stack;

    std::byte* operand_stack;
    u64 operand_stack_size;
    u64 operand_stack_top;

    std::byte* memory;
    u64 memory_size;

    std::byte* call_stack; // should point to some value in `memory`
    u64 call_stack_size;
    u64 call_stack_top;
    u64 frame_pointer;

    template<typename T>
    T pop();

    template<typename T>
    void push(T value);

    template<typename T>
    T pop_call_stack();

    template<typename T>
    void push_call_stack(T value);

    template<typename T>
    void memload();

    template<typename T>
    void memstore();

    void localload(u64 index);

    void localstore(u64 index);
};

#endif /* VM_HPP */
