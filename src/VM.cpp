#include <cstring>
#include <type_traits>

#include "VM.hpp"

template<typename T>
T read(std::byte* pointer, u64 offset)
{
    T value;
    std::memcpy(&value, pointer + offset, sizeof(T));
    return value;
}

template<typename T>
void write(std::byte* pointer, u64 offset, T value)
{
    std::memcpy(pointer + offset, &value, sizeof(T));
}

template<typename T, typename R>
R bitcast(T value)
{
    if constexpr (std::is_same_v<T, R>) {
        return value;
    } else {
        R casted_value;
        std::memcpy(&casted_value, &value, sizeof(R));
        return casted_value;
    }
}

template<typename T>
T VM::pop()
{
    static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
    operand_stack_top -= sizeof(T);
    return read<T>(operand_stack, operand_stack_top);
}

template<typename T>
void VM::push(T value)
{
    static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
    write(operand_stack, operand_stack_top, value);
    operand_stack_top += sizeof(T);
}

template<typename T>
void VM::memload()
{
    u64 address = pop<u64>();
    T value = read<T>(memory, address);
    push(bitcast<T, Word>(value));
}

template<typename T>
void VM::memstore()
{
    Word value = pop<Word>();
    u64 address = pop<u64>();
    write(memory, address, bitcast<Word, T>(value));
}

void VM::localload(u64 index)
{
    push(read<Word>(call_stack, frame_pointer + sizeof(Word) * index));
}

void VM::localstore(u64 index)
{
    write(call_stack, frame_pointer + sizeof(Word) * index, pop<Word>());
}

template<typename T>
T VM::pop_call_stack()
{
    static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
    call_stack_top -= sizeof(T);
    return read<T>(call_stack, call_stack_top);
}

template<typename T>
void VM::push_call_stack(T value)
{
    static_assert(sizeof(T) == sizeof(Word), "T must have the same size as Word");
    write(call_stack, call_stack_top, value);
    call_stack_top += sizeof(T);
}

void VM::initialize(
    const std::vector<Instruction>& instructions,
    u64 memory_size,
    u64 call_stack_size,
    u64 operand_stack_size)
{
    this->instructions = instructions;
    this->program_size = instructions.size();
    this->program_counter = 0;

    this->managed_memory = std::make_unique<std::byte[]>(memory_size);
    this->managed_operand_stack = std::make_unique<std::byte[]>(operand_stack_size);

    this->operand_stack = managed_operand_stack.get();
    this->operand_stack_size = operand_stack_size;
    this->operand_stack_top = 0;

    this->memory = managed_memory.get();
    this->memory_size = memory_size;

    this->call_stack = memory + (memory_size - call_stack_size);
    this->call_stack_size = call_stack_size;
    this->call_stack_top = 0;
    this->frame_pointer = 0;
}

#define GENERIC_BINARY(T, R, op) \
    do {                         \
        T y = pop<T>();          \
        T x = pop<T>();          \
        R r = x op y;            \
        push(r);                 \
    } while (0)

#define I_ARITH_BINARY(op) GENERIC_BINARY(i64, i64, op)
#define I_LOGIC_BINARY(op) GENERIC_BINARY(i64, i64, op)
#define I_BITWISE_BINARY(op) GENERIC_BINARY(i64, i64, op)

#define F_ARITH_BINARY(op) GENERIC_BINARY(f64, f64, op)
#define F_LOGIC_BINARY(op) GENERIC_BINARY(f64, i64, op)

#define GENERIC_UNARY(T, R, op) \
    do {                        \
        T x = pop<T>();         \
        R r = op x;             \
        push(r);                \
    } while (0)

#define I_ARITH_UNARY(op) GENERIC_UNARY(i64, i64, op)
#define I_LOGIC_UNARY(op) GENERIC_UNARY(i64, i64, op)
#define I_BITWISE_UNARY(op) GENERIC_UNARY(i64, i64, op)

#define F_ARITH_UNARY(op) GENERIC_UNARY(f64, f64, op)

