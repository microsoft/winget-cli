// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"

#include <winget/InstallerMetadataCollectionContext.h>

using namespace AppInstaller;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Correlation;
using namespace AppInstaller::Repository::Metadata;
using namespace TestCommon;

namespace
{
    // Indicates to set minimal defaults on TestInput
    struct MinimalDefaults_t {} MinimalDefaults;

    struct TestInput
    {
        TestInput() = default;

        TestInput(MinimalDefaults_t)
        {
            Version = "1.0";
            SupportedMetadataVersion = "1.0";
            SubmissionIdentifier = "1";
            InstallerHash = "ABCD";
            SubmissionData = std::make_optional<Manifest::Manifest>();
            SubmissionData->DefaultLocalization.Locale = "en-us";
            SubmissionData->DefaultLocalization.Add<Manifest::Localization::PackageName>("Name");
            SubmissionData->DefaultLocalization.Add<Manifest::Localization::Publisher>("Publisher");
        }

        TestInput(MinimalDefaults_t, const std::string& productVersion, const std::string& productCode, Manifest::InstallerTypeEnum installerType) : TestInput(MinimalDefaults)
        {
            CurrentMetadata = std::make_optional<ProductMetadata>();
            CurrentMetadata->SchemaVersion.Assign("1.0");
            CurrentMetadata->ProductVersionMin.Assign(productVersion);
            CurrentMetadata->ProductVersionMax.Assign(productVersion);
            auto& installerMetadata = CurrentMetadata->InstallerMetadataMap[InstallerHash.value()];
            installerMetadata.SubmissionIdentifier = SubmissionIdentifier.value();
            installerMetadata.AppsAndFeaturesEntries.push_back({});
            auto& entry = installerMetadata.AppsAndFeaturesEntries.back();
            entry.DisplayName = SubmissionData->DefaultLocalization.Get<Manifest::Localization::PackageName>();
            entry.Publisher = SubmissionData->DefaultLocalization.Get<Manifest::Localization::Publisher>();
            entry.DisplayVersion = productVersion;
            entry.ProductCode = productCode;
            entry.InstallerType = installerType;
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

        std::wstring ToJSON()
        {
            web::json::value json;

            if (Version)
            {
                json[L"version"] = JSON::GetStringValue(Version.value());
            }

            if (SupportedMetadataVersion)
            {
                json[L"supportedMetadataVersion"] = JSON::GetStringValue(SupportedMetadataVersion.value());
            }

            if (MaximumMetadataSize)
            {
                json[L"maximumMetadataSize"] = MaximumMetadataSize.value();
            }

            if (CurrentMetadata)
            {
                json[L"currentMetadata"] = CurrentMetadata->ToJson(Utility::Version{ "1.0" }, 0);
            }

            if (SubmissionIdentifier)
            {
                json[L"submissionIdentifier"] = JSON::GetStringValue(SubmissionIdentifier.value());
            }

            if (InstallerHash)
            {
                json[L"installerHash"] = JSON::GetStringValue(InstallerHash.value());
            }

            if (SubmissionData)
            {
                web::json::value submissionData;

                submissionData[L"DefaultLocale"] = LocaleToJSON(SubmissionData->DefaultLocalization);

                // TODO: Implement other locales

                json[L"submissionData"] = std::move(submissionData);
            }

            return json.serialize();
        }

    private:
        web::json::value LocaleToJSON(const Manifest::ManifestLocalization& localization) const
        {
            web::json::value locale;

            locale[L"PackageLocale"] = JSON::GetStringValue(localization.Locale);

            if (localization.Contains(Manifest::Localization::PackageName))
            {
                locale[L"PackageName"] = JSON::GetStringValue(localization.Get<Manifest::Localization::PackageName>());
            }

            if (localization.Contains(Manifest::Localization::Publisher))
            {
                locale[L"Publisher"] = JSON::GetStringValue(localization.Get<Manifest::Localization::Publisher>());
            }

            // TODO: Implement any other needed fields

            return locale;
        }
    };

