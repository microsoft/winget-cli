#pragma once

#include <valijson/adapters/adapter.hpp>

namespace valijson {
namespace adapters {

/**
 * @brief   An interface that provides minimal access to a stored JSON value.
 *
 * The main reason that this interface exists is to support the 'enum'
 * constraint. Each Adapter type is expected to provide an implementation of
 * this interface. That class should be able to maintain its own copy of a
 * JSON value, independent of the original document.
 *
 * This interface currently provides just the clone and equalTo functions, but
 * could be expanded to include other functions declared in the Adapter
 * interface.
 *
 * @todo  it would be nice to better integrate this with the Adapter interface
 */
class FrozenValue
{
public:

    /**
     * @brief   Virtual destructor defined to ensure deletion via base-class
     *          pointers is safe.
     */
    virtual ~FrozenValue() { }

    /**
     * @brief   Clone the stored value and return a pointer to a new FrozenValue
     *          object containing the value.
     */
    virtual FrozenValue *clone() const = 0;

    /**
     * @brief   Return true if the stored value is equal to the value contained
     *          by an Adapter instance.
     *
     * @param   adapter  Adapter to compare value against
     * @param   strict   Flag to use strict type comparison
     *
     * @returns true if values are equal, false otherwise
     */
    virtual bool equalTo(const Adapter &adapter, bool strict) const = 0;

};

}  // namespace adapters
}  // namespace valijson
