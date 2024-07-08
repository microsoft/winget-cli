// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CurlConnection.h"

#include "../ErrorHandling.h"
#include "../ReportingHandler.h"
#include "../TestOverride.h"
#include "HttpHeader.h"

#include <curl/curl.h>

#include <chrono>
#include <cstring>
#include <optional>
#include <thread>

#define THROW_IF_CURL_ERROR(curlCall, error)                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        auto __curlCode = (curlCall);                                                                                  \
        std::string __message = "Curl error: " + std::string(curl_easy_strerror(__curlCode));                          \
        THROW_CODE_IF_NOT_LOG(error, __curlCode == CURLE_OK, m_handler, std::move(__message));                         \
    } while ((void)0, 0)

#define THROW_IF_CURL_SETUP_ERROR(curlCall) THROW_IF_CURL_ERROR(curlCall, ConnectionSetupFailed)
#define THROW_IF_CURL_UNEXPECTED_ERROR(curlCall) THROW_IF_CURL_ERROR(curlCall, ConnectionUnexpectedError)

// Setting a hard limit of 100k characters for the response to avoid rogue servers sending huge amounts of data
#define MAX_RESPONSE_CHARACTERS 100000

using namespace SFS;
using namespace SFS::details;
using namespace std::chrono_literals;

namespace
{
// Curl callback for writing data to a std::string. Must return the number of bytes written.
// This callback may be called multiple times for a single request, and will keep appending
// to userData until the request is complete. The data received is not null-terminated.
// For SFS, this data will likely be a JSON string.
size_t WriteCallback(char* contents, size_t sizeInBytes, size_t numElements, void* userData)
{
    auto readBufferPtr = static_cast<std::string*>(userData);
    if (readBufferPtr)
    {
        size_t totalSize = sizeInBytes * numElements;

        // Checking final response size to avoid unexpected amounts of data
        if ((readBufferPtr->length() + totalSize) > MAX_RESPONSE_CHARACTERS)
        {
            return CURL_WRITEFUNC_ERROR;
        }

        readBufferPtr->append(contents, totalSize);
        return totalSize;
    }
    return CURL_WRITEFUNC_ERROR;
}

struct CurlErrorBuffer
{
  public:
    CurlErrorBuffer(CURL* handle, const ReportingHandler& reportingHandler)
        : m_handle(handle)
        , m_reportingHandler(reportingHandler)
    {
        m_errorBuffer[0] = '\0';
        SetBuffer();
    }

    ~CurlErrorBuffer()
    {
        LOG_IF_FAILED(UnsetBuffer(), m_reportingHandler);
    }

    void SetBuffer()
    {
        THROW_CODE_IF_NOT_LOG(ConnectionSetupFailed,
                              curl_easy_setopt(m_handle, CURLOPT_ERRORBUFFER, m_errorBuffer) == CURLE_OK,
                              m_reportingHandler,
                              "Failed to set up error buffer for curl");
    }

    Result UnsetBuffer()
    {
        return curl_easy_setopt(m_handle, CURLOPT_ERRORBUFFER, nullptr) == CURLE_OK
                   ? Result::Success
                   : Result(Result::ConnectionSetupFailed, "Failed to unset curl error buffer");
    }

    char* Get()
    {
        return m_errorBuffer;
    }

  private:
    CURL* m_handle;
    const ReportingHandler& m_reportingHandler;

