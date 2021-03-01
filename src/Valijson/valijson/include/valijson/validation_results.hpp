#pragma once

#include <deque>
#include <string>
#include <vector>

namespace valijson {

/**
 * @brief  Class that encapsulates the storage of validation errors.
 *
 * This class maintains an internal FIFO queue of errors that are reported
 * during validation. Errors are pushed on to the back of an internal
 * queue, and can retrieved by popping them from the front of the queue.
 */
class ValidationResults
{
public:

    /**
     * @brief  Describes a validation error.
     *
     * This struct is used to pass around the context and description of a
     * validation error.
     */
    struct Error
    {
        /**
         * @brief  Construct an Error object with no context or description.
         */
        Error() { }

        /**
         * @brief  Construct an Error object using a context and description.
         *
         * @param  context      Context string to use
         * @param  description  Description string to use
         */
        Error(const std::vector<std::string> &context, const std::string &description)
          : context(context),
            description(description) { }

        /// Path to the node that failed validation.
        std::vector<std::string> context;

        /// A detailed description of the validation error.
        std::string description;
    };

    /**
     * @brief  Return begin iterator for results in the queue.
     */
    std::deque<Error>::const_iterator begin() const
    {
        return errors.begin();
    }

    /**
     * @brief  Return end iterator for results in the queue.
     */
    std::deque<Error>::const_iterator end() const
    {
        return errors.end();
    }

    /**
     * @brief  Return the number of errors in the queue.
     */
    size_t numErrors() const
    {
        return errors.size();
    }

    /**
     * @brief  Copy an Error and push it on to the back of the queue.
     *
     * @param  error  Reference to an Error object to be copied.
     */
    void pushError(const Error &error)
    {
        errors.push_back(error);
    }

    /**
     * @brief  Push an error onto the back of the queue.
     *
     * @param  context      Context of the validation error.
     * @param  description  Description of the validation error.
     */
    void
    pushError(const std::vector<std::string> &context, const std::string &description)
    {
        errors.push_back(Error(context, description));
    }

    /**
     * @brief  Pop an error from the front of the queue.
     *
     * @param  error  Reference to an Error object to populate.
     *
     * @returns  true if an Error was popped, false otherwise.
     */
    bool
    popError(Error &error)
    {
        if (errors.empty()) {
            return false;
        }

        error = errors.front();
        errors.pop_front();
        return true;
    }

private:

    /// FIFO queue of validation errors that have been reported
    std::deque<Error> errors;

};

} // namespace valijson
