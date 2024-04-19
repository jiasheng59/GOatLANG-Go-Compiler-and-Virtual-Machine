#include <cstring>
#include <type_traits>
#include <iostream>

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
            case Opcode::nop:
                break;
            case Opcode::exit:
                next_program_counter = program_size;
                break;
            case Opcode::load:
                localload(instruction.index);
                break;
            case Opcode::store:
                localstore(instruction.index);
                break;
            case Opcode::push:
                push(instruction.value);
                break;
            case Opcode::pop:
                pop<Word>();
                break;
            case Opcode::aload:
                memload<Word>();
                break;
            case Opcode::bload:
                memload<Byte>();
                break;
            case Opcode::astore:
                memstore<Word>();
                break;
            case Opcode::bstore:
                memstore<Byte>();
                break;
            case Opcode::i2f:
                push(static_cast<f64>(pop<i64>()));
                break;
            case Opcode::f2i:
                push(static_cast<i64>(pop<f64>()));
                break;
            // INTEGER ARITHMETIC
            case Opcode::iadd:
                I_ARITH_BINARY(+);
                break;
            case Opcode::isub:
                I_ARITH_BINARY(-);
                break;
            case Opcode::imul:
                I_ARITH_BINARY(*);
                break;
            case Opcode::idiv:
                I_ARITH_BINARY(/);
                break;
            case Opcode::irem:
                I_ARITH_BINARY(%);
                break;
            case Opcode::ineg:
                I_ARITH_UNARY(-);
                break;
            case Opcode::iinc:
                I_ARITH_UNARY(++);
                break;
            case Opcode::idec:
                I_ARITH_UNARY(--);
                break;
            // INTEGER BITWISE
            case Opcode::ishl:
                I_BITWISE_BINARY(<<);
                break;
            case Opcode::ishr:
                I_BITWISE_BINARY(>>);
                break;
            case Opcode::ixor:
                I_BITWISE_BINARY(^);
                break;
            case Opcode::ior:
                I_BITWISE_BINARY(|);
                break;
            case Opcode::iand:
                I_BITWISE_BINARY(&);
                break;
            case Opcode::inot:
                I_BITWISE_UNARY(~);
                break;
            // FLOATING POINT ARITHMETIC
            case Opcode::fadd:
                F_ARITH_BINARY(+);
                break;
            case Opcode::fsub:
                F_ARITH_BINARY(-);
                break;
            case Opcode::fmul:
                F_ARITH_BINARY(*);
                break;
            case Opcode::fdiv:
                F_ARITH_BINARY(/);
                break;
            case Opcode::fneg:
                F_ARITH_UNARY(-);
                break;
            // INTEGER COMPARISON
            case Opcode::ieq:
                I_LOGIC_BINARY(==);
                break;
            case Opcode::ilt:
                I_LOGIC_BINARY(<);
                break;
            case Opcode::igt:
                I_LOGIC_BINARY(>);
                break;
            case Opcode::ine:
                I_LOGIC_BINARY(!=);
                break;
            case Opcode::ile:
                I_LOGIC_BINARY(<=);
                break;
            case Opcode::ige:
                I_LOGIC_BINARY(>=);
                break;
            // FLOATING POINT COMPARISON
            case Opcode::feq:
                F_LOGIC_BINARY(==);
                break;
            case Opcode::flt:
                F_LOGIC_BINARY(<);
                break;
            case Opcode::fgt:
                F_LOGIC_BINARY(>);
                break;
            case Opcode::fne:
                F_LOGIC_BINARY(!=);
                break;
            case Opcode::fle:
                F_LOGIC_BINARY(<=);
                break;
            case Opcode::fge:
                F_LOGIC_BINARY(>=);
                break;
            // LOGICAL NOT
            case Opcode::lnot:
                I_LOGIC_UNARY(!);
                break;
            // CONTROL FLOW
            case Opcode::goto_:
                next_program_counter = instruction.index;
                break;
            case Opcode::ifne:
                if (pop<i64>() != 0) {
                    next_program_counter = instruction.index;
                }
                break;
            case Opcode::ifeq:
                if (pop<i64>() == 0) {
                    next_program_counter = instruction.index;
                }
                break;
            case Opcode::call: {
                u64 function_index = instruction.index;
                Function& function = functions[function_index];
                // store the function index (for GC)
                push_call_stack(function_index);
                // store the old frame
                push_call_stack(frame_pointer);
                // store the old program counter (of the next instruction)
                push_call_stack(next_program_counter);
                // allocate a new frame
                frame_pointer = call_stack_top;
                call_stack_top += sizeof(Word) * function.varc;
                // store arguments (in reverse order)
                for (u16 i = 1; i <= function.argc; ++i) {
                    localstore(function.argc - i);
                }
                // jump to the function
                next_program_counter = function.instruction_index;
                break;
            }
            case Opcode::ret:
                // deallocate the current frame
                call_stack_top = frame_pointer;
                // restore the old program counter
                next_program_counter = pop_call_stack<u64>();
                // restore the old frame
                frame_pointer = pop_call_stack<u64>();
                pop_call_stack<u64>(); /* function index */
                break;
        }
        program_counter = next_program_counter;
    }
}

/*
int main()
{
    // let's write a simple program to calculate the sum of all number from 1 to n
    int n = 100;
    #define I Instruction
    // 0.  push n
    // 1.  call 3 1 1
    // 2.  exit
    // 3.  push 0
    // 4.  load 0
    // 5.  ifne 7
    // 6.  ret
    // 7.  load 0
    // 8.  iadd
    // 9.  load 0
    // 10. idec
    // 11. store 0
    // 12. goto_ 4
    std::vector<I> instructions{
        I{
            .opcode = OpCode::push,
            .value = bitcast<i64, Word>(n),
        },
        I{
            .opcode = OpCode::call,
            .index = 3,
            .nargs = 1,
            .nvars = 1,
        },
        I{
            .opcode = OpCode::exit,
        },
        I{
            .opcode = OpCode::push,
            .value = bitcast<i64, Word>(0),
        },
        I{
            .opcode = OpCode::load,
            .index = 0,
        },
        I{
            .opcode = OpCode::ifne,
            .index = 7,
        },
        I{
            .opcode = OpCode::ret,
        },
        I{
            .opcode = OpCode::load,
            .index = 0,
        },
        I{
            .opcode = OpCode::iadd,
        },
        I{
            .opcode = OpCode::load,
            .index = 0,
        },
        I{
            .opcode = OpCode::idec,
        },
        I{
            .opcode = OpCode::store,
            .index = 0,
        },
        I{
            .opcode = OpCode::goto_,
            .index = 4,
        },
    };
    #undef I
    VM vm{};
    vm.initialize(instructions, 1024,256, 1024);
    vm.run();
    std::cout << "result is: " << read<i64>(vm.operand_stack, 0) << "\n";
    return 0;
}
*/