    struct TestOutput
    {
        TestOutput(const std::string& json)
        {
            web::json::value input = web::json::value::parse(Utility::ConvertToUTF16(json));

            auto versionString = JSON::GetRawStringValueFromJsonNode(input, L"version");
            if (versionString)
            {
                Version = std::move(versionString);
            }

            auto installerHashString = JSON::GetRawStringValueFromJsonNode(input, L"installerHash");
            if (installerHashString)
            {
                InstallerHash = std::move(installerHashString);
            }

            auto statusString = JSON::GetRawStringValueFromJsonNode(input, L"status");
            if (statusString)
            {
                Status = std::move(statusString);
            }

            auto metadataValue = JSON::GetJsonValueFromNode(input, L"metadata");
            if (metadataValue && !metadataValue->get().is_null())
            {
                Metadata = std::make_optional<ProductMetadata>();
                Metadata->FromJson(metadataValue->get());
            }

            auto diagnosticsValue = JSON::GetJsonValueFromNode(input, L"diagnostics");
            if (diagnosticsValue)
            {
                auto errorHRNumber = JSON::GetRawIntValueFromJsonNode(diagnosticsValue.value(), L"errorHR");
                if (errorHRNumber)
                {
                    ErrorHR = std::move(errorHRNumber);
                }

                auto errorTextString = JSON::GetRawStringValueFromJsonNode(diagnosticsValue.value(), L"errorText");
                if (errorTextString)
                {
                    ErrorText = std::move(errorTextString);
                }
            }
        }

        std::optional<std::string> Version;
        std::optional<std::string> InstallerHash;
        std::optional<std::string> Status;
        std::optional<ProductMetadata> Metadata;
        std::optional<HRESULT> ErrorHR;
        std::optional<std::string> ErrorText;

        bool IsError() const
        {
            return Status && Status.value() == "Error";
        }

        bool IsSuccess() const
        {
            return Status && Status.value() == "Success";
        }

        bool IsLowConfidence() const
        {
            return Status && Status.value() == "LowConfidence";
        }

        void ValidateFieldPresence() const
        {
            REQUIRE(Version);
            REQUIRE(InstallerHash);
            REQUIRE(Status);

            REQUIRE(IsSuccess() == Metadata.has_value());

            REQUIRE(IsError() == ErrorHR.has_value());
            REQUIRE(IsError() == ErrorText.has_value());
        }
    };

    struct TestARPCorrelationData : public ARPCorrelationData
    {
        ARPCorrelationResult CorrelateForNewlyInstalled(const Manifest::Manifest&) override
        {
            return CorrelateForNewlyInstalledResult;
        }

        ARPCorrelationResult CorrelateForNewlyInstalledResult;
    };

    InstallerMetadataCollectionContext CreateTestContext(std::unique_ptr<ARPCorrelationData>&& data, TestInput& input)
    {
        return { std::move(data), input.ToJSON() };
    }

    InstallerMetadataCollectionContext CreateTestContext(TestInput& input)
    {
        return CreateTestContext(std::make_unique<TestARPCorrelationData>(), input);
    }

    TestOutput GetOutput(InstallerMetadataCollectionContext& context)
    {
        std::ostringstream strstr;
        context.Complete(strstr);

        return { strstr.str() };
    }

    TestOutput GetOutput(TestInput& input)
    {
        InstallerMetadataCollectionContext context = CreateTestContext(input);
        return GetOutput(context);
    }

