#ifndef CODE_HPP
#define CODE_HPP

#include <vector>

#include "BitSet.hpp"
#include "Common.hpp"

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
using NativeFunction = void (*)(Runtime&, Thread&);

class Type
{
public:
    u64 size;
    u64 index = 0;

    Type() = delete;
    Type(u64 size) : size{size} {}

    virtual std::string get_name() const = 0;
};

class IntType : public Type
{
public:
    IntType() : Type{8} {}

    virtual std::string get_name() const override { return "int"; }
};

class FloatType : public Type
{
public:
    FloatType() : Type{8} {}

    virtual std::string get_name() const override { return "float"; }
};

class BoolType : public Type
{
public:
    BoolType() : Type{8} {}

    virtual std::string get_name() const override { return "bool"; }
};

class FunctionType : public Type
{
public:
    std::vector<Type*> arg_types;
    Type* return_type;

    FunctionType(const std::vector<Type*>& arg_types, Type* return_type) : Type{0},
                                                                           arg_types{arg_types},
                                                                           return_type{return_type}
    {
    }

    virtual std::string get_name() const override
    {
        bool first = true;
        std::string name = "func(";
        for (Type* arg_type : arg_types) {
            if (first) {
                first = false;
            } else {
                name += ", ";
            }
            name += arg_type->get_name();
        }
        name += ")";
        if (return_type) {
            name += " ";
            name += return_type->get_name();
        }
        return name;
    }
};

class NativeFunctionType : public Type
{
public:
    NativeFunctionType() : Type{0} {}

    virtual std::string get_name() const override { return "native function"; }
};

class ClosureType : public Type
{
public:
    FunctionType* function_type;
    u64 capc;
    ClosureType(FunctionType* function_type, u64 capc) : Type{8 + capc * 8},
                                                         function_type{function_type},
                                                         capc{capc}
    {
    }

    virtual std::string get_name() const override
    {
        return "closure " + std::to_string(capc) + " -> " + function_type->get_name();
    }
};

class CallableType : public Type
{
public:
    Type* function_type;

    CallableType(FunctionType* function_type) : Type{8}, function_type{function_type}
    {
    }

    virtual std::string get_name() const override
    {
        return "callable -> " + function_type->get_name();
    }
};

class StringType : public Type
{
public:
    StringType() : Type{8} {}

    virtual std::string get_name() const override { return "string"; }
};

class ChannelType : public Type
{
public:
    Type* element_type;
    ChannelType(Type* element_type) : Type{8},
                                      element_type{element_type}
    {
    }

    virtual std::string get_name() const override
    {
        std::string name = "chan";
        if (element_type) {
            name += " ";
            name += element_type->get_name();
        }
        return name;
    }
};

struct Function
{
    u16 capc = 0;
    u16 argc = 0;
    u16 varc = 0;
    u64 index = 0;
    BitSet pointer_map;
    std::vector<Instruction> code;
};

struct ClosureHeader
{
    u64 index;
};

struct NativeChannel
{
    u64 index;
};

struct NativeString
{
    u64 index;
};

#endif /* CODE_HPP */
