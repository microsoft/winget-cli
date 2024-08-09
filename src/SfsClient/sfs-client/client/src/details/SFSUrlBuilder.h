// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "UrlBuilder.h"

namespace SFS::details
{
// Wrapper struct that only exists to allow the SFSUrlBuilder to have a constructor that takes a custom URL
struct SFSCustomUrl
{
    explicit SFSCustomUrl(std::string url) : url(std::move(url))
    {
    }

    std::string url;
};

class SFSUrlBuilder : private UrlBuilder
{
  public:
    SFSUrlBuilder(const std::string& accountId,
                  std::string instanceId,
                  std::string nameSpace,
                  const ReportingHandler& handler);

    SFSUrlBuilder(const SFSCustomUrl& customUrl,
                  std::string instanceId,
                  std::string nameSpace,
                  const ReportingHandler& handler);

    std::string GetLatestVersionUrl(const std::string& product);
    std::string GetLatestVersionBatchUrl();
    std::string GetSpecificVersionUrl(const std::string& product, const std::string& version);
    std::string GetDownloadInfoUrl(const std::string& product, const std::string& version);

    using UrlBuilder::GetUrl;

  private:
    SFSUrlBuilder& SetNamesUrlPath();
    SFSUrlBuilder& SetVersionsUrlPath(const std::string& product);

    std::string m_instanceId;
    std::string m_nameSpace;
};
} // namespace SFS::details