    void BadInputTest(TestInput& input)
    {
        TestOutput output = GetOutput(input);
        REQUIRE(output.IsError());
        output.ValidateFieldPresence();
    }
}

TEST_CASE("MetadataCollection_MinimumInput", "[metadata_collection]")
{
    TestInput input(MinimalDefaults);
    TestOutput output = GetOutput(input);
    REQUIRE(!output.IsError());
    output.ValidateFieldPresence();

    REQUIRE(output.Version.value() == input.Version.value());
    REQUIRE(output.InstallerHash.value() == input.InstallerHash.value());
}

TEST_CASE("MetadataCollection_BadInput", "[metadata_collection]")
{
    TestInput input(MinimalDefaults);

#define RESET_FIELD_SECTION(_field_) \
    SECTION("No " #_field_) \
    { \
        input._field_.reset(); \
        BadInputTest(input); \
    }

    RESET_FIELD_SECTION(Version);
    RESET_FIELD_SECTION(SupportedMetadataVersion);
    RESET_FIELD_SECTION(SubmissionIdentifier);
    RESET_FIELD_SECTION(InstallerHash);
    RESET_FIELD_SECTION(SubmissionData);

#undef RESET_FIELD_SECTION
}

TEST_CASE("MetadataCollection_LowConfidence", "[metadata_collection]")
{
    TestInput input(MinimalDefaults);
    // The default test correlation object won't have a package correlation set
    TestOutput output = GetOutput(input);
    REQUIRE(output.IsLowConfidence());
    REQUIRE(!output.Metadata);
    output.ValidateFieldPresence();
}

TEST_CASE("MetadataCollection_NewPackage", "[metadata_collection]")
{
    TestInput input(MinimalDefaults);
    auto correlationData = std::make_unique<TestARPCorrelationData>();

    Manifest::Manifest manifest;
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Test Package Name");
    manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>("Test Publisher");
    manifest.Version = "1.2.3";
    manifest.Installers.push_back({});
    manifest.Installers[0].ProductCode = "{guid}";

    IPackageVersion::Metadata metadata;
    metadata[PackageVersionMetadata::InstalledType] = Manifest::InstallerTypeToString(Manifest::InstallerTypeEnum::Msi);

    correlationData->CorrelateForNewlyInstalledResult.Package = std::make_shared<TestPackageVersion>(manifest, metadata);

    InstallerMetadataCollectionContext context = CreateTestContext(std::move(correlationData), input);
    TestOutput output = GetOutput(context);

    REQUIRE(output.IsSuccess());
    output.ValidateFieldPresence();

    REQUIRE(output.Metadata->ProductVersionMin.ToString() == output.Metadata->ProductVersionMax.ToString());
    REQUIRE(output.Metadata->ProductVersionMin.ToString() == manifest.Version);
    REQUIRE(output.Metadata->InstallerMetadataMap.size() == 1);
    REQUIRE(output.Metadata->InstallerMetadataMap.count(input.InstallerHash.value()) == 1);
    const auto& entry = output.Metadata->InstallerMetadataMap[input.InstallerHash.value()];
    REQUIRE(entry.SubmissionIdentifier == input.SubmissionIdentifier.value());
    REQUIRE(entry.AppsAndFeaturesEntries.size() == 1);
    REQUIRE(entry.AppsAndFeaturesEntries[0].DisplayName == manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>());
    REQUIRE(entry.AppsAndFeaturesEntries[0].Publisher == manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>());
    REQUIRE(entry.AppsAndFeaturesEntries[0].DisplayVersion == manifest.Version);
    REQUIRE(entry.AppsAndFeaturesEntries[0].ProductCode == manifest.Installers[0].ProductCode);
    REQUIRE(entry.AppsAndFeaturesEntries[0].InstallerType == Manifest::InstallerTypeEnum::Msi);
    REQUIRE(output.Metadata->HistoricalMetadataList.empty());
}

TEST_CASE("MetadataCollection_SameSubmission_SameInstaller", "[metadata_collection]")
{
    std::string version = "1.3.5";
    std::string productCode = "{guid}";
    Manifest::InstallerTypeEnum installerType = Manifest::InstallerTypeEnum::Msi;

    TestInput input(MinimalDefaults, version, productCode, installerType);
    auto correlationData = std::make_unique<TestARPCorrelationData>();

    Manifest::Manifest manifest;
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Different Language Name");
    // Same publisher
    manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>(input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].Publisher);
    manifest.Version = version;
    manifest.Installers.push_back({});
    manifest.Installers[0].ProductCode = productCode;

    IPackageVersion::Metadata metadata;
    metadata[PackageVersionMetadata::InstalledType] = Manifest::InstallerTypeToString(installerType);

    correlationData->CorrelateForNewlyInstalledResult.Package = std::make_shared<TestPackageVersion>(manifest, metadata);

    InstallerMetadataCollectionContext context = CreateTestContext(std::move(correlationData), input);
    TestOutput output = GetOutput(context);

    REQUIRE(output.IsSuccess());
    output.ValidateFieldPresence();

    REQUIRE(output.Metadata->ProductVersionMin.ToString() == output.Metadata->ProductVersionMax.ToString());
    REQUIRE(output.Metadata->ProductVersionMin.ToString() == manifest.Version);
    REQUIRE(output.Metadata->InstallerMetadataMap.size() == 1);
    REQUIRE(output.Metadata->InstallerMetadataMap.count(input.InstallerHash.value()) == 1);
    const auto& entry = output.Metadata->InstallerMetadataMap[input.InstallerHash.value()];
    REQUIRE(entry.SubmissionIdentifier == input.SubmissionIdentifier.value());
    REQUIRE(entry.AppsAndFeaturesEntries.size() == 2);

    // One should have all values, and the other should have just a different name
    // Base which one is which off of whether Publisher is set
    for (const auto& featureEntry : entry.AppsAndFeaturesEntries)
    {
        if (featureEntry.Publisher.empty())
        {
            REQUIRE(featureEntry.DisplayName == manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>());
            REQUIRE(featureEntry.DisplayVersion.empty());
            REQUIRE(featureEntry.ProductCode.empty());
            REQUIRE(featureEntry.InstallerType == Manifest::InstallerTypeEnum::Unknown);
        }
        else
        {
            REQUIRE(featureEntry.DisplayName == input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].DisplayName);
            REQUIRE(featureEntry.Publisher == manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>());
            REQUIRE(featureEntry.DisplayVersion == manifest.Version);
            REQUIRE(featureEntry.ProductCode == manifest.Installers[0].ProductCode);
            REQUIRE(featureEntry.InstallerType == Manifest::InstallerTypeEnum::Msi);
        }
    }
    REQUIRE(output.Metadata->HistoricalMetadataList.empty());
}

