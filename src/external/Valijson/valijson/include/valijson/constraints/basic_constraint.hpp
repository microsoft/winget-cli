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

    Constraint * clone(CustomAlloc allocFn, CustomFree) const override
    {
        void *ptr = allocFn(sizeof(ConstraintType));
        if (!ptr) {
            throwRuntimeError("Failed to allocate memory for cloned constraint");
        }

        return new (ptr) ConstraintType(*static_cast<const ConstraintType*>(this));
    }

protected:

    Allocator m_allocator;
};

} // namespace constraints
} // namespace valijson
