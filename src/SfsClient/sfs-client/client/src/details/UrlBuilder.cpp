// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UrlBuilder.h"

#include "ErrorHandling.h"
#include "ReportingHandler.h"

#include <curl/curl.h>

#include <memory>

#define THROW_IF_CURL_URL_ERROR(curlCall, error)                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        auto __curlUrlCode = (curlCall);                                                                               \
        THROW_CODE_IF_NOT_LOG(error, __curlUrlCode == CURLUE_OK, m_handler, GetCurlUrlStrError(__curlUrlCode));        \
    } while ((void)0, 0)

#define THROW_IF_CURL_URL_SETUP_ERROR(curlCall) THROW_IF_CURL_URL_ERROR(curlCall, ConnectionUrlSetupFailed)

using namespace SFS::details;

namespace
{
struct CurlCharDeleter
{
    void operator()(char* val)
    {
        if (val)
        {
            curl_free(val);
        }
    }
};

using CurlCharPtr = std::unique_ptr<char, CurlCharDeleter>;

std::string GetCurlUrlStrError(CURLUcode code)
{
    return "Curl URL error: " + std::string(curl_url_strerror(code));
}
} // namespace

UrlBuilder::UrlBuilder(const ReportingHandler& handler) : m_handler(handler)
{
    m_handle = curl_url();
    THROW_CODE_IF_NOT_LOG(ConnectionUrlSetupFailed, m_handle, m_handler, "Curl URL error: Failed to create URL");
}

UrlBuilder::UrlBuilder(const std::string& url, const ReportingHandler& handler) : UrlBuilder(handler)
{
    SetUrl(url);
}

UrlBuilder::~UrlBuilder()
{
    curl_url_cleanup(m_handle);
}

std::string UrlBuilder::GetUrl() const
{
    CurlCharPtr url;
    char* urlPtr = url.get();
    THROW_IF_CURL_URL_SETUP_ERROR(curl_url_get(m_handle, CURLUPART_URL, &urlPtr, 0 /*flags*/));
    return urlPtr;
}

UrlBuilder& UrlBuilder::SetScheme(Scheme scheme)
{
    switch (scheme)
    {
    case Scheme::Https:
        THROW_IF_CURL_URL_SETUP_ERROR(curl_url_set(m_handle, CURLUPART_SCHEME, "https", 0 /*flags*/));
        break;
    }
    return *this;
}

UrlBuilder& UrlBuilder::SetHost(const std::string& host)
{
    THROW_CODE_IF_LOG(InvalidArg, host.empty(), m_handler, "Host must not empty");
    THROW_IF_CURL_URL_SETUP_ERROR(curl_url_set(m_handle, CURLUPART_HOST, host.c_str(), 0 /*flags*/));
    return *this;
}

UrlBuilder& UrlBuilder::SetPath(const std::string& path)
{
    THROW_CODE_IF_LOG(InvalidArg, path.empty(), m_handler, "Path must not empty");
    m_path = path;
    THROW_IF_CURL_URL_SETUP_ERROR(curl_url_set(m_handle, CURLUPART_PATH, m_path.c_str(), 0 /*flags*/));
    return *this;
}

UrlBuilder& UrlBuilder::AppendPath(const std::string& path)
{
    return AppendPath(path, false /*encode*/);
}

UrlBuilder& UrlBuilder::AppendPathEncoded(const std::string& path)
{
    return AppendPath(path, true /*encode*/);
}

UrlBuilder& UrlBuilder::AppendPath(const std::string& path, bool encode)
{
    THROW_CODE_IF_LOG(InvalidArg, path.empty(), m_handler, "Path must not empty");
    if (!m_path.empty() && m_path.back() != '/')
    {
        m_path += '/';
    }

    if (encode)
    {
        m_path += URLEncode(path);
    }
    else
    {
        m_path += path;
    }

    THROW_IF_CURL_URL_SETUP_ERROR(curl_url_set(m_handle, CURLUPART_PATH, m_path.c_str(), 0 /*flags*/));
    return *this;
}

UrlBuilder& UrlBuilder::ResetPath()
{
    m_path.clear();
    THROW_IF_CURL_URL_SETUP_ERROR(curl_url_set(m_handle, CURLUPART_PATH, "", 0 /*flags*/));
    return *this;
}

UrlBuilder& UrlBuilder::SetQuery(const std::string& key, const std::string& value)
{
    THROW_CODE_IF_LOG(InvalidArg, key.empty() || value.empty(), m_handler, "Query key and value must not empty");
    const std::string query = URLEncode(key) + "=" + URLEncode(value);
    THROW_IF_CURL_URL_SETUP_ERROR(curl_url_set(m_handle, CURLUPART_QUERY, query.c_str(), 0 /*flags*/));
    return *this;
}

UrlBuilder& UrlBuilder::AppendQuery(const std::string& key, const std::string& value)
{
    THROW_CODE_IF_LOG(InvalidArg, key.empty() || value.empty(), m_handler, "Query key and value must not empty");
    const std::string query = URLEncode(key) + "=" + URLEncode(value);
    THROW_IF_CURL_URL_SETUP_ERROR(curl_url_set(m_handle, CURLUPART_QUERY, query.c_str(), CURLU_APPENDQUERY));
    return *this;
}

UrlBuilder& UrlBuilder::ResetQuery()
{
    THROW_IF_CURL_URL_SETUP_ERROR(curl_url_set(m_handle, CURLUPART_QUERY, "", 0 /*flags*/));
    return *this;
}

UrlBuilder& UrlBuilder::SetUrl(const std::string& url)
{
    THROW_CODE_IF_LOG(InvalidArg, url.empty(), m_handler, "Url must not empty");
    THROW_IF_CURL_URL_SETUP_ERROR(curl_url_set(m_handle, CURLUPART_URL, url.c_str(), 0 /*flags*/));
    return *this;
}

std::string UrlBuilder::URLEncode(const std::string& str) const
{
    CurlCharPtr encodedStr{curl_easy_escape(nullptr /*ignored*/, str.c_str(), static_cast<int>(str.length()))};
    THROW_CODE_IF_NOT_LOG(ConnectionUrlSetupFailed, encodedStr, m_handler, "Failed to URL-encode string");
    return encodedStr.get();
}
