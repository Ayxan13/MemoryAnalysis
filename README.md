# MemoryAnalysis
An overload of overload of global operator new and global operator delete to explicitly check for memory leaks and freeing invalid memory. 


This uses `boost/stacktrace.hpp` to point out exactly where you messed up. You must have boost configured.
This uses [MallocAllocator.h](https://github.com/Ayxan13/MallocAllocator/blob/master/MallocAllocator.h)