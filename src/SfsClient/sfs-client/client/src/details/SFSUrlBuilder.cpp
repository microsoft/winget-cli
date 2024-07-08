// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "SFSUrlBuilder.h"

using namespace SFS::details;

constexpr const char* c_apiVersion = "v2";
constexpr const char* c_apiDomain = "api.cdp.microsoft.com";

SFSUrlBuilder::SFSUrlBuilder(const std::string& accountId,
                             std::string instanceId,
                             std::string nameSpace,
                             const ReportingHandler& handler)
    : UrlBuilder(handler)
    , m_instanceId(std::move(instanceId))
    , m_nameSpace(std::move(nameSpace))
{
    SetScheme(Scheme::Https);
    SetHost(accountId + "." + std::string(c_apiDomain));
}

SFSUrlBuilder::SFSUrlBuilder(const SFSCustomUrl& customUrl,
                             std::string instanceId,
                             std::string nameSpace,
                             const ReportingHandler& handler)
    : UrlBuilder(handler)
    , m_instanceId(std::move(instanceId))
    , m_nameSpace(std::move(nameSpace))
{
    SetUrl(customUrl.url);
}

std::string SFSUrlBuilder::GetLatestVersionUrl(const std::string& product)
{
    SetVersionsUrlPath(product);
    AppendPath("latest");
    SetQuery("action", "select");
    return GetUrl();
}

std::string SFSUrlBuilder::GetLatestVersionBatchUrl()
{
    SetNamesUrlPath();
    SetQuery("action", "BatchUpdates");
    return GetUrl();
}

std::string SFSUrlBuilder::GetSpecificVersionUrl(const std::string& product, const std::string& version)
{
    SetVersionsUrlPath(product);
    AppendPathEncoded(version);
    return GetUrl();
}

std::string SFSUrlBuilder::GetDownloadInfoUrl(const std::string& product, const std::string& version)
{
    SetVersionsUrlPath(product);
    AppendPathEncoded(version);
    AppendPath("files");
    SetQuery("action", "GenerateDownloadInfo");
    return GetUrl();
}

SFSUrlBuilder& SFSUrlBuilder::SetNamesUrlPath()
{
    ResetPath().ResetQuery();
    AppendPath("api").AppendPath(c_apiVersion);
    AppendPath("contents").AppendPathEncoded(m_instanceId);
    AppendPath("namespaces").AppendPathEncoded(m_nameSpace);
    AppendPath("names");
    return *this;
}

SFSUrlBuilder& SFSUrlBuilder::SetVersionsUrlPath(const std::string& product)
{
    SetNamesUrlPath();
    AppendPathEncoded(product);
    AppendPath("versions");
    return *this;
}
