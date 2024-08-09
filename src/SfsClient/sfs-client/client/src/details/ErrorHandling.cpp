// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ErrorHandling.h"

#include "ReportingHandler.h"

using namespace SFS::details;

void SFS::details::LogFailedResult(const SFS::Result& result,
                                   const ReportingHandler& handler,
                                   const char* file,
                                   unsigned line)
{
    if (result.IsFailure())
    {
        LOG_ERROR(handler,
                  "FAILED [%s] %s%s(%s:%u)",
                  std::string(ToString(result.GetCode())).c_str(),
                  result.GetMsg().c_str(),
                  result.GetMsg().empty() ? "" : " ",
                  file,
                  line);
    }
}

void SFS::details::LogIfFailed(const Result& result, const ReportingHandler& handler, const char* file, unsigned line)
{
    if (result.IsFailure())
    {
        LogFailedResult(result, handler, file, line);
    }
}

void SFS::details::ThrowLog(Result result, const ReportingHandler& handler, const char* file, unsigned line)
{
    assert(result.IsFailure());
    LogFailedResult(result, handler, file, line);
    throw SFSException(std::move(result));
}

void SFS::details::ThrowIfFailedLog(Result result, const ReportingHandler& handler, const char* file, unsigned line)
{
    if (result.IsFailure())
    {
        LogFailedResult(result, handler, file, line);
        throw SFSException(std::move(result));
    }
}

void SFS::details::ThrowCodeIf(Result::Code code, bool condition, std::string message)
{
    if (condition)
    {
        Result result(code, std::move(message));
        assert(result.IsFailure());
        throw SFSException(std::move(result));
    }
}

void SFS::details::ThrowCodeIfLog(Result::Code code,
                                  bool condition,
                                  const ReportingHandler& handler,
                                  const char* file,
                                  unsigned line,
                                  std::string message)
{
    if (condition)
    {
        Result result(code, std::move(message));
        assert(result.IsFailure());
        LogFailedResult(result, handler, file, line);
        throw SFSException(std::move(result));
    }
}
