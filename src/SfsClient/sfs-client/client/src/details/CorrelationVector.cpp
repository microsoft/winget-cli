// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CorrelationVector.h"
#include "ErrorHandling.h"

#include <correlation_vector/correlation_vector.h>

#include <stdexcept>

using namespace SFS::details;
using namespace microsoft;

CorrelationVector::CorrelationVector() : m_cv(std::make_unique<correlation_vector>())
{
}

CorrelationVector::CorrelationVector(const std::string& cv, const ReportingHandler& handler)
{
    THROW_CODE_IF_LOG(InvalidArg, cv.empty(), handler, "cv must not be empty");

    try
    {
        m_cv = std::make_unique<correlation_vector>(correlation_vector::extend(cv));
    }
    catch (std::invalid_argument& e)
    {
        THROW_LOG(Result(Result::InvalidArg, "baseCV is not a valid correlation vector: " + std::string(e.what())),
                  handler);
    }
}

CorrelationVector::~CorrelationVector() = default;
CorrelationVector::CorrelationVector(CorrelationVector&&) = default;
CorrelationVector& CorrelationVector::operator=(CorrelationVector&&) = default;

std::string CorrelationVector::IncrementAndGet()
{
    // Only increment after it's used at least once
    if (m_isFirstUse)
    {
        m_isFirstUse = false;
    }
    else
    {
        Increment();
    }

    return m_cv->to_string();
}

void CorrelationVector::Increment()
{
    m_cv->increment();
}
