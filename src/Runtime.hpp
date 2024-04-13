#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <mutex>
#include <condition_variable>
#include <vector>
#include <unordered_set>

#include "Common.hpp"
#include "Code.hpp"
#include "Heap.hpp"
#include "Thread.hpp"

struct Configuration
{
    u64 heap_size;
    u64 call_stack_size;
    u64 operand_stack_size;
};

class Runtime
{
public:
    Runtime() = default;
    Runtime(const Runtime&) = delete;
    Runtime(Runtime&&) = delete;

    Runtime& operator=(const Runtime&) = delete;
    Runtime& operator=(Runtime&&) = delete;

    std::vector<Function>& get_function_table()
    {
        return function_table;
    }

    std::vector<NativeFunction>& get_native_function_table()
    {
        return native_function_table;
    }

    std::vector<Type>& get_type_table()
    {
        return type_table;
    }

    Heap& get_heap()
    {
        return heap;
    }

    std::unordered_set<Thread*>& get_thread_pool()
    {
        return thread_pool;
    }

    std::mutex& get_thread_pool_mutex()
    {
        return thread_pool_mutex;
    }

    std::condition_variable& get_termination_condition()
    {
        return termination_condition;
    }

    void set_init_function_index(u64 new_init_function_index)
    {
        init_function_index = new_init_function_index;
    }

    void start();

    u64 init_function_index = 0;

    std::vector<Function> function_table;
    std::vector<NativeFunction> native_function_table;
    std::vector<Type> type_table;

    Heap heap;
    Configuration configuration;

    std::unordered_set<Thread*> thread_pool;
    std::mutex thread_pool_mutex;
    // main thread can only terminate
    // after all spawned thread terminated
    // this behavior is different compared to Go, but easier to implement
    std::condition_variable termination_condition;
};

#endif /* RUNTIME_HPP */
