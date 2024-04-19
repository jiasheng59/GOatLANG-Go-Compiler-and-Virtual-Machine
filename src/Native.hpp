// Native function in this language.
// <newthread>
// <newchan>
// <chansend>
// <chanrecv>
// later we can implement the select statement
//

#include <queue>

#include "Thread.hpp"
#include "Runtime.hpp"

// rule for native object?
// we will have a special area for "native object"
// and then in GC we will need to handle this area,
// potentially by going through the area and deallocate the pointers
struct NativeChannel
{
    u64 index;
};

void new_thread(Runtime& runtime, Thread& thread);
void new_chan(Runtime& runtime, Thread& thread);
void chan_send(Runtime& runtime, Thread& thread);
void chan_recv(Runtime& runtime, Thread& thread);
