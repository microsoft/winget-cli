#include "pch.h"

struct MemGuard
{
    MemGuard(void* ptr)
        : m_ptr{ ptr }
    {}

    ~MemGuard()
    {
        xlang_mem_free(m_ptr);
    }

    void* m_ptr{};
};

TEST_CASE("Mem alloc")
{
    SECTION("0 byte allocations")
    {
        MemGuard ptr1{ xlang_mem_alloc(0) };
        MemGuard ptr2{ xlang_mem_alloc(0) };

        REQUIRE(ptr1.m_ptr != nullptr);
        REQUIRE(ptr2.m_ptr != nullptr);
        REQUIRE(ptr1.m_ptr != ptr2.m_ptr);
    }
    SECTION("1 byte allocations")
    {
        MemGuard ptr1{ xlang_mem_alloc(1) };
        MemGuard ptr2{ xlang_mem_alloc(1) };

        REQUIRE(ptr1.m_ptr != nullptr);
        REQUIRE(ptr2.m_ptr != nullptr);
        REQUIRE(ptr1.m_ptr != ptr2.m_ptr);

        // Ensure I can write to those locations
        *static_cast<char*>(ptr1.m_ptr) = 'a';
        *static_cast<char*>(ptr2.m_ptr) = 'b';
    }
    SECTION("1 MB allocation")
    {
        // 1 MB is small enough that it should be essentially guaranteed to succeed
        // in this setting, but big enough that a misconfigured heap might fail.
        constexpr size_t size = 0x100000;
        MemGuard ptr{ xlang_mem_alloc(size) };
        REQUIRE(ptr.m_ptr != nullptr);
        char* begin = static_cast<char*>(ptr.m_ptr);
        std::fill(begin, begin + size, 'a');
    }
    SECTION("Impossibly huge allocation")
    {
        // This is waaaay too much memory, requiring the entire address space.
        // Any implementation that claims success is lying. :)
        MemGuard ptr{ xlang_mem_alloc(std::numeric_limits<size_t>::max()) };
        REQUIRE(ptr.m_ptr == nullptr);
        // This will also check xlang_mem_free with null
    }
}
