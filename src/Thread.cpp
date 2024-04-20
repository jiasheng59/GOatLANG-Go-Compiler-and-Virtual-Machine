#include <mutex>

#include "Runtime.hpp"
#include "Thread.hpp"

Thread::Thread(Runtime& runtime) : runtime{&runtime},
                                   instruction_stream{},
                                   call_stack{runtime.configuration.call_stack_size},
                                   operand_stack{runtime.configuration.operand_stack_size}
{
}

void Thread::initialize()
{
    std::lock_guard lock{runtime->get_thread_pool_mutex()};
    runtime->get_thread_pool().insert(this);
}

void Thread::finalize()
{
    std::lock_guard lock{runtime->get_thread_pool_mutex()};
    auto& thread_pool = runtime->get_thread_pool();
    thread_pool.erase(this);
    if (thread_pool.empty()) {
        runtime->get_termination_condition().notify_all();
    }
}

void Thread::start()
{
    initialize();
    run();
    finalize();
}

void Thread::run()
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

    Heap& heap = runtime->get_heap();
    std::vector<Function>& function_table = runtime->get_function_table();
    std::vector<NativeFunction>& native_function_table = runtime->get_native_function_table();
    auto& type_table = runtime->type_table;

#define INVOKE_FUNCTION(function)                                       \
    do {                                                                \
        u64 program_counter = instruction_stream.get_program_counter(); \
        call_stack.push_frame(function, program_counter);               \
        instruction_stream.jump_to(function);                           \
    } while (false)

    const Instruction* ptr;
    while ((ptr = instruction_stream.next()) != nullptr) {
        const Instruction& instruction = *ptr;
        switch (instruction.opcode) {
            case Opcode::nop:
                break;
            case Opcode::load: {
                Word word = call_stack.load_local<Word>(instruction.index);
                operand_stack.push(word);
                break;
            }
            case Opcode::store: {
                Word word = operand_stack.pop<Word>();
                call_stack.store_local(instruction.index, word);
                break;
            }
            case Opcode::push: {
                operand_stack.push(instruction.value);
                break;
            }
            case Opcode::pop: {
                operand_stack.pop<Word>();
                break;
            }
            case Opcode::dup: {
                Word word = operand_stack.peek<Word>();
                operand_stack.push(word);
                break;
            }
            case Opcode::swap: {
                Word y = operand_stack.pop<Word>();
                Word x = operand_stack.pop<Word>();
                operand_stack.push(y);
                operand_stack.push(x);
                break;
            }
            case Opcode::wload: {
                u64 address = operand_stack.pop<u64>() + sizeof(Word) * instruction.index;
                Word word = heap.load<Word>(address);
                operand_stack.push(word);
                break;
            }
            case Opcode::bload: {
                u64 address = operand_stack.pop<u64>() + sizeof(Byte) * instruction.index;
                Byte byte = heap.load<Byte>(address);
                Word word = bitcast<Byte, Word>(byte);
                operand_stack.push(word);
                break;
            }
            // TODO
            case Opcode::wstore: {
                Word word = operand_stack.pop<Word>();
                u64 address = operand_stack.pop<u64>() + sizeof(Word) * instruction.index;
                heap.store(address, word);
                break;
            }
            case Opcode::bstore: {
                Word word = operand_stack.pop<Word>();
                Byte byte = bitcast<Word, Byte>(word);
                u64 address = operand_stack.pop<u64>() + sizeof(Byte) * instruction.index;
                heap.store(address, byte);
                break;
            }
            case Opcode::i2f: {
                i64 i = operand_stack.pop<i64>();
                f64 f = static_cast<f64>(i);
                operand_stack.push(f);
                break;
            }
            case Opcode::f2i: {
                f64 f = operand_stack.pop<f64>();
                i64 i = static_cast<i64>(f);
                operand_stack.push(i);
                break;
            }
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
            case Opcode::lnot:
                I_LOGIC_UNARY(!);
                break;
            case Opcode::goto_: {
                instruction_stream.set_program_counter(instruction.index);
                break;
            }
            case Opcode::if_t: {
                if (operand_stack.pop<i64>() != 0) {
                    instruction_stream.set_program_counter(instruction.index);
                }
                break;
            }
            case Opcode::if_f: {
                if (operand_stack.pop<i64>() == 0) {
                    instruction_stream.set_program_counter(instruction.index);
                }
                break;
            }
            case Opcode::invoke_static: {
                const auto& function = function_table[instruction.index];
                INVOKE_FUNCTION(function);
                break;
            }
            case Opcode::invoke_dynamic: {
                u64 address = operand_stack.pop<u64>();
                auto& closure_header = heap.load<ClosureHeader>(address);
                const auto& function = function_table[closure_header.index];
                for (u16 i = 0; i < function.capc; ++i) {
                    u64 cap_address = heap.load<u64>(address + sizeof(ClosureHeader) + sizeof(u64) * i);
                    call_stack.store_local(i, cap_address);
                }
                INVOKE_FUNCTION(function);
                break;
            }
            case Opcode::invoke_native: {
                u64 native_function_index = instruction.index;
                const auto& native_function = native_function_table[native_function_index];
                native_function(*runtime, *this);
                break;
            }
            case Opcode::ret: {
                u64 program_counter = call_stack.pop_frame();
                if (call_stack.empty()) {
                    return;
                }
                const auto& frame_data = call_stack.peek_frame_data();
                const auto& function = function_table[frame_data.function_index];
                instruction_stream.jump_to(function, program_counter);
                break;
            }
            case Opcode::new_: {
                const auto& type = *type_table[instruction.index];
                u64 address = heap.allocate(type, 1);
                operand_stack.push(address);
                break;
            }
        }
    }
}