TEST_CASE("MetadataCollection_SameSubmission_NewInstaller", "[metadata_collection]")
{
    std::string versionPresent = "1.3.5";
    std::string versionIncoming = "1.3.5.1";
    std::string productCodePresent = "{guid}";
    std::string productCodeIncoming = "{guid_different}";
    Manifest::InstallerTypeEnum installerType = Manifest::InstallerTypeEnum::Msi;

    TestInput input(MinimalDefaults, versionPresent, productCodePresent, installerType);
    // Change the incoming hash to be new
    input.InstallerHash = input.InstallerHash.value() + "_DIFFERENT";
    auto correlationData = std::make_unique<TestARPCorrelationData>();

    Manifest::Manifest manifest;
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Name (but different architecture)");
    // Same publisher
    manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>(input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].Publisher);
    manifest.Version = versionIncoming;
    manifest.Installers.push_back({});
    manifest.Installers[0].ProductCode = productCodeIncoming;

    IPackageVersion::Metadata metadata;
    metadata[PackageVersionMetadata::InstalledType] = Manifest::InstallerTypeToString(installerType);

    correlationData->CorrelateForNewlyInstalledResult.Package = std::make_shared<TestPackageVersion>(manifest, metadata);

    InstallerMetadataCollectionContext context = CreateTestContext(std::move(correlationData), input);
    TestOutput output = GetOutput(context);

    REQUIRE(output.IsSuccess());
    output.ValidateFieldPresence();

    REQUIRE(output.Metadata->ProductVersionMin.ToString() == versionPresent);
    REQUIRE(output.Metadata->ProductVersionMax.ToString() == versionIncoming);
    REQUIRE(output.Metadata->InstallerMetadataMap.size() == 2);

    for (const auto& installerMetadata : output.Metadata->InstallerMetadataMap)
    {
        const auto& entry = installerMetadata.second;

        REQUIRE(entry.SubmissionIdentifier == input.SubmissionIdentifier.value());
        REQUIRE(entry.AppsAndFeaturesEntries.size() == 1);

        const auto& featureEntry = entry.AppsAndFeaturesEntries.front();
        REQUIRE(featureEntry.Publisher == manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>());

        if (featureEntry.ProductCode == productCodePresent)
        {
            REQUIRE(featureEntry.DisplayName == input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].DisplayName);
            REQUIRE(featureEntry.DisplayVersion == versionPresent);
            REQUIRE(featureEntry.InstallerType == Manifest::InstallerTypeEnum::Msi);
        }
        else
        {
            REQUIRE(featureEntry.DisplayName == manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>());
            REQUIRE(featureEntry.DisplayVersion == versionIncoming);
            REQUIRE(featureEntry.InstallerType == Manifest::InstallerTypeEnum::Msi);
            REQUIRE(featureEntry.ProductCode == productCodeIncoming);
        }
    }

    REQUIRE(output.Metadata->HistoricalMetadataList.empty());
}

