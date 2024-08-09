// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Logging.h"
#include "ReportingHandler.h"
#include "RequestParams.h"
#include "entity/FileEntity.h"
#include "entity/VersionEntity.h"

#include <memory>
#include <string>

namespace SFS
{
class AppContent;
class Content;

namespace details
{
class Connection;
class ConnectionManager;
struct ConnectionConfig;

class SFSClientInterface
{
  public:
    virtual ~SFSClientInterface()
    {
    }

    //
    // Combined API calls for retrieval of metadata & download URLs
    //

    /**
     * @brief Retrieve combined metadata & download URLs from the latest version of specified products
     * @note At the moment only a single product request is supported
     * @param requestParams Parameters that define this request
     */
    virtual std::vector<Content> GetLatestDownloadInfo(const RequestParams& requestParams) const = 0;

    /**
     * @brief Retrieve combined metadata & download URLs from the latest version of specified apps
     * @note At the moment only a single product request is supported
     * @param requestParams Parameters that define this request
     */
    virtual std::vector<AppContent> GetLatestAppDownloadInfo(const RequestParams& requestParams) const = 0;

    //
    // Individual APIs 1:1 with service endpoints
    //

    /**
     * @brief Gets the metadata for the latest available version for the specified product request
     * @return Entity that describes the latest version of the product
     * @throws SFSException if the request fails
     */
    virtual std::unique_ptr<VersionEntity> GetLatestVersion(const ProductRequest& productRequest,
                                                            Connection& connection) const = 0;

    /**
     * @brief Gets the metadata for the latest available version for the specified product requests
     * @return Vector of entities that describe the latest version of the products
     * @throws SFSException if the request fails
     */
    virtual VersionEntities GetLatestVersionBatch(const std::vector<ProductRequest>& productRequests,
                                                  Connection& connection) const = 0;

    /**
     * @brief Gets the metadata for a specific version of the specified product
     * @return Entity that describes the latest version of the product
     * @throws SFSException if the request fails
     */
    virtual std::unique_ptr<VersionEntity> GetSpecificVersion(const std::string& product,
                                                              const std::string& version,
                                                              Connection& connection) const = 0;

    /**
     * @brief Gets the files metadata for a specific version of the specified product
     * @return Vector of File entities for the specific version of the product
     * @throws SFSException if the request fails
     */
    virtual FileEntities GetDownloadInfo(const std::string& product,
                                         const std::string& version,
                                         Connection& connection) const = 0;

    /**
     * @brief Returns a new Connection to be used by the SFSClient to make requests
     * @param config Configurations for the connection object
     */
    virtual std::unique_ptr<Connection> MakeConnection(const ConnectionConfig& config) const = 0;

    const ReportingHandler& GetReportingHandler() const
    {
        return m_reportingHandler;
    }

  protected:
    ReportingHandler m_reportingHandler;
};
} // namespace details
} // namespace SFS