void VM::run()
{
    while (program_counter < program_size) {
        u64 next_program_counter = program_counter + 1;
        Instruction& instruction = instructions[program_counter];
        switch (instruction.opcode) {
            case OpCode::nop:
                break;
            case OpCode::load:
                localload(instruction.index);
                break;
            case OpCode::store:
                localstore(instruction.index);
                break;
            case OpCode::push:
                push(instruction.value);
                break;
            case OpCode::pop:
                pop<Word>();
                break;
            case OpCode::aload:
                memload<Word>();
                break;
            case OpCode::bload:
                memload<Byte>();
                break;
            case OpCode::astore:
                memstore<Word>();
                break;
            case OpCode::bstore:
                memstore<Byte>();
                break;
            case OpCode::i2f:
                push(static_cast<f64>(pop<i64>()));
                break;
            case OpCode::f2i:
                push(static_cast<i64>(pop<f64>()));
                break;
            // INTEGER ARITHMETIC
            case OpCode::iadd:
                I_ARITH_BINARY(+);
                break;
            case OpCode::isub:
                I_ARITH_BINARY(-);
                break;
            case OpCode::imul:
                I_ARITH_BINARY(*);
                break;
            case OpCode::idiv:
                I_ARITH_BINARY(/);
                break;
            case OpCode::irem:
                I_ARITH_BINARY(%);
                break;
            case OpCode::ineg:
                I_ARITH_UNARY(-);
                break;
            // INTEGER BITWISE
            case OpCode::ishl:
                I_BITWISE_BINARY(<<);
                break;
            case OpCode::ishr:
                I_BITWISE_BINARY(>>);
                break;
            case OpCode::ixor:
                I_BITWISE_BINARY(^);
                break;
            case OpCode::ior:
                I_BITWISE_BINARY(|);
                break;
            case OpCode::iand:
                I_BITWISE_BINARY(&);
                break;
            case OpCode::inot:
                I_BITWISE_UNARY(~);
                break;
            // FLOATING POINT ARITHMETIC
            case OpCode::fadd:
                F_ARITH_BINARY(+);
                break;
            case OpCode::fsub:
                F_ARITH_BINARY(-);
                break;
            case OpCode::fmul:
                F_ARITH_BINARY(*);
                break;
            case OpCode::fdiv:
                F_ARITH_BINARY(/);
                break;
            case OpCode::fneg:
                F_ARITH_UNARY(-);
                break;
            // INTEGER COMPARISON
            case OpCode::ieq:
                I_LOGIC_BINARY(==);
                break;
            case OpCode::ilt:
                I_LOGIC_BINARY(<);
                break;
            case OpCode::igt:
                I_LOGIC_BINARY(>);
                break;
            case OpCode::ine:
                I_LOGIC_BINARY(!=);
                break;
            case OpCode::ile:
                I_LOGIC_BINARY(<=);
                break;
            case OpCode::ige:
                I_LOGIC_BINARY(>=);
                break;
            // FLOATING POINT COMPARISON
            case OpCode::feq:
                F_LOGIC_BINARY(==);
                break;
            case OpCode::flt:
                F_LOGIC_BINARY(<);
                break;
            case OpCode::fgt:
                F_LOGIC_BINARY(>);
                break;
            case OpCode::fne:
                F_LOGIC_BINARY(!=);
                break;
            case OpCode::fle:
                F_LOGIC_BINARY(<=);
                break;
            case OpCode::fge:
                F_LOGIC_BINARY(>=);
                break;
            // LOGICAL NOT
            case OpCode::lnot:
                I_LOGIC_UNARY(!);
                break;
            // CONTROL FLOW
            case OpCode::goto_:
                next_program_counter = instruction.index;
                break;
            case OpCode::ifne:
                if (pop<i64>() != 0) {
                    next_program_counter = instruction.index;
                }
                break;
            case OpCode::ifeq:
                if (pop<i64>() == 0) {
                    next_program_counter = instruction.index;
                }
                break;
            case OpCode::call:
                // store the old frame in the call stack
                push_call_stack(frame_pointer);
                // store the old program counter (of the next instruction)
                // in the call stack
                push_call_stack(next_program_counter);
                // allocate a new frame
                frame_pointer = call_stack_top;
                call_stack_top += sizeof(Word) * instruction.nvars;
                // store arguments (in reverse order)
                for (u8 i = 1; i <= instruction.nargs; ++i) {
                    localstore(instruction.nargs - i);
                }
                // jump to the function
                next_program_counter = instruction.index;
                break;
            case OpCode::ret:
                // deallocate the current frame
                operand_stack_top = frame_pointer;
                // restore the old program counter
                next_program_counter = pop_call_stack<u64>();
                // restore the old frame
                frame_pointer = pop_call_stack<u64>();
                break;
            default:
                // error
                // -Werror=switch
                next_program_counter = program_size;

        }
        program_counter = next_program_counter;
    }
}
