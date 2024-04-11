#include "Thread.hpp"
#include "Runtime.hpp"

#define GENERIC_BINARY(T, R, op)      \
    do {                              \
        T y = operand_stack.pop<T>(); \
        T x = operand_stack.pop<T>(); \
        R r = x op y;                 \
        operand_stack.push(r);        \
    } while (false)

#define I_ARITH_BINARY(op) GENERIC_BINARY(i64, i64, op)
#define I_LOGIC_BINARY(op) GENERIC_BINARY(i64, i64, op)
#define I_BITWISE_BINARY(op) GENERIC_BINARY(i64, i64, op)

#define F_ARITH_BINARY(op) GENERIC_BINARY(f64, f64, op)
#define F_LOGIC_BINARY(op) GENERIC_BINARY(f64, i64, op)

#define GENERIC_UNARY(T, R, op)       \
    do {                              \
        T x = operand_stack.pop<T>(); \
        R r = op x;                   \
        operand_stack.push(r);        \
    } while (0)

#define I_ARITH_UNARY(op) GENERIC_UNARY(i64, i64, op)
#define I_LOGIC_UNARY(op) GENERIC_UNARY(i64, i64, op)
#define I_BITWISE_UNARY(op) GENERIC_UNARY(i64, i64, op)

#define F_ARITH_UNARY(op) GENERIC_UNARY(f64, f64, op)

void Thread::run()
{
    const Function* current_function = &runtime.function_table[function_index];
    const std::vector<Instruction>* code = &current_function->code;

    while (program_counter < code->size()) {
        u64 next_program_counter = program_counter + 1;
        const Instruction& instruction = (*code)[program_counter];
        switch (instruction.opcode) {
            case OpCode::nop:
                break;
            case OpCode::load: {
                Word word = call_stack.load_local<Word>(instruction.index);
                operand_stack.push(word);
                break;
            }
            case OpCode::store: {
                Word word = operand_stack.pop<Word>();
                call_stack.store_local(instruction.index, word);
                break;
            }
            case OpCode::push:
                operand_stack.push(instruction.value);
                break;
            case OpCode::pop:
                operand_stack.pop<Word>();
                break;
            case OpCode::wload: {
                u64 address = operand_stack.pop<u64>() + instruction.index;
                Word word = heap.load<Word>(address);
                operand_stack.push(word);
                break;
            }
            case OpCode::bload: {
                u64 address = operand_stack.pop<u64>() + instruction.index;
                Byte byte = heap.load<Byte>(address);
                Word word = bitcast<Byte, Word>(byte);
                operand_stack.push(word);
                break;
            }
            case OpCode::wstore: {
                Word word = operand_stack.pop<Word>();
                u64 address = operand_stack.pop<u64>() + instruction.index;
                heap.store(address, word);
                break;
            }
            case OpCode::bstore: {
                Word word = operand_stack.pop<Word>();
                Byte byte = bitcast<Word, Byte>(word);
                u64 address = operand_stack.pop<u64>() + instruction.index;
                heap.store(address, byte);
                break;
            }
            case OpCode::i2f: {
                i64 i = operand_stack.pop<i64>();
                f64 f = static_cast<f64>(i);
                operand_stack.push(f);
                break;
            }
            case OpCode::f2i: {
                f64 f = operand_stack.pop<f64>();
                i64 i = static_cast<i64>(f);
                operand_stack.push(i);
                break;
            }
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
            case OpCode::iinc:
                I_ARITH_UNARY(++);
                break;
            case OpCode::idec:
                I_ARITH_UNARY(--);
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
            case OpCode::goto_: {
                next_program_counter = instruction.index;
                break;
            }
            case OpCode::if_t: {
                if (operand_stack.pop<i64>()) {
                    next_program_counter = instruction.index;
                }
                break;
            }
            case OpCode::if_f: {
                if (!operand_stack.pop<i64>()) {
                    next_program_counter = instruction.index;
                }
                break;
            }
            // FUNCTION INVOCATION
            case OpCode::invoke_static: {
                // call_stack.push_frame(function_index, next_program_counter);
                
                // function_index = instruction.index;
                // next_program_counter = 0;
                // current_function = &function_table[function_index];
                // code = &current_function->code;
                
                // // allocate a new frame
                // for (u16 i = 1; i <= current_function->argc; ++i) {
                //     Word word = operand_stack.pop<Word>();
                //     call_stack.store_local(current_function->argc - i, word);
                // }
                // break;
            }
            case OpCode::invoke_dynamic: {
                // expect an address to a closure
                // u64 address = operand_stack.pop<u64>();
                // ClosureHeader closure_header = heap.load<ClosureHeader>(address);

                // function_index = closure_header.function_index;
                // next_program_counter = 0;
                // current_function = &function_table[function_index];
                // code = &current_function->code;

                // call_stack.push_frame(function_index, next_program_counter);
                // break;
            }
            case OpCode::invoke_native: {
                u64 native_function_index = instruction.index;
                NativeFunction native_function = runtime.native_function_table[native_function_index];
                native_function(runtime, *this);
                break;
            }
            case OpCode::ret: {
                // call_stack.pop_frame()
            }
            // MEMORY ALLOCATION
            case OpCode::new_: {
                u64 address = heap.allocate(instruction.index, 1);
                operand_stack.push(address);
                break;
            }
            case OpCode::new_array: {
                u64 count = operand_stack.pop<u64>();
                u64 address = heap.allocate(instruction.index, count);
                operand_stack.push(address);
                break;
            }
            case OpCode::array_length: {
                u64 address = operand_stack.pop<u64>();
                u64 count = heap.count(address);
                operand_stack.push(count);
                break;
            }
        }
        program_counter = next_program_counter;
    }
}
