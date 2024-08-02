// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::UriValidation
{
    // The decision made based on the Uri validation.
    enum class UriValidationDecision
    {
        Allow,
        Block,
    };

    // The result of a Uri validation.
    class UriValidationResult
    {
    private:
        // The decision made based on the Uri validation.
        UriValidationDecision m_decision;

        // Uri to give feedback to smart screen about the decision.
        std::string m_feedback;
    public:
        UriValidationResult(UriValidationDecision decision) : m_decision(decision), m_feedback(std::string()) {}
        UriValidationResult(UriValidationDecision decision, std::string feedback) : m_decision(decision), m_feedback(feedback) {}
        UriValidationDecision Decision() const { return m_decision; }
        std::string Feedback() const { return m_feedback; }
    };

    // Validate the given Uri.
    UriValidationResult UriValidation(const std::string& uri);
}
