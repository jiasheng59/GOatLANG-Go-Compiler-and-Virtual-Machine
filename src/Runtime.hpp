#include <deque>
#include <memory>
#include <vector>
#include <thread>

#include "Code.hpp"
#include "Heap.hpp"
#include "Common.hpp"
#include "Thread.hpp"

class Runtime
{
public:
    std::vector<Function> function_table;
    std::vector<NativeFunction> native_function_table;
    std::vector<Type> type_table;

    Heap heap;
    // should it be a vector? or should it be a set instead?
    // the thing is, after a thread finish its execution
    // we need to remove this thread from the list of currently running thread

    std::vector<Thread> threads;
};
