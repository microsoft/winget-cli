#pragma once

#include <valijson/constraints/constraint.hpp>
#include <valijson/constraints/constraint_visitor.hpp>

#include <valijson/internal/custom_allocator.hpp>

namespace valijson {
namespace constraints {

/**
 * @brief   Template class that implements the accept() and clone() functions of
 *          the Constraint interface.
 *
 * @tparam  ConstraintType   name of the concrete constraint type, which must
 *                           provide a copy constructor.
 */
template<typename ConstraintType>
struct BasicConstraint: Constraint
{
    typedef internal::CustomAllocator<void *> Allocator;

    typedef std::basic_string<char, std::char_traits<char>,
            internal::CustomAllocator<char> > String;

    BasicConstraint()
      : allocator() { }

    BasicConstraint(Allocator::CustomAlloc allocFn, Allocator::CustomFree freeFn)
      : allocator(allocFn, freeFn) { }

    BasicConstraint(const BasicConstraint &other)
      : allocator(other.allocator) { }

    virtual ~BasicConstraint<ConstraintType>() { }

    virtual bool accept(ConstraintVisitor &visitor) const
    {
        return visitor.visit(*static_cast<const ConstraintType*>(this));
    }

    virtual Constraint * clone(CustomAlloc allocFn, CustomFree freeFn) const
    {
        void *ptr = allocFn(sizeof(ConstraintType));
        if (!ptr) {
            throw std::runtime_error(
                    "Failed to allocate memory for cloned constraint");
        }

        try {
            return new (ptr) ConstraintType(
                    *static_cast<const ConstraintType*>(this));
        } catch (...) {
            freeFn(ptr);
            throw;
        }
    }

protected:

    Allocator allocator;
};

} // namespace constraints
} // namespace valijson