    char m_errorBuffer[CURL_ERROR_SIZE];
};

Result CurlCodeToResult(CURLcode curlCode, char* errorBuffer)
{
    Result::Code code;
    switch (curlCode)
    {
    case CURLE_OPERATION_TIMEDOUT:
        code = Result::HttpTimeout;
        break;
    default:
        code = Result::ConnectionUnexpectedError;
        break;
    }

    const bool isErrorStringRegistered = errorBuffer && errorBuffer[0] != '\0';
    std::string message = isErrorStringRegistered ? errorBuffer : "Curl error";

    return Result(code, std::move(message));
}

bool IsSuccessfulSFSHttpCode(long httpCode)
{
    return httpCode == 200;
}

Result HttpCodeToResult(long httpCode)
{
    if (IsSuccessfulSFSHttpCode(httpCode))
    {
        return Result::Success;
    }

    switch (httpCode)
    {
    case 400:
    {
        return Result(Result::HttpBadRequest, "400 Bad Request");
    }
    case 404:
    {
        return Result(Result::HttpNotFound, "404 Not Found");
    }
    case 405:
    {
        return Result(Result::HttpMethodNotAllowed, "405 Method Not Allowed");
    }
    case 429:
    {
        return Result(Result::HttpTooManyRequests, "429 Too Many Requests");
    }
    case 503:
    {
        return Result(Result::HttpServiceNotAvailable, "503 Service Unavailable");
    }
    default:
    {
        return Result(Result::HttpUnexpected, "Unexpected HTTP code " + std::to_string(httpCode));
    }
    }
}

bool IsRetriableHttpError(long httpCode)
{
    switch (httpCode)
    {
    case 429: // Too Many Requests - Rate Limiting
    case 500: // InternalServerError - Can be triggered within server timeouts, network issue
    case 502: // BadGateway - Likely an issue with routing
    case 503: // ServerBusy
    case 504: // GatewayTimeout
        return true;
    default:
        return false;
    }
}

std::optional<std::string> GetResponseHeader(CURL* handle,
                                             HttpHeader httpHeader,
                                             const ReportingHandler& reportingHandler)
{
    const std::string headerName = ToString(httpHeader);

    // This struct only represents data inside the CURL handle, and must not be manually freed
    curl_header* header;
    const int lastRequest = -1;
    CURLHcode curlhCode = curl_easy_header(handle, headerName.c_str(), 0 /*index*/, CURLH_HEADER, lastRequest, &header);
    switch (curlhCode)
    {
    case CURLHE_OK:
        return header->value;
    case CURLHE_BADINDEX:
    case CURLHE_MISSING:
    case CURLHE_NOHEADERS:
        return std::nullopt;
    default:
        THROW_LOG(
            Result(Result::ConnectionUnexpectedError,
                   "Failed to get response header " + headerName + " with CURLH code " + std::to_string(curlhCode)),
            reportingHandler);
    }
    return std::nullopt;
}

std::chrono::milliseconds ParseRetryAfterValue(const std::string& retryAfter, const ReportingHandler& reportingHandler)
{
    LOG_VERBOSE(reportingHandler, "Parsing Retry-After value [%s]", retryAfter.c_str());
    std::chrono::seconds retryAfterSec{0};
    try
    {
        retryAfterSec = std::chrono::seconds(std::stoi(retryAfter));
    }
    catch (std::invalid_argument&)
    {
        // Value is not an integer, but may still be in HTTP Date format
        const time_t retryAfterSecSinceEpoch = curl_getdate(retryAfter.c_str(), nullptr /*unused*/);
        if (retryAfterSecSinceEpoch == -1)
        {
            THROW_LOG(Result(Result::ConnectionUnexpectedError,
                             "Retry-After header value could not be converted to an integer or an HTTP Date"),
                      reportingHandler);
        }

        // Get number of seconds since epoch for now to calculate the difference
        const auto epoch = std::chrono::system_clock::now().time_since_epoch();
        const auto nowSecSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(epoch);

        retryAfterSec = std::chrono::seconds(retryAfterSecSinceEpoch) - nowSecSinceEpoch;
    }
    catch (std::out_of_range&)
    {
        THROW_LOG(Result(Result::ConnectionUnexpectedError, "Retry-After header value is not in the expected range"),
                  reportingHandler);
    }
    if (retryAfterSec <= 0s)
    {
        THROW_LOG(Result(Result::ConnectionUnexpectedError, "Invalid Retry-After header value"), reportingHandler);
    }
    return retryAfterSec;
}
} // namespace

namespace SFS::details
{
struct CurlHeaderList
{
  public:
    CurlHeaderList() = default;

    ~CurlHeaderList()
    {
        curl_slist_free_all(m_slist);
    }

    /**
     * @throws SFSException if the header cannot be added to the list.
     */
    void Add(HttpHeader header, const std::string& value)
    {
        const std::string data = ToString(header) + ": " + value;
        const auto ret = curl_slist_append(m_slist, data.c_str());
        if (!ret)
        {
            throw SFSException(Result::ConnectionSetupFailed, "Failed to add header " + data + " to CurlHeaderList");
        }
        m_slist = ret;
    }

    struct curl_slist* m_slist{nullptr};
};
} // namespace SFS::details

CurlConnection::CurlConnection(const ConnectionConfig& config, const ReportingHandler& handler)
    : Connection(config, handler)
{
    m_handle = curl_easy_init();
    THROW_CODE_IF_NOT_LOG(ConnectionSetupFailed, m_handle, m_handler, "Failed to init curl connection");

    // Turning timeout signals off to avoid issues with threads
    // See https://curl.se/libcurl/c/threadsafe.html
    THROW_CODE_IF_NOT_LOG(ConnectionSetupFailed,
                          curl_easy_setopt(m_handle, CURLOPT_NOSIGNAL, 1L) == CURLE_OK,
                          m_handler,
                          "Failed to set up curl");

    // TODO #41: Pass AAD token in the header if it is available
    // TODO #42: Cert pinning with service
}

CurlConnection::~CurlConnection()
{
    if (m_handle)
    {
        curl_easy_cleanup(m_handle);
    }
}

std::string CurlConnection::Get(const std::string& url)
{
    THROW_CODE_IF_LOG(InvalidArg, url.empty(), m_handler, "url cannot be empty");

    THROW_IF_CURL_SETUP_ERROR(curl_easy_setopt(m_handle, CURLOPT_HTTPGET, 1L));
    THROW_IF_CURL_SETUP_ERROR(curl_easy_setopt(m_handle, CURLOPT_HTTPHEADER, nullptr));

    CurlHeaderList headers;
    return CurlPerform(url, headers);
}

