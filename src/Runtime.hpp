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
    u64 init_function_index;
};

class Runtime
{
public:
    Runtime() {};
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

    const Configuration& get_configuration() const
    {
        return configuration;
    }

    void start();

    Configuration configuration;
    std::vector<Function> function_table;
    std::vector<NativeFunction> native_function_table;
    std::vector<Type> type_table;

    Heap heap;

    std::unordered_set<Thread*> thread_pool;
    std::mutex thread_pool_mutex;
    // main thread can only terminate
    // after all spawned thread terminated
    // this behavior is different compared to Go, but easier to implement
    std::condition_variable termination_condition;
};

class RuntimeBuilder
{
public:
    template<typename T>
    void function_table(T&& new_function_table)
    {
        function_table = std::forward<T>(new_function_table);
    }

    template<typename T>
    void native_function_table(T&& new_native_function_table)
    {
        native_function_table = std::forward<T>(new_native_function_table);
    }

    template<typename T>
    void type_table(T&& new_type_table)
    {
        type_table = std::forward<T>(new_type_table);
    }

    void default_configuration()
    {
        configuration.heap_size = 64 * 1024 * 1024; // 64 MB
        configuration.call_stack_size = 8 * 1024; // 8 KB
        configuration.operand_stack_size = 1 * 1024; // 1 KB, 128 values
        configuration.init_function_index = 0;
    }

    Runtime build()
    {
        return Runtime{};
    }
private:
    Configuration configuration;

    std::vector<Function> function_table;
    std::vector<NativeFunction> native_function_table;
    std::vector<Type> type_table;
};

#endif /* RUNTIME_HPP */
