/*******************************
 * Written by Ayxan Haqverdili *
 *      March 8 2020           *
 *******************************/

#ifndef MEMORY_ANALYSIS_H
#define MEMORY_ANALYSIS_H
#pragma once // Won't hurt https://stackoverflow.com/a/1144110/10147399

#include <boost/stacktrace.hpp> // boost::stacktrace
#include <cstdio>               // fprintf
#include <type_traits>          // decay
#include <unordered_map>        // unordered_map
#include <utility>              // move

 // An allocator that uses malloc as its memory source
#include "MallocAllocator.h"

// new/delete -> Object vs new[]/delete[] -> Array
enum class AllocationType
{
        Object,
        Array,
};

// Hash map that uses malloc to avoid infinite recursion in operator new
template<typename Key, typename T>
using MallocHashMap =
std::unordered_map<Key,
        T,
        std::hash<Key>,
        std::equal_to<Key>,
        MallocAllocator<std::pair<const Key, T>>>;

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

// returns AllocationType::Object for AllocationType::Array and
// AllocationType::Array for AllocationType::Object
constexpr AllocationType
otherAllocType(AllocationType const at) noexcept
{
        return (at == AllocationType::Object) ? AllocationType::Array
                : AllocationType::Object;
}

// Keeps track of the memory allocated and freed.
template<AllocationType at>
static MallocHashMap<void*, MemoryAllocation>&
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

struct MemInitializer
{ // dummy object to initialize the memory map
        MemInitializer()
        {
                (void)get_mem_map<AllocationType::Array>();
                (void)get_mem_map<AllocationType::Object>();
        }
        MemInitializer(MemInitializer const&) = delete;
        MemInitializer(MemInitializer&&) = delete;
        MemInitializer& operator=(MemInitializer const&) = delete;
        MemInitializer& operator=(MemInitializer&&) = delete;
};

inline const MemInitializer defaultStaticMemInitializer;

#endif // !MEMORY_ANALYSIS_H
