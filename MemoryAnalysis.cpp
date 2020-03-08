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




#include "MemoryAnalysis.h"



// test if the memory was allocated with new or new[]
template<AllocationType at>
static bool
allocatedAs(void* const ptr)
{
        auto& memmap = get_mem_map<at>();
        auto const it = memmap.find(ptr);
        return it != memmap.end();
}

MallocStackTrace
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