TEST_CASE("MetadataCollection_NewSubmission", "[metadata_collection]")
{
    std::string versionPresent = "1.3.5";
    std::string versionIncoming = "1.4.0";
    std::string productCodePresent = "{guid}";
    std::string productCodeIncoming = "{guid_different}";
    Manifest::InstallerTypeEnum installerType = Manifest::InstallerTypeEnum::Msi;

    TestInput input(MinimalDefaults, versionPresent, productCodePresent, installerType);
    input.SubmissionIdentifier = input.SubmissionIdentifier.value() + "_NEW";
    input.InstallerHash = input.InstallerHash.value() + "_DIFFERENT";
    auto correlationData = std::make_unique<TestARPCorrelationData>();

    Manifest::Manifest manifest;
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].DisplayName);
    manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>(input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].Publisher);
    manifest.Version = versionIncoming;
    manifest.Installers.push_back({});
    manifest.Installers[0].ProductCode = productCodeIncoming;

    IPackageVersion::Metadata metadata;
    metadata[PackageVersionMetadata::InstalledType] = Manifest::InstallerTypeToString(installerType);

    correlationData->CorrelateForNewlyInstalledResult.Package = std::make_shared<TestPackageVersion>(manifest, metadata);

    InstallerMetadataCollectionContext context = CreateTestContext(std::move(correlationData), input);
    TestOutput output = GetOutput(context);

    REQUIRE(output.IsSuccess());
    output.ValidateFieldPresence();

    REQUIRE(output.Metadata->ProductVersionMin.ToString() == output.Metadata->ProductVersionMax.ToString());
    REQUIRE(output.Metadata->ProductVersionMin.ToString() == manifest.Version);
    REQUIRE(output.Metadata->InstallerMetadataMap.size() == 1);
    REQUIRE(output.Metadata->InstallerMetadataMap.count(input.InstallerHash.value()) == 1);
    const auto& entry = output.Metadata->InstallerMetadataMap[input.InstallerHash.value()];
    REQUIRE(entry.SubmissionIdentifier == input.SubmissionIdentifier.value());
    REQUIRE(entry.AppsAndFeaturesEntries.size() == 1);
    REQUIRE(entry.AppsAndFeaturesEntries[0].DisplayName == manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>());
    REQUIRE(entry.AppsAndFeaturesEntries[0].Publisher == manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>());
    REQUIRE(entry.AppsAndFeaturesEntries[0].DisplayVersion == manifest.Version);
    REQUIRE(entry.AppsAndFeaturesEntries[0].ProductCode == manifest.Installers[0].ProductCode);
    REQUIRE(entry.AppsAndFeaturesEntries[0].InstallerType == Manifest::InstallerTypeEnum::Msi);

    REQUIRE(output.Metadata->HistoricalMetadataList.size() == 1);
    const auto& historicalEntry = output.Metadata->HistoricalMetadataList[0];
    REQUIRE(historicalEntry.ProductVersionMin.ToString() == input.CurrentMetadata->ProductVersionMin.ToString());
    REQUIRE(historicalEntry.ProductVersionMax.ToString() == input.CurrentMetadata->ProductVersionMax.ToString());
    const auto& appsAndFeaturesEntry = input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries.front();
    REQUIRE(historicalEntry.Names.size() == 1);
    REQUIRE(*historicalEntry.Names.begin() == appsAndFeaturesEntry.DisplayName);
    REQUIRE(historicalEntry.ProductCodes.size() == 1);
    REQUIRE(*historicalEntry.ProductCodes.begin() == appsAndFeaturesEntry.ProductCode);
    REQUIRE(historicalEntry.Publishers.size() == 1);
    REQUIRE(*historicalEntry.Publishers.begin() == appsAndFeaturesEntry.Publisher);
}
