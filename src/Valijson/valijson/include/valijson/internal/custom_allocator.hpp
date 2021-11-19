#pragma once

namespace valijson {
namespace internal {

template<class T>
class CustomAllocator
{
public:
    /// Typedef for custom new-/malloc-like function
    typedef void * (*CustomAlloc)(size_t size);

    /// Typedef for custom free-like function
    typedef void (*CustomFree)(void *);

    // Standard allocator typedefs
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    template<typename U>
    struct rebind
    {
        typedef CustomAllocator<U> other;
    };

    CustomAllocator()
      : m_allocFn(::operator new),
        m_freeFn(::operator delete) { }

    CustomAllocator(CustomAlloc allocFn, CustomFree freeFn)
      : m_allocFn(allocFn),
        m_freeFn(freeFn) { }

    CustomAllocator(const CustomAllocator &other)
      : m_allocFn(other.m_allocFn),
        m_freeFn(other.m_freeFn) { }

    template<typename U>
    CustomAllocator(CustomAllocator<U> const &other)
      : m_allocFn(other.m_allocFn),
        m_freeFn(other.m_freeFn) { }

    CustomAllocator & operator=(const CustomAllocator &other)
    {
        m_allocFn = other.m_allocFn;
        m_freeFn = other.m_freeFn;

        return *this;
    }

    pointer address(reference r)
    {
        return &r;
    }

    const_pointer address(const_reference r)
    {
        return &r;
    }

    pointer allocate(size_type cnt, const void * = nullptr)
    {
        return reinterpret_cast<pointer>(m_allocFn(cnt * sizeof(T)));
    }

    void deallocate(pointer p, size_type)
    {
        m_freeFn(p);
    }

    size_type max_size() const
    {
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    void construct(pointer p, const T& t)
    {
        new(p) T(t);
    }

    void destroy(pointer p)
    {
        p->~T();
    }

    bool operator==(const CustomAllocator &other) const
    {
        return other.m_allocFn == m_allocFn && other.m_freeFn == m_freeFn;
    }

    bool operator!=(const CustomAllocator &other) const
    {
        return !operator==(other);
    }

    CustomAlloc m_allocFn;

    CustomFree m_freeFn;
};

} // end namespace internal
} // end namespace valijson
