/*******************************
 * Written by Ayxan Haqverdili *
 *      March 8 2020           *
 *******************************/

#ifndef MEMORY_ANALYSIS_H
#define MEMORY_ANALYSIS_H
#pragma once // Won't hurt https://stackoverflow.com/a/1144110/10147399

#include <unordered_map> // unordered_map

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

// Memory allocation info with, address, size, stackTrace
struct MemoryAllocation;

// Keeps track of the memory allocated and freed.
template<AllocationType at>
MallocHashMap<void*, MemoryAllocation>&
get_mem_map();

struct MemInitializer
{ // dummy object to initialize the memory map
        MemInitializer();
        MemInitializer(MemInitializer const&) = delete;
        MemInitializer(MemInitializer&&) = delete;
        MemInitializer& operator=(MemInitializer const&) = delete;
        MemInitializer& operator=(MemInitializer&&) = delete;
};

static MemInitializer
defaultStaticMemInitializer; // Ensure to initialize the memory map

#endif // !MEMORY_ANALYSIS_H