std::string CurlConnection::Post(const std::string& url, const std::string& data)
{
    THROW_CODE_IF_LOG(InvalidArg, url.empty(), m_handler, "url cannot be empty");

    CurlHeaderList headerList;
    headerList.Add(HttpHeader::ContentType, "application/json");

    THROW_IF_CURL_SETUP_ERROR(curl_easy_setopt(m_handle, CURLOPT_POST, 1L));
    THROW_IF_CURL_SETUP_ERROR(curl_easy_setopt(m_handle, CURLOPT_COPYPOSTFIELDS, data.c_str()));

    CurlHeaderList headers;
    headers.Add(HttpHeader::ContentType, "application/json");
    return CurlPerform(url, headers);
}

std::string CurlConnection::CurlPerform(const std::string& url, CurlHeaderList& headers)
{
    THROW_IF_CURL_SETUP_ERROR(curl_easy_setopt(m_handle, CURLOPT_URL, url.c_str()));

    const std::string cv = m_cv.IncrementAndGet();
    headers.Add(HttpHeader::MSCV, cv);
    headers.Add(HttpHeader::UserAgent, GetUserAgentValue());

    THROW_IF_CURL_SETUP_ERROR(curl_easy_setopt(m_handle, CURLOPT_HTTPHEADER, headers.m_slist));

    // Setting up error buffer where error messages get written - this gets unset in the destructor
    CurlErrorBuffer errorBuffer(m_handle, m_handler);

    std::string readBuffer;
    THROW_IF_CURL_SETUP_ERROR(curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, WriteCallback));
    THROW_IF_CURL_SETUP_ERROR(curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &readBuffer));

    // Retry the connection a specified number of times
    const unsigned totalAttempts = 1 + m_maxRetries;
    for (unsigned i = 0; i < totalAttempts; i++)
    {
        const unsigned attempt = i + 1;
        LOG_INFO(m_handler, "Request attempt %u out of %u (cv: %s)", attempt, totalAttempts, cv.c_str());
        const bool lastAttempt = attempt == totalAttempts;

        // Clear the buffer before each attempt
        readBuffer.clear();

        // Perform the request
        const auto result = curl_easy_perform(m_handle);
        if (result != CURLE_OK)
        {
            THROW_LOG(CurlCodeToResult(result, errorBuffer.Get()), m_handler);
        }

        // Check request status to stop or retry
        long httpCode = 0;
        THROW_IF_CURL_UNEXPECTED_ERROR(curl_easy_getinfo(m_handle, CURLINFO_RESPONSE_CODE, &httpCode));

        if (IsSuccessfulSFSHttpCode(httpCode))
        {
            break;
        }

        const Result httpResult = HttpCodeToResult(httpCode);
        if (!CanRetryRequest(lastAttempt, httpCode))
        {
            THROW_LOG(httpResult, m_handler);
        }

        ProcessRetry(attempt, httpResult);
    }

    return readBuffer;
}

bool CurlConnection::CanRetryRequest(bool lastAttempt, long httpCode)
{
    if (lastAttempt)
    {
        LOG_INFO(m_handler, "No retry as this is the last attempt");
        return false;
    }

    if (!IsRetriableHttpError(httpCode))
    {
        LOG_INFO(m_handler, "Error %ld is not retriable, stopping", httpCode);
        return false;
    }

    return true;
}

void CurlConnection::ProcessRetry(int attempt, const Result& httpResult)
{
    // Wait before retrying. Prefer the Retry-After information if available
    std::chrono::milliseconds retryDelay{0};
    const std::optional<std::string> retryAfter = GetResponseHeader(m_handle, HttpHeader::RetryAfter, m_handler);
    if (retryAfter)
    {
        // TODO #93: Enforce Retry-After value across calls to avoid caller spamming server
        retryDelay = ParseRetryAfterValue(*retryAfter, m_handler);
    }
    else
    {
        // Apply exponential back-off with a factor of 2
        static std::chrono::milliseconds s_baseRetryDelay = 15s; // Value recommended as interval by the service

        std::chrono::milliseconds baseRetryDelay = s_baseRetryDelay;

        // Value can be overriden in tests
        if (auto override = test::GetTestOverrideAsInt(test::TestOverride::BaseRetryDelayMs))
        {
            baseRetryDelay = std::chrono::milliseconds{*override};
        }

        retryDelay = baseRetryDelay * (1 << (attempt - 1));
    }

    LOG_IF_FAILED(httpResult, m_handler);
    LOG_INFO(m_handler, "Sleeping for %lld ms", static_cast<long long>(retryDelay.count()));
    std::this_thread::sleep_for(retryDelay);
}
