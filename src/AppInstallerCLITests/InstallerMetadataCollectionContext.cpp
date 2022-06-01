// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"

#include <winget/InstallerMetadataCollectionContext.h>

using namespace AppInstaller;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Correlation;
using namespace AppInstaller::Repository::Metadata;

namespace
{
    struct MinimalDefaults {};

    struct TestInput
    {
        TestInput() = default;

        TestInput(MinimalDefaults)
        {

        }

        std::optional<std::string> Version;
        std::optional<std::string> SupportedMetadataVersion;
        std::optional<size_t> MaximumMetadataSize;
        std::optional<ProductMetadata> CurrentMetadata;
        std::optional<std::string> SubmissionIdentifier;
        std::optional<std::string> InstallerHash;
        // Not currently used
        // std::optional<Manifest::Manifest> CurrentManifest;
        // Schema 1.0 only cares about DefaultLocale and Locales
        std::optional<Manifest::Manifest> SubmissionData;

        std::wstring ToJSON() const
        {
            // TODO: Implement
        }
    };

    struct TestOutput
    {
        TestOutput(std::string_view json)
        {
            // TODO: Implement
        }

        std::optional<std::string> Version;
        std::optional<std::string> InstallerHash;
        std::optional<std::string> Status;
        std::optional<ProductMetadata> Metadata;
        std::optional<HRESULT> ErrorHR;
        std::optional<std::string> ErrorText;
    };

    struct TestARPCorrelationData : public ARPCorrelationData
    {
        ARPCorrelationResult CorrelateForNewlyInstalled(const Manifest::Manifest&) override
        {
            return CorrelateForNewlyInstalledResult;
        }

        ARPCorrelationResult CorrelateForNewlyInstalledResult;
    };

    InstallerMetadataCollectionContext CreateTestContext(std::unique_ptr<ARPCorrelationData>&& data, const TestInput& input)
    {
        return { std::move(data), input.ToJSON() };
    }
}

TEST_CASE("BadInput", "[metadata_collection]")
{
}

TEST_CASE("MinimumInput", "[metadata_collection]")
{
}

TEST_CASE("LowConfidence", "[metadata_collection]")
{
}

TEST_CASE("SameSubmission_SameInstaller", "[metadata_collection]")
{
}

TEST_CASE("SameSubmission_NewInstaller", "[metadata_collection]")
{
}

TEST_CASE("NewSubmission", "[metadata_collection]")
{
}
