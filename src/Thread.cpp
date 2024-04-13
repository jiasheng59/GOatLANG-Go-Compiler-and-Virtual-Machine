#include <mutex>

#include "Thread.hpp"
#include "Runtime.hpp"

void Thread::initialize()
{
    std::lock_guard lock{runtime.get_thread_pool_mutex()};
    runtime.get_thread_pool().insert(this);
}

void Thread::finalize()
{
    std::lock_guard lock{runtime.get_thread_pool_mutex()};
    auto& thread_pool = runtime.get_thread_pool();
    thread_pool.erase(this);
    if (thread_pool.empty()) {
        runtime.get_termination_condition().notify_all();
    }
}

void Thread::start(u64 init_function_index)
{
    initialize();
    run(init_function_index);
    finalize();
}

void Thread::run(u64 init_function_index)
{
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
        } while (false)

    #define I_ARITH_UNARY(op) GENERIC_UNARY(i64, i64, op)
    #define I_LOGIC_UNARY(op) GENERIC_UNARY(i64, i64, op)
    #define I_BITWISE_UNARY(op) GENERIC_UNARY(i64, i64, op)
    #define F_ARITH_UNARY(op) GENERIC_UNARY(f64, f64, op)

    Heap& heap = runtime.get_heap();
    std::vector<Function>& function_table = runtime.get_function_table();
    std::vector<NativeFunction>& native_function_table = runtime.get_native_function_table();
    instruction_stream.jump_to(function_table[init_function_index]);

    #define INVOKE_FUNCTION(function)                                            \
        do {                                                                     \
            u64 program_counter = instruction_stream.get_program_counter();      \
            call_stack.push_frame(function, program_counter);                    \
            instruction_stream.jump_to(function);                                \
            for (u16 i = 1; i <= function.argc; ++i) {                           \
                Word word = operand_stack.pop<Word>();                           \
                call_stack.store_local(function.capc + function.argc - i, word); \
            }                                                                    \
        } while (false)

    const Instruction* ptr;
    while ((ptr = instruction_stream.next()) != nullptr) {
        const Instruction& instruction = *ptr;
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
            case OpCode::push: {
                operand_stack.push(instruction.value);
                break;
            }
            case OpCode::pop: {
                operand_stack.pop<Word>();
                break;
            }
            case OpCode::wload: {
                u64 address = operand_stack.pop<u64>() + sizeof(Word) * instruction.index;
                Word word = heap.load<Word>(address);
                operand_stack.push(word);
                break;
            }
            case OpCode::bload: {
                u64 address = operand_stack.pop<u64>() + sizeof(Byte) * instruction.index;
                Byte byte = heap.load<Byte>(address);
                Word word = bitcast<Byte, Word>(byte);
                operand_stack.push(word);
                break;
            }
            case OpCode::wstore: {
                Word word = operand_stack.pop<Word>();
                u64 address = operand_stack.pop<u64>() + sizeof(Word) * instruction.index;
                heap.store(address, word);
                break;
            }
            case OpCode::bstore: {
                Word word = operand_stack.pop<Word>();
                Byte byte = bitcast<Word, Byte>(word);
                u64 address = operand_stack.pop<u64>() + sizeof(Byte) * instruction.index;
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
            case OpCode::lnot:
                I_LOGIC_UNARY(!);
                break;
            case OpCode::goto_: {
                instruction_stream.set_program_counter(instruction.index);
                break;
            }
            case OpCode::if_t: {
                if (operand_stack.pop<i64>() != 0) {
                    instruction_stream.set_program_counter(instruction.index);
                }
                break;
            }
            case OpCode::if_f: {
                if (operand_stack.pop<i64>() == 0) {
                    instruction_stream.set_program_counter(instruction.index);
                }
                break;
            }
            case OpCode::invoke_static: {
                const auto& function = function_table[instruction.index];
                INVOKE_FUNCTION(function);
                break;
            }
            case OpCode::invoke_dynamic: {
                u64 address = operand_stack.pop<u64>();
                ClosureHeader closure_header = heap.load<ClosureHeader>(address);
                const auto& function = function_table[closure_header.function_index];
                INVOKE_FUNCTION(function);
                for (u16 i = 0; i < function.capc; ++i) {
                    u64 cap_address = heap.load<u64>(address + sizeof(ClosureHeader) + sizeof(u64) * i);
                    call_stack.store_local(i, cap_address);
                }
                break;
            }
            case OpCode::invoke_native: {
                u64 native_function_index = instruction.index;
                const auto& native_function = native_function_table[native_function_index];
                native_function(runtime, *this);
                break;
            }
            case OpCode::ret: {
                u64 program_counter = call_stack.pop_frame();
                const auto& frame_data = call_stack.peek_frame_data();
                const auto& function = function_table[frame_data.function_index];
                instruction_stream.jump_to(function, program_counter);
                break;
            }
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
    }
}
