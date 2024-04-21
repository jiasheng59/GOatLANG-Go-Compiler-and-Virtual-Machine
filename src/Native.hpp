#ifndef NATIVE_HPP
#define NATIVE_HPP

class Runtime;
class Thread;

void new_thread(Runtime& runtime, Thread& thread);
void new_chan(Runtime& runtime, Thread& thread);
void chan_send(Runtime& runtime, Thread& thread);
void chan_recv(Runtime& runtime, Thread& thread);
void sprint(Runtime& runtime, Thread& thread);
void iprint(Runtime& runtime, Thread& thread);
void fprint(Runtime& runtime, Thread& thread);
void new_slice(Runtime& runtime, Thread& thread);

#endif
