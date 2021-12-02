#pragma once

#include <cstdio>
#include <set>

#include <valijson/subschema.hpp>
#include <valijson/exceptions.hpp>

namespace valijson {

/**
 * Represents the root of a JSON Schema
 *
 * The root is distinct from other sub-schemas because it is the canonical
 * starting point for validation of a document against a given a JSON Schema.
 */
class Schema: public Subschema
{
public:
    /**
     * @brief  Construct a new Schema instance with no constraints
     */
    Schema()
      : sharedEmptySubschema(newSubschema()) { }

    /**
     * @brief  Construct a new Schema using custom memory management
     *         functions
     *
     * @param  allocFn  malloc- or new-like function to allocate memory
     *                  within Schema, such as for Subschema instances
     * @param  freeFn   free-like function to free memory allocated with
     *                  the `customAlloc` function
     */
    Schema(CustomAlloc allocFn, CustomFree freeFn)
      : Subschema(allocFn, freeFn),
        sharedEmptySubschema(newSubschema()) { }

    // Disable copy construction
    Schema(const Schema &) = delete;

    // Disable copy assignment
    Schema & operator=(const Schema &) = delete;

    /**
     * @brief  Clean up and free all memory managed by the Schema
     *
     * Note that any Subschema pointers created and returned by this Schema
     * should be considered invalid.
     */
    ~Schema() override
    {
        sharedEmptySubschema->~Subschema();
        m_freeFn(const_cast<Subschema *>(sharedEmptySubschema));
        sharedEmptySubschema = nullptr;

#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            for (auto subschema : subschemaSet) {
                subschema->~Subschema();
                m_freeFn(subschema);
            }
#if VALIJSON_USE_EXCEPTIONS
        } catch (const std::exception &e) {
            fprintf(stderr, "Caught an exception while destroying Schema: %s",
                    e.what());
        }
#endif
    }

    /**
     * @brief  Copy a constraint to a specific sub-schema
     *
     * @param  constraint  reference to a constraint that will be copied into
     *                     the sub-schema
     * @param  subschema   pointer to the sub-schema that will own the copied
     *                     constraint
     *
     * @throws std::runtime_error if the sub-schema is not owned by this Schema
     *         instance
     */
    void addConstraintToSubschema(const Constraint &constraint,
            const Subschema *subschema)
    {
        // TODO: Check heirarchy for subschemas that do not belong...

        mutableSubschema(subschema)->addConstraint(constraint);
    }

    /**
     * @brief  Create a new Subschema instance that is owned by this Schema
     *
     * @returns  const pointer to the new Subschema instance
     */
    const Subschema * createSubschema()
    {
        Subschema *subschema = newSubschema();

#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            if (!subschemaSet.insert(subschema).second) {
                throwRuntimeError(
                        "Failed to store pointer for new sub-schema");
            }
#if VALIJSON_USE_EXCEPTIONS
        } catch (...) {
            subschema->~Subschema();
            m_freeFn(subschema);
            throw;
        }
#endif
        return subschema;
    }

    /**
     * @brief  Return a pointer to the shared empty schema
     */
    const Subschema * emptySubschema() const
    {
        return sharedEmptySubschema;
    }

    /**
     * @brief  Get a pointer to the root sub-schema of this Schema instance
     */
    const Subschema * root() const
    {
        return this;
    }

    void setAlwaysInvalid(const Subschema *subschema, bool value)
    {
        mutableSubschema(subschema)->setAlwaysInvalid(value);
    }

    /**
     * @brief  Update the description for one of the sub-schemas owned by this
     *         Schema instance
     *
     * @param  subschema    sub-schema to update
     * @param  description  new description
     */
    void setSubschemaDescription(const Subschema *subschema,
            const std::string &description)
    {
        mutableSubschema(subschema)->setDescription(description);
    }

    /**
     * @brief  Update the ID for one of the sub-schemas owned by this Schema
     *         instance
     *
     * @param  subschema  sub-schema to update
     * @param  id         new ID
     */
    void setSubschemaId(const Subschema *subschema, const std::string &id)
    {
        mutableSubschema(subschema)->setId(id);
    }

    /**
     * @brief  Update the title for one of the sub-schemas owned by this Schema
     *         instance
     *
     * @param  subschema  sub-schema to update
     * @param  title      new title
     */
    void setSubschemaTitle(const Subschema *subschema, const std::string &title)
    {
        mutableSubschema(subschema)->setTitle(title);
    }

private:

    Subschema *newSubschema()
    {
        void *ptr = m_allocFn(sizeof(Subschema));
        if (!ptr) {
            throwRuntimeError(
                    "Failed to allocate memory for shared empty sub-schema");
        }

#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            return new (ptr) Subschema();
#if VALIJSON_USE_EXCEPTIONS
        } catch (...) {
            m_freeFn(ptr);
            throw;
        }
#endif
    }

    Subschema * mutableSubschema(const Subschema *subschema)
    {
        if (subschema == this) {
            return this;
        }

        if (subschema == sharedEmptySubschema) {
            throwRuntimeError(
                    "Cannot modify the shared empty sub-schema");
        }

        auto *noConst = const_cast<Subschema*>(subschema);
        if (subschemaSet.find(noConst) == subschemaSet.end()) {
            throwRuntimeError(
                    "Subschema pointer is not owned by this Schema instance");
        }

        return noConst;
    }

    /// Set of Subschema instances owned by this schema
    std::set<Subschema*> subschemaSet;

    /// Empty schema that can be reused by multiple constraints
    const Subschema *sharedEmptySubschema;
};

} // namespace valijson
