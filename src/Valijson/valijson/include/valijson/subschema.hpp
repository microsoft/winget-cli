#pragma once

#include <vector>

#include <memory>

#include <valijson/constraints/constraint.hpp>
#include <valijson/internal/optional.hpp>

namespace valijson {

/**
 * Represents a sub-schema within a JSON Schema
 *
 * While all JSON Schemas have at least one sub-schema, the root, some will
 * have additional sub-schemas that are defined as part of constraints that are
 * included in the schema. For example, a 'oneOf' constraint maintains a set of
 * references to one or more nested sub-schemas. As per the definition of a
 * oneOf constraint, a document is valid within that constraint if it validates
 * against one of the nested sub-schemas.
 */
class Subschema
{
public:
    /// Typedef for custom new-/malloc-like function
    typedef void * (*CustomAlloc)(size_t size);

    /// Typedef for custom free-like function
    typedef void (*CustomFree)(void *);

    /// Typedef the Constraint class into the local namespace for convenience
    typedef constraints::Constraint Constraint;

    /// Typedef for a function that can be applied to each of the Constraint
    /// instances owned by a Schema.
    typedef std::function<bool (const Constraint &)> ApplyFunction;

    /**
     * @brief  Construct a new Subschema object
     */
    Subschema()
      : allocFn(::operator new)
      , freeFn(::operator delete)
      , alwaysInvalid(false) { }

    /**
     * @brief  Construct a new Subschema using custom memory management
     *         functions
     *
     * @param  allocFn  malloc- or new-like function to allocate memory
     *                  within Schema, such as for Subschema instances
     * @param  freeFn   free-like function to free memory allocated with
     *                  the `customAlloc` function
     */
    Subschema(CustomAlloc allocFn, CustomFree freeFn)
      : allocFn(allocFn)
      , freeFn(freeFn)
      , alwaysInvalid(false) { }

    /**
     * @brief  Clean up and free all memory managed by the Subschema
     */
    virtual ~Subschema()
    {
        try {
            for (auto constConstraint : constraints) {
                Constraint *constraint = const_cast<Constraint *>(constConstraint);
                constraint->~Constraint();
                freeFn(constraint);
            }
            constraints.clear();
        } catch (const std::exception &e) {
            fprintf(stderr, "Caught an exception in Subschema destructor: %s",
                    e.what());
        }
    }

    /**
     * @brief  Add a constraint to this sub-schema
     *
     * The constraint will be copied before being added to the list of
     * constraints for this Subschema. Note that constraints will be copied
     * only as deep as references to other Subschemas - e.g. copies of
     * constraints that refer to sub-schemas, will continue to refer to the
     * same Subschema instances.
     *
     * @param  constraint  Reference to the constraint to copy
     */
    void addConstraint(const Constraint &constraint)
    {
        Constraint *newConstraint = constraint.clone(allocFn, freeFn);
        try {
            constraints.push_back(newConstraint);
        } catch (...) {
            newConstraint->~Constraint();
            freeFn(newConstraint);
            throw;
        }
    }

    /**
     * @brief  Invoke a function on each child Constraint
     *
     * This function will apply the callback function to each constraint in
     * the Subschema, even if one of the invokations returns \c false. However,
     * if one or more invokations of the callback function return \c false,
     * this function will also return \c false.
     *
     * @returns  \c true if all invokations of the callback function are
     *           successful, \c false otherwise
     */
    bool apply(ApplyFunction &applyFunction) const
    {
        bool allTrue = true;
        for (const Constraint *constraint : constraints) {
            allTrue = allTrue && applyFunction(*constraint);
        }

        return allTrue;
    }

    /**
     * @brief  Invoke a function on each child Constraint
     *
     * This is a stricter version of the apply() function that will return
     * immediately if any of the invokations of the callback function return
     * \c false.
     *
     * @returns  \c true if all invokations of the callback function are
     *           successful, \c false otherwise
     */
    bool applyStrict(ApplyFunction &applyFunction) const
    {
        for (const Constraint *constraint : constraints) {
            if (!applyFunction(*constraint)) {
                return false;
            }
        }

        return true;
    }

    bool getAlwaysInvalid() const
    {
        return alwaysInvalid;
    }

    /**
     * @brief  Get the description associated with this sub-schema
     *
     * @throws  std::runtime_error if a description has not been set
     *
     * @returns  string containing sub-schema description
     */
    std::string getDescription() const
    {
        if (description) {
            return *description;
        }

        throw std::runtime_error("Schema does not have a description");
    }

    /**
     * @brief  Get the ID associated with this sub-schema
     *
     * @throws  std::runtime_error if an ID has not been set
     *
     * @returns  string containing sub-schema ID
     */
    std::string getId() const
    {
        if (id) {
            return *id;
        }

        throw std::runtime_error("Schema does not have an ID");
    }

    /**
     * @brief  Get the title associated with this sub-schema
     *
     * @throws  std::runtime_error if a title has not been set
     *
     * @returns  string containing sub-schema title
     */
    std::string getTitle() const
    {
        if (title) {
            return *title;
        }

        throw std::runtime_error("Schema does not have a title");
    }

    /**
     * @brief  Check whether this sub-schema has a description
     *
     * @return boolean value
     */
    bool hasDescription() const
    {
        return static_cast<bool>(description);
    }

    /**
     * @brief  Check whether this sub-schema has an ID
     *
     * @return  boolean value
     */
    bool hasId() const
    {
        return static_cast<bool>(id);
    }

    /**
     * @brief  Check whether this sub-schema has a title
     *
     * @return  boolean value
     */
    bool hasTitle() const
    {
        return static_cast<bool>(title);
    }

    void setAlwaysInvalid(bool value)
    {
        alwaysInvalid = value;
    }

    /**
     * @brief  Set the description for this sub-schema
     *
     * The description will not be used for validation, but may be used as part
     * of the user interface for interacting with schemas and sub-schemas. As
     * an example, it may be used as part of the validation error descriptions
     * that are produced by the Validator and ValidationVisitor classes.
     *
     * @param  description  new description
     */
    void setDescription(const std::string &description)
    {
        this->description = description;
    }

    void setId(const std::string &id)
    {
        this->id = id;
    }

    /**
     * @brief  Set the title for this sub-schema
     *
     * The title will not be used for validation, but may be used as part
     * of the user interface for interacting with schemas and sub-schema. As an
     * example, it may be used as part of the validation error descriptions
     * that are produced by the Validator and ValidationVisitor classes.
     *
     * @param  title  new title
     */
    void setTitle(const std::string &title)
    {
        this->title = title;
    }

protected:

    CustomAlloc allocFn;

    CustomFree freeFn;

private:

    // Disable copy construction
    Subschema(const Subschema &);

    // Disable copy assignment
    Subschema & operator=(const Subschema &);

    bool alwaysInvalid;

    /// List of pointers to constraints that apply to this schema.
    std::vector<const Constraint *> constraints;

    /// Schema description (optional)
    opt::optional<std::string> description;

    /// Id to apply when resolving the schema URI
    opt::optional<std::string> id;

    /// Title string associated with the schema (optional)
    opt::optional<std::string> title;
};

} // namespace valijson
