// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <memory>
#include <string>

namespace microsoft
{
class correlation_vector;
}

namespace SFS::details
{
class ReportingHandler;

class CorrelationVector
{
  public:
    CorrelationVector();
    CorrelationVector(const std::string& cv, const ReportingHandler& handler);

    ~CorrelationVector();

    CorrelationVector(CorrelationVector&&);
    CorrelationVector& operator=(CorrelationVector&&);

    /**
     * @brief Returns the current correlation vector and increments the internal state
     */
    std::string IncrementAndGet();

  private:
    void Increment();

    std::unique_ptr<microsoft::correlation_vector> m_cv;
    bool m_isFirstUse = true;
};
} // namespace SFS::details
