// Native function in this language.
// <newthread>
// <newchan>
// <chansend>
// <chanrecv>
// later we can implement the select statement
//

#include "Thread.hpp"
#include "Runtime.hpp"

void new_thread(Runtime& runtime, Thread& thread);
void new_chan(Runtime& runtime, Thread& thread);
void chan_send(Runtime& runtime, Thread& thread);
void chan_recv(Runtime& runtime, Thread& thread);
