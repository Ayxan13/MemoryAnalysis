/*******************************
 * Written by Ayxan Haqverdili *
 *      March 7 2020           *
 *******************************/

 /*
  * An overload of overload of global operator new
  * and global operator delete to explicitly check
  * for memory leaks and freeing invalid memory.
  * This uses boost/stacktrace.hpp to point out
  * exactly where you messed up. It is just for
  * educational and possibly debugging purposes:
  */

#include <boost/stacktrace.hpp> // boost::stacktrace
#include <cstdio>               // fprintf
#include <type_traits>          // decay
#include <utility>              // move

#include "MemoryAnalysis.h"

  // returns AllocationType::Object for AllocationType::Array and
  // AllocationType::Array for AllocationType::Object
static constexpr AllocationType
otherAllocType(AllocationType const at) noexcept
{
        return (at == AllocationType::Object) ? AllocationType::Array
                : AllocationType::Object;
}

// Stack trace that uses malloc to avoid infinite recursion in operator new
using MallocStackTrace = boost::stacktrace::basic_stacktrace<
        MallocAllocator<boost::stacktrace::frame>>;

// Memory allocation info with, address, size, stackTrace
struct MemoryAllocation
{
        // the constructor is so that we can use emplace in unordered_map
        MemoryAllocation(void const* const address_,
                std::size_t const size_,
                MallocStackTrace trace_) noexcept
                : address{ address_ }
                , size{ size_ }
                , stackTrace{ std::move(trace_) }
        {}
        void const* address;
        std::size_t size;
        MallocStackTrace stackTrace;
};

// Keeps track of the memory allocated and freed.
template<AllocationType at>
MallocHashMap<void*, MemoryAllocation>&
get_mem_map()
{
        struct Mem // wrapper around std::unordered_map to check for memory leaks in
                   // destructor
        {
                typename std::decay<decltype(get_mem_map<at>())>::type map; // return type

                ~Mem() noexcept
                {
                        for (auto const& p : map) {
                                auto const& memAlloc = p.second;
                                std::fprintf(stderr,
                                        "-------------------------------------------------------"
                                        "----------\n"
                                        "Memory leak! %zu bytes. Memory was allocated in\n%s"
                                        "-------------------------------------------------------"
                                        "----------\n\n",
                                        memAlloc.size,
                                        to_string(memAlloc.stackTrace).c_str());
                        }
                }
        };

        static Mem mem;
        return mem.map;
}

// test if the memory was allocated with new or new[]
template<AllocationType at>
static bool
allocatedAs(void* const ptr)
{
        auto& memmap = get_mem_map<at>();
        auto const it = memmap.find(ptr);
        return it != memmap.end();
}

static MallocStackTrace
new_delete_stack_trace()
{
        return MallocStackTrace{ 4, 100 };
}

template<AllocationType at>
static void* // template for new and new[]
new_base(std::size_t const count)
{
        void* p = std::malloc(count);

        if (!p)
                throw std::bad_alloc{};

        auto& mem_map = get_mem_map<at>();

        mem_map.emplace(std::piecewise_construct,
                std::forward_as_tuple(p),
                std::forward_as_tuple(p, count, new_delete_stack_trace()));
        return p;
}

template<AllocationType at>
static void // template for delete and delete[]
delete_base(void* const ptr) noexcept
{
        if (ptr) {                     // it's fine to delete nullptr
                if (!allocatedAs<at>(ptr)) { // check for invalid memory free
                        std::fprintf(
                                stderr,
                                "-----------------------------------------------------------------\n"
                                "Invalid memory freed memory at %p\n",
                                ptr);
                        if (allocatedAs<otherAllocType(at)>(ptr)) {
                                auto constexpr myNew = at == AllocationType::Object ? "" : "[]";
                                auto constexpr otherNew = at == AllocationType::Array ? "" : "[]";

                                std::fprintf(
                                        stderr,
                                        "\tNote: this memory was allocated with `new%s` but deleted with "
                                        "`delete%s` instead of `delete%s`\n",
                                        otherNew,
                                        myNew,
                                        otherNew);
                        }
                        std::fprintf(
                                stderr,
                                "%s\n---------------------------------------------------------"
                                "--------\n\n",
                                to_string(new_delete_stack_trace()).c_str());

                        std::_Exit(-1); // If we've messed up, exit
                }

                std::free(ptr);               // free the pointer
                get_mem_map<at>().erase(ptr); // remove from the map
        }
}

void*
operator new(std::size_t const count)
{
        return new_base<AllocationType::Object>(count);
}

void*
operator new[](std::size_t const count)
{
        return new_base<AllocationType::Array>(count);
}

void
operator delete(void* const ptr) noexcept
{
        delete_base<AllocationType::Object>(ptr);
}

void
operator delete[](void* const ptr) noexcept
{
        delete_base<AllocationType::Array>(ptr);
}

MemInitializer::MemInitializer()
{
        operator delete(operator new(0)); // initialize memory maps
        operator delete[](operator new[](0));
}
