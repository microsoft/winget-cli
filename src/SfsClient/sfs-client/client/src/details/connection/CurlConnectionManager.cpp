// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CurlConnectionManager.h"

#include "../ErrorHandling.h"
#include "CurlConnection.h"

#include <curl/curl.h>

using namespace SFS;
using namespace SFS::details;

namespace
{
// Curl recommends checking for expected features in runtime
void CheckCurlFeatures(const ReportingHandler& handler)
{
    curl_version_info_data* ver = curl_version_info(CURLVERSION_NOW);
    THROW_CODE_IF_NOT_LOG(HttpUnexpected, ver, handler);

    THROW_CODE_IF_NOT_LOG(HttpUnexpected, (ver->features & CURL_VERSION_SSL), handler, "Curl was not built with SSL");
    THROW_CODE_IF_NOT_LOG(HttpUnexpected,
                          (ver->features & CURL_VERSION_THREADSAFE),
                          handler,
                          "Curl is not thread safe");

    // For thread safety we need the DNS resolutions to be asynchronous (which happens because of c-ares)
    THROW_CODE_IF_NOT_LOG(HttpUnexpected,
                          (ver->features & CURL_VERSION_ASYNCHDNS),
                          handler,
                          "Curl was not built with async DNS resolutions");
}
} // namespace

CurlConnectionManager::CurlConnectionManager(const ReportingHandler& handler) : ConnectionManager(handler)
{
    THROW_CODE_IF_NOT_LOG(HttpUnexpected,
                          curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK,
                          m_handler,
                          "Curl failed to initialize");
    CheckCurlFeatures(m_handler);
}

CurlConnectionManager::~CurlConnectionManager()
{
    curl_global_cleanup();
}

std::unique_ptr<Connection> CurlConnectionManager::MakeConnection(const ConnectionConfig& config)
{
    return std::make_unique<CurlConnection>(config, m_handler);
}
