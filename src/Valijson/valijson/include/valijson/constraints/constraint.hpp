#pragma once

#include <memory>
#include <type_traits>

namespace valijson {
namespace constraints {

class ConstraintVisitor;

/**
 * @brief  Interface that must be implemented by concrete constraint types.
 *
 * @todo  Consider using something like the boost::cloneable concept here.
 */
struct Constraint
{
    /// Typedef for custom new-/malloc-like function
    typedef void * (*CustomAlloc)(size_t size);

    /// Typedef for custom free-like function
    typedef void (*CustomFree)(void *);

    /// Deleter type to be used with std::unique_ptr / std::shared_ptr
    /// @tparam  T  Const or non-const type (same as the one used in unique_ptr/shared_ptr)
    template<typename T>
    struct CustomDeleter
    {
        CustomDeleter(CustomFree freeFn)
          : m_freeFn(freeFn) { }

        void operator()(T *ptr) const
        {
            auto *nonconst = const_cast<typename std::remove_const<T>::type *>(ptr);
            nonconst->~T();
            m_freeFn(nonconst);
        }

    private:
        CustomFree m_freeFn;
    };

    /// Exclusive-ownership pointer to automatically handle deallocation
    typedef std::unique_ptr<const Constraint, CustomDeleter<const Constraint>> OwningPointer;

    /**
     * @brief  Virtual destructor.
     */
    virtual ~Constraint() = default;

    /**
     * @brief  Perform an action on the constraint using the visitor pattern.
     *
     * Note that Constraints cannot be modified by visitors.
     *
     * @param  visitor  Reference to a ConstraintVisitor object.
     *
     * @returns  the boolean value returned by one of the visitor's visit
     *           functions.
     */
    virtual bool accept(ConstraintVisitor &visitor) const = 0;

    /**
     * @brief  Make a copy of a constraint.
     *
     * Note that this should be a deep copy of the constraint.
     *
     * @returns  an owning-pointer to the new constraint.
     */
    virtual OwningPointer clone(CustomAlloc, CustomFree) const = 0;

};

} // namespace constraints
} // namespace valijson
