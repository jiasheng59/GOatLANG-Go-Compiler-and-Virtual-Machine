#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <mutex>
#include <condition_variable>

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

    std::vector<Function> function_table;
    std::vector<NativeFunction> native_function_table;
    std::vector<Type> type_table;

    Heap heap;
    Configuration configuration;

    std::vector<Thread> threads;
    std::mutex threads_mutex;
    // main thread can only terminate
    // after all spawned thread terminated
    // this behavior is different compared to Go, but easier to implement
    std::condition_variable no_thread;
};

#endif /* RUNTIME_HPP */
