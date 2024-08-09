// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Result.h"
#include "SFSException.h"

#include <cassert>

#define SFS_CATCH_RETURN()                                                                                             \
    catch (const std::bad_alloc&)                                                                                      \
    {                                                                                                                  \
        return Result::OutOfMemory;                                                                                    \
    }                                                                                                                  \
    catch (const SFS::details::SFSException& e)                                                                        \
    {                                                                                                                  \
        return e.GetResult();                                                                                          \
    }                                                                                                                  \
    catch (const std::exception&)                                                                                      \
    {                                                                                                                  \
        return Result::Unexpected;                                                                                     \
    }                                                                                                                  \
    catch (...)                                                                                                        \
    {                                                                                                                  \
        return Result::Unexpected;                                                                                     \
    }

#define SFS_CATCH_LOG_RETHROW(handler)                                                                                 \
    catch (const SFS::details::SFSException& e)                                                                        \
    {                                                                                                                  \
        SFS::details::LogFailedResult(e.GetResult(), handler, __FILE__, __LINE__);                                     \
        throw;                                                                                                         \
    }

#define RETURN_IF_FAILED(result)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        auto __result = (result); /* Assigning to a variable ensures a code block gets called only once */             \
        if (__result.IsFailure())                                                                                      \
        {                                                                                                              \
            return __result;                                                                                           \
        }                                                                                                              \
    } while ((void)0, 0)

#define RETURN_IF_FAILED_LOG(result, handler)                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        auto __result = (result); /* Assigning to a variable ensures a code block gets called only once */             \
        if (__result.IsFailure())                                                                                      \
        {                                                                                                              \
            SFS::details::LogFailedResult(__result, handler, __FILE__, __LINE__);                                      \
            return __result;                                                                                           \
        }                                                                                                              \
    } while ((void)0, 0)

#define LOG_IF_FAILED(result, handler) LogIfFailed(result, handler, __FILE__, __LINE__)

#define THROW_LOG(result, handler) ThrowLog(result, handler, __FILE__, __LINE__)

#define THROW_IF_FAILED_LOG(result, handler) ThrowIfFailedLog(result, handler, __FILE__, __LINE__)

#define THROW_CODE_IF(code, condition, ...) ThrowCodeIf(SFS::Result::code, condition, ##__VA_ARGS__)

#define THROW_CODE_IF_LOG(code, condition, handler, ...)                                                               \
    ThrowCodeIfLog(SFS::Result::code, condition, handler, __FILE__, __LINE__, ##__VA_ARGS__)

#define THROW_CODE_IF_NOT_LOG(code, condition, handler, ...)                                                           \
    ThrowCodeIfLog(SFS::Result::code, !(condition), handler, __FILE__, __LINE__, ##__VA_ARGS__)

namespace SFS::details
{
class ReportingHandler;

void LogFailedResult(const Result& result, const ReportingHandler& handler, const char* file, unsigned line);
void LogIfFailed(const Result& result, const ReportingHandler& handler, const char* file, unsigned line);

void ThrowLog(Result result, const ReportingHandler& handler, const char* file, unsigned line);
void ThrowIfFailedLog(Result result, const ReportingHandler& handler, const char* file, unsigned line);
void ThrowCodeIf(Result::Code code, bool condition, std::string message = {});
void ThrowCodeIfLog(Result::Code code,
                    bool condition,
                    const ReportingHandler& handler,
                    const char* file,
                    unsigned line,
                    std::string message = {});
} // namespace SFS::details
