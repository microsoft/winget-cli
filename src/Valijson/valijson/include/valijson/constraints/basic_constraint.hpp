#pragma once

#include <valijson/constraints/constraint.hpp>
#include <valijson/constraints/constraint_visitor.hpp>
#include <valijson/internal/custom_allocator.hpp>
#include <valijson/exceptions.hpp>

namespace valijson {
namespace constraints {

/**
 * @brief   Template class that implements the accept() and clone() functions of the Constraint interface.
 *
 * @tparam  ConstraintType   name of the concrete constraint type, which must provide a copy constructor.
 */
template<typename ConstraintType>
struct BasicConstraint: Constraint
{
    typedef internal::CustomAllocator<void *> Allocator;

    typedef std::basic_string<char, std::char_traits<char>, internal::CustomAllocator<char>> String;

    BasicConstraint()
      : m_allocator() { }

    BasicConstraint(Allocator::CustomAlloc allocFn, Allocator::CustomFree freeFn)
      : m_allocator(allocFn, freeFn) { }

    BasicConstraint(const BasicConstraint &other)
      : m_allocator(other.m_allocator) { }

    ~BasicConstraint() override = default;

    bool accept(ConstraintVisitor &visitor) const override
    {
        return visitor.visit(*static_cast<const ConstraintType*>(this));
    }

    OwningPointer clone(CustomAlloc allocFn, CustomFree freeFn) const override
    {
        // smart pointer to automatically free raw memory on exception
        typedef std::unique_ptr<Constraint, CustomFree> RawOwningPointer;
        auto ptr = RawOwningPointer(static_cast<Constraint*>(allocFn(sizeof(ConstraintType))), freeFn);
        if (!ptr) {
            throwRuntimeError("Failed to allocate memory for cloned constraint");
        }

        // constructor might throw but the memory will be taken care of anyways
        (void)new (ptr.get()) ConstraintType(*static_cast<const ConstraintType*>(this));

        // implicitly convert to smart pointer that will also destroy object instance
        return ptr;
    }

protected:

    Allocator m_allocator;
};

} // namespace constraints
} // namespace valijson
