// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "SFSClientInterface.h"

#include "ClientConfig.h"
#include "Content.h"
#include "Logging.h"
#include "Result.h"
#include "SFSUrlBuilder.h"

#include <memory>
#include <optional>
#include <string>

namespace SFS::details
{
template <typename ConnectionManagerT>
class SFSClientImpl : public SFSClientInterface
{
  public:
    SFSClientImpl(ClientConfig&& config);
    ~SFSClientImpl() override = default;

    //
    // Combined API calls for retrieval of metadata & download URLs
    //

    /**
     * @brief Retrieve combined metadata & download URLs from the latest version of specified products
     * @note At the moment only a single product request is supported
     * @param requestParams Parameters that define this request
     */
    std::vector<Content> GetLatestDownloadInfo(const RequestParams& requestParams) const override;

    /**
     * @brief Retrieve combined metadata & download URLs from the latest version of specified apps
     * @note At the moment only a single product request is supported
     * @param requestParams Parameters that define this request
     */
    std::vector<AppContent> GetLatestAppDownloadInfo(const RequestParams& requestParams) const override;

    //
    // Individual APIs 1:1 with service endpoints (SFSClientInterface)
    //

    /**
     * @brief Gets the metadata for the latest available version for the specified product request
     * @return Entity that describes the latest version of the product
     * @throws SFSException if the request fails
     */
    std::unique_ptr<VersionEntity> GetLatestVersion(const ProductRequest& productRequest,
                                                    Connection& connection) const override;

    /**
     * @brief Gets the metadata for the latest available version for the specified product requests
     * @return Vector of entities that describe the latest version of the products
     * @throws SFSException if the request fails
     */
    VersionEntities GetLatestVersionBatch(const std::vector<ProductRequest>& productRequests,
                                          Connection& connection) const override;

    /**
     * @brief Gets the metadata for a specific version of the specified product
     * @return Entity that describes the latest version of the product
     * @throws SFSException if the request fails
     */
    std::unique_ptr<VersionEntity> GetSpecificVersion(const std::string& product,
                                                      const std::string& version,
                                                      Connection& connection) const override;

    /**
     * @brief Gets the files metadata for a specific version of the specified product
     * @return Vector of File entities for the specific version of the product
     * @throws SFSException if the request fails
     */
    FileEntities GetDownloadInfo(const std::string& product,
                                 const std::string& version,
                                 Connection& connection) const override;

    /**
     * @brief Returns a new Connection to be used by the SFSClient to make requests
     * @param config Configurations for the connection object
     */
    std::unique_ptr<Connection> MakeConnection(const ConnectionConfig& config) const override;

    //
    // Configuration methods
    //

    /**
     * @brief Allows one to override the base URL used to make calls to the SFS service
     * @details Not exposed to the user. Used for testing purposes only
     * @param customBaseUrl The custom base URL to use
     */
    void SetCustomBaseUrl(std::string customBaseUrl);

    /**
     * @return A SFSUrlBuilder object that can be used to build URLs for the SFS service
     */
    SFSUrlBuilder MakeUrlBuilder() const;

  private:
    std::string m_accountId;
    std::string m_instanceId;
    std::string m_nameSpace;

    std::unique_ptr<ConnectionManagerT> m_connectionManager;

    std::optional<std::string> m_customBaseUrl;
};
} // namespace SFS::details
